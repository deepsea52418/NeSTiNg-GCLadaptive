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

simple VlanEtherTrafGenGCL extends EtherTrafGen
{
    parameters:
        @class(VlanEtherTrafGenGCL);
        bool vlanTagEnabled = default(true);	// When true, the application's packets will be VLAN encapsulated by the VLANEncap module
        int pcp = default(0);					// PCP (Priority Code Point) value that is added to the packets control information
        bool dei = default(false);				// DEI (Drop Eligible Indicator) that is added to the packets control information
        int vid = default(1);					// VID (VLAN Identifier) value that is added to the packets control information
        bool verbose = default(false);
        string gateControllerModule = default(""); // Path to the gate controller module
        string result_file_location = default("VlanEtherTrafGenGCL-result"); // 默认的结果文件
}
