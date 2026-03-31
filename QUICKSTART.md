# LocalAI Chat 快速开始指南

## 5 分钟快速上手

### 前置要求

- Android Studio Hedgehog 或更高版本
- Android SDK 24+ (Android 7.0)
- NDK r25+ 
- Git
- 一台 ARM64 Android 设备

### 第一步：获取项目

```bash
# 克隆项目
git clone <project-url> LocalAIChat
cd LocalAIChat
```

### 第二步：下载 llama.cpp

**方式一：使用构建脚本（推荐）**

```bash
# Linux/macOS
chmod +x build.sh
./build.sh download

# Windows
build.bat download
```

**方式二：手动下载**

```bash
cd app/src/main/cpp
git clone --depth 1 https://github.com/ggerganov/llama.cpp.git
```

### 第三步：在 Android Studio 中打开

1. 打开 Android Studio
2. 选择 "Open an Existing Project"
3. 选择 LocalAIChat 目录
4. 等待 Gradle 同步完成（首次可能需要几分钟）

### 第四步：构建 APK

**方式一：使用构建脚本**

```bash
# Linux/macOS
./build.sh build

# Windows
build.bat build
```

**方式二：使用 Android Studio**

1. 点击菜单 `Build` → `Make Project` (或按 Ctrl+F9)
2. 等待编译完成（首次编译 llama.cpp 可能需要 10-15 分钟）

### 第五步：安装和运行

**方式一：使用 ADB**

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

**方式二：使用 Android Studio**

1. 连接 Android 设备
2. 点击 Run 按钮 (绿色三角形)

### 第六步：加载模型

#### 获取 GGUF 模型

推荐模型：

| 模型 | 大小 | 下载地址 |
|------|------|----------|
| Qwen2.5-1.5B-Instruct-Q4_K_M | ~1GB | [HuggingFace](https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF) |
| Qwen2.5-3B-Instruct-Q4_K_M | ~2GB | [HuggingFace](https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF) |
| Llama-3.2-3B-Instruct-Q4_K_M | ~2GB | [HuggingFace](https://huggingface.co/meta-llama/Llama-3.2-3B-Instruct-GGUF) |

国内用户可使用 [ModelScope 镜像](https://modelscope.cn/models)

#### 加载模型到应用

1. 将模型文件复制到手机：
   ```bash
   adb push model.gguf /sdcard/Download/
   ```

2. 在应用中：
   - 点击"选择模型文件"
   - 导航到 `/sdcard/Download/`
   - 选择 `.gguf` 文件
   - 等待模型加载完成

3. 开始聊天！

## 常见问题

### 构建失败

**问题**: CMake 找不到 llama.cpp

**解决**: 确保已下载 llama.cpp 到 `app/src/main/cpp/llama.cpp/`

```bash
ls app/src/main/cpp/llama.cpp/llama.h
# 应该能看到 llama.h 文件
```

### 模型加载失败

**问题**: 提示"模型加载失败"

**解决**:
1. 确认模型是 GGUF 格式
2. 检查设备是否有足够内存
3. 尝试更小的模型

### 应用崩溃

**问题**: 打开应用后崩溃

**解决**:
1. 查看 logcat 日志：
   ```bash
   adb logcat | grep -E "LocalAI|llama"
   ```
2. 检查 NDK 版本是否正确
3. 确保 CPU 架构是 ARM64

## 下一步

- 阅读 [完整文档](README.md)
- 了解 [JNI API](README.md#jni-api-参考)
- 学习 [性能优化](README.md#性能优化建议)
- 自定义 [生成参数](README.md#generationparams-参数)

## 获取帮助

遇到问题？查看：
- [FAQ](README.md#常见问题-faq)
- [llama.cpp 文档](https://github.com/ggerganov/llama.cpp)
- 提交 Issue 到项目仓库
