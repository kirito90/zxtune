@ECHO OFF

call make\platforms\setup_msvs71.bat

call build.bat "zxtune123 zxtune-qt" windows x86
