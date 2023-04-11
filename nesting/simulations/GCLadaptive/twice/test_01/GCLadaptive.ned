package nesting.simulations.GCLadaptive.twice.test_01;

import ned.DatarateChannel;
import nesting.node.ethernet.VlanEtherHostQ;
import nesting.node.ethernet.VlanEtherHostSched;
import nesting.node.ethernet.VlanEtherHostQGCL;
import nesting.node.ethernet.VlanEtherHostQGCL2;
import nesting.node.ethernet.VlanEtherSwitchPreemptable;


network GCLadaptive
{
    @display("bgb=6443.299,4345.224");
    types:
        channel C extends DatarateChannel
        {
            delay = 0.1us;
            datarate = 1Gbps;
        }
    submodules:
        switchA: VlanEtherSwitchPreemptable {
            parameters:
                @display("p=2396.2227,1695.0238;is=vl");
            gates:
                ethg[13];
        }
        switchB: VlanEtherSwitchPreemptable {
            parameters:
                @display("p=4113.3315,1695.0238;is=vl");
            gates:
                ethg[3];
        }
        BEsender1: VlanEtherHostQ {
            @display("p=2517.6902,425.13626");
        }
        BEsender2: VlanEtherHostQ {
            @display("p=3174.719,530.04004");
        }
        TTsender1: VlanEtherHostSched {
            @display("p=1087.6863,574.21");
        }
        TTsender2: VlanEtherHostSched {
            @display("p=419.61502,982.78253");
        }
        TTsender3: VlanEtherHostSched {
            @display("p=419.61502,1568.035");
        }
        TTsender4: VlanEtherHostSched {
            @display("p=369.92377,2092.554");
        }
        TTsender5: VlanEtherHostSched {
            @display("p=419.61502,2694.37");
        }
        TTsender6: VlanEtherHostSched {
            @display("p=369.92377,3588.8127");
        }
        TTsender7: VlanEtherHostSched {
            @display("p=1308.5363,2898.6562");
        }
        TTsender8: VlanEtherHostSched {
            @display("p=1733.6726,3252.0164");
        }
        TTsender9: VlanEtherHostSched {
            @display("p=2429.35,3588.8127");
        }
        TTsender10: VlanEtherHostSched {
            @display("p=3296.1863,3920.0876");
        }
        BEServer: VlanEtherHostQ {
            @display("p=4113.3315,2931.784");
        }
        TT_BEServer: VlanEtherHostQ {
            @display("p=5708.9727,1700.545");
        }
    connections:
        TTsender1.ethg <--> C <--> switchA.ethg[0];
        TTsender2.ethg <--> C <--> switchA.ethg[1];
        TTsender3.ethg <--> C <--> switchA.ethg[5];
        TTsender4.ethg <--> C <--> switchA.ethg[6];
        TTsender5.ethg <--> C <--> switchA.ethg[7];
        TTsender6.ethg <--> C <--> switchA.ethg[8];
        TTsender7.ethg <--> C <--> switchA.ethg[9];
        TTsender8.ethg <--> C <--> switchA.ethg[10];
        TTsender9.ethg <--> C <--> switchA.ethg[11];
        TTsender10.ethg <--> C <--> switchA.ethg[12];
        BEsender1.ethg <--> C <--> switchA.ethg[2];
        BEsender2.ethg <--> C <--> switchA.ethg[3];
        switchA.ethg[4] <--> C <--> switchB.ethg[2];
        TT_BEServer.ethg <--> C <--> switchB.ethg[0];
        BEServer.ethg <--> C <--> switchB.ethg[1];

}
