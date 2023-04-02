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
import nesting.application.ethernet.VlanEtherTrafGenSched;
import nesting.common.time.IClock;
import nesting.common.time.IOscillator;


//
// @deprecated
//
// Simple Host with the capability to send out frames according to a given schedule
//
// @see ~VlanEtherTrafGen
//
module VlanEtherHostSched
{
    parameters:
        string fcsMode @enum("declared","computed") = default("declared");
        @networkNode();
        @labels(node,ethernet-node);
        @display("i=device/pc2;bgb=626,596");
        *.fcsMode = fcsMode;
        **.clockModule = default(absPath(".legacyClock"));
        **.oscillatorModule = default(absPath(".oscillator"));
        **.interfaceTableModule = default(absPath(".interfaceTable"));
    gates:
        inout ethg @labels(EtherFrame-conn);
    submodules:
        trafGenSchedApp: VlanEtherTrafGenSched {
            @display("p=208,171;is=vl");
        }
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=453,342;is=l");
        }
        oscillator: <default("IdealOscillator")> like IOscillator {
            @display("p=453,86;is=l");
        }
        legacyClock: <default("LegacyClock")> like IClock {
            @display("p=453,202;is=l");
        }
        eth: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=208,454,row,150;q=txQueue;is=vl");
                encap.typename = "EtherEncap";
                qEncap.typename = "EnhancedIeee8021qEncap";
                mac.typename = "EtherMACFullDuplexPreemptable";
                mac.mtu = 1500B;
                queue.typename = "Queuing";
                queue.tsAlgorithms[*].macModule = absPath(".mac");
                queue.gateController.macModule = absPath(".mac");
                queue.numberOfQueues = 1;
        }
    connections:
        trafGenSchedApp.out --> eth.upperLayerIn;
        eth.upperLayerOut --> trafGenSchedApp.in;

        eth.phys <--> ethg;
}
