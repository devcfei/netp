@echo off
REM Start the server in a new window
start "NETP Server" cmd /c "install\Debug\bin\netp_sample.exe server"

REM Wait for server to start
timeout /t 2

REM Start 10 clients in separate windows
for /l %%x in (1, 1, 10) do (
    start "NETP Client %%x" cmd /c "install\Debug\bin\netp_sample.exe client"
    timeout /t 1
)

echo All clients started. Press any key to terminate all processes...
pause

REM Kill all the processes
taskkill /F /FI "WindowTitle eq NETP Server"
for /l %%x in (1, 1, 10) do (
    taskkill /F /FI "WindowTitle eq NETP Client %%x"
)

echo Test completed. 