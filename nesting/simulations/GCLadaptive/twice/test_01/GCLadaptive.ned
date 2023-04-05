package nesting.simulations.GCLadaptive.twice.test_01;

import ned.DatarateChannel;
import nesting.node.ethernet.VlanEtherHostQ;
import nesting.node.ethernet.VlanEtherHostSched;
import nesting.node.ethernet.VlanEtherHostQGCL;
import nesting.node.ethernet.VlanEtherHostQGCL2;
import nesting.node.ethernet.VlanEtherSwitchPreemptable;


network GCLadaptive
{
    @display("bgb=4413.3926,2049.3325");
    types:
        channel C extends DatarateChannel
        {
            delay = 0.1us;
            datarate = 1Gbps;
        }
    submodules:
        switchA: VlanEtherSwitchPreemptable {
            parameters:
                @display("p=569.296,314.496;is=vl");
            gates:
                ethg[5];
        }
        switchB: VlanEtherSwitchPreemptable {
            parameters:
                @display("p=1144.416,314.496;is=vl");
            gates:
                ethg[3];
        }
        workstation1: VlanEtherHostQ {
            @display("p=331.96802,706.16003");
        }
        workstation2: VlanEtherHostQ {
            @display("p=824.0575,703.9325");
        }
        robotController1: VlanEtherHostSched {
            @display("p=569.296,72.8");
        }
        robotController2: VlanEtherHostSched {
            @display("p=205.296,315.952");
        }
        backupServer: VlanEtherHostQ {
            @display("p=1143.59,888.925");
        }
        roboticArm: VlanEtherHostQGCL2 {
            @display("p=1563.744,315.952");
            trafGenAppGCL2.gateControllerModule_a = "^.^.switchA.eth[4].queue.gateController";
            trafGenAppGCL2.gateControllerModule_b = "^.^.switchB.eth[0].queue.gateController";
        }
    connections:
        robotController1.ethg <--> C <--> switchA.ethg[0];
        robotController2.ethg <--> C <--> switchA.ethg[1];
        workstation1.ethg <--> C <--> switchA.ethg[2];
        workstation2.ethg <--> C <--> switchA.ethg[3];
        switchA.ethg[4] <--> C <--> switchB.ethg[2];
        roboticArm.ethg <--> C <--> switchB.ethg[0];
        backupServer.ethg <--> C <--> switchB.ethg[1];

}
