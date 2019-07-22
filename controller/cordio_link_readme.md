Cordio Link Release Notes
=========================

Legal
-----

Copyright (c) 2013-2019 Arm Ltd.
Copyright (c) 2019 Packetcraft, Inc.

Compliance
----------

This release complies with the Bluetooth Core.TCRL.2018-2 definitions using the following test
specification versions:

    * LL.TS.5.1.0, Link Layer (LL) Bluetooth Test Specification
    * HCI.TS.5.1.0, Host Controller Interface (HCI) Bluetooth Test Specification

Compliance is verified with the Ellisys EBQ compliance tester using the following version:

    * EBQ 2017-2.6969
      Note : The following test cases will fail because of tester issue : Sync packet offset calculation is wrong in LL_PERIODIC_SYNC_IND PDU when offset_adjust field is 1.
             LL/CON/MAS/BV-99-C, LL/CON/MAS/BV-100-C, LL/CON/MAS/BV-101-C, LL/CON/SLA/BV-103-C, LL/CON/SLA/BV-104-C, LL/CON/SLA/BV-105-C

Compliance is verified with the Harmony compliance tester using the following version:

    * 19.1.16916.18388


Changes
-------

The following changes were made in r19.02

Enhancements in this release:

    WBUSW-2402  MEP16 Allow ADI field to be optional in the AUX_SCAN_RSP
    WBUSW-2938  Enable ADI field by default in the AUX_SCAN_RSP after Madrid spec is adopted
    WBUSW-3108  Update HCI definition and LL definition to conform Madrid spec
    WBUSW-3139  CIS master needs to send additional ack if there is still subevent
    WBUSW-3144  PAST implementation update for Madrid version r15
    WBUSW-3193  Add support of adjusting CIS offset in the LL_CIS_RSP on the CIS slave
    WBUSW-3195  Add support of multiple CIS streams phase I
    WBUSW-3203  Add op flag for CIS master to skip sending additional NULL PDU
    WBUSW-3208  LE Extended Advertising Enable returns incorrect status when no scan response data provided
    WBUSW-3227  MEP9 HCI support for debug keys in LE Secure Connections
    WBUSW-3228  Add Opflag to optionally include/exclude advertising address in AUX_ADV_IND PDU
    WBUSW-3229  Add opflag to include/exclude the ADI field in AUX_SCAN_RSP
    WBUSW-3242  Add test command LE Modify Sleep Clock Accuracy command
    WBUSW-3256  Add support of multiple CIS stream phases II
    WBUSW-3258  Add test command to change the advertising address in AUX_CONNECT_RSP
    WBUSW-3261  Update window widening calculation algorithm
    WBUSW-3268  Add support of verifying connection parameter update on the slave when the instance is the current connection event
    WBUSW-3279  Add check for HCI_LE_Periodic_Adv_Enable
    WBUSW-3281  Add check for HCI_LE_Extended_Create_Connection
    WBUSW-3292  Add support of multiple CIS streams phase III
    WBUSW-3324  Update LL_CIS_EST_EVT to spec version 18a
    WBUSW-3327  Update HCI_SET_CIG_PARAMETER to spec version 18a
    WBUSW-3331  Update LL_CIS_REQ PDU to spec version 18a
    WBUSW-3333  Add support of checking if random address is initialized for some HCI command
    WBUSW-3348  Add interleaved packing scheme for multiple CISs

Resolved defects in this release:

    WBUSW-2981  Connection fails to be established with coded phy when connection interval is smaller than 30ms
    WBUSW-3080  HCI_LE_Periodic_Advertising_Sync_Established event has wrong info in it
    WBUSW-3103  LL_PERIODIC_SYNC_IND PDU has wrong info in it
    WBUSW-3119  One variable size definition is incorrect
    WBUSW-3120  CIS slave sometime transmits at the incorrect timing
    WBUSW-3141  ACL sometimes gets lost when CIS is present
    WBUSW-3142  CIS crashes sometimes when doing uni-directional data transfer
    WBUSW-3154  Flush scheme sometimes doesn't work correctly on the slave when RX timeout happens
    WBUSW-3167  Advertiser cannot handle connection indication with connection interval of 0
    WBUSW-3177  Return incorrect error code when creating connection with invalid interval
    WBUSW-3199  Sometime CIS bod is incorrectly terminated twice on master or slave
    WBUSW-3206  Sometimes there is memory leak for the CIS master Rx operation
    WBUSW-3213  HCI_LE_Periodic_Advertising_Sync_Transfer_Received event is sent too early
    WBUSW-3216  CIS link drops when CIS bod can't be executed in time
    WBUSW-3218  Wrong address type in the LL_PERIODIC_SYNC_IND PDU when address is RPA
    WBUSW-3219  Local RPA sometimes is incorrect for the enhanced connection complete event
    WBUSW-3222  ADV_SCAN_IND is not using RPA for advertising address when local IRK is provided
    WBUSW-3235  Failed to create sync with RPA address
    WBUSW-3260  Not enough memory for PCA10040 with large periodic advertising data size
    WBUSW-3262  High duty cycle connectable directed advertising shall use fixed sequential channels for primary advertising operation
    WBUSW-3280  Wrong address type in LE_PER_ADV_SYNC_TRANS_REC_EVT when RPA address is used
    WBUSW-3291  Seeing unexpected periodic advertising sync lost event
    WBUSW-3295  HCI_Reset is not completed when periodic sync transfer is pending
    WBUSW-3304  Incomplete Periodic Advertising Report event is received
    WBUSW-3306  Assertion error when dynamically update extended advertising data
    WBUSW-3307  Tx PDU sometimes is not cleaned up correctly after it is being flushed on CIS slave
    WBUSW-3314  Failed to disable advertising for multiple advertising sets
    WBUSW-3338  Flush scheme doesn't work for the second CIS in sequential packing scheme with multiple CIS.
    WBUSW-3363  Buffer is not freed correctly sometimes when restarting advertising
    WBUSW-3369  Command complete event for scan disable is received too late in some cases
    WBUSW-3376  IUT receives incorrect status for LE_Periodic_Advertising_Sync_Transfer_Received event
    WBUSW-3418  Connection update with invalid handle returns incorrect status
    WBUSW-3420  HCI_LE_Set_PHY with invalid handle returns invalid status
    WBUSW-3423  HCI LE Set Data Length with invalid handle returns incorrect status
    WBUSW-3451  CIS slave sometimes fails to receive
    WBUSW-3479  There is unused value in the code
    WBUSW-3505  Intermittent periodic sync lost when PAST transfer happens back to back
    WBUSW-3511  Scheduling conflict between AUX ADV and periodic ADV is not handled correctly

Known limitations in this release:

    WBUSW-3183  Scanner fails to receive scan request intermittently when scan interval and scan window are same
    WBUSW-3367  Primary scan fails to receive ADV packets intermittently when 6 periodic scan is ongoing with maximum periodic data size

Incomplete features in this release:

    None
