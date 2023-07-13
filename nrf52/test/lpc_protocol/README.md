# 
This directory contains test code to apply an LPC FW update over UART from a 
PC host. This was used to develop the LPC interface protocol rather than running 
on-target.

The LPC55S69-EVK is used for this test because it has USART0 routed to pins, while the Elemind FF board has USART0 routed to the Nordic. ISP is only available on this USART port.

## Setup
Connect an FTDI to the RX, TX and GND pins on the right edge of the EVK. Note the pins are labeled from the point-of-view of the FTDI/host, not the EVK.

Power the EVK via the 5v micro-USB port. 

## Build
There is a build script which builds the test exectuable. It is simply called `build` and runs GCC.

## Running the test

### Enter ISP mode on the EVK
* Press the ISP button
* Press and release the reset button
* Release the ISP button

### Run the test
```
./build && ./testuart.out /dev/cu.usbserial-A9876543
```

Results are printed on the command line.