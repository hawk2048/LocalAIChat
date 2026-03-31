# LocalAI Chat 项目文件清单

## 项目根目录

```
LocalAIChat/
├── README.md                    # 完整项目文档
├── QUICKSTART.md               # 快速开始指南
├── FILE_MANIFEST.md            # 本文件清单
├── build.sh                    # Linux/macOS 构建脚本
├── build.bat                   # Windows 构建脚本
├── settings.gradle.kts         # Gradle 设置
├── build.gradle.kts            # 根项目构建配置
├── gradle.properties           # Gradle 属性
├── local.properties.example    # 本地配置示例
│
└── app/                        # 主应用模块
    ├── build.gradle.kts        # 应用构建配置
    └── src/main/
        ├── AndroidManifest.xml # 应用清单
        │
        ├── java/com/localai/chat/
        │   │
        │   ├── MainActivity.kt          # 入口 Activity
        │   │
        │   ├── engine/                  # LLM 引擎层
        │   │   ├── LlmEngine.kt        # 单例引擎类 (254行)
        │   │   ├── GenerationParams.kt # 生成参数定义
        │   │   ├── ModelInfo.kt        # 模型信息数据类
        │   │   ├── StreamCallback.kt   # 流式生成回调接口
        │   │   └── ProgressCallback.kt # 加载进度回调接口
        │   │
        │   ├── data/                    # 数据层
        │   │   ├── Message.kt           # 消息实体类
        │   │   ├── ChatMessage.kt       # 聊天消息封装
        │   │   ├── ChatRepository.kt    # 聊天历史仓库
        │   │   └── ModelManager.kt      # 模型文件管理器
        │   │
        │   ├── viewmodel/               # ViewModel 层
        │   │   └── ChatViewModel.kt     # 聊天 ViewModel (225行)
        │   │
        │   └── ui/                      # UI 层
        │       ├── MainScreen.kt        # 主界面 Composable
        │       └── theme/               # Material 3 主题
        │           ├── Color.kt         # 颜色定义
        │           ├── Theme.kt         # 主题配置
        │           └── Type.kt          # 字体排版
        │
        ├── cpp/                         # Native C++ 层
        │   ├── CMakeLists.txt          # CMake 构建配置
        │   ├── llama_wrapper.h         # LlamaWrapper 头文件
        │   ├── llama_wrapper.cpp       # Llama.cpp 封装实现 (1263行)
        │   ├── llm_engine.h            # 引擎辅助类头文件
        │   ├── llm_engine.cpp          # 引擎辅助类实现
        │   ├── jni_interface.cpp       # JNI 接口实现 (~550行)
        │   └── llama.cpp/              # llama.cpp 子模块 (需下载)
        │       ├── llama.h
        │       ├── llama.cpp
        │       ├── ggml.h
        │       └── ... (其他源文件)
        │
        ├── res/                         # Android 资源
        │   ├── drawable/               # 图标和图形
        │   │   └── ic_launcher.xml     # 应用图标
        │   ├── mipmap-*/               # 不同分辨率图标
        │   └── values/                 # 字符串和样式
        │       ├── strings.xml         # 字符串资源
        │       └── themes.xml          # 主题样式
        │
        └── assets/                      # 资产文件
            └── (可放置默认模型)
```

## 核心文件说明

### Kotlin 代码

| 文件 | 行数 | 说明 |
|------|------|------|
| LlmEngine.kt | ~254 | LLM 引擎单例类，封装所有 Native 方法调用 |
| ChatViewModel.kt | ~225 | 聊天 ViewModel，管理 UI 状态和消息流 |
| GenerationParams.kt | ~50 | 生成参数数据类 |
| MainScreen.kt | ~300 | Material 3 主界面 Composable |

### C++ 代码

| 文件 | 行数 | 说明 |
|------|------|------|
| llama_wrapper.cpp | ~1263 | Llama.cpp 封装，实现模型加载、生成、流式输出 |
| jni_interface.cpp | ~550 | JNI 接口层，连接 Kotlin 和 C++ |
| llm_engine.cpp | ~200 | 引擎辅助类，提供工具函数 |
| CMakeLists.txt | ~80 | CMake 构建配置 |

### 配置文件

| 文件 | 说明 |
|------|------|
| build.gradle.kts (app) | 应用模块配置，依赖声明 |
| build.gradle.kts (root) | 根项目配置 |
| settings.gradle.kts | 项目设置 |
| gradle.properties | Gradle 构建属性 |
| AndroidManifest.xml | 应用清单，权限声明 |

## 代码统计

| 类别 | 文件数 | 代码行数 |
|------|--------|----------|
| Kotlin | 12 | ~1,500 |
| C++ | 5 | ~2,100 |
| 配置 | 6 | ~300 |
| 文档 | 3 | ~800 |
| **总计** | **26** | **~4,700** |

## 功能模块

### 1. 模型加载模块
- `LlmEngine.initialize()` - 同步加载
- `LlmEngine.initializeWithProgress()` - 带进度加载
- `ProgressCallback` - 进度回调接口

### 2. 文本生成模块
- `LlmEngine.generate()` - 同步生成
- `LlmEngine.generateStream()` - 流式生成
- `LlmEngine.chatStream()` - 对话模式生成
- `StreamCallback` - 流式输出回调

### 3. 上下文管理模块
- `LlmEngine.clearContext()` - 清空上下文
- `LlmEngine.getContextSize()` - 获取上下文大小
- `LlmEngine.getRemainingContext()` - 获取剩余上下文

### 4. 参数配置模块
- `GenerationParams` - 生成参数
- `LlmEngine.setParams()` - 动态设置参数

### 5. UI 模块
- `MainScreen` - 主界面
- `ChatViewModel` - 状态管理
- `SettingsDialog` - 设置对话框

## Native 方法列表

| 方法 | 说明 |
|------|------|
| nativeInit | 初始化模型 |
| nativeInitWithProgress | 带进度初始化 |
| nativeGenerate | 同步生成 |
| nativeGenerateStream | 流式生成 |
| nativeStopStream | 停止生成 |
| nativeIsInitialized | 检查初始化状态 |
| nativeIsGenerating | 检查生成状态 |
| nativeGetModelInfo | 获取模型信息 |
| nativeTokenize | 分词 |
| nativeDetokenize | 反分词 |
| nativeGetTokenCount | 统计 token 数量 |
| nativeClearContext | 清空上下文 |
| nativeGetContextSize | 获取上下文大小 |
| nativeGetRemainingContext | 获取剩余上下文 |
| nativeSetParams | 设置参数 |
| nativeRelease | 释放资源 |

## 依赖库

### Kotlin 依赖
- AndroidX Core KTX 1.12.0
- Lifecycle ViewModel Compose 2.7.0
- DataStore Preferences 1.0.0
- Activity Compose 1.8.2
- Compose BOM 2024.02.00
- Material 3

### Native 依赖
- llama.cpp (最新版)
- ggml (随 llama.cpp)
- Android NDK r26+

## 构建输出

```
app/build/outputs/apk/
├── debug/
│   └── app-debug.apk       # Debug 版本 APK
└── release/
    └── app-release.apk     # Release 版本 APK (需签名)
```

## 资源需求

| 项目 | 最低要求 | 推荐配置 |
|------|----------|----------|
| Android SDK | 24 (7.0) | 34 (14.0) |
| NDK | r25 | r26+ |
| 设备内存 | 4GB | 8GB+ |
| 设备 CPU | ARM64 | ARMv8.2+ |
| 存储空间 | 3GB | 10GB+ |

## 后续扩展建议

1. **模型管理增强**
   - 在线模型下载
   - 模型自动选择
   - 多模型切换

2. **性能优化**
   - GPU 加速 (Vulkan/OpenCL)
   - NNAPI 集成
   - 模型预加载

3. **功能扩展**
   - 语音输入/输出
   - 图片理解 (多模态)
   - 聊天导出/导入

4. **UI 改进**
   - 深色模式优化
   - 自定义主题
   - 无障碍支持
