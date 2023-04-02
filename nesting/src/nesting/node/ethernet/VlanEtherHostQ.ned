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
import nesting.application.ethernet.VlanEtherTrafGen;
import nesting.common.time.IClock;
import nesting.common.time.IOscillator;


//
// @deprecated
//
// Simple Host with the capability to create and receive VLAN tagged ethernet
// traffic. Mainly used for test purposes.
//
// @see ~VlanEtherTrafGen
//
module VlanEtherHostQ
{
    parameters:
        @display("i=device/pc2;bgb=1023.26404,828.894");
        @networkNode();
        @labels(node,ethernet-node);
        **.clockModule = default(absPath(".legacyClock"));
        **.oscillatorModule = default(absPath(".oscillator"));
        **.interfaceTableModule = default(absPath(".interfaceTable"));
    gates:
        inout ethg @labels(EtherFrame-conn);
    submodules:
        trafGenApp: VlanEtherTrafGen {
            @display("p=535.458,168.036;is=vl");
        }
        oscillator: <default("IdealOscillator")> like IOscillator {
            @display("p=211.926,323.532");
        }
        legacyClock: <default("LegacyClock")> like IClock {
            @display("p=211.926,156.75");
        }
        eth: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=536.712,546.744,row,150;q=txQueue;is=vl");
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
            @display("p=211.926,499.092;is=l");
        }
    connections:
        eth.upperLayerIn <-- trafGenApp.out;
        eth.upperLayerOut --> trafGenApp.in;
        eth.phys <--> ethg;
}
