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

#include "nesting/application/ethernet/VlanEtherTrafGenGCL2.h"
#include "nesting/linklayer/vlan/EnhancedVlanTag_m.h"
#include "nesting/application/ethernet/VlanEtherTrafGenSched.h"
// 引入gatecontroller头文件
#include "nesting/ieee8021q/queue/gating/GateController.h"
//
#include "inet/common/TimeTag_m.h"
#include "inet/linklayer/common/Ieee802SapTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/Protocol.h"
#include "inet/common/Simsignals.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/common/IProtocolRegistrationListener.h"

namespace nesting {

    Define_Module(VlanEtherTrafGenGCL2);
    void VlanEtherTrafGenGCL2::initialize(int stage) {
        EtherTrafGen::initialize(stage);
       if (stage == INITSTAGE_LOCAL) {
            vlanTagEnabled = &par("vlanTagEnabled");
            pcp = &par("pcp");
            dei = &par("dei");
            vid = &par("vid");
            gateController_a = getModuleFromPar<GateController>(par("gateControllerModule_a"), this);
            gateController_b = getModuleFromPar<GateController>(par("gateControllerModule_b"), this);
            string filename = (&par("result_file_location"))->stringValue() ;
            this->result_file.open(filename, ios::out | ios::trunc);

             // 初始化GCL自适应控制参数
            increasesteplength = par("increasesteplength");
            decreasesteplength = par("decreasesteplength");
            maxIncreasesteplength = par("maxIncreasesteplength");
            maxDecreasesteplength = par("maxDecreasesteplength");
            upperLimitTime = par("upperLimitTime");
            lowerLimitTime = par("lowerLimitTime");
            if (increasesteplength <= SIMTIME_ZERO || decreasesteplength <= SIMTIME_ZERO ){
                throw cRuntimeError("Invalid increasesteplength/decreasesteplength parameters");
            }
            if (maxIncreasesteplength <= SIMTIME_ZERO || maxDecreasesteplength <= SIMTIME_ZERO ){
                throw cRuntimeError("Invalid maxIncreasesteplength/maxDecreasesteplength parameters");
            }
            if (upperLimitTime <= SIMTIME_ZERO || lowerLimitTime <= SIMTIME_ZERO ){
                throw cRuntimeError("Invalid upperLimitTime/lowerLimitTime parameters");
            }
            if (upperLimitTime < lowerLimitTime){
                throw cRuntimeError("Invalid parameters! upperLimitTime < lowerLimitTime parameters");
            }
            EV_INFO << "increasesteplength = " << increasesteplength <<"us"<<endl;
            EV_INFO << "decreasesteplength = " << decreasesteplength <<"us"<<endl;
            EV_INFO << "maxIncreasesteplength = " << maxIncreasesteplength <<"us"<<endl;
            EV_INFO << "maxDecreasesteplength = " << maxDecreasesteplength <<"us"<<endl;
            EV_INFO << "upperLimitTime = " << upperLimitTime <<"us"<<endl;
            EV_INFO << "lowerLimitTime = " << lowerLimitTime <<"us"<<endl;
            //
        } else if (stage == INITSTAGE_LINK_LAYER) {
            registerService(*VlanEtherTrafGenSched::L2_PROTOCOL, nullptr, gate("in"));
            registerProtocol(*VlanEtherTrafGenSched::L2_PROTOCOL, gate("out"), nullptr);
        }
    }

    void VlanEtherTrafGenGCL2::sendBurstPackets() {
        int n = numPacketsPerBurst->intValue();
        for (int i = 0; i < n; i++) {
            seqNum++;

            char msgname[40];
            sprintf(msgname, "pk-%d-%ld", getId(), seqNum);

            // create new packet
            Packet *datapacket = new Packet(msgname, IEEE802CTRL_DATA);
            long len = packetLength->intValue();
            const auto& payload = makeShared<ByteCountChunk>(B(len));
            // set creation time
            auto timeTag = payload->addTag<CreationTimeTag>();
            timeTag->setCreationTime(simTime());

            datapacket->insertAtBack(payload);
            datapacket->removeTagIfPresent<PacketProtocolTag>();
            datapacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(VlanEtherTrafGenSched::L2_PROTOCOL);
            // TODO check which protocol to insert
            auto sapTag = datapacket->addTagIfAbsent<Ieee802SapReq>();
            sapTag->setSsap(ssap);
            sapTag->setDsap(dsap);

            // create control info for encap modules
            auto macTag = datapacket->addTag<MacAddressReq>();
            macTag->setDestAddress(destMacAddress);

            // create VLAN control info
            if (vlanTagEnabled->boolValue()) {
                EnhancedVlanReq* vlanReq = datapacket->addTag<EnhancedVlanReq>();
                vlanReq->setPcp(pcp->intValue());
                vlanReq->setDe(dei->boolValue());
                vlanReq->setVlanId(vid->intValue());
            }

            EV_TRACE << getFullPath() << ": Send packet `" << datapacket->getName()
                     << "' dest=" << macTag->getDestAddress() << " length="
                     << datapacket->getBitLength() << "B type="
                     << IEEE802CTRL_DATA << " vlan-tagged="
                     << vlanTagEnabled->boolValue();
            if (vlanTagEnabled->boolValue()) {
                EV_TRACE << " pcp=" << pcp->intValue() << " dei=" << dei->boolValue() << " vid=" << vid->intValue();
            }
            EV_TRACE << endl;

            emit(packetSentSignal, datapacket);
            send(datapacket, "out");
            packetsSent++;
        }
    }

    void VlanEtherTrafGenGCL2::receivePacket(Packet *msg)
    {
        // 计算报文延迟
        auto data = msg->peekData();  // 获取msg中所有数据
        auto regions = data->getAllTags<CreationTimeTag>(); // 获取数据包的创建时间
        // SimTime是omnet中定义是时间型数据类型
        SimTime delay;
        for (auto& region : regions) { 
            auto creationTime = region.getTag()->getCreationTime(); 
            delay = simTime() - creationTime; // 计算延迟
        }

        // 记录收到报文的时间等信息
        this->result_file << "{ \"time\": "<<simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"e2edelay\": " << delay << " } "   << endl;
        
        
        static bool is_first_increase_switch = true;
        static bool is_first_decrease_switch = true;


        // 选择调节的交换机(大延迟)
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 && delay >= SimTime(75, SIMTIME_US)){
            // 重置减小的交换机判断
            static bool is_first_decrease_switch = true;
            // 判断是否调节过共用的交换机
            if ( is_first_increase_switch == true ){
                // 没有调节过,则调节公共交换机
                gateController = gateController_a;
                is_first_increase_switch = false;
            }else{
                // 如果已经调节过，则调节非公共交换机
                gateController = gateController_b;
                is_first_increase_switch = true;
            }
        }

        // 选择调节的交换机(小延迟)
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 && delay <= SimTime(45, SIMTIME_US)){
            // 重置增大的交换机判断
            static bool is_first_increase_switch = true;
            // 判断是否调节过共用的交换机
            if ( is_first_decrease_switch == true ){
                // 没有调节过,则调节公共交换机
                gateController = gateController_a;
                is_first_decrease_switch = false;
            }else{
                // 如果已经调节过，则调节非公共交换机
                gateController = gateController_b;
                is_first_decrease_switch = true;
            }
        }


        // 判断当前报文是否是TT流，若不是TT流，则不进行调节
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 ){
            // 获取gatecontroller中的newSchedule对象
            newSchedule = gateController ->getnewSchedule();
            // 获取循环时间
            simtime_t schedule_cycle = newSchedule->getCycleTime();
            // 获取当前Index
            unsigned int currentscheduleIndex = gateController->getscheduleIndex();
            // 获取下一个Index
            unsigned int nextscheduleIndex = (currentscheduleIndex + 1) % newSchedule->getControlListLength();
            // 获取下次执行的GCL时隙大小
            simtime_t time_interval = newSchedule->getTimeInterval(currentscheduleIndex);
            // 获取正在执行的GCL时隙大小
            simtime_t current_interval = gateController->getCurrentScheduleInterval(currentscheduleIndex);
            // 计算目标调节时隙大小
            simtime_t target_time_interval = time_interval;
            // 获取gatecontroller的isIncreased参数，判断当前的newSchedule的TT时隙有没有被增大，如果没有被增大，可以被变小,目的是宁可浪费带宽也要保障TT流传输
            bool isIncreased = gateController->getisIncreased();
            
            // 根据报文延迟重新计算时隙大小 trunc()函数将截取浮点数的整数部
            if (delay >= upperLimitTime) {
                gateController->setisIncreased(true);
                target_time_interval = time_interval.trunc(SIMTIME_US) + increasesteplength;
                if( target_time_interval >= schedule_cycle * 0.9){
                    // 两端预留10%
                    target_time_interval = schedule_cycle * 0.9;
                }else if((target_time_interval - current_interval >= maxIncreasesteplength) || \
                         (current_interval - target_time_interval >= maxIncreasesteplength)) {
                    target_time_interval = current_interval + maxIncreasesteplength;
                }
            }
            if (delay <= lowerLimitTime && isIncreased == false ) {
                target_time_interval = time_interval.trunc(SIMTIME_US) - decreasesteplength;
                if( target_time_interval <= schedule_cycle * 0.1){
                    // 两端预留10%
                    target_time_interval = schedule_cycle * 0.1;
                }else if((target_time_interval - current_interval >= maxDecreasesteplength) || \
                         (current_interval - target_time_interval >= maxDecreasesteplength)) {
                    target_time_interval = current_interval - maxDecreasesteplength; 
                }
            } 

            // 更新GCL
            newSchedule->setTimeInterval(currentscheduleIndex , target_time_interval.trunc(SIMTIME_US));
            newSchedule->setTimeInterval(nextscheduleIndex , schedule_cycle-target_time_interval.trunc(SIMTIME_US));

            // 每次写入当前时间间隔大小
            this->result_file << "{ \"time\": "<<simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"pcp=7-interval\": " << target_time_interval.trunc(SIMTIME_US) << ", \"pcp<7-interval\": " << schedule_cycle - target_time_interval.trunc(SIMTIME_US) << " } "<< endl;
        }

        // 原receivePacket()函数功能
        EV_INFO << "Received packet `" << msg->getName() << "' length= " << msg->getByteLength() << "B\n";
        packetsReceived++;
        emit(packetReceivedSignal, msg);
        delete msg;
    }

} // namespace nesting
