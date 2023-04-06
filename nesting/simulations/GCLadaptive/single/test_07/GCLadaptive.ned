package nesting.simulations.GCLadaptive.single.test_07;

import ned.DatarateChannel;
import nesting.node.ethernet.VlanEtherHostQ;
import nesting.node.ethernet.VlanEtherHostSched;
import nesting.node.ethernet.VlanEtherHostQGCL;
import nesting.node.ethernet.VlanEtherSwitchPreemptable;


network GCLadaptive
{
    @display("bgb=2192.736,1161.8881");
    types:
        channel C extends DatarateChannel
        {
            delay = 0.1us;
            datarate = 1Gbps;
        }
    submodules:
        switchA: VlanEtherSwitchPreemptable {
            parameters:
                @display("p=992.992,369.824;is=vl");
            gates:
                ethg[13];
        }
        workstation1: VlanEtherHostQ {
            @display("p=248.976,141.23201");
        }
        workstation2: VlanEtherHostQ {
            @display("p=700.336,72.8");
        }
        robotController1: VlanEtherHostSched {
            @display("p=147.056,428.064");
        }
        robotController2: VlanEtherHostSched {
            @display("p=126.672005,631.904");
        }
        robotController3: VlanEtherHostSched {
            @display("p=192.192,739.648");
        }
        robotController4: VlanEtherHostSched {
            @display("p=240.24,891.072");
        }
        robotController5: VlanEtherHostSched {
            @display("p=372.736,1059.968");
        }
        robotController6: VlanEtherHostSched {
            @display("p=608.60803,918.736");
        }
        robotController7: VlanEtherHostSched {
            @display("p=854.672,739.648");
        }
        robotController8: VlanEtherHostSched {
            @display("p=992.992,861.952");
        }
        robotController9: VlanEtherHostSched {
            @display("p=1112.384,974.064");
        }
        robotController10: VlanEtherHostSched {
            @display("p=1320.592,1030.848");
        }
        roboticArm: VlanEtherHostQ {
            @display("p=1680.224,372.736");
        }
    connections:
        robotController1.ethg <--> C <--> switchA.ethg[0];
        workstation1.ethg <--> C <--> switchA.ethg[1];
        workstation2.ethg <--> C <--> switchA.ethg[2];
        roboticArm.ethg <--> C <--> switchA.ethg[3];
        robotController2.ethg <--> C <--> switchA.ethg[4];
        robotController3.ethg <--> C <--> switchA.ethg[5];
        robotController4.ethg <--> C <--> switchA.ethg[6];
        robotController5.ethg <--> C <--> switchA.ethg[7];
        robotController6.ethg <--> C <--> switchA.ethg[8];
        robotController7.ethg <--> C <--> switchA.ethg[9];
        robotController8.ethg <--> C <--> switchA.ethg[10];
        robotController9.ethg <--> C <--> switchA.ethg[11];
        robotController10.ethg <--> C <--> switchA.ethg[12];

}
