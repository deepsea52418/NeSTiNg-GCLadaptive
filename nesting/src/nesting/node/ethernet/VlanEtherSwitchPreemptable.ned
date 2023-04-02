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

import inet.common.MessageDispatcher;
import inet.common.queue.Delayer;
import inet.linklayer.contract.IMacRelayUnit;
import inet.linklayer.ethernet.EthernetInterface;
import inet.networklayer.common.InterfaceTable;
import nesting.common.time.IClock;
import nesting.common.time.IClock2;
import nesting.common.time.IOscillator;
import nesting.ieee8021q.relay.FilteringDatabase;


//
// This module implements a switch that supports frame preemption.
//
module VlanEtherSwitchPreemptable
{
    parameters:
        @networkNode();
        @display("i=device/switch;bgb=3095.2258,1866.2");
        **.vlanTagType = default("c");
        **.interfaceTableModule = default(absPath(".interfaceTable"));
        **.filteringDatabaseModule = default(absPath(".filteringDatabase"));
        **.clockModule = default(absPath(".legacyClock"));
        **.oscillatorModule = default(absPath(".oscillator"));
    gates:
        inout ethg[];
    submodules:
        relayUnit: <default("ForwardingRelayUnit")> like IMacRelayUnit {
            numberOfPorts = sizeof(ethg);
            @display("p=1325.002,789.136;is=l");
        }
        processingDelay: Delayer {
            @display("p=2103.4739,789.136,r,200;i=block/delay;is=l");
            delay = default(4us);
        }
        oscillator: <default("IdealOscillator")> like IOscillator {
            @display("p=413.22998,789.136;is=l");
        }
        legacyClock: <default("LegacyClock")> like IClock {
            @display("p=413.22998,1119.72;is=l");
        }
        clock: <default("RealtimeClock")> like IClock2 {
            @display("p=429.22598,1490.294;is=l");
        }
        filteringDatabase: FilteringDatabase {
            @display("p=429.22598,493.21;is=l");
        }
        interfaceTable: InterfaceTable {
            @display("p=458.55197,170.624;is=l");
        }
        up: MessageDispatcher {
            parameters:
                @display("p=1325.002,199.95;b=1200,5");
        }
        down: MessageDispatcher {
            parameters:
                @display("p=1325.002,1197.0339;b=1200,5");
        }
        eth[sizeof(ethg)]: EthernetInterface {
            mac.typename = "EtherMACFullDuplexPreemptable";
            encap.typename = "EtherEncapDummy";
            qEncap.typename = "Ieee8021qEncap";
            queue.typename = "Queuing";
            mac.mtu = 1500B;
            mac.promiscuous = true;
            queue.tsAlgorithms[*].macModule = absPath(".mac");
            queue.gateController.macModule = absPath(".mac");
            @display("p=1325.002,1540.948,r,150;is=vl");
        }
    connections:
        for i=0..sizeof(ethg)-1 {
            eth[i].phys <--> ethg[i];
            eth[i].upperLayerOut --> down.in++;
            down.out++ --> eth[i].upperLayerIn;
        }
        down.out++ --> relayUnit.ifIn;
        relayUnit.ifOut --> processingDelay.in;
        processingDelay.out --> down.in++;
        relayUnit.upperLayerOut --> up.in++;
        relayUnit.upperLayerIn <-- up.out++;
}

