@ECHO OFF

SET self=%0%
call %self:setup_mingw=setup_build%

SET MINGW_DIR=%BUILD_TOOLS_DIR%\MinGW\bin
ECHO %PATH% | FIND "%MINGW_DIR%" > NUL && GOTO Quit

SET BUILD_ARCH=x86
SET TOOLSET=mingw
SET PATH=%MINGW_DIR%;%PATH%
call %self:setup_mingw=setup_boost%
call %self:setup_mingw=setup_qt%
SET CPATH=%INCLUDE%
SET LIBRARY_PATH=%LIB%
:Quit
SET self=
