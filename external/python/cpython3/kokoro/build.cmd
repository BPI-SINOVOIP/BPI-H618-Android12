:: Expected arguments:
:: %1 = python_src
:: %2 = dest_dir

SET PYTHON_SRC=%1
SET DEST=%2

cd %PYTHON_SRC%
md %DEST%
IF %ERRORLEVEL% NEQ 0 goto :end

:: Deletes Android.bp or it will be packaged.
DEL Lib\Android.bp
IF %ERRORLEVEL% NEQ 0 goto :end

ECHO ## Building python...
CALL PCbuild\build.bat -c Release -p x64
IF %ERRORLEVEL% NEQ 0 goto :end

ECHO ## Packaging python...
CALL PCBuild\amd64\python.exe PC\layout --zip %DEST%\python3-windows-%KOKORO_BUILD_ID%.zip --include-dev
IF %ERRORLEVEL% NEQ 0 goto :end

:: Packages all downloaded externals in externals
ECHO ## Packaging externals...
powershell Compress-Archive -Path .\externals -DestinationPath %DEST%\python3-externals-%KOKORO_BUILD_ID%.zip
IF %ERRORLEVEL% NEQ 0 goto :end

:end
exit /b %ERRORLEVEL%
