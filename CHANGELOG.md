# 更新日志 (Changelog)

本文档记录 LocalAI Chat 的所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

---

## [1.0.0] - 2026-03-31

### 首次发布 🎉

LocalAI Chat 首个正式版本发布！这是一个完全离线运行的 Android AI 聊天应用。

### 新增功能 (Added)

#### 核心功能
- ✨ **离线 LLM 推理** - 基于 llama.cpp，完全本地运行，无需网络
- ✨ **GGUF 模型支持** - 支持所有 GGUF 格式模型（Qwen、Llama、Mistral 等）
- ✨ **流式生成** - 实时显示生成的文本，支持中断
- ✨ **对话历史** - 自动保存聊天记录，支持多轮对话

#### 模型管理
- ✨ **模型加载进度** - 实时显示模型加载进度和状态
- ✨ **模型信息展示** - 显示参数量、上下文长度、量化格式等
- ✨ **快速模型切换** - 支持运行时切换不同模型

#### 参数配置
- ✨ **Temperature** - 温度调节 (0.1 - 2.0)
- ✨ **Top-P 采样** - 核采样参数调节
- ✨ **Top-K 采样** - Top-K 参数调节
- ✨ **重复惩罚** - Repeat Penalty 防止重复生成
- ✨ **最大生成长度** - 可配置最大 token 数
- ✨ **上下文长度** - 可配置上下文窗口大小
- ✨ **线程数** - 根据设备 CPU 调整推理线程

#### UI/UX
- ✨ **Material 3 设计** - 现代化 Jetpack Compose 界面
- ✨ **深色模式** - 自动跟随系统主题
- ✨ **响应式布局** - 适配手机和平板

#### Native 层
- ✨ **LlamaWrapper 单例** - C++ 层高效封装 llama.cpp
- ✨ **完整 JNI 接口** - 16 个 Native 方法全覆盖
- ✨ **线程安全** - 多线程安全设计
- ✨ **内存优化** - 支持 mmap 和 mlock

### 技术实现 (Technical)

#### Kotlin 代码 (~1,500 行)
| 模块 | 文件 | 说明 |
|------|------|------|
| Engine | LlmEngine.kt | 单例引擎类 |
| Engine | GenerationParams.kt | 生成参数定义 |
| Engine | ModelInfo.kt | 模型信息数据类 |
| Engine | StreamCallback.kt | 流式回调接口 |
| Engine | ProgressCallback.kt | 进度回调接口 |
| Data | Message.kt, ChatMessage.kt | 消息实体 |
| Data | ChatRepository.kt | 聊天仓库 |
| Data | ModelManager.kt | 模型管理 |
| ViewModel | ChatViewModel.kt | 状态管理 |
| UI | MainScreen.kt | 主界面 |
| UI | theme/* | Material 3 主题 |

#### C++ 代码 (~2,100 行)
| 模块 | 文件 | 说明 |
|------|------|------|
| Core | llama_wrapper.cpp/.h | Llama.cpp 封装 (1263行) |
| Core | llm_engine.cpp/.h | 引擎辅助类 |
| JNI | jni_interface.cpp | JNI 接口层 (~550行) |
| Build | CMakeLists.txt | CMake 配置 |

#### JNI API (16 个方法)
```cpp
// 初始化
nativeInit, nativeInitWithProgress

// 生成
nativeGenerate, nativeGenerateStream, nativeStopStream

// 状态查询
nativeIsInitialized, nativeIsGenerating, nativeGetModelInfo

// 上下文管理
nativeClearContext, nativeGetContextSize, nativeGetRemainingContext

// Token 操作
nativeTokenize, nativeDetokenize, nativeGetTokenCount

// 参数设置
nativeSetParams

// 资源管理
nativeRelease
```

### 构建系统 (Build)

- ✅ Gradle Kotlin DSL 配置
- ✅ CMake Native 构建集成
- ✅ 跨平台构建脚本 (build.sh / build.bat)
- ✅ 自动下载 llama.cpp 子模块

### 文档 (Documentation)

- 📚 README.md - 完整项目文档 (~500 行)
- 📚 QUICKSTART.md - 5 分钟快速上手指南
- 📚 FILE_MANIFEST.md - 文件清单和代码统计
- 📚 CHANGELOG.md - 版本变更记录

---

## [0.9.0-beta] - 2026-03-28

### 新增
- Beta 版本，核心功能测试完成
- 基本的模型加载和文本生成
- JNI 接口初步实现

### 已知问题
- 性能优化尚未完成
- 部分模型兼容性问题

---

## [0.5.0-alpha] - 2026-03-20

### 新增
- 项目初始化
- 基础 UI 框架
- MVVM 架构搭建

---

## 未来计划 (Roadmap)

### [1.1.0] - 计划中

#### 性能优化
- 🔲 GPU 加速 (Vulkan)
- 🔲 NNAPI 集成
- 🔲 模型预加载缓存

#### 功能增强
- 🔲 多模型同时加载
- 🔲 在线模型下载
- 🔲 模板系统（预设 prompt）

#### UI 改进
- 🔲 聊天导出/导入
- 🔲 自定义主题颜色
- 🔲 Markdown 渲染

### [1.2.0] - 远期规划

#### 多模态支持
- 🔲 图像理解 (LLaVA)
- 🔲 语音输入
- 🔲 语音输出 (TTS)

#### 高级功能
- 🔲 Function Calling
- 🔲 RAG 文档问答
- 🔲 Agent 模式

---

## 版本说明

### 版本号规则

- **主版本号 (Major)**: 不兼容的 API 修改
- **次版本号 (Minor)**: 向下兼容的功能性新增
- **修订号 (Patch)**: 向下兼容的问题修正

### 发布周期

- **Major/Minor 版本**: 根据功能开发进度发布
- **Patch 版本**: 按需发布，修复紧急问题

---

## 贡献者

感谢所有为本项目做出贡献的开发者！

<!-- 在此添加贡献者名单 -->

---

## 链接

- [GitHub Repository](https://github.com/your-repo/LocalAIChat)
- [问题反馈](https://github.com/your-repo/LocalAIChat/issues)
- [llama.cpp](https://github.com/ggerganov/llama.cpp)
