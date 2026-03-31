# LocalAI Chat - Android 离线 AI 聊天应用

一个完全离线运行的 Android AI 聊天应用，使用 llama.cpp 在本地设备上运行大型语言模型 (LLM)。

## 特性

- 🔒 **完全离线** - 无需网络连接，数据不上传云端
- 🚀 **高性能推理** - 基于 llama.cpp，支持 ARM NEON/FP16 优化
- 📱 **Material 3 UI** - 现代化 Jetpack Compose 界面
- 🔄 **流式生成** - 实时显示生成的文本，支持中断
- ⚙️ **参数可调** - 温度、Top-P、Top-K 等参数自由调整
- 💾 **历史记录** - 自动保存聊天历史

## 架构

```
┌─────────────────────────────────────────────────────────┐
│                    Presentation Layer                    │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ MainScreen  │  │ ChatViewModel│  │   Settings    │  │
│  │  (Compose)  │──│   (MVVM)     │──│    Dialog     │  │
│  └─────────────┘  └──────────────┘  └───────────────┘  │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│                     Domain Layer                         │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │  LlmEngine  │  │ Generation   │  │  ModelInfo    │  │
│  │ (Singleton) │  │   Params     │  │               │  │
│  └─────────────┘  └──────────────┘  └───────────────┘  │
└────────────────────────┬────────────────────────────────┘
                         │ JNI Bridge
┌────────────────────────▼────────────────────────────────┐
│                     Native Layer (C++17)                 │
│  ┌─────────────────┐  ┌──────────────────────────────┐  │
│  │  jni_interface  │──│      llama_wrapper.cpp       │  │
│  │    (JNI层)      │  │   (LlamaWrapper 单例封装)     │  │
│  └─────────────────┘  └──────────────┬───────────────┘  │
│                                      │                   │
│                       ┌──────────────▼───────────────┐   │
│                       │      llama.cpp 库            │   │
│                       │   (GGUF 模型推理引擎)         │   │
│                       └──────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## 技术栈

| 层级 | 技术 |
|------|------|
| UI | Jetpack Compose + Material 3 |
| 架构 | MVVM + Repository Pattern |
| 异步 | Kotlin Coroutines + Flow |
| 存储 | DataStore Preferences |
| Native | C++17 + JNI |
| 推理引擎 | llama.cpp (GGUF) |
| 构建系统 | Gradle (Kotlin DSL) + CMake |

## 项目结构

```
app/src/main/
├── java/com/localai/chat/
│   ├── engine/                    # LLM 引擎层
│   │   ├── LlmEngine.kt          # 单例引擎类
│   │   ├── GenerationParams.kt   # 生成参数
│   │   ├── ModelInfo.kt          # 模型信息
│   │   ├── StreamCallback.kt     # 流式回调
│   │   └── ProgressCallback.kt   # 进度回调
│   │
│   ├── data/                      # 数据层
│   │   ├── Message.kt            # 消息实体
│   │   ├── ChatRepository.kt     # 聊天仓库
│   │   └── ModelManager.kt       # 模型管理
│   │
│   ├── viewmodel/                 # ViewModel 层
│   │   └── ChatViewModel.kt      # 聊天 ViewModel
│   │
│   ├── ui/                        # UI 层
│   │   ├── MainScreen.kt         # 主界面
│   │   └── theme/                # Material 3 主题
│   │
│   └── MainActivity.kt           # 入口 Activity
│
├── cpp/                           # Native C++ 层
│   ├── llama_wrapper.cpp/.h      # Llama.cpp 封装 (1263行)
│   ├── jni_interface.cpp         # JNI 接口层 (~550行)
│   ├── llm_engine.cpp/.h         # 引擎辅助类
│   └── CMakeLists.txt            # CMake 配置
│
├── assets/                        # 资源文件
│   └── (可放置默认模型)
│
└── AndroidManifest.xml
```

## 快速开始

### 1. 环境要求

- Android Studio Hedgehog 或更高版本
- Android SDK 24+ (Android 7.0)
- NDK r25+ (推荐 r26)
- CMake 3.22.1+
- 8GB+ RAM (推荐 12GB+)
- ARM64 设备 (ARMv8-A)

### 2. 集成 llama.cpp

#### 方式一：Git Submodule（推荐）

```bash
cd /path/to/LocalAIChat
git submodule add https://github.com/ggerganov/llama.cpp.git
git submodule update --init --recursive
```

#### 方式二：手动下载

1. 从 [llama.cpp releases](https://github.com/ggerganov/llama.cpp/releases) 下载源码
2. 解压到 `app/src/main/cpp/llama.cpp/`

### 3. 构建 Native 库

```bash
# Windows
gradlew.bat assembleDebug

# macOS/Linux
./gradlew assembleDebug
```

首次构建会编译 llama.cpp，可能需要 5-15 分钟。

### 4. 获取模型

推荐以下 GGUF 格式模型：

| 模型 | 参数量 | 最低内存 | 推荐设备 |
|------|--------|----------|----------|
| Qwen2.5-1.5B-Instruct | 1.5B | 2GB | 低端机 |
| Qwen2.5-3B-Instruct | 3B | 4GB | 中端机 |
| Llama-3.2-3B-Instruct | 3B | 4GB | 中端机 |
| Qwen2.5-7B-Instruct | 7B | 8GB | 高端机 |
| Llama-3.1-8B-Instruct | 8B | 8GB | 高端机 |

模型下载地址：
- [Hugging Face](https://huggingface.co/models?search=gguf)
- [ModelScope](https://modelscope.cn/models) (国内镜像)

### 5. 运行应用

1. 安装 APK 到设备
2. 启动应用
3. 点击"选择模型文件"加载 GGUF 模型
4. 开始聊天！

## JNI API 参考

### LlmEngine 单例 API

```kotlin
// 获取引擎实例
val engine = LlmEngine.getInstance()

// 初始化模型
engine.initialize(modelPath: String): Boolean

// 带进度初始化
engine.initializeWithProgress(
    modelPath: String,
    callback: ProgressCallback
): Boolean

// 同步生成
engine.generate(
    prompt: String,
    params: GenerationParams? = null
): String

// 流式生成
engine.generateStream(
    prompt: String,
    callback: StreamCallback,
    params: GenerationParams? = null
)

// 对话流式生成（支持历史）
engine.chatStream(
    userMessage: String,
    callback: StreamCallback,
    systemPrompt: String? = null,
    history: List<Pair<String, String>>? = null,
    params: GenerationParams? = null
)

// 停止生成
engine.stopGeneration()

// 检查状态
engine.isInitialized(): Boolean
engine.isGenerating(): Boolean

// 获取模型信息
engine.getModelInfo(): ModelInfo?

// 上下文管理
engine.clearContext()
engine.getContextSize(): Int
engine.getRemainingContext(): Int

// 设置参数
engine.setParams(params: GenerationParams)

// 释放资源
engine.release()
```

### GenerationParams 参数

```kotlin
data class GenerationParams(
    val maxTokens: Int = 512,      // 最大生成长度
    val temperature: Float = 0.7f, // 温度 (0.1-2.0)
    val topP: Float = 0.9f,        // Top-P 采样
    val topK: Int = 40,            // Top-K 采样
    val repeatPenalty: Float = 1.1f, // 重复惩罚
    val repeatLastN: Int = 64,     // 重复惩罚窗口
    val nThreads: Int = 4,         // 线程数
    val nCtx: Int = 2048,          // 上下文长度
    val nBatch: Int = 512,         // 批处理大小
    val useMmap: Boolean = true,   // 使用内存映射
    val useMlock: Boolean = false  // 锁定内存
)
```

### StreamCallback 回调

```kotlin
interface StreamCallback {
    fun onToken(token: String)  // 收到一个 token
    fun onComplete()            // 生成完成
    fun onError(error: String?) // 发生错误
}
```

### ProgressCallback 回调

```kotlin
interface ProgressCallback {
    fun onProgress(progress: Float, message: String?)  // 进度更新
    fun onComplete()                                   // 加载完成
    fun onError(error: String?)                        // 加载错误
}
```

## Native C++ API 参考

### Native 方法签名

```cpp
// 初始化
JNIEXPORT jboolean JNICALL nativeInit(
    JNIEnv* env, jobject thiz, jstring modelPath);

JNIEXPORT jboolean JNICALL nativeInitWithProgress(
    JNIEnv* env, jobject thiz, jstring modelPath, jobject callback);

// 生成
JNIEXPORT jstring JNICALL nativeGenerate(
    JNIEnv* env, jobject thiz, jstring prompt, jobject params);

JNIEXPORT void JNICALL nativeGenerateStream(
    JNIEnv* env, jobject thiz, jstring prompt, jobject callback, jobject params);

JNIEXPORT void JNICALL nativeStopStream(
    JNIEnv* env, jobject thiz);

// 状态查询
JNIEXPORT jboolean JNICALL nativeIsInitialized(
    JNIEnv* env, jobject thiz);

JNIEXPORT jboolean JNICALL nativeIsGenerating(
    JNIEnv* env, jobject thiz);

JNIEXPORT jobject JNICALL nativeGetModelInfo(
    JNIEnv* env, jobject thiz);

// 上下文管理
JNIEXPORT void JNICALL nativeClearContext(
    JNIEnv* env, jobject thiz);

JNIEXPORT jint JNICALL nativeGetContextSize(
    JNIEnv* env, jobject thiz);

JNIEXPORT jint JNICALL nativeGetRemainingContext(
    JNIEnv* env, jobject thiz);

// Token 操作
JNIEXPORT jintArray JNICALL nativeTokenize(
    JNIEnv* env, jobject thiz, jstring text);

JNIEXPORT jstring JNICALL nativeDetokenize(
    JNIEnv* env, jobject thiz, jintArray tokens);

JNIEXPORT jint JNICALL nativeGetTokenCount(
    JNIEnv* env, jobject thiz, jstring text);

// 参数设置
JNIEXPORT void JNICALL nativeSetParams(
    JNIEnv* env, jobject thiz, jobject params);

// 资源释放
JNIEXPORT void JNICALL nativeRelease(
    JNIEnv* env, jobject thiz);
```

### LlamaWrapper 核心类

```cpp
class LlamaWrapper {
public:
    static LlamaWrapper* getInstance();
    
    // 模型管理
    bool loadModel(const std::string& modelPath, ProgressCallback* callback = nullptr);
    void unloadModel();
    bool isLoaded() const;
    
    // 生成
    std::string generate(const std::string& prompt, const GenerationParams& params);
    void generateStream(const std::string& prompt, 
                        StreamCallback* callback,
                        const GenerationParams& params);
    void stopGeneration();
    bool isGenerating() const;
    
    // 对话
    void chat(const std::string& userMessage,
              StreamCallback* callback,
              const std::string& systemPrompt = "",
              const std::vector<std::pair<std::string, std::string>>& history = {},
              const GenerationParams& params = GenerationParams());
    
    // 上下文
    void clearContext();
    int getContextSize() const;
    int getRemainingContext() const;
    
    // Token 操作
    std::vector<int> tokenize(const std::string& text, bool addBos = true);
    std::string detokenize(const std::vector<int>& tokens);
    int countTokens(const std::string& text);
    
    // 模型信息
    ModelInfo getModelInfo() const;
    
    // 参数设置
    void setParams(const GenerationParams& params);
    
private:
    LlamaWrapper();
    ~LlamaWrapper();
    
    // llama.cpp 上下文和模型
    llama_model* model_;
    llama_context* ctx_;
    std::vector<llama_token> tokens_;
    std::atomic<bool> stop_flag_;
    std::mutex mutex_;
};
```

## 性能优化建议

### 模型选择

1. **量化格式**：优先选择 Q4_K_M 或 Q5_K_M 量化
   - Q4_K_M：速度最快，质量略降
   - Q5_K_M：质量与速度平衡
   - Q8_0：质量最高，内存占用大

2. **参数量选择**：
   - 4GB RAM 设备：≤3B 参数模型
   - 6GB RAM 设备：≤5B 参数模型
   - 8GB RAM 设备：≤7B 参数模型

### 推理参数调优

```kotlin
// 快速生成（低质量）
GenerationParams(
    maxTokens = 256,
    temperature = 0.5f,
    topP = 0.8f,
    topK = 20,
    nThreads = 4
)

// 平衡模式
GenerationParams(
    maxTokens = 512,
    temperature = 0.7f,
    topP = 0.9f,
    topK = 40,
    nThreads = 4
)

// 高质量生成（较慢）
GenerationParams(
    maxTokens = 1024,
    temperature = 0.8f,
    topP = 0.95f,
    topK = 60,
    nThreads = 6
)
```

### 线程数设置

- **2核 CPU**：nThreads = 2
- **4核 CPU**：nThreads = 4
- **6核 CPU**：nThreads = 4-6
- **8核+ CPU**：nThreads = 6

> 注意：线程数并非越多越好，过多线程会导致上下文切换开销

### 内存优化

```kotlin
// 低内存设备
GenerationParams(
    nCtx = 1024,      // 较短上下文
    nBatch = 256,     // 较小批次
    useMmap = true,   // 必须开启
    useMlock = false  // 不锁定内存
)

// 高内存设备
GenerationParams(
    nCtx = 4096,      // 长上下文
    nBatch = 512,     // 大批次
    useMmap = true,
    useMlock = true   // 可锁定内存提升性能
)
```

## 常见问题 (FAQ)

### Q1: 模型加载失败

**原因**：
- 模型文件损坏或格式不正确
- 内存不足
- 模型路径错误

**解决方案**：
1. 确认模型是 GGUF 格式
2. 检查模型文件完整性（MD5 校验）
3. 尝试更小的模型或量化版本
4. 确保模型路径正确（不含中文或特殊字符）

### Q2: 生成速度慢

**优化方案**：
1. 使用 Q4_K_M 量化模型
2. 减少 `nCtx` 上下文长度
3. 减少 `maxTokens` 生成长度
4. 关闭不必要的应用释放内存

### Q3: 应用崩溃

**可能原因**：
- 内存溢出 (OOM)
- Native 库加载失败
- NDK 版本不匹配

**调试方法**：
```bash
# 查看 logcat 日志
adb logcat | grep -E "LocalAI|llama|JNI"

# 检查 Native 崩溃
adb logcat | grep -i "fatal\|crash"
```

### Q4: 生成的文本乱码或重复

**原因**：
- Temperature 设置不当
- Repeat Penalty 未启用
- 模型对中文支持不佳

**解决方案**：
1. 调整 temperature (推荐 0.7-0.9)
2. 启用 repeatPenalty (推荐 1.1-1.2)
3. 使用支持中文的模型（如 Qwen 系列）

### Q5: 如何在应用内嵌入模型？

1. 将模型放入 `app/src/main/assets/` 目录
2. 启动时复制到内部存储：

```kotlin
fun copyModelFromAssets(context: Context, modelName: String): String {
    val outFile = File(context.filesDir, modelName)
    if (!outFile.exists()) {
        context.assets.open(modelName).use { input ->
            outFile.outputStream().use { output ->
                input.copyTo(output)
            }
        }
    }
    return outFile.absolutePath
}
```

## 构建配置

### CMakeLists.txt 关键配置

```cmake
# 启用 CPU 优化
add_definitions(-DGGML_USE_CPU)

# ARM64 优化
if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
endif()

# 编译 llama.cpp
add_subdirectory(llama.cpp)

# 链接库
target_link_libraries(llama_wrapper
    llama
    ggml
    log
    android
)
```

### build.gradle.kts 配置

```kotlin
android {
    defaultConfig {
        ndk {
            abiFilters += listOf("arm64-v8a")
        }
    }
    
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    
    ndkVersion = "26.1.10909125"
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.datastore:datastore-preferences:1.0.0")
    implementation("androidx.activity:activity-compose:1.8.2")
    implementation(platform("androidx.compose:compose-bom:2024.02.00"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.material3:material3")
}
```

## 文档索引

| 文档 | 说明 |
|------|------|
| [README.md](./README.md) | 项目完整文档（本文档） |
| [QUICKSTART.md](./QUICKSTART.md) | 5 分钟快速开始指南 |
| [USAGE_GUIDE.md](./USAGE_GUIDE.md) | 功能使用详细指南 |
| [CHANGELOG.md](./CHANGELOG.md) | 版本更新日志 |
| [FILE_MANIFEST.md](./FILE_MANIFEST.md) | 完整文件清单 |

## 许可证

本项目采用 MIT 许可证。

llama.cpp 采用 MIT 许可证。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 致谢

- [llama.cpp](https://github.com/ggerganov/llama.cpp) - 高性能 LLM 推理引擎
- [Hugging Face](https://huggingface.co/) - 模型托管平台
- [Qwen Team](https://github.com/QwenLM) - 优秀的中文 LLM 模型

---

**开发者**: LocalAI Chat Team  
**版本**: 1.0.0  
**最后更新**: 2026-03-30
