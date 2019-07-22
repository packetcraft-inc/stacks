Cordio Mesh Release Notes
=========================

Legal
-----

Copyright (c) 2018-2019 Arm Ltd.
Copyright (c) 2019 Packetcraft, Inc.


Changes
-------

The following changes were made in r19.02

Enhancements in this release:

    WBUSW-3197  Create Mesh Provisioner App GUI
    WBUSW-3282  Create Mesh and Mesh Model Event ranges
    WBUSW-3329  Integrate MM-014 from Github

Resolved defects in this release:

    WBUSW-3321  Fix Mesh Coverity Issues
    WBUSW-3337  Add Provisioning Fail Reason to WSF message header status
    WBUSW-3373  Mesh should have build-time choice of ECC implementation
    WBUSW-3403  Mesh Proxy MTU should be determined by ATT MTU
    WBUSW-3411  Set status and other fields of Mesh header events
    WBUSW-3415  Fail to build mesh apps using Keil
    WBUSW-3416  Fix Mesh Unit Tests

Known limitations in this release:

    None

Incomplete features in this release:

    None


Compliance
----------

Compliance is verified with the Profile Tuning Suites (PTS) compliance tester using the following version:

    * PTS 7.3.0

The following limitations exist with this version of the PTS:

    * MESH/NODE/CFG/HBS/BV-02-C: (TSE ID 11180) Heartbeat Subscription Get always receives back CountLog=0x00 when heartbeat subscription has been disabled
    * MMDL/SR/LHSL/BV-08-C: (TSE ID 10735) OnPowerUp=0x00 does not mean Default for Light Lightness in Light HSL
    * MESH/NODE/FRND/FN/BV-19-C: (CASE0047010) Test fails with error message "The IUT should not transmit more than one Network PDU to LPN" even IUT sends the right PDU
    * MESH/NODE/IVU/BV-04-C: (CASE0047932) Secure Network Beacon with IVU=0 is sent too early and just once
