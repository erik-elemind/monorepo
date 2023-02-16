@echo off

ECHO FIRMWARE UPGRADE SCRIPT v1.0

:: Get the first .bin file found in the folder
FOR %%F IN (./*.bin) DO (
 set filename=%%F
 goto start_programming
)
:start_programming

ECHO ================ Programming "%filename%" over USB ================
blhost -t 50000 -u 0x1FC9,0x0020 -- fill-memory 0x1C000 4 0xC1000204 word
blhost -t 50000 -u 0x1FC9,0x0020 -- fill-memory 0x1C004 4 0x20000000 word
blhost -t 50000 -u 0x1FC9,0x0020 -- configure-memory 9 0x1C000
blhost -t 50000 -u 0x1FC9,0x0020 -- flash-erase-region 0x8000000 0x400000
blhost -t 50000 -u 0x1FC9,0x0020 -- flash-erase-region 0x8400000 0x400000
blhost -t 50000 -u 0x1FC9,0x0020 -- write-memory 0x8000000 %filename%
blhost -t 50000 -u 0x1FC9,0x0020 -- reset 
ECHO ================ Done ================
pause
