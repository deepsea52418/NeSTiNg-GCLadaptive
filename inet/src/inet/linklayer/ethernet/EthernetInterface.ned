//
// Copyright (C) 2004 Andras Varga
//               2010 Zoltan Bojthe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package inet.linklayer.ethernet;

import inet.common.queue.IOutputQueue;
import inet.linklayer.contract.IEtherMac;
import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.contract.ITrafficConditioner;
import inet.linklayer.ieee8021q.IIeee8021qEncap;
import inet.networklayer.common.InterfaceEntry;

//
// Ethernet network interface. Complements ~EtherMac and ~EtherEncap
// with an output queue for QoS and RED support.
//
module EthernetInterface extends InterfaceEntry like IEthernetInterface
{
    parameters:
        @class(InterfaceEntry);
        @lifecycleSupport;
        string interfaceTableModule;
        string address = default("auto");   // MAC address as hex string (12 hex digits), or
                                            // "auto". "auto" values will be replaced by
                                            // a generated MAC address in init stage 0.
        bool csmacdSupport = default(false);  // by default CSMA/CD is turned off, so only point-to-point duplex links are supported.
        string fcsMode @enum("declared","computed") = default("declared");
        @display("i=block/ifcard;bgl=2;bgb=886,891");
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        *.fcsMode = fcsMode;
    gates:
        input upperLayerIn;
        output upperLayerOut;
        inout phys @labels(EtherFrame);
    submodules:
        ingressTC: <default("")> like ITrafficConditioner if typename != "" {
            @display("p=656,167;is=vl");
        }
        egressTC: <default("")> like ITrafficConditioner if typename != "" {
            @display("p=128,167;is=vl");
        }
        queue: <default("")> like IOutputQueue if typename != "" {
            parameters:
                @display("p=178,588;q=l2queue;is=vl");
        }
        mac: <default(csmacdSupport ? "EtherMac" : "EtherMacFullDuplex")> like IEtherMac {
            parameters:
                queueModule = (exists(queue) ? "^.queue" : "");
                txQueueLimit = (exists(queue) ? 1 : 10000); // queue sends one packet at a time
                @display("p=445,698;is=vl");
        }
        encap: <default("EtherEncap")> like IEtherEncap {
            parameters:
                @display("p=445,288;is=vl");
        }
        qEncap: <default("")> like IIeee8021qEncap if typename != "" {
            parameters:
                @display("p=178,353;is=vl");
        }
    connections:
        mac.upperLayerOut --> qEncap.lowerLayerIn if exists(qEncap);
        mac.upperLayerOut --> encap.lowerLayerIn if !exists(qEncap);

        mac.upperLayerIn <-- queue.out if exists(queue);
        mac.upperLayerIn <-- qEncap.lowerLayerOut if !exists(queue) && exists(qEncap);
        mac.upperLayerIn <-- encap.lowerLayerOut if !exists(queue) && !exists(qEncap);

        encap.lowerLayerOut --> qEncap.upperLayerIn if exists(qEncap);
        encap.lowerLayerIn <-- qEncap.upperLayerOut if exists(qEncap);

        qEncap.lowerLayerOut --> queue.in if exists(qEncap) && exists(queue);
        encap.lowerLayerOut --> queue.in if !exists(qEncap) && exists(queue);

        mac.phys <--> { @display("m=s"); } <--> phys;

        // no ingressTC
        encap.upperLayerOut --> { @display("m=n"); } --> upperLayerOut if !exists(ingressTC);

        // there is ingressTC
        if exists(ingressTC) {
            encap.upperLayerOut --> ingressTC.in;
            ingressTC.out --> { @display("m=n"); } --> upperLayerOut;
        }

        // no egressTC
        upperLayerIn --> { @display("m=n"); } --> encap.upperLayerIn if !exists(egressTC);

        // there is egressTC
        if exists(egressTC) {
            upperLayerIn --> { @display("m=n"); } --> egressTC.in;
            egressTC.out --> encap.upperLayerIn;
        }
}

