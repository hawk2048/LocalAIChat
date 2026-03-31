#!/bin/bash

# LocalAI Chat 项目构建脚本
# 用于检查依赖、下载 llama.cpp 并构建项目

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$PROJECT_ROOT/app/src/main/cpp"
LLAMA_DIR="$CPP_DIR/llama.cpp"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查命令是否存在
check_command() {
    if ! command -v $1 &> /dev/null; then
        print_error "$1 未安装，请先安装 $1"
        return 1
    else
        print_success "$1 已安装: $(command -v $1)"
        return 0
    fi
}

# 检查环境
check_environment() {
    print_info "检查构建环境..."
    
    local all_ok=true
    
    # 检查 Java
    if check_command java; then
        java -version 2>&1 | head -n 1
    else
        all_ok=false
    fi
    
    # 检查 Git
    if ! check_command git; then
        all_ok=false
    fi
    
    # 检查 Android SDK
    if [ -z "$ANDROID_HOME" ]; then
        print_warning "ANDROID_HOME 环境变量未设置"
        print_info "请设置 ANDROID_HOME 指向 Android SDK 目录"
        all_ok=false
    else
        print_success "ANDROID_HOME: $ANDROID_HOME"
    fi
    
    # 检查 NDK
    if [ -n "$ANDROID_NDK_HOME" ]; then
        print_success "ANDROID_NDK_HOME: $ANDROID_NDK_HOME"
    else
        print_warning "ANDROID_NDK_HOME 环境变量未设置"
        print_info "将在构建时自动检测 NDK"
    fi
    
    if [ "$all_ok" = true ]; then
        print_success "环境检查通过"
        return 0
    else
        print_error "环境检查失败，请先安装缺失的依赖"
        return 1
    fi
}

# 下载 llama.cpp
download_llama_cpp() {
    print_info "检查 llama.cpp..."
    
    if [ -d "$LLAMA_DIR" ]; then
        if [ -f "$LLAMA_DIR/llama.h" ]; then
            print_success "llama.cpp 已存在"
            return 0
        else
            print_warning "llama.cpp 目录存在但不完整，重新下载..."
            rm -rf "$LLAMA_DIR"
        fi
    fi
    
    print_info "下载 llama.cpp..."
    
    # 使用 git clone
    cd "$CPP_DIR"
    git clone --depth 1 https://github.com/ggerganov/llama.cpp.git
    
    if [ -f "$LLAMA_DIR/llama.h" ]; then
        print_success "llama.cpp 下载完成"
        return 0
    else
        print_error "llama.cpp 下载失败"
        return 1
    fi
}

# 清理构建
clean_build() {
    print_info "清理构建..."
    
    cd "$PROJECT_ROOT"
    
    if [ -d "app/build" ]; then
        rm -rf app/build
        print_success "已清理 app/build"
    fi
    
    if [ -d ".gradle" ]; then
        rm -rf .gradle
        print_success "已清理 .gradle"
    fi
    
    if [ -d "build" ]; then
        rm -rf build
        print_success "已清理 build"
    fi
    
    print_success "清理完成"
}

# 构建 APK
build_apk() {
    local build_type=${1:-"debug"}
    
    print_info "开始构建 $build_type APK..."
    
    cd "$PROJECT_ROOT"
    
    # 检查 gradlew
    if [ ! -f "gradlew" ]; then
        print_error "gradlew 不存在，请先运行 './setup.sh' 或手动创建 Gradle Wrapper"
        return 1
    fi
    
    # 赋予执行权限
    chmod +x gradlew
    
    # 构建
    if [ "$build_type" = "release" ]; then
        ./gradlew assembleRelease
    else
        ./gradlew assembleDebug
    fi
    
    # 检查结果
    local apk_path=""
    if [ "$build_type" = "release" ]; then
        apk_path="$PROJECT_ROOT/app/build/outputs/apk/release/app-release.apk"
    else
        apk_path="$PROJECT_ROOT/app/build/outputs/apk/debug/app-debug.apk"
    fi
    
    if [ -f "$apk_path" ]; then
        print_success "APK 构建成功: $apk_path"
        ls -lh "$apk_path"
        return 0
    else
        print_error "APK 构建失败"
        return 1
    fi
}

# 安装到设备
install_apk() {
    print_info "安装 APK 到设备..."
    
    if ! check_command adb; then
        print_error "adb 未找到，请确保 Android SDK platform-tools 在 PATH 中"
        return 1
    fi
    
    local apk_path="$PROJECT_ROOT/app/build/outputs/apk/debug/app-debug.apk"
    
    if [ ! -f "$apk_path" ]; then
        print_error "APK 文件不存在，请先运行 './build.sh debug' 构建"
        return 1
    fi
    
    adb install -r "$apk_path"
    
    if [ $? -eq 0 ]; then
        print_success "APK 安装成功"
        return 0
    else
        print_error "APK 安装失败"
        return 1
    fi
}

# 显示帮助
show_help() {
    echo "LocalAI Chat 构建脚本"
    echo ""
    echo "用法: $0 <命令> [选项]"
    echo ""
    echo "命令:"
    echo "  check       检查构建环境"
    echo "  download    下载 llama.cpp"
    echo "  clean       清理构建"
    echo "  build       构建 debug APK"
    echo "  release     构建 release APK"
    echo "  install     安装 APK 到设备"
    echo "  all         执行完整构建流程 (check -> download -> build)"
    echo "  help        显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 check           # 检查环境"
    echo "  $0 build           # 构建 debug APK"
    echo "  $0 release         # 构建 release APK"
    echo "  $0 all             # 完整构建"
}

# 主函数
main() {
    local command=${1:-"help"}
    
    case $command in
        check)
            check_environment
            ;;
        download)
            download_llama_cpp
            ;;
        clean)
            clean_build
            ;;
        build)
            check_environment && download_llama_cpp && build_apk debug
            ;;
        release)
            check_environment && download_llama_cpp && build_apk release
            ;;
        install)
            install_apk
            ;;
        all)
            check_environment && download_llama_cpp && build_apk debug
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            print_error "未知命令: $command"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
