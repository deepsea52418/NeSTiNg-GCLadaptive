package nesting.simulations.GCLadaptive.single.test_01;

import ned.DatarateChannel;
import nesting.node.ethernet.VlanEtherHostQ;
import nesting.node.ethernet.VlanEtherHostSched;
import nesting.node.ethernet.VlanEtherHostQGCL;
import nesting.node.ethernet.VlanEtherSwitchPreemptable;


network GCLadaptive
{
    @display("bgb=974,724");
    types:
        channel C extends DatarateChannel
        {
            delay = 0.1us;
            datarate = 1Gbps;
        }
    submodules:
        switchA: VlanEtherSwitchPreemptable {
            parameters:
                @display("p=372,315;is=vl");
            gates:
                ethg[4];
        }
        workstation1: VlanEtherHostQ {
            @display("p=109,316");
        }
        workstation2: VlanEtherHostQ {
            @display("p=372,68");
        }
        robotController: VlanEtherHostSched {
            @display("p=372,564");
        }
        roboticArm: VlanEtherHostQ {
            @display("p=752,316");
        }
    connections:
        robotController.ethg <--> C <--> switchA.ethg[0];
        workstation1.ethg <--> C <--> switchA.ethg[1];
        workstation2.ethg <--> C <--> switchA.ethg[2];
        roboticArm.ethg <--> C <--> switchA.ethg[3];

}
