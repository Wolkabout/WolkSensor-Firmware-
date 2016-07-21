@echo off
del %~2 /q
for /F "tokens=1,2,3" %%i in (%~1) do (
if %%i EQU #define (
  if %%j EQU FW_VERSION_MAJOR set major=%%k
  if %%j EQU FW_VERSION_MINOR set minor=%%k
)
)
echo %major%.%minor% >> %~2
@echo on
