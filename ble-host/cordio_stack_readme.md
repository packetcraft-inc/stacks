Cordio Stack Release Notes
==========================

Legal
-----

Copyright (c) 2013-2019 Arm Ltd.
Copyright (c) 2019 Packetcraft, Inc.


Changes
-------

The following changes were made in r19.02

Enhancements in this release:

    WBUSW-2543  Implement a GATT Pending Mechanism
    WBUSW-2848  Add generic dual-chip serial transport sample code
    WBUSW-2977  Add Core Spec Minor Functional Enhancements Batch 1 to BLE Host
    WBUSW-3068  SMP DB needs better solution for case where DB is full
    WBUSW-3187  Add Angle of Arrival (AoA) Feature Support to BLE Host
    WBUSW-3194  HCI_LE_RAND_CMD_CMPL_CBACK_EVT data shift is inefficient.
    WBUSW-3230  Use Ceiling of Advertising Duration with Extended Advertising
    WBUSW-3330  Add delayed response to ATT write

Resolved defects in this release:

    WBUSW-2873  Set n/a parameters' value to 0 for ATTS_DB_HASH_CMAC_CMPL in att callback
    WBUSW-3201  Issues in Stack and Profiles Coverity
    WBUSW-3288  Shadowed declaration in function smpScActCalcSharedSecre

Known limitations in this release:

    None

Incomplete features in this release:

    None


Compliance
----------

This release complies with the Bluetooth Core.TCRL.2017-2 definitions using the following test
specification versions:

    * GAP.TS.5.0.2
    * L2CAP.TS.5.0.2
    * GATT.TS.5.0.2
    * SMP.TS.5.0.2
