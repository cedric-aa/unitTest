Mesh_CB
##########################################
Overview
********
Mesh_CB is a device that includes three (BLE) models: 
- btMeshUnitControl, 
- btMeshActivation,
- levelMotorServer. 
The purpose of Mesh_CB is purpose is to bridge the communication between the CB and devices within the mesh network,
It receives commands from the CB via UART and sends them over BLE to other devices within the mesh network.
It also receives messages from the mesh network via BLE and sends them to the CB via UART.

Mesh provisioning
=================


Mesh models
===========

The following table shows the mesh light composition data for this sample:

   +---------------------+  +---------------------+                         +---------------------+
   |  Element 1          |    Element  2          |                         | Element 10          |
   +=====================+  +---------------------+                         +---------------------+
   | Config Server       |    mortorLevel         |   +--------------->     | mortorLevel         |
   +---------------------+  +---------------------+                         +---------------------+
   | Health Server       |                       
   +---------------------+  
   | btMeshUnitControl   |                         
   +---------------------+
   | btMeshActivation    |
   +---------------------+
   | mortorLevel         |
   +---------------------+
