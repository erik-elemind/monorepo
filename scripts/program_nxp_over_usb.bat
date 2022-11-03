@echo off

ECHO Programming NXP iMXRT685S over USB
blhost -t 50000 -u 0x1FC9,0x0020 -- fill-memory 0x1C000 4 0xC1000204 word
blhost -t 50000 -u 0x1FC9,0x0020 -- fill-memory 0x1C004 4 0x20000000 word
blhost -t 50000 -u 0x1FC9,0x0020 -- configure-memory 9 0x1C000
blhost -t 50000 -u 0x1FC9,0x0020 -- flash-erase-region 0x8000000 0x80000
blhost -t 50000 -u 0x1FC9,0x0020 -- write-memory 0x8000000 ".\morpheus_fw_imxrt685.bin"
blhost -t 50000 -u 0x1FC9,0x0020 -- reset 

pause
