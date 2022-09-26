@echo off

:: ToDo: still need to resolve final build step issues
ECHO Launch docker container and build NRF52 image
cd ..
docker run --rm -it -v %cd%:/usr/morpheus_fw_nrf52 tgage4321/nrfbuild:1.1

pause
