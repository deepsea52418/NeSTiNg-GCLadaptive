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

package nesting.application.ethernet;

import inet.applications.ethernet.EtherTrafGen;


//
// @deprecated
//
// 该模型在VlanEtherTrafGen基础上加入gatecontroller，可以实现根据E2Edelay动态调整GCL
// 支持两个交换机

simple VlanEtherTrafGenGCL2 extends EtherTrafGen
{
    parameters:
        @class(VlanEtherTrafGenGCL2);
        bool vlanTagEnabled = default(true);	// When true, the application's packets will be VLAN encapsulated by the VLANEncap module
        int pcp = default(0);					// PCP (Priority Code Point) value that is added to the packets control information
        bool dei = default(false);				// DEI (Drop Eligible Indicator) that is added to the packets control information
        int vid = default(1);					// VID (VLAN Identifier) value that is added to the packets control information
        bool verbose = default(false);
        string gateControllerModule_a = default(""); // 交换机a的路径
        string gateControllerModule_b = default(""); // 交换机b的路径
        string result_file_location = default("VlanEtherTrafGenGCL2-result"); // 默认的结果文件
}
