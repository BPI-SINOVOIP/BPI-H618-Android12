:: Code under repo is checked out to %KOKORO_ARTIFACTS_DIR%\git.
:: The final directory name in this path is determined by the scm name specified
:: in the job configuration

SET DEST=%KOKORO_ARTIFACTS_DIR%\dest
SET PYTHON_SRC=%KOKORO_ARTIFACTS_DIR%\git\cpython3

call %~dp0\build.cmd "%PYTHON_SRC%" "%DEST%"

exit /b %ERRORLEVEL%
