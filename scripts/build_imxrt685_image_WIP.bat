:: Build an Eclipse project from the command line on Windows using MCUXpresso IDE

@echo off

:: Path to GNU tools and compiler: arm-none-eabi-gcc.exe, ....
SET TOOLCHAIN_PATH=C:\nxp\MCUXpressoIDE_11.6.0_8187\ide\tools\bin

:: Variable to the command line Eclipse IDE executable
SET IDE=C:\nxp\MCUXpressoIDE_11.6.0_8187\ide\mcuxpressoidec.exe

ECHO Extending PATH if not already present
ECHO %PATH%|findstr /i /c:"%TOOLCHAIN_PATH:"=%">nul || set PATH=%PATH%;%TOOLCHAIN_PATH%

ECHO Launching Eclipse IDE
ECHO Clean Project ... 
"%IDE%" -nosplash --launcher.suppressErrors -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data "c:\Workspace" -cleanBuild morpheus_fw_imxrt685/Debug

ECHO Build Project ... 
"%IDE%" -nosplash -noExit --launcher.suppressErrors -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data "c:\Workspace" -build morpheus_fw_imxrt685/Debug