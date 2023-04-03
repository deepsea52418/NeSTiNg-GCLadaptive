//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "nesting/ieee8021q/queue/gating/GateController.h"

namespace nesting {

Define_Module(GateController);

GateController::~GateController() {
    transmissionGates.clear();
    
    assert(currentSchedule != nullptr);
    
    // delete currentSchedule;
    // 删除currrentSchedule
    if (currentSchedule != nullptr) {
        delete currentSchedule;
    }
    if (nextSchedule != nullptr) {
        delete nextSchedule;
    }
    // 析构newSchedule
    if (newSchedule != nullptr) {
        delete newSchedule;
    }
    cancelEvent(&updateScheduleMsg);
}

void GateController::initialize(int stage) {
    // 初始化时钟
    //initialize clock and gate references in first stage
    if (stage == INITSTAGE_LOCAL) {
        scheduleIndex = 0;
        // 根据ned初始化时钟模型
        // Keep reference to clock module
        cModule* clockModule = getModuleFromPar<cModule>(par("clockModule"), this);
        clock = check_and_cast<IClock*>(clockModule);

        // 记录GCL更新情况文件
        string filename = (&par("result_file_location"))->stringValue() ;
        this->result_file.open(filename, ios::out | ios::trunc);

        // 根据ned初始化gate模型
        // Iterate through transmission gates an keep them as references
        TransmissionGate* transmissionGateVectorModule = getModuleFromPar<
                TransmissionGate>(par("transmissionGateVectorModule"), this);
        cModule::SubmoduleIterator it = cModule::SubmoduleIterator(
                transmissionGateVectorModule->getParentModule());
        for (; !it.end(); it++) {
            cModule* subModule = *it;
            if (subModule->isName(transmissionGateVectorModule->getName())) {
                TransmissionGate* transmissionGate = check_and_cast<
                        TransmissionGate*>(subModule);
                transmissionGates.push_back(transmissionGate);
            }
        }

        // 初始化mac层模型
        cModule* macMod = getModuleFromPar<cModule>(par("macModule"), this);

        EtherMACFullDuplexPreemptable* macTmp =
                dynamic_cast<EtherMACFullDuplexPreemptable*>(macMod);
        if (macTmp != nullptr) {
            preemptMacModule = check_and_cast<EtherMACFullDuplexPreemptable*>(
                    macMod);
            macModule = nullptr;
        } else {
            macModule = check_and_cast<inet::EtherMacFullDuplex*>(macMod);
            preemptMacModule = nullptr;
        }

        WATCH(scheduleIndex);
    }
    // 初始化GCL调度表
    //initialize schedule in second stage when clock is initialized
    else if (stage == INITSTAGE_LINK_LAYER) {

        switchString =
                this->getModuleByPath(par("switchModule"))->getFullName();
        portString =
                std::to_string(
                        this->getModuleByPath(par("networkInterfaceModule"))->getIndex());

        lastChange = simTime();

        // 初始化isIncreased
        isIncreased = false;

        currentSchedule = new Schedule<GateBitvector>();
        currentSchedule->addControlListEntry(SimTime(1, SIMTIME_S), GateBitvector("11111111"));
        cXMLElement* xml = par("initialSchedule").xmlValue();
        loadScheduleOrDefault(xml);
        if (par("enableHoldAndRelease")) {
            //Schedule hold for the first entry if needed.
            //This is needed because hold is only always requested for the following entry,
            //but not for the current one. Therefore the first entry would not be held.
            // 机制允许发送方在网络上占用一定的带宽，直到数据准备就绪才释放该带宽，
            // 以便将数据流发送到接收方。这个机制保证了在发送时间敏感数据流时的可靠性，
            // 因为它允许发送方在准备好发送数据之前占用带宽，避免了数据传输时的竞争。
            // 如果设置了HoldAndRelease,则会在每次调度前保留资源，在第一次加载GCL时不会启用（第一次不需要保留）
            for (TransmissionGate* transmissionGate : transmissionGates) {
                if ((!currentSchedule->isEmpty()
                        && currentSchedule->getScheduledObject(0).test(
                                transmissionGate->getIndex()))
                        && transmissionGate->isExpressQueue()) {
                    preemptMacModule->hold(SIMTIME_ZERO);
                    break;
                }
            }
        }
        clock->subscribeTick(this, 0);
    }
}

// 返回模型所需要的初始化阶段数
int GateController::numInitStages() const {
    return INITSTAGE_LINK_LAYER + 1;
}

// 处理消息
void GateController::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage() && msg == &updateScheduleMsg) {
        updateSchedule();
    } else {
        throw cRuntimeError("Cannot handle this message!");
    }
}

// 在时钟滴答作响并调度 updateScheduleMsg 消息时被调用。
void GateController::tick(IClock *clock, short kind) {
    Enter_Method("tick()");
    scheduleAt(simTime(), &updateScheduleMsg);
}

// 根据当前时间和GCL计算下一次调度的时间间隔
simtime_t GateController::scheduleNextTickEvent() {
    // combined length of bitvectors is longer than cycle time,
    // 应该是每次执行完GCL更新后，会立马计算下一次调度时间间隔
    // 如果GCL周期<所有GCL列的间隔时间之和，则会在GCL周期到达时更新。
    // 否则就等待当前GCL列的持续时间间隔后更新
    if (clock->getTime() + currentSchedule->getTimeInterval(scheduleIndex)
            > cycleStart + currentSchedule->getCycleTime()) {
        return (cycleStart + currentSchedule->getCycleTime() - clock->getTime());
    } else {
        return currentSchedule->getTimeInterval(scheduleIndex);
    }
    return 0;
}

// 根据当前调度和传输速率，计算最大可传输bit数
unsigned int GateController::calculateMaxBit(int gateIndex) {
    double transmitRate;
    if (preemptMacModule != nullptr) {
        transmitRate = preemptMacModule->getTxRate();
    } else {
        transmitRate = macModule->getTxRate();
    }
    if (transmitRate <= 0) {
        return 0;
    }
    simtime_t timeSinceLastChange = clock->getTime() - lastChange;

    double bits = 0;
    //Has the lookahead already touched the next Schedule
    bool touchedNextSchedule = false;
    int currentIndex = (scheduleIndex + currentSchedule->getControlListLength() - 1)
            % currentSchedule->getControlListLength();
    // sum up time gate is opened until now since last change
    simtime_t cumSumGateLength = SIMTIME_ZERO;
    simtime_t tmp = SIMTIME_ZERO;
    for (int i = 0; i <= (currentIndex - 1); i++) {
        // tmp necessary because =+ not defined for simtime_t
        tmp = cumSumGateLength + currentSchedule->getTimeInterval(i);
        cumSumGateLength = tmp;
    }
    GateBitvector bitvector = currentSchedule->getScheduledObject(currentIndex);
    while (bits < kEthernet2MaximumTransmissionUnitBitLength.get()) {
        //if the bitvector is now closed, return all bit summed up until now
        if (!bitvector.test(gateIndex)) {
            return static_cast<int>(bits);
        }
        //only look in current schedule if there is no nextSchedule
        if (!nextSchedule) {
            tmp = cumSumGateLength + currentSchedule->getTimeInterval(currentIndex);
            cumSumGateLength = tmp;
            // take cycle end as upper limit if it is shorter
            if (cycleStart + cumSumGateLength
                    > cycleStart + currentSchedule->getCycleTime()) {
                // calculation needs cumSumGateLength before it became greater than cycle length
                simtime_t timeLeftInCycle = currentSchedule->getCycleTime()
                        - (timeSinceLastChange + cumSumGateLength
                                - currentSchedule->getTimeInterval(currentIndex));
                bits = bits
                        + (timeLeftInCycle
                                / (SimTime(1, SIMTIME_S) / transmitRate));
                currentIndex = 0;
            } else { // else take bitvector length as upper limit
                bits = bits
                        + ((currentSchedule->getTimeInterval(currentIndex))
                                - timeSinceLastChange)
                                / (SimTime(1, SIMTIME_S) / transmitRate);
                currentIndex = (currentIndex + 1) % currentSchedule->getControlListLength();
            }
            timeSinceLastChange = SIMTIME_ZERO;
            if (currentIndex == 0) {
                cumSumGateLength = SIMTIME_ZERO;
            }
            bitvector = currentSchedule->getScheduledObject(currentIndex);
        } else {
            //if there is a nextSchedule but it is not yet being looked at
            bool hitCycleEnd = false;
            if (!touchedNextSchedule) {
                tmp = cumSumGateLength
                        + currentSchedule->getTimeInterval(currentIndex);
                cumSumGateLength = tmp;
                if (cycleStart + cumSumGateLength
                        > cycleStart + currentSchedule->getCycleTime()) {
                    simtime_t timeLeftInCycle = currentSchedule->getCycleTime()
                            - (timeSinceLastChange + cumSumGateLength
                                    - currentSchedule->getTimeInterval(currentIndex));
                    bits = bits
                            + (timeLeftInCycle
                                    / (SimTime(1, SIMTIME_S) / transmitRate));
                    hitCycleEnd = true;
                } else {
                    bits = bits
                            + (currentSchedule->getTimeInterval(currentIndex)
                                    - timeSinceLastChange)
                                    / (SimTime(1, SIMTIME_S) / transmitRate);
                }
                timeSinceLastChange = SIMTIME_ZERO;
                //if currentIndex is not the last index in currentSchedule and cycle end not hit
                if (currentIndex < (int) currentSchedule->getControlListLength() - 1
                        && !hitCycleEnd) {
                    currentIndex = (currentIndex + 1) % currentSchedule->getControlListLength();
                    bitvector = currentSchedule->getScheduledObject(
                            currentIndex);
                } else {
                    //if it is the last index in currentSchedule or schedule hit cycle end, look into nextSchedule from now on
                    touchedNextSchedule = true;
                    currentIndex = 0;
                    bitvector = nextSchedule->getScheduledObject(currentIndex);
                    // nextSchedule cycle starts in the future, therefore cycleStart of new schedule is the end of the old schedule
                    if (hitCycleEnd) {
                        cycleStart = cycleStart
                                + currentSchedule->getCycleTime();
                    } else {
                        cycleStart = cycleStart + cumSumGateLength;
                    }
                    cumSumGateLength = SIMTIME_ZERO;
                }
            } else {
                //if the nextSchedule is being looked at
                tmp = cumSumGateLength + nextSchedule->getTimeInterval(currentIndex);
                cumSumGateLength = tmp;
                if (cycleStart + cumSumGateLength
                        > cycleStart + nextSchedule->getCycleTime()) {
                    simtime_t timeLeftInCycle = currentSchedule->getCycleTime()
                    - (timeSinceLastChange + cumSumGateLength
                            - currentSchedule->getTimeInterval(currentIndex));
                    bits = bits + (timeLeftInCycle
                            / (SimTime(1, SIMTIME_S) / transmitRate));
                    currentIndex = 0;
                } else {
                    bits = bits
                    + ((nextSchedule->getTimeInterval(currentIndex))
                            - timeSinceLastChange)
                    / (SimTime(1, SIMTIME_S) / transmitRate);
                    currentIndex = (currentIndex + 1) % nextSchedule->getControlListLength();
                }
                timeSinceLastChange = SIMTIME_ZERO;
                if (currentIndex == 0) {
                    cumSumGateLength = SIMTIME_ZERO;
                }
                bitvector = nextSchedule->getScheduledObject(currentIndex);
            }
        }
    }
    //otherwise, return the full MTU bits
    return kEthernet2MaximumTransmissionUnitBitLength.get();
}

// 加载门控调度器的调度序列，并在序列为空时抛出异常。如果成功加载调度序列，则将其存储在变量nextSchedule中。
void GateController::loadScheduleOrDefault(cXMLElement* xml) {
    Schedule<GateBitvector> *schedule = ScheduleFactory::createGateBitvectorSchedule(xml);
    if (schedule->getControlListLength() == 0) {
        throw cRuntimeError("Tried to load schedule with zero entries.");
    }

    EV_DEBUG << getFullPath() << ": Loading schedule. Cycle is "
                    << schedule->getCycleTime() << ". Entry count is "
                    << schedule->getControlListLength() << ". Time is "
                    << clock->getTime().inUnit(SIMTIME_US) << endl;
    nextSchedule = schedule;
    // 在初始化的时候需要创建newSchedule，否则无法读取第一个GCL
    newSchedule = ScheduleFactory::copySchedule(nextSchedule);
}

// 设置门状态
void GateController::setGateStates(GateBitvector bitvector, bool release) {
    for (TransmissionGate* transmissionGate : transmissionGates) { 
        transmissionGate->setGateState(bitvector.test(transmissionGate->getIndex()), release);
    }
}

// 更新当前门控调度器的状态，包括当前的门控状态、时间信息和计时器订阅。
// 同时支持 Hold&Release 的特性，即在下一个门控状态之前检查是否需要将Mac组件的帧暂停。
void GateController::updateSchedule()
{
    // When the current schedule index is 0, this means that the current
    // schedule's cycle was not started or was just finished. Therefore in this
    // case a new schedule is loaded if available.
    // If cycleStart + cycleTime is greater than the current time
    // , a new cycle hast started and the scheduleIndex needs to be reset to 0.
    // scheduleIndex==0时表示一个GCL刚执行完或者还没有开始执行
    // 若没有nextSchedule，则会一直执行当前的GCL
    if (scheduleIndex == 0 && nextSchedule) {
        // Print warning if the feature is used in combination with frame preemption
        // enableHoldAndRelease和抢占无法同时启用（enableHoldAndRelease就已经保留了资源，可以不用抢占）
        if(preemptMacModule != nullptr) {
            if( preemptMacModule->isFramePreemptionEnabled() && par("enableHoldAndRelease")) {
                EV_WARN << "Using schedule swap in combination with Hold&Release (Frame Preemption) can lead to wrong hold periods."<<endl;
            }
        }

        // Load new schedule and delete the old one if there is new schedule.
        // 可以通过改变nextSchedule变量, 调整数据.
        // delete currentSchedule;
        // currentSchedule = nextSchedule;
        // nextSchedule = nullptr;
        
        // 删除旧的GCL申请的空间
        delete currentSchedule;
        // 用vlanEtherTrafGenGCL更新的newSchedule更新GCL
        currentSchedule = ScheduleFactory::copySchedule(newSchedule);
        nextSchedule = newSchedule;
        // 重置isIncreased
        isIncreased = false;
        // 记录GCL更新情况
        this->result_file << "{ \"time\": "<<simTime() << ", \"cycletime\": \"" << currentSchedule->getCycleTime() << "\""\
            << ", \"TT-gcl\": \"" << currentSchedule->getScheduledObject(scheduleIndex)  << "\"" \
            << ", \"TT-interval\": " << currentSchedule->getTimeInterval(scheduleIndex) \
            << ", \"BE-gcl\": \"" << currentSchedule->getScheduledObject((scheduleIndex + 1) % currentSchedule->getControlListLength()) <<  "\"" \
            << ", \"BE-interval\": " << currentSchedule->getTimeInterval((scheduleIndex + 1) % currentSchedule->getControlListLength()) << " } "   << endl;


        // If an empty schedule was loaded, all gates are opened and there is no
        // need to subscribe to clock ticks
        if (currentSchedule->isEmpty()) {
            openAllGates();
            return;
        }
    }

    // 如果一个调度结束了，那么就应该从新的GCL的scheduleIndex = 0开始执行
    if((cycleStart + currentSchedule->getCycleTime() == clock->getTime())
    && currentSchedule->getCycleTime() != 0) {
        // need to be set to 0, if cycle time has ended
        scheduleIndex = 0;
    }
    // 如果重新执行GCL，则GCL循环开始时间=当前时间
    if(scheduleIndex == 0) {
        cycleStart = clock->getTime();
    }

    // Get next gatestate bitvector
    // 获取需要执行的GCL列的信息
    GateBitvector bitvector = currentSchedule->getScheduledObject(scheduleIndex);
    bool releaseNeeded = false;
    if(par("enableHoldAndRelease")) {
        //Check whether some express gate is open
        bool someExpressGateOpen=false;
        for (TransmissionGate* transmissionGate : transmissionGates) {
            if(bitvector.test(transmissionGate->getIndex()) && transmissionGate->isExpressQueue()) {
                someExpressGateOpen=true;
                break;
            }
        }
        //If the Mac component was on hold and no express gate is opened, release it
        releaseNeeded = !someExpressGateOpen && currentlyOnHold();
        if(releaseNeeded) {
            if(preemptMacModule != nullptr) {
                preemptMacModule->release();
            }
        }
    }
    //Set gate states for every gate
    setGateStates(bitvector, releaseNeeded);
    // from high prio to low prio
    EV_INFO << "Setting gates to "<< bitvector << " at t="
            << clock->getTime().inUnit(SIMTIME_US) << "us." << endl;

    // Subscribe to the tick, on which a new schedule entry is loaded.
    clock->subscribeTick(this, scheduleNextTickEvent() / clock->getClockRate());
    lastChange = clock->getTime();

    if(par("enableHoldAndRelease")) {
        //Get following bitvector to be able to schedule hold with advance
        GateBitvector nextVector;
        if(nextSchedule && scheduleIndex == currentSchedule->getControlListLength()-1) {
            //If we are at the last entry of the current schedule, look at the first entry of the next one
            nextVector = nextSchedule->getScheduledObject(0);
        } else {
            nextVector = currentSchedule->getScheduledObject((scheduleIndex + 1) % currentSchedule->getControlListLength());
        }

        for (TransmissionGate* transmissionGate : transmissionGates) {
            //Schedule hold if any express gate is open in the next schdule state
            if(nextVector.test(transmissionGate->getIndex()) && transmissionGate->isExpressQueue()) {
                if(preemptMacModule!=nullptr) {
                    preemptMacModule->hold((currentSchedule->getTimeInterval(scheduleIndex)) - preemptMacModule->getHoldAdvance());
                    break;
                }
            }
        }
    }

    // Switch to next schedule entry
    scheduleIndex = (scheduleIndex + 1) % currentSchedule->getControlListLength();
}

// 打开所有门
void GateController::openAllGates() {
    GateBitvector bitvectorAllGatesOpen;
    bitvectorAllGatesOpen.set();
    setGateStates(bitvectorAllGatesOpen, true);
}

// 监测当前门是否处于保留状态（HoldAndRelease)
bool GateController::currentlyOnHold() {
    if (preemptMacModule != nullptr) {
        return preemptMacModule->isOnHold();
    } else {
        return false;
    }
}

// 自写函数
// 获取下一次加载的GCL信息
Schedule<GateBitvector>* GateController::getnewSchedule(){
    return this->newSchedule;
}

// 自写函数
// 获取当前GCL的Index信息，由于当前的Index是执行的下一条GCL的序号，需要-1
unsigned int GateController::getscheduleIndex(){
    return (this->scheduleIndex -1)% this->currentSchedule->getControlListLength();
}
// 自写函数
// 获取当前GCL的isIncreased信息
bool GateController::getisIncreased(){
    return this->isIncreased;
}
// 自写函数
// 设置当前GCL的isIncreased信息
void GateController::setisIncreased(bool is_Increased){
    this->isIncreased = is_Increased;
}

// 自写函数
// 获取当前GCL
simtime_t GateController::getCurrentScheduleInterval(int scheduleIndex){
    return this->currentSchedule->getTimeInterval(scheduleIndex);
}

}
// namespace nesting
