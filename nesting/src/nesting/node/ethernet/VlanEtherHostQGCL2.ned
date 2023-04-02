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

package nesting.node.ethernet;

import inet.linklayer.contract.IEthernetInterface;
import inet.networklayer.common.InterfaceTable;
import nesting.application.ethernet.VlanEtherTrafGenGCL2;
import nesting.common.time.IClock;
import nesting.common.time.IOscillator;


//
// 带有gatecontroller应用的终端，可根据PCP=7流量的E2Edelay对GCL进行自适应调节
// 可以控制两个交换机
//
module VlanEtherHostQGCL2
{
    parameters:
        @display("i=device/pc2;bgb=607,623");
        @networkNode();
        @labels(node,ethernet-node);
        **.clockModule = default(absPath(".legacyClock"));
        **.oscillatorModule = default(absPath(".oscillator"));
        **.interfaceTableModule = default(absPath(".interfaceTable"));
    gates:
        inout ethg @labels(EtherFrame-conn);
    submodules:
        trafGenAppGCL2: VlanEtherTrafGenGCL2 {
            @display("p=326,82");
        }
        oscillator: <default("IdealOscillator")> like IOscillator {
            @display("p=119,223;is=s");
        }
        legacyClock: <default("LegacyClock")> like IClock {
            @display("p=135,51;is=s");
        }
        eth: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=297,419,row,150;q=txQueue");
                encap.typename = "EtherEncap";
                qEncap.typename = "EnhancedIeee8021qEncap";
                mac.typename = "EtherMACFullDuplexPreemptable";
                mac.mtu = 1500B;
                queue.typename = "Queuing";
                queue.tsAlgorithms[*].macModule = absPath(".mac");
                queue.gateController.macModule = absPath(".mac");
                queue.numberOfQueues = 1;
        }
        interfaceTable: InterfaceTable {
            @display("p=112,353;is=s");
        }
    connections:
        eth.upperLayerIn <-- trafGenAppGCL2.out;
        eth.upperLayerOut --> trafGenAppGCL2.in;
        eth.phys <--> ethg;
}
