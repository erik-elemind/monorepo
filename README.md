# Morpheus iMXRT685

## Prerequisites 
MCUXpresso

[Glow](https://www.nxp.com/design/software/development-software/eiq-ml-development-environment/eiq-inference-with-glow-nn:eIQ-Glow) is needed to build the Glow bundle for ML applications.

A JLink is needed for wired flashing (optional). Please download and install the 
[Segger Software and Documentation pack](https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack).

**THIS REPO USES GIT SUBMODULES**
Please run `git submodule update --init`, clean and refresh project prior to building.

## Build process
Use MCUXPresso

<!-- ## Generating LPC OTA packages -->
<!-- To generate an LPC firmware package, run the `lpcpkg.sh` script

First step is to generate a hex file from MCUxpresso. In the project explorer, expand "Binaries", and right click morpheus_firmware.axf. Then select "Binary Utilities" and "Create hex". 

Then provide the path to the LPC hex file to the script, like so:

```
./lpcpkg.sh -f ../../images/morpheus_firmware.hex
``` -->

## Flashing 
Use MCUXpresso, Ozone, JLink Commander

## Glow Bundle
Glow Bundle refers to the files that are generated when converting a .tflite model using NXP's Glow compiler.

Documentation: https://github.com/pytorch/glow/blob/master/docs/AOT.md

Command: `model-compiler -backend=CPU ‑model=<tflite-model-path> ‑emit‑bundle=<bundle-dir> ‑target=arm ‑mcpu=cortex‑m33 ‑float‑abi=hard ‑use‑cmsis ‑network‑name=test_model ‑dump‑graph‑DAG="model_graph.dot"`

Add the bundle generated in `-emit-bundle=` path to glow_bundle dir:
- `test_model.o, test_model.h, test_model.weights.bin`

Refresh, clean, and rebuild the project to use.

#TODO:
- add links
- add scripts for Glow compiling
- add scripts for builds