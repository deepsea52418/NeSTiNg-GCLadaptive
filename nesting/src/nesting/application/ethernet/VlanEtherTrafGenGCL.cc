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

#include "nesting/application/ethernet/VlanEtherTrafGenGCL.h"
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

    Define_Module(VlanEtherTrafGenGCL);
    void VlanEtherTrafGenGCL::initialize(int stage) {
        EtherTrafGen::initialize(stage);
        if (stage == INITSTAGE_LOCAL) {
            vlanTagEnabled = &par("vlanTagEnabled");
            pcp = &par("pcp");
            dei = &par("dei");
            vid = &par("vid");
            gateController = getModuleFromPar<GateController>(par("gateControllerModule"), this);
            string filename = (&par("result_file_location"))->stringValue() ;
            this->result_file.open(filename, ios::out | ios::trunc);
        } else if (stage == INITSTAGE_LINK_LAYER) {
            registerService(*VlanEtherTrafGenSched::L2_PROTOCOL, nullptr, gate("in"));
            registerProtocol(*VlanEtherTrafGenSched::L2_PROTOCOL, gate("out"), nullptr);
        }
    }

    void VlanEtherTrafGenGCL::sendBurstPackets() {
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

    void VlanEtherTrafGenGCL::receivePacket(Packet *msg)
    {
        // 对其父类EtherTrafGen的receivePacket()函数进行重新，并修改其父类，使其可以动态链接

        // 计算报文延迟，这段代码参考的其他函数写的
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
        // 判断当前报文是否是TT流，若不是TT流，则不进行调节
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 ){
            // 获取gatecontroller中的newSchedule对象
            newSchedule = gateController ->getnewSchedule();
            simtime_t schedule_cycle = newSchedule->getCycleTime();
            // 获取当前Index
            unsigned int currentscheduleIndex = gateController->getscheduleIndex();
            // 获取下一个Index
            unsigned int nextscheduleIndex = (currentscheduleIndex + 1) % newSchedule->getControlListLength();
            // 获取当前时隙大小
            simtime_t time_interval = newSchedule->getTimeInterval(currentscheduleIndex);
            simtime_t target_time_interval = time_interval;
            // 设置调节步长
            int upsteplength = 10;
            int downsteplength = 10;
            // 获取gatecontroller的isIncreased参数，判断当前的newSchedule的TT时隙有没有被增大，如果没有被增大，可以被变小
            bool isIncreased = gateController->getisIncreased();
            // 根据报文延迟重新计算时隙大小
            // trunc()函数将截取浮点数的整数部
            // 设置触发调节上限为延迟>75us
            if (delay >= SimTime(25, SIMTIME_US)) {
                gateController->setisIncreased(true);
                if ((time_interval.trunc(SIMTIME_US) + SimTime(upsteplength, SIMTIME_US)) >= (schedule_cycle * 0.9)){
                    target_time_interval = schedule_cycle * 0.9;
                    EV_INFO<<"here 1    "<< target_time_interval << "currentscheduleIndex =  "<< currentscheduleIndex <<endl;
                }else {
                    target_time_interval = time_interval.trunc(SIMTIME_US) + SimTime(upsteplength, SIMTIME_US);
                    EV_INFO<<"here 2    "<< target_time_interval <<  "currentscheduleIndex =  "<< currentscheduleIndex <<endl;
                }
            }
            // 设置触发调节上限为延迟<45us
            if (delay <= SimTime(17, SIMTIME_US) && isIncreased == false ) {
                if ((time_interval.trunc(SIMTIME_US) - SimTime(downsteplength, SIMTIME_US)) <= (schedule_cycle * 0.1)){
                    target_time_interval = schedule_cycle * 0.1;
                    EV_INFO<<"here 3    "<< target_time_interval << "currentscheduleIndex =  "<< currentscheduleIndex << endl;
                }else {
                    target_time_interval = time_interval.trunc(SIMTIME_US) - SimTime(downsteplength, SIMTIME_US);
                    EV_INFO<<"here 4    "<< target_time_interval <<  "currentscheduleIndex =  "<< currentscheduleIndex <<endl;
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
