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

#include <algorithm>

#include "nesting/linklayer/ieee8021q/EnhancedIeee8021qEncap.h"
#include "nesting/linklayer/vlan/EnhancedVlanTag_m.h"

#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"
// 增加TimeTag_m头文件，实现计算延迟
#include "inet/common/TimeTag_m.h"

namespace nesting {

Define_Module(EnhancedIeee8021qEncap);

void EnhancedIeee8021qEncap::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        vlanTagType = par("vlanTagType");
        parseParameters("inboundVlanIdFilter", "inboundVlanIdMap", inboundVlanIdFilter, inboundVlanIdMap);
        parseParameters("outboundVlanIdFilter", "outboundVlanIdMap", outboundVlanIdFilter, outboundVlanIdMap);
        WATCH_VECTOR(inboundVlanIdFilter);
        WATCH_MAP(inboundVlanIdMap);
        WATCH_VECTOR(outboundVlanIdFilter);
        WATCH_MAP(outboundVlanIdMap);
        tagTcpTraffic = par("tagTcpTraffic");
        // 增加结果统计
        bool hasTrafficRecorder = (&par("hasTrafficRecorder")) -> boolValue();
        if (hasTrafficRecorder) {
            string filename = (&par("result_file_location")) -> stringValue();
            this->result_file.open(filename, ios::out | ios::trunc);
        }

    }
}

void EnhancedIeee8021qEncap::parseParameters(const char *filterParameterName, const char *mapParameterName, std::vector<int>& vlanIdFilter, std::map<int, int>& vlanIdMap)
{
    cStringTokenizer filterTokenizer(par(filterParameterName));
    while (filterTokenizer.hasMoreTokens())
        vlanIdFilter.push_back(atoi(filterTokenizer.nextToken()));
    cStringTokenizer mapTokenizer(par(mapParameterName));
    while (mapTokenizer.hasMoreTokens()) {
        auto fromVlanId = atoi(mapTokenizer.nextToken());
        auto toVlanId = atoi(mapTokenizer.nextToken());
        vlanIdMap[fromVlanId] = toVlanId;
    }
}

Ieee8021qHeader *EnhancedIeee8021qEncap::findVlanTag(const Ptr<EthernetMacHeader>& ethernetMacHeader)
{
    if (*vlanTagType == 'c')
        return ethernetMacHeader->getCTagForUpdate();
    else if (*vlanTagType == 's')
        return ethernetMacHeader->getSTagForUpdate();
    else
        throw cRuntimeError("Unknown VLAN tag type");
}

Ieee8021qHeader *EnhancedIeee8021qEncap::addVlanTag(const Ptr<EthernetMacHeader>& ethernetMacHeader)
{
    auto vlanTag = new Ieee8021qHeader();
    ethernetMacHeader->setChunkLength(ethernetMacHeader->getChunkLength() + B(4));
    if (*vlanTagType == 'c')
        ethernetMacHeader->setCTag(vlanTag);
    else if (*vlanTagType == 's')
        ethernetMacHeader->setSTag(vlanTag);
    else
        throw cRuntimeError("Unknown VLAN tag type");
    return vlanTag;
}

Ieee8021qHeader *EnhancedIeee8021qEncap::removeVlanTag(const Ptr<EthernetMacHeader>& ethernetMacHeader)
{
    ethernetMacHeader->setChunkLength(ethernetMacHeader->getChunkLength() - B(4));
    if (*vlanTagType == 'c')
        return ethernetMacHeader->dropCTag();
    else if (*vlanTagType == 's')
        return ethernetMacHeader->dropSTag();
    else
        throw cRuntimeError("Unknown VLAN tag type");
}

void EnhancedIeee8021qEncap::processPacket(Packet *packet, std::vector<int>& vlanIdFilter, std::map<int, int>& vlanIdMap, cGate *gate)
{
    // 提取报文中的CreationTimeTag,计算延迟
    auto data = packet->peekData();  
    auto regions = data->getAllTags<CreationTimeTag>(); 
    SimTime delay;
    for (auto& region : regions) { 
        auto creationTime = region.getTag()->getCreationTime(); 
        delay = simTime() - creationTime; 
    }
    
    packet->trimFront();
    const auto& ethernetMacHeader = packet->removeAtFront<EthernetMacHeader>();

    Ieee8021qHeader *vlanTag = findVlanTag(ethernetMacHeader);
    auto oldVlanId = -1;
    auto oldPcp = 0;
    auto oldDe = false;
    if (vlanTag != nullptr) {
        oldVlanId = vlanTag->getVid();
        oldPcp = vlanTag->getPcp();
        oldDe = vlanTag->getDe();
    }

    auto vlanReq = packet->removeTagIfPresent<EnhancedVlanReq>();
    auto newVlanId = oldVlanId;
    auto newPcp = oldPcp;
    auto newDe = oldDe;
    if (vlanReq != nullptr) {
        newVlanId = vlanReq->getVlanId();
        newPcp = vlanReq->getPcp();
        newDe = vlanReq->getDe();
    }
    // Tag TCP segments with PCP value equal to port number module 8
    else if (tagTcpTraffic && packet->getArrivalGate()->isName("upperLayerIn")) {
        auto transportProtocolInd = packet->findTag<TransportProtocolInd>();
        if (transportProtocolInd != nullptr) {
            using namespace inet::tcp;
            auto tcpHeader = dynamicPtrCast<const TcpHeader, const Chunk>(transportProtocolInd->getTransportProtocolHeader());
            newPcp = tcpHeader->getDestPort() % 8;
            if (vlanReq == nullptr) {
                vlanReq = new EnhancedVlanReq();
            }
        }
    }
    // nesting 是做了简单的Qci协议的，通过outboundVlanIdMap/inboundVlanIdMap参数，判断规定的vid，只有符合规定的vid可以传输。也做了VID的转换，应该就是通过这一行实现的
    bool acceptPacket = vlanIdFilter.empty() || std::find(vlanIdFilter.begin(), vlanIdFilter.end(), newVlanId) != vlanIdFilter.end();
    if (acceptPacket) {
        // Translate VLAN id.
        // 在vlanIdMap里面寻找vid隐射规则，如果找到了就按照规则写vid
        auto it = vlanIdMap.find(newVlanId);
        if (it != vlanIdMap.end()) {
            newVlanId = it->second;
        }

        // (1) If there is a request primitive and the frame is untagged, we have to tag it.
        // (2) or when the VLAN id gets translated.
        if ((vlanReq != nullptr && vlanTag == nullptr)
                || (vlanTag == nullptr && oldVlanId == -1 && newVlanId != -1)) {
            vlanTag = addVlanTag(ethernetMacHeader);
        }

        // If the frame has a vlan tag we update its values.
        if (vlanTag != nullptr) {
            // Set VLAN ID.
            if (newVlanId != oldVlanId) {
                EV_WARN << "Changing VLAN ID: new = " << newVlanId << ", old = " << oldVlanId << ".\n";
            }
            vlanTag->setVid(newVlanId);
            vlanTag->setPcp(newPcp);
            vlanTag->setDe(newDe);

            // Update FCS.
            packet->insertAtFront(ethernetMacHeader);
            auto oldFcs = packet->removeAtBack<EthernetFcs>();
            EtherEncap::addFcs(packet, oldFcs->getFcsMode());
        } else {
            packet->insertAtFront(ethernetMacHeader);
        }

        // Add indication primitive.
        auto vlanInd = packet->addTagIfAbsent<EnhancedVlanInd>();
        vlanInd->setVlanId(newVlanId);
        vlanInd->setPcp(newPcp);
        
        // 增加结果记录功能
        if (hasTrafficRecorder){
            this->result_file << "{ \"time\": "<<simTime() << ", \"src\": \"" << ethernetMacHeader->getSrc() << "\""\
            << ", \"dest\": \"" << ethernetMacHeader->getDest() << "\"" \
            << ", \"pcp\": " << newPcp << ", \"vid\": " << newVlanId \
            << ", \"delay\": " << delay << " } "<< endl;
        }
        
        send(packet, gate);
    } else {
        EV_WARN << "Received VLAN ID = " << oldVlanId << " is not accepted, dropping packet.\n";
        PacketDropDetails details;
        details.setReason(OTHER_PACKET_DROP);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

void EnhancedIeee8021qEncap::handleMessage(cMessage *message)
{
    if (message->getArrivalGate()->isName("upperLayerIn"))
        processPacket(check_and_cast<Packet *>(message), outboundVlanIdFilter, outboundVlanIdMap, gate("lowerLayerOut"));
    else if (message->getArrivalGate()->isName("lowerLayerIn"))
        processPacket(check_and_cast<Packet *>(message), inboundVlanIdFilter, inboundVlanIdMap, gate("upperLayerOut"));
    else
        throw cRuntimeError("Unknown message");
}

} // namespace nesting

