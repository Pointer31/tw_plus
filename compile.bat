call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
:nochmal
..\bam\bam server_debug

if %errorlevel% == 0 start "" teeworlds_srv_d.exe

echo.
set /p again=Again? 

if "%again%" == "" goto nochmal