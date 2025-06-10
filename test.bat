@echo off
setlocal enabledelayedexpansion

:menu
cls
echo NETP Robust Test Menu
echo ====================
echo 1. Run Regular Client Test
echo 2. Run Benchmark Test
echo 3. Exit
echo.
set /p choice="Enter your choice (1-3): "

if "%choice%"=="1" goto regular_test
if "%choice%"=="2" goto benchmark_test
if "%choice%"=="3" goto end
goto menu

:regular_test
REM Start the server in a new window
start "NETP Server" cmd /c "install\Debug\bin\netp_echo.exe server"

REM Wait for server to start
timeout /t 2

REM Start 10 clients in separate windows
for /l %%x in (1, 1, 10) do (
    start "NETP Client %%x" cmd /c "install\Debug\bin\netp_echo.exe client"
    timeout /t 1
)

echo All clients started. Press any key to terminate all processes...
pause

REM Kill all the processes
taskkill /F /FI "WindowTitle eq NETP Server"
for /l %%x in (1, 1, 10) do (
    taskkill /F /FI "WindowTitle eq NETP Client %%x"
)

echo Regular test completed.
timeout /t 3
goto menu

:benchmark_test
REM Start the server in a new window
start "NETP Server" cmd /c "install\Debug\bin\netp_echo.exe server"

REM Wait for server to start
timeout /t 2

:benchmark_loop
cls
echo Benchmark Test Menu
echo =================
echo 1. Run parallel stress test (5x50 clients)
echo 2. Run parallel stress test (5x100 clients)
echo 3. Run parallel stress test (5x200 clients)
echo 4. Run custom parallel stress test
echo 5. Return to main menu
echo.
set /p bench_choice="Enter your choice (1-5): "

if "%bench_choice%"=="1" (
    set num_clients=50
    set parallel_count=5
    set duration=60
    goto run_stress_test
)
if "%bench_choice%"=="2" (
    set num_clients=100
    set parallel_count=5
    set duration=60
    goto run_stress_test
)
if "%bench_choice%"=="3" (
    set num_clients=200
    set parallel_count=5
    set duration=60
    goto run_stress_test
)
if "%bench_choice%"=="4" (
    set /p num_clients="Enter number of clients per instance (1-1000): "
    set /p parallel_count="Enter number of parallel instances (1-20): "
    set /p duration="Enter duration in seconds (1-3600): "
    goto run_stress_test
)
if "%bench_choice%"=="5" (
    taskkill /F /FI "WindowTitle eq NETP Server"
    goto menu
)
goto benchmark_loop

:run_stress_test
if !parallel_count! lss 1 set parallel_count=1
if !parallel_count! gtr 20 set parallel_count=20

set /p iterations="Enter number of test rounds (1-100): "
if !iterations! lss 1 set iterations=1
if !iterations! gtr 100 set iterations=100

echo.
echo Starting parallel stress test with:
echo - Clients per instance: !num_clients!
echo - Parallel instances: !parallel_count!
echo - Total clients: !num_clients! x !parallel_count! = !num_clients!!parallel_count!
echo - Duration per round: !duration! seconds
echo - Number of rounds: !iterations!
echo.
echo Press any key to start the test...
pause >nul

for /l %%r in (1,1,!iterations!) do (
    echo.
    echo Running round %%r of !iterations!
    echo -----------------------------------
    
    REM Launch multiple benchmark instances in parallel
    for /l %%p in (1,1,!parallel_count!) do (
        echo Starting benchmark instance %%r-%%p
        start "NETP Benchmark %%r-%%p" cmd /c "install\Debug\bin\netp_echo.exe benchmark !num_clients! 16 4092 !duration!"
        REM Small delay between launches to prevent connection storm
        timeout /t 1 >nul
    )
    
    REM Wait for the benchmark duration
    echo Waiting for round to complete...
    timeout /t !duration! >nul
    
    REM Kill all benchmark processes from this round
    echo Cleaning up round %%r...
    taskkill /F /FI "WindowTitle eq NETP Benchmark %%r-*" >nul 2>&1
    
    REM Add a delay between rounds
    if not %%r==!iterations! (
        echo Waiting 10 seconds before next round...
        timeout /t 10 >nul
    )
)

echo.
echo Parallel stress test completed. All rounds finished.
echo Press any key to continue...
pause >nul
goto benchmark_loop

:end
REM Ensure all processes are cleaned up
taskkill /F /FI "WindowTitle eq NETP Server" 2>nul
taskkill /F /FI "WindowTitle eq NETP Client*" 2>nul
taskkill /F /FI "WindowTitle eq NETP Benchmark*" 2>nul
echo All processes terminated. Goodbye! 