Packetcraft Protocol Software
=============================

Packetcraft protocol software is a collection of embedded stacks implementing the Bluetooth Low Energy Link Layer, Host, Profile and Mesh specification (www.bluetooth.org).

This repository contains open source release of Packetcraft's software. This is a qualified release and may be used in products. Please consult the [Bluetooth Qualification Process](https://www.bluetooth.com/develop-with-bluetooth/qualification-listing) for further details regarding additional certification requirements.


Release notes
-------------

This latest release of the Packetcraft Host and Packetcraft Controller is Bluetooth 5.2 qualified and implements the following new Bluetooth 5.2 features: LE Isochronous Channels, Enhanced Attribute Protocol, and LE Power Control.

This release includes the following completed requirements for r20.05:

    FW-3340 Isochronous Demo: single BIS data stream
    FW-3354 TCRL.2019-1 compliant
    FW-3359 Core v5.2: LE Isochronous Channels (ISO)
    FW-3360 Core v5.2: LE Power Control
    FW-3361 Core v5.2: Enhanced ATT (EATT)
    FW-3617 Core v5.2: Isochronous Abstraction Layer (ISOAL)
    FW-3726 Core v5.2: Host Support for LE Isochronous Channels (ISO)
    FW-3727 Nordic nRF5 SDK 16.0.0
    FW-3730 Compile BLE host for 64-bit platform
    FW-3736 Light CTL Model
    FW-3738 Mesh v1.0.1 compliant
    FW-3739 TCRL.2019-2 qualification
    FW-3750 Laird BL654 platform
    FW-3767 SBC codec
    FW-3803 GCC compiler support for gcc-arm-none-eabi-9-2019-q4-major
    FW-3820 Protect against SweynTooth vulnerability


Getting Started
---------------

**1. Toolchain**

If using a system with package manager such as Ubuntu, use the following command line to install dependent tools:

```
sudo apt-get install build-essential binutils-arm-none-eabi
```

Alternatively download and install the GNU Arm Embedded Toolchain from here and add the path to the `bin` folder to your PATH environment.

* [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)


**2. Build**

Select a project to build. The following folders contains buildable projects:

* Packetcraft Profile sample application: [ble-apps/build](ble-apps/build)
* Packetcraft Mesh sample application: [ble-mesh-apps/build](ble-mesh-apps/build)
* Packetcraft Controller sample application: [controller/build](controller/build)

Consult the Sample App Developer's Guide (below) for more information about the application usage.

For example, to build Packetcraft's sample Bluetooth Low Energy Tag device use the following make command from the root repo folder:

```
make -C ble-apps/build/tag/gcc
```

This command will build a complete device image including the Tag sample application, Profiles, Host, Link Layer and platform drivers for the Nordic nRF52840 / PCA10056 development board. The resulting image is located at `ble-apps/build/tag/gcc/bin/tag.bin`.


**3. Program**

To install the firmware image on a Nordic PCA10056 developmnent board, plug a USB cable into your PCA10056. Drag-n-Drop the resulting image from the previous step onto the mass storage drive called "JLINK".


Documentation
-------------

For more information consult the following documents:

* Packetcraft BLE Mesh Sample App Developer's Guide - TBD
* Packetcraft Mesh Developer's Guide - TBD
* [Packetcraft BLE Sample App Developer's Guide](https://os.mbed.com/docs/mbed-cordio/19.02/sample-apps/index.html)
* [Packetcraft Profiles Developer's Guide](https://os.mbed.com/docs/mbed-cordio/19.02/profiles/index.html)
* [Packetcraft Host Developer's Guide](https://os.mbed.com/docs/mbed-cordio/19.02/stack/index.html)
* [Packetcraft Controller Developer's Guide](https://os.mbed.com/docs/mbed-cordio/19.02/controller/index.html)
* [Packetcraft WSF Developer's Guide](https://os.mbed.com/docs/mbed-cordio/19.02/wsf/index.html)
* [Packetcraft PAL Developer's Guide](https://os.mbed.com/docs/mbed-cordio/19.02/porting-pal/index.html)


Certification
-------------

Bluetooth LE Mesh solution implementing of the Bluetooth Mesh Profile 1.0 and the Bluetooth Mesh Model 1.0 wireless technical specifications

* [QDID 116593](https://launchstudio.bluetooth.com/ListingDetails/66212)

Bluetooth LE Host protocol stack implementing Bluetooth Core 5.2 specification

* [QDID 146344](https://launchstudio.bluetooth.com/ListingDetails/103670)

Bluetooth LE Link Layer protocol stack implementing Bluetooth 5.2 specification

* [QDID 146281](https://launchstudio.bluetooth.com/ListingDetails/103599)


Verification
------------

Packetcraft Mesh is verified with the TCRL.2019-2 compliance tester using the following:

* Bluetooth Profile Tuning Suites 7.6.1

Packetcraft Host is verified with the TCRL.2019-2 compliance tester using the following:

* Bluetooth Profile Tuning Suites 7.6.1

Packetcraft Profiles is verified with the TCRL.2018-2 compliance tester using the following:

* Bluetooth Profile Tuning Suites 7.3.0

Packetcraft Link Layer conforms to the Bluetooth TCRL.2019-2 requirements verified with the following:

* Teledyne Harmony LE Tester version 19.12.16916.21195

This product was compiled and tested with the following version of GNU GCC

* gcc-arm-none-eabi-9-2019-q4-major


Platforms
---------

This release was tested on the following platforms. Note: platforms listed may not be available in this repository.

* Nordic nRF52840 / PCA10056 development kit / Nordic nRF5 SDK 16.0.0 (make configuration: "PLATFORM=nordic BOARD=PCA10056")
* Nordic nRF52832 / PCA10040 development kit / Nordic nRF5 SDK 16.0.0 (make configuration: "PLATFORM=nordic BOARD=PCA10040")
* Laird BL654 / 451-00004 USB adapter / Nordic nRF5 SDK 16.0.0 (make configuration: "PLATFORM=laird")
