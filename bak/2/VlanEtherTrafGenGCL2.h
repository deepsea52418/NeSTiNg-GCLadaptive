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

#ifndef __NESTING_VETHERTRAFGEN_H
#define __NESTING_VETHERTRAFGEN_H

#include <omnetpp.h>

#include "inet/applications/ethernet/EtherTrafGen.h"
#include "nesting/ieee8021q/queue/gating/GateController.h"

#include <fstream>
#include <iostream>

using namespace omnetpp;
using namespace inet;
using namespace std;

namespace nesting {

class GateController;
/**
 * @deprecated
 * See the NED file for a detailed description
 */
class VlanEtherTrafGenGCL2: public EtherTrafGen {
private:
    // Parameters from NED file
    cPar* vlanTagEnabled;
    cPar* pcp;
    cPar* dei;
    cPar* vid;
    cPar* result_file_location;

    // 调度表存储GCL信息
    Schedule<GateBitvector>* currentSchedule;
    Schedule<GateBitvector>* nextSchedule;
    
    // 结果文件
    fstream result_file;
    // 门控组件
    GateController* gateController;
    GateController* gateController_a;
    GateController* gateController_b;

protected:
    virtual ~VlanEtherTrafGenGCL2();
    virtual void initialize(int stage) override;
    virtual void sendBurstPackets() override;
    // 自写函数
    virtual void receivePacket(Packet *msg) override;
};

} // namespace nesting

#endif /* __NESTING_VETHERTRAFGEN_H */
