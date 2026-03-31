#ifndef LLAMA_WRAPPER_H
#define LLAMA_WRAPPER_H

/**
 * llama.cpp wrapper for Android
 * 
 * 完整的llama.cpp集成实现，支持：
 * - GGUF模型加载
 * - 流式token生成
 * - 多线程推理
 * - 上下文管理
 */

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

namespace localai {

// ============================================================
// 类型定义
// ============================================================

/**
 * Token回调函数类型
 * @param token 生成的token文本
 * @param userData 用户数据指针
 */
using TokenCallback = void(*)(const char* token, void* userData);

/**
 * 进度回调函数类型
 * @param progress 加载进度 (0.0 - 1.0)
 * @param userData 用户数据指针
 */
using ProgressCallback = void(*)(float progress, void* userData);

/**
 * 生成参数
 */
struct GenerationParams {
    int maxTokens = 512;           // 最大生成token数
    float temperature = 0.7f;      // 温度参数
    float topP = 0.9f;             // Top-p采样
    int topK = 40;                 // Top-k采样
    float repeatPenalty = 1.1f;    // 重复惩罚
    int repeatLastN = 64;          // 重复惩罚窗口
    int nThreads = 4;              // 推理线程数
    int nCtx = 2048;               // 上下文长度
    int nBatch = 512;              // 批处理大小
    bool useMmap = true;           // 使用内存映射
    bool useMlock = false;         // 锁定内存（需要root）
};

/**
 * 模型信息
 */
struct ModelInfo {
    std::string name;              // 模型名称
    std::string architecture;      // 架构类型
    int contextLength = 0;         // 上下文长度
    int embeddingLength = 0;       // 嵌入维度
    int vocabSize = 0;             // 词汇表大小
    int layerCount = 0;            // 层数
    int headCount = 0;             // 注意力头数
    std::string quantization;      // 量化类型
    size_t fileSize = 0;           // 文件大小
    size_t memoryRequired = 0;     // 所需内存
};

// ============================================================
// LlamaWrapper 类
// ============================================================

/**
 * llama.cpp的C++封装类
 * 提供线程安全的API接口
 */
class LlamaWrapper {
public:
    static LlamaWrapper& getInstance();
    
    // 初始化与释放
    bool initialize(const char* modelPath, const GenerationParams& params = {});
    bool initializeWithProgress(
        const char* modelPath,
        ProgressCallback callback,
        void* userData,
        const GenerationParams& params = {}
    );
    void release();
    bool isInitialized() const;
    
    // 文本生成
    std::string generate(const std::string& prompt, const GenerationParams& params = {});
    void generateStream(
        const std::string& prompt,
        TokenCallback callback,
        void* userData,
        const GenerationParams& params = {}
    );
    
    // 控制
    void stopGeneration();
    bool isGenerating() const;
    
    // 信息获取
    ModelInfo getModelInfo() const;
    std::string getLastToken() const;
    int getTokenCount() const;
    
    // 上下文管理
    void clearContext();
    int getContextSize() const;
    int getRemainingContext() const;
    
    // Tokenization
    std::vector<int> tokenize(const std::string& text);
    std::string detokenize(const std::vector<int>& tokens);
    int getTokenCount(const std::string& text);
    
private:
    LlamaWrapper();
    ~LlamaWrapper();
    
    // 禁止拷贝
    LlamaWrapper(const LlamaWrapper&) = delete;
    LlamaWrapper& operator=(const LlamaWrapper&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

// ============================================================
// C接口函数（供JNI调用）
// ============================================================

extern "C" {
    /**
     * 初始化llama上下文
     * @param modelPath GGUF模型文件路径
     * @return 成功返回true
     */
    bool llama_init_from_file(const char* modelPath);
    
    /**
     * 使用进度回调初始化
     */
    bool llama_init_with_progress(
        const char* modelPath,
        ProgressCallback callback,
        void* userData
    );
    
    /**
     * 同步生成文本
     * @param prompt 输入提示
     * @param outputBuffer 输出缓冲区
     * @param bufferSize 缓冲区大小
     * @param maxTokens 最大生成token数
     * @return 生成的token数
     */
    int llama_generate(
        const char* prompt,
        char* outputBuffer,
        int bufferSize,
        int maxTokens
    );
    
    /**
     * 流式生成文本
     * @param prompt 输入提示
     * @param callback Token回调
     * @param userData 用户数据
     * @param maxTokens 最大生成token数
     * @return 成功返回true
     */
    bool llama_generate_stream(
        const char* prompt,
        TokenCallback callback,
        void* userData,
        int maxTokens
    );
    
    /**
     * 停止生成
     */
    void llama_stop_generation();
    
    /**
     * 释放资源
     */
    void llama_free();
    
    /**
     * 检查是否已初始化
     */
    bool llama_is_initialized();
    
    /**
     * 检查是否正在生成
     */
    bool llama_is_generating();
    
    /**
     * 获取模型信息
     */
    const char* llama_get_model_info();
    
    /**
     * 清除上下文
     */
    void llama_clear_context();
    
    /**
     * 获取上下文大小
     */
    int llama_get_context_size();
    
    /**
     * 获取剩余上下文
     */
    int llama_get_remaining_context();
    
    /**
     * 设置生成参数
     */
    void llama_set_params(
        float temperature,
        float topP,
        int topK,
        float repeatPenalty,
        int nThreads
    );
}

} // namespace localai

#endif // LLAMA_WRAPPER_H
