@echo off
setlocal enabledelayedexpansion
 
set batchName=%~n0
set batchPath=%~dp0
set buildType=Release
set buildTarget=install


for %%i in (%*) do (
    set param=%%i
    if "!param:~0,1!"=="-" (
        if !param! == -d (
            set buildType=Debug
        )
    ) else (
        echo Parameter !param! not with "-"
        set buildTarget=!param!
    )
)


echo ==========Build Configuration======
echo Build Type: %buildType%
echo Build Target: %buildTarget%
echo ===================================



if %buildType% == Debug (
    cmake -S . -B build/Debug -DCMAKE_INSTALL_PREFIX="install/Debug" || goto :ConfigError
    cmake --build build/Debug --config Debug --target %buildTarget% || goto :Error
) else (
    cmake -S . -B build/Release -DCMAKE_INSTALL_PREFIX="install/Release" || goto :ConfigError
    cmake --build build/Release --config Release --target %buildTarget% || goto :Error
)


:ConfigError
if %ERRORLEVEL% neq 0 (
    echo "============================"
    echo "CMAKE configuration failed!"
    echo "============================"
    exit /b %ERRORLEVEL%
)

:Error
if %ERRORLEVEL% neq 0 (
    echo "============================"
    echo "Build failed!"
    echo "============================"
    exit /b %ERRORLEVEL%
)


:End
if %ERRORLEVEL% equ 0 (
    echo "============================"
    echo "Build success!"
    echo "============================"
    exit /b %ERRORLEVEL%
)

 
endlocal
