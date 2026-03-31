@echo off
REM LocalAI Chat 项目构建脚本 (Windows)
REM 用于检查依赖、下载 llama.cpp 并构建项目

setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0
set CPP_DIR=%PROJECT_ROOT%app\src\main\cpp
set LLAMA_DIR=%CPP_DIR%\llama.cpp

REM 颜色代码 (Windows 10+)
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

goto :main

:print_info
echo %BLUE%[INFO]%NC% %~1
goto :eof

:print_success
echo %GREEN%[SUCCESS]%NC% %~1
goto :eof

:print_warning
echo %YELLOW%[WARNING]%NC% %~1
goto :eof

:print_error
echo %RED%[ERROR]%NC% %~1
goto :eof

:check_environment
call :print_info "检查构建环境..."

REM 检查 Java
java -version >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Java 未安装，请先安装 JDK 17+"
    exit /b 1
)
call :print_success "Java 已安装"
java -version 2>&1 | findstr /i "version"

REM 检查 Git
git --version >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Git 未安装，请先安装 Git"
    exit /b 1
)
call :print_success "Git 已安装"

REM 检查 ANDROID_HOME
if "%ANDROID_HOME%"=="" (
    call :print_warning "ANDROID_HOME 环境变量未设置"
    call :print_info "请设置 ANDROID_HOME 指向 Android SDK 目录"
) else (
    call :print_success "ANDROID_HOME: %ANDROID_HOME%"
)

call :print_success "环境检查通过"
exit /b 0

:download_llama_cpp
call :print_info "检查 llama.cpp..."

if exist "%LLAMA_DIR%\llama.h" (
    call :print_success "llama.cpp 已存在"
    exit /b 0
)

if exist "%LLAMA_DIR%" (
    call :print_warning "llama.cpp 目录存在但不完整，重新下载..."
    rmdir /s /q "%LLAMA_DIR%"
)

call :print_info "下载 llama.cpp..."

cd /d "%CPP_DIR%"
git clone --depth 1 https://github.com/ggerganov/llama.cpp.git

if exist "%LLAMA_DIR%\llama.h" (
    call :print_success "llama.cpp 下载完成"
    exit /b 0
) else (
    call :print_error "llama.cpp 下载失败"
    exit /b 1
)

:clean_build
call :print_info "清理构建..."

cd /d "%PROJECT_ROOT%"

if exist "app\build" (
    rmdir /s /q "app\build"
    call :print_success "已清理 app\build"
)

if exist ".gradle" (
    rmdir /s /q ".gradle"
    call :print_success "已清理 .gradle"
)

if exist "build" (
    rmdir /s /q "build"
    call :print_success "已清理 build"
)

call :print_success "清理完成"
exit /b 0

:build_apk
set BUILD_TYPE=%~1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=debug

call :print_info "开始构建 %BUILD_TYPE% APK..."

cd /d "%PROJECT_ROOT%"

if not exist "gradlew.bat" (
    call :print_error "gradlew.bat 不存在，请先运行 setup 或手动创建 Gradle Wrapper"
    exit /b 1
)

if "%BUILD_TYPE%"=="release" (
    call gradlew.bat assembleRelease
) else (
    call gradlew.bat assembleDebug
)

if "%BUILD_TYPE%"=="release" (
    set APK_PATH=%PROJECT_ROOT%app\build\outputs\apk\release\app-release.apk
) else (
    set APK_PATH=%PROJECT_ROOT%app\build\outputs\apk\debug\app-debug.apk
)

if exist "!APK_PATH!" (
    call :print_success "APK 构建成功: !APK_PATH!"
    dir "!APK_PATH!"
    exit /b 0
) else (
    call :print_error "APK 构建失败"
    exit /b 1
)

:show_help
echo LocalAI Chat 构建脚本
echo.
echo 用法: %~nx0 ^<命令^>
echo.
echo 命令:
echo   check       检查构建环境
echo   download    下载 llama.cpp
echo   clean       清理构建
echo   build       构建 debug APK
echo   release     构建 release APK
echo   all         执行完整构建流程
echo   help        显示此帮助信息
echo.
echo 示例:
echo   %~nx0 check       # 检查环境
echo   %~nx0 build       # 构建 debug APK
echo   %~nx0 all         # 完整构建
goto :eof

:main
set COMMAND=%~1
if "%COMMAND%"=="" set COMMAND=help

if "%COMMAND%"=="check" (
    call :check_environment
) else if "%COMMAND%"=="download" (
    call :download_llama_cpp
) else if "%COMMAND%"=="clean" (
    call :clean_build
) else if "%COMMAND%"=="build" (
    call :check_environment && call :download_llama_cpp && call :build_apk debug
) else if "%COMMAND%"=="release" (
    call :check_environment && call :download_llama_cpp && call :build_apk release
) else if "%COMMAND%"=="all" (
    call :check_environment && call :download_llama_cpp && call :build_apk debug
) else if "%COMMAND%"=="help" (
    call :show_help
) else (
    call :print_error "未知命令: %COMMAND%"
    call :show_help
    exit /b 1
)

endlocal
