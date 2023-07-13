@echo off

ECHO Launch docker container and build NRF52 bootloader
cd ..
docker run --rm -it -v %cd%:/usr/morpheus_fw_nrf52 tgage4321/nrfbuild:1.1 bootloader

pause
