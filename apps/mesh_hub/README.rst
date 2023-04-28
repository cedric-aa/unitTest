Mesh_HUB
###############################
Mesh_HUB is a device that includes (BLE) models: 
-  btMeshUnitControl,
-  btMeshActivation,
-  btMeshLevelMotorCli,
-  btMeshsensorCli

The purpose of Mesh_HUB is purpose is to bridge the communication between the hub and the devices within the mesh network,
It receives commands from the hub via UART and sends them over BLE to other devices within the mesh network.
It also receives messages from the mesh network via BLE and sends them to the hub via UART.

such as:
 - get the Status of the unitControl 
 - get the temperature level from a sensor.
 - set a fullCmd to the unitControl

Provisioning
============


Models
======

The following table shows the mesh sensor observer composition data:

   +-----------------+  +-------------------+
   |  Element 1      |  | Element 2         |
   +=================+  +===================+
   | Config Server   |  |btMeshLevelMotorCli|
   +-----------------+  +-------------------+
   | Health Server   |  | btMeshsensorCli   |
   +-----------------+  +-------------------+
   |btMeshActivation |
   +-----------------+
   |btMeshUnitControl|
   +-----------------+

