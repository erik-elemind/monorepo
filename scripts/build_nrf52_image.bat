@echo off

:: ToDo: Note will eventually add a proper image in docker hub we will simply pull down
ECHO Build docker Image
cd ..
docker build . -t="elemind/nrfbuild:1.0"

:: ToDo: still need to resolve final build step issues
ECHO Launch docker container and build NRF52 image
docker run --rm -it -v %cd%:/usr/project elemind/nrfbuild:1.0 

:: Notes on docker image
:: It is using a alpine package and not the normal dist, need to resolve that. 
:: Running make dist takes an eternity, so probably best long term is to use image 
:: Need to figure out Python versioning for nrfutils, currently last step fails

pause
