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

#ifndef __MAIN_GATECONTROLLER_H_
#define __MAIN_GATECONTROLLER_H_

#include <omnetpp.h>

#include <memory>
#include <string>
#include <vector>

#include <fstream>
#include <iostream>

#include "inet/common/ModuleAccess.h"
#include "inet/common/InitStages.h"
#include "inet/linklayer/ethernet/EtherMacFullDuplex.h"

#include "nesting/linklayer/framePreemption/EtherMACFullDuplexPreemptable.h"
#include "nesting/common/schedule/Schedule.h"
#include "nesting/common/schedule/ScheduleFactory.h"
#include "nesting/ieee8021q/Ieee8021q.h"
#include "nesting/ieee8021q/queue/TransmissionSelection.h"
#include "nesting/ieee8021q/queue/gating/TransmissionGate.h"
#include "nesting/common/time/IClock.h"
#include "nesting/common/time/IClockListener.h"

using namespace omnetpp;
using namespace inet;
using namespace std;

class TransmissionGate;
class EtherMACFullDuplexPreemptable;

namespace nesting {

/**
 * See the NED file for a detailed description
 */
class GateController: public cSimpleModule, public IClockListener {
private:
    /** Current schedule. Is never null. */
    Schedule<GateBitvector>* currentSchedule;

    /**
     * Next schedule to load after the current schedule finishes it's cycle.
     * Can be null.
     */
    Schedule<GateBitvector>* nextSchedule;

    // /** Index for the current entry in the schedule. */
    // unsigned int scheduleIndex;

    /**
     * Clock reference, needed to get the current time and subscribe
     * clock events.
     */
    IClock* clock;

    /**
     * Specifies the cycle start time.
     */
    simtime_t cycleStart;

    /** Reference to transmission gate vector module */
    std::vector<TransmissionGate*> transmissionGates;

    EtherMACFullDuplexPreemptable* preemptMacModule;
    inet::EtherMacFullDuplex* macModule;
    std::string switchString;
    std::string portString;
    simtime_t lastChange;

    cMessage updateScheduleMsg = cMessage("updateSchedule");

    // 记录GCL更新情况
    cPar* result_file_location;
    fstream result_file;

    // 做个测试，把这个成员放私有里面有没有问题
    // 为了保障TT流的传输，在一个周期里，如果时隙被调整变大，就不能被调整变小，只有当所有TT流都要求时隙减小时才减小
    bool isIncreased;
protected:
    /** @see cSimpleModule::initialize(int) */
    virtual void initialize(int stage) override;

    /** Schedules the next tick */
    virtual simtime_t scheduleNextTickEvent();

    /** @see cSimpleModule::numInitStages() */
    virtual int numInitStages() const override;

    /** @see cSimpleModule::handleMessage(cMessage*) */
    virtual void handleMessage(cMessage *msg) override;

    /** Opens all transmission gates. */
    virtual void openAllGates(); // TODO use setGateStates internal

    virtual void setGateStates(GateBitvector bitvector, bool release);

    virtual void updateSchedule();

public:
    // 为了传参，把这个参数从私有转到公有
    /** Index for the current entry in the schedule. */
    unsigned int scheduleIndex;

    // 创建一个newSchedule,保存当前调度方案，并用于更新GCL自适应
    // 更新方式是vlanEtherTrafGenGCL读取newSchedule,进行自适应调节
    Schedule<GateBitvector>* newSchedule;

    // // 为了保障TT流的传输，在一个周期里，如果时隙被调整变大，就不能被调整变小，只有当所有TT流都要求时隙减小时才减小
    // bool isIncreased;

    virtual ~GateController();

    /** @see IClockListener::tick(IClock*) */
    virtual void tick(IClock *clock, short kind) override;

    /** Calculate the maximum bit size that can be transmitted until the next gate state change.
     *  Returns an unsigned integer value.
     **/
    virtual unsigned int calculateMaxBit(int gateIndex);

    /** extracts and loads the correct schedule from xml file, or an empty one if none is defined */
    virtual void loadScheduleOrDefault(cXMLElement* xml);

    virtual bool currentlyOnHold();
    
    // 获取当前门控调度
    virtual Schedule<GateBitvector>* getnewSchedule();

    // 获取当前GCL的Index
    virtual unsigned int getscheduleIndex();

    // 获取GCL的isIncreased
    virtual bool getisIncreased();

    // 设置GCL的isIncreased
    virtual void setisIncreased(bool is_Increased);
};

} // namespace nesting

#endif
