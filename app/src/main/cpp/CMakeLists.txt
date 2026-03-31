cmake_minimum_required(VERSION 3.22.1)
project("localai")

# ============================================================
# C++ 标准设置
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================
# 编译选项
# ============================================================
add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
    -O3
    -fvisibility=hidden
)

# Debug模式选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -DDEBUG)
endif()

# ============================================================
# CPU架构优化
# ============================================================
if(ANDROID_ABI STREQUAL "arm64-v8a")
    # ARM64 优化
    add_compile_options(-march=armv8.2-a+fp16+dotprod)
    add_definitions(-D__ARM_NEON -D__ARM_FEATURE_FMA)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    # ARM32 优化
    add_compile_options(-mfpu=neon -mfloat-abi=softfp)
    add_definitions(-D__ARM_NEON)
elseif(ANDROID_ABI STREQUAL "x86_64")
    # x86_64 优化
    add_compile_options(-msse4.1 -mfma)
    add_definitions(-D__AVX2__)
elseif(ANDROID_ABI STREQUAL "x86")
    # x86 优化
    add_compile_options(-msse4.1)
endif()

# ============================================================
# llama.cpp 集成配置
# ============================================================
option(LLAMA_SUPPORT "Enable llama.cpp support" ON)
option(LLAMA_CUBLAS "Enable CUDA support (requires NVIDIA GPU)" OFF)
option(LLAMA_METAL "Enable Metal support (requires Apple Silicon)" OFF)

# llama.cpp 源文件路径（workflow 下载到当前目录）
set(LLAMA_DIR ${CMAKE_SOURCE_DIR})

# llama.cpp 核心源文件
set(LLAMA_SOURCES
    # 核心文件 - 从llama.cpp仓库获取
    ${LLAMA_DIR}/llama.cpp
    ${LLAMA_DIR}/ggml.c
    ${LLAMA_DIR}/ggml-alloc.c
    ${LLAMA_DIR}/ggml-backend.c
    ${LLAMA_DIR}/ggml-quants.c
)

# 定义 llama.cpp 编译选项
if(LLAMA_SUPPORT)
    add_definitions(-DLLAMA_SUPPORT=1)
    
    # 禁用一些可能不可用的功能
    add_definitions(-DLLAMA_NO_ACCELERATE)
    
    if(NOT LLAMA_CUBLAS)
        add_definitions(-DLLAMA_NO_CUBLAS)
    endif()
    
    if(NOT LLAMA_METAL)
        add_definitions(-DLLAMA_NO_METAL)
    endif()
endif()

# ============================================================
# 本地源文件
# ============================================================
set(LOCALAI_SOURCES
    localai.cpp
    llm_engine.cpp
    llama_wrapper.cpp
    jni_interface.cpp
)

# ============================================================
# 创建共享库
# ============================================================
add_library(localai SHARED 
    ${LOCALAI_SOURCES}
    ${LLAMA_SOURCES}
)

# ============================================================
# 包含路径
# ============================================================
target_include_directories(localai PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${LLAMA_DIR}
)

# ============================================================
# 链接库
# ============================================================
target_link_libraries(localai
    android
    log
    z
)

# OpenMP 支持（可选，用于多线程加速）
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(localai OpenMP::OpenMP_CXX)
    message(STATUS "OpenMP found and enabled")
endif()

# ============================================================
# 完整 llama.cpp 集成说明
# ============================================================
# 要使用完整的 llama.cpp 功能，请按以下步骤操作：
#
# 1. 下载 llama.cpp 源码：
#    cd app/src/main/cpp/third_party
#    git clone --depth 1 https://github.com/ggerganov/llama.cpp.git .
#
# 2. 取消上述 LLAMA_SOURCES 中的注释
#
# 3. 如果需要 GPU 加速：
#    - 对于 Adreno GPU（高通）：需要 OpenCL 或 Qualcomm SNPE
#    - 对于 Mali GPU（三星等）：需要 OpenCL
#    - 目前 Android 上最推荐使用 CPU 后端
#
# 4. 根据目标设备选择合适的量化模型：
#    - Q4_0: 最小体积，速度最快，质量稍低
#    - Q5_1: 平衡选项
#    - Q8_0: 更高质量，更大体积
#
# 5. 重新编译项目：
#    ./gradlew assembleDebug
# ============================================================
