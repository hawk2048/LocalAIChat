/**
 * llama_wrapper.cpp - llama.cpp核心调用实现
 * 
 * 完整的llama.cpp集成实现，支持：
 * - GGUF模型加载与推理
 * - 流式token生成
 * - 多线程推理优化
 * - KV缓存管理
 * - 采样参数控制
 */

#include "llama_wrapper.h"
#include "third_party/llama.h"
#include "third_party/ggml.h"

#include <android/log.h>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <fstream>
#include <thread>
#include <sstream>
#include <iomanip>

#define LOG_TAG "LlamaWrapper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace localai {

// ============================================================
// 辅助函数
// ============================================================

/**
 * 创建单token的batch（用于生成循环）
 */
static struct llama_batch llama_batch_get_one(llama_token token, int32_t pos) {
    static llama_token tokens[1];
    static int32_t pos_array[1];
    static int32_t n_seq_id_array[1] = {1};
    static llama_token seq_id_array[1] = {0};
    
    tokens[0] = token;
    pos_array[0] = pos;
    
    struct llama_batch batch = {
        .token = tokens,
        .embd = nullptr,
        .pos = pos_array,
        .n_seq_id = n_seq_id_array,
        .seq_id = &seq_id_array,
        .logits = nullptr,
        .n_tokens = 1,
    };
    
    return batch;
}

/**
 * 创建输入batch
 */
static struct llama_batch llama_batch_from_tokens(
    const std::vector<llama_token>& tokens,
    int32_t startPos = 0
) {
    struct llama_batch batch;
    batch.n_tokens = static_cast<int32_t>(tokens.size());
    
    // 分配内存
    batch.token = new llama_token[tokens.size()];
    batch.pos = new int32_t[tokens.size()];
    batch.n_seq_id = new int32_t[tokens.size()];
    batch.seq_id = new llama_token*[tokens.size()];
    batch.logits = new int8_t[tokens.size()];
    batch.embd = nullptr;
    
    for (size_t i = 0; i < tokens.size(); i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = startPos + static_cast<int32_t>(i);
        batch.n_seq_id[i] = 1;
        batch.seq_id[i] = new llama_token[1];
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == tokens.size() - 1) ? 1 : 0; // 只需要最后一个token的logits
    }
    
    return batch;
}

/**
 * 释放batch内存
 */
static void llama_batch_free_custom(struct llama_batch& batch) {
    if (batch.token) delete[] batch.token;
    if (batch.pos) delete[] batch.pos;
    if (batch.n_seq_id) delete[] batch.n_seq_id;
    if (batch.seq_id) {
        for (int32_t i = 0; i < batch.n_tokens; i++) {
            if (batch.seq_id[i]) delete[] batch.seq_id[i];
        }
        delete[] batch.seq_id;
    }
    if (batch.logits) delete[] batch.logits;
    
    batch.token = nullptr;
    batch.pos = nullptr;
    batch.n_seq_id = nullptr;
    batch.seq_id = nullptr;
    batch.logits = nullptr;
    batch.n_tokens = 0;
}

// ============================================================
// LlamaWrapper::Impl - 内部实现
// ============================================================

struct LlamaWrapper::Impl {
    // llama.cpp 对象
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    llama_sampler* sampler = nullptr;
    
    // 生成参数
    GenerationParams params;
    
    // 状态
    std::atomic<bool> initialized{false};
    std::atomic<bool> generating{false};
    std::atomic<bool> shouldStop{false};
    
    // Token相关
    std::vector<llama_token> inputTokens;
    std::vector<llama_token> outputTokens;
    std::vector<llama_token> cachedTokens;
    
    // 模型信息
    ModelInfo modelInfo;
    
    // 位置追踪
    int32_t currentPos = 0;
    
    // 线程安全
    mutable std::mutex mutex;
    
    // 默认系统提示
    static constexpr const char* DEFAULT_SYSTEM_PROMPT = 
        "You are a helpful AI assistant. Respond concisely and accurately.";
    
    // ========================================================
    // 资源管理
    // ========================================================
    
    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex);
        
        // 释放采样器
        if (sampler) {
            llama_sampler_free(sampler);
            sampler = nullptr;
        }
        
        // 释放上下文
        if (ctx) {
            llama_free(ctx);
            ctx = nullptr;
        }
        
        // 释放模型
        if (model) {
            llama_free_model(model);
            model = nullptr;
        }
        
        initialized = false;
        generating = false;
        shouldStop = false;
        inputTokens.clear();
        outputTokens.clear();
        cachedTokens.clear();
        currentPos = 0;
        
        LOGI("LlamaWrapper resources released");
    }
    
    // ========================================================
    // 模型加载
    // ========================================================
    
    bool loadModel(const char* modelPath, ProgressCallback progressCb, void* userData) {
        LOGI("Loading model from: %s", modelPath);
        
        // 检查文件是否存在
        std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            LOGE("Model file not found: %s", modelPath);
            return false;
        }
        
        size_t fileSize = file.tellg();
        file.close();
        
        LOGI("Model file size: %.2f GB", fileSize / (1024.0 * 1024.0 * 1024.0));
        
        // 报告进度：开始加载
        if (progressCb) progressCb(0.0f, userData);
        
        // 初始化后端（只需调用一次）
        static bool backendInitialized = false;
        if (!backendInitialized) {
            llama_backend_init();
            backendInitialized = true;
            LOGI("LLaMA backend initialized");
        }
        
        // 配置模型参数
        llama_model_params model_params = llama_model_default_params();
        model_params.use_mmap = params.useMmap;
        model_params.use_mlock = params.useMlock;
        model_params.vocab_only = false;
        
        // 报告进度：开始加载模型
        if (progressCb) progressCb(0.1f, userData);
        
        // 加载模型
        LOGI("Loading model with mmap=%s, mlock=%s", 
             params.useMmap ? "true" : "false",
             params.useMlock ? "true" : "false");
        
        model = llama_load_model_from_file(modelPath, model_params);
        if (!model) {
            LOGE("Failed to load model from: %s", modelPath);
            LOGE("Ensure the model is a valid GGUF file");
            return false;
        }
        
        // 报告进度：模型加载完成
        if (progressCb) progressCb(0.6f, userData);
        
        // 配置上下文参数
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = params.nCtx;
        ctx_params.n_batch = params.nBatch;
        ctx_params.n_ubatch = params.nBatch;
        ctx_params.n_threads = params.nThreads;
        ctx_params.n_threads_batch = params.nThreads;
        ctx_params.seed = 0xFFFFFFFF; // 随机种子
        ctx_params.logits_all = false; // 只计算最后一个token的logits
        ctx_params.flash_attn = false; // Android暂时不支持flash attention
        
        LOGI("Creating context with n_ctx=%d, n_batch=%d, n_threads=%d",
             ctx_params.n_ctx, ctx_params.n_batch, ctx_params.n_threads);
        
        // 创建上下文
        ctx = llama_new_context_with_model(model, ctx_params);
        if (!ctx) {
            LOGE("Failed to create context");
            llama_free_model(model);
            model = nullptr;
            return false;
        }
        
        // 报告进度：上下文创建完成
        if (progressCb) progressCb(0.8f, userData);
        
        // 初始化采样器
        sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
        if (!sampler) {
            LOGW("Failed to create sampler chain, will use simple sampling");
        } else {
            // 添加采样器到链
            // 顺序：repetition penalty -> top_k -> top_p -> temperature
            setupSampler();
            LOGI("Sampler chain initialized");
        }
        
        // 更新模型信息
        updateModelInfo(modelPath, fileSize);
        
        // 重置位置
        currentPos = 0;
        
        // 报告进度：完成
        if (progressCb) progressCb(1.0f, userData);
        
        LOGI("Model loaded successfully");
        LOGI("  Context length: %d", params.nCtx);
        LOGI("  Threads: %d", params.nThreads);
        LOGI("  Batch size: %d", params.nBatch);
        LOGI("  Vocab size: %d", modelInfo.vocabSize);
        
        return true;
    }
    
    // ========================================================
    // 采样器配置
    // ========================================================
    
    void setupSampler() {
        if (!sampler) return;
        
        // 注意：这些函数需要实际的llama.cpp实现
        // 这里使用简化的采样方法
        
        LOGI("Sampler configured: temp=%.2f, top_p=%.2f, top_k=%d, repeat_penalty=%.2f",
             params.temperature, params.topP, params.topK, params.repeatPenalty);
    }
    
    // ========================================================
    // 模型信息更新
    // ========================================================
    
    void updateModelInfo(const char* modelPath, size_t fileSize) {
        modelInfo.fileSize = fileSize;
        
        // 提取文件名
        std::string pathStr(modelPath);
        size_t lastSlash = pathStr.find_last_of("/\\");
        modelInfo.name = (lastSlash != std::string::npos) ? 
                         pathStr.substr(lastSlash + 1) : pathStr;
        
        if (model && ctx) {
            // 获取上下文长度
            modelInfo.contextLength = static_cast<int>(llama_n_ctx(ctx));
            
            // 获取词汇表大小
            modelInfo.vocabSize = llama_n_vocab(model);
            
            // 获取嵌入维度
            // modelInfo.embeddingLength = llama_n_embd(model);
            
            // 获取层数
            // modelInfo.layerCount = llama_n_layer(model);
            
            // 获取头数
            // modelInfo.headCount = llama_n_head(model);
            
            // 估算内存使用
            modelInfo.memoryRequired = estimateMemoryUsage();
            
            // 尝试检测量化类型
            modelInfo.quantization = detectQuantization();
        }
        
        LOGI("Model info updated:");
        LOGI("  Name: %s", modelInfo.name.c_str());
        LOGI("  Context: %d tokens", modelInfo.contextLength);
        LOGI("  Vocab size: %d", modelInfo.vocabSize);
        LOGI("  Embedding dim: %d", modelInfo.embeddingLength);
        LOGI("  Layers: %d", modelInfo.layerCount);
        LOGI("  Quantization: %s", modelInfo.quantization.c_str());
        LOGI("  Memory required: %.2f MB", modelInfo.memoryRequired / (1024.0 * 1024.0));
    }
    
    // ========================================================
    // 内存估算
    // ========================================================
    
    size_t estimateMemoryUsage() const {
        if (!model) return 0;
        
        // 粗略估算：
        // 模型文件大小 + KV缓存 + 工作缓冲区
        
        size_t modelSize = modelInfo.fileSize;
        
        // KV缓存大小估算
        // KV cache = n_ctx * n_layer * n_embd * 2 * sizeof(float16)
        // 简化估算：每token约 2KB
        size_t kvCacheSize = static_cast<size_t>(params.nCtx) * 2048;
        
        // 工作缓冲区
        size_t workSize = static_cast<size_t>(params.nBatch) * 1024;
        
        return modelSize + kvCacheSize + workSize;
    }
    
    // ========================================================
    // 量化类型检测
    // ========================================================
    
    std::string detectQuantization() const {
        if (!modelInfo.name.empty()) {
            // 从文件名推断量化类型
            const std::string& name = modelInfo.name;
            
            if (name.find("Q4_K_M") != std::string::npos) return "Q4_K_M";
            if (name.find("Q4_K_S") != std::string::npos) return "Q4_K_S";
            if (name.find("Q5_K_M") != std::string::npos) return "Q5_K_M";
            if (name.find("Q5_K_S") != std::string::npos) return "Q5_K_S";
            if (name.find("Q6_K") != std::string::npos) return "Q6_K";
            if (name.find("Q4_0") != std::string::npos) return "Q4_0";
            if (name.find("Q5_0") != std::string::npos) return "Q5_0";
            if (name.find("Q8_0") != std::string::npos) return "Q8_0";
            if (name.find("IQ4_XS") != std::string::npos) return "IQ4_XS";
            if (name.find("IQ3_M") != std::string::npos) return "IQ3_M";
            if (name.find("IQ3_S") != std::string::npos) return "IQ3_S";
            if (name.find("F16") != std::string::npos) return "F16";
            if (name.find("F32") != std::string::npos) return "F32";
        }
        
        return "Unknown";
    }
    
    // ========================================================
    // Tokenization
    // ========================================================
    
    std::vector<llama_token> tokenizeText(const std::string& text, bool addBos = true) {
        if (!model) {
            LOGE("Model not loaded for tokenization");
            return {};
        }
        
        // 预分配token数组
        std::vector<llama_token> tokens;
        tokens.resize(text.size() + 8); // 预估空间
        
        // 调用tokenize
        int n = llama_tokenize(
            model,
            text.c_str(),
            static_cast<int32_t>(text.size()),
            tokens.data(),
            static_cast<int32_t>(tokens.size()),
            addBos,
            false  // special tokens
        );
        
        if (n < 0) {
            // 缓冲区不够，重新分配
            tokens.resize(-n);
            n = llama_tokenize(
                model,
                text.c_str(),
                static_cast<int32_t>(text.size()),
                tokens.data(),
                static_cast<int32_t>(tokens.size()),
                addBos,
                false
            );
        }
        
        if (n >= 0) {
            tokens.resize(n);
        } else {
            LOGE("Tokenization failed: %d", n);
            tokens.clear();
        }
        
        return tokens;
    }
    
    // ========================================================
    // Detokenization
    // ========================================================
    
    std::string detokenizeTokens(const std::vector<llama_token>& tokens) {
        if (!model) return "";
        
        std::string result;
        result.reserve(tokens.size() * 8);
        
        char buf[32];
        for (llama_token token : tokens) {
            int len = llama_token_to_piece(
                model,
                token,
                buf,
                sizeof(buf),
                0,    // lstrip
                false // special
            );
            
            if (len > 0) {
                result.append(buf, len);
            }
        }
        
        return result;
    }
    
    // ========================================================
    // 单个token转文本
    // ========================================================
    
    std::string tokenToString(llama_token token) {
        if (!model) return "";
        
        char buf[32];
        int len = llama_token_to_piece(model, token, buf, sizeof(buf), 0, false);
        
        return (len > 0) ? std::string(buf, len) : "";
    }
    
    // ========================================================
    // 采样下一个token
    // ========================================================
    
    llama_token sampleNextToken() {
        if (!ctx || !model) {
            LOGE("Context or model not available for sampling");
            return -1;
        }
        
        // 获取最后一个token的logits
        float* logits = llama_get_logits_ith(ctx, -1);
        if (!logits) {
            LOGE("Failed to get logits");
            return -1;
        }
        
        int nVocab = llama_n_vocab(model);
        
        // 应用重复惩罚
        if (params.repeatPenalty > 1.0f && !outputTokens.empty()) {
            // 获取最近的tokens用于惩罚
            size_t penaltyWindow = std::min(outputTokens.size(), static_cast<size_t>(params.repeatLastN));
            std::vector<llama_token> recentTokens(
                outputTokens.end() - penaltyWindow,
                outputTokens.end()
            );
            
            // 应用惩罚（简化版本）
            for (llama_token token : recentTokens) {
                if (token >= 0 && token < nVocab) {
                    logits[token] /= params.repeatPenalty;
                }
            }
        }
        
        // 温度为0：贪婪采样
        if (params.temperature <= 0.0f) {
            llama_token best = 0;
            float bestLogit = logits[0];
            for (int i = 1; i < nVocab; i++) {
                if (logits[i] > bestLogit) {
                    bestLogit = logits[i];
                    best = i;
                }
            }
            return best;
        }
        
        // 应用温度
        std::vector<float> probs(nVocab);
        float maxLogit = logits[0];
        for (int i = 1; i < nVocab; i++) {
            if (logits[i] > maxLogit) maxLogit = logits[i];
        }
        
        float sum = 0.0f;
        for (int i = 0; i < nVocab; i++) {
            probs[i] = expf((logits[i] - maxLogit) / params.temperature);
            sum += probs[i];
        }
        
        // 归一化
        for (int i = 0; i < nVocab; i++) {
            probs[i] /= sum;
        }
        
        // Top-K过滤
        if (params.topK > 0 && params.topK < nVocab) {
            // 找到第K大的概率
            std::vector<float> sortedProbs = probs;
            std::partial_sort(sortedProbs.begin(), sortedProbs.begin() + params.topK, 
                            sortedProbs.end(), std::greater<float>());
            float threshold = sortedProbs[params.topK - 1];
            
            // 过滤
            sum = 0.0f;
            for (int i = 0; i < nVocab; i++) {
                if (probs[i] < threshold) {
                    probs[i] = 0.0f;
                } else {
                    sum += probs[i];
                }
            }
            
            // 重新归一化
            if (sum > 0.0f) {
                for (int i = 0; i < nVocab; i++) {
                    probs[i] /= sum;
                }
            }
        }
        
        // Top-P (nucleus) 过滤
        if (params.topP < 1.0f) {
            std::vector<std::pair<float, int>> probIndices;
            for (int i = 0; i < nVocab; i++) {
                if (probs[i] > 0.0f) {
                    probIndices.push_back({probs[i], i});
                }
            }
            
            std::sort(probIndices.begin(), probIndices.end(), std::greater<std::pair<float, int>>());
            
            float cumsum = 0.0f;
            sum = 0.0f;
            for (auto& [prob, idx] : probIndices) {
                cumsum += prob;
                if (cumsum <= params.topP) {
                    sum += prob;
                } else {
                    probs[idx] = 0.0f;
                }
            }
            
            // 重新归一化
            if (sum > 0.0f) {
                for (int i = 0; i < nVocab; i++) {
                    probs[i] /= sum;
                }
            }
        }
        
        // 随机采样
        float r = static_cast<float>(rand()) / RAND_MAX;
        float cumsum = 0.0f;
        for (int i = 0; i < nVocab; i++) {
            cumsum += probs[i];
            if (r <= cumsum) {
                return i;
            }
        }
        
        // 回退到最大概率
        llama_token best = 0;
        float bestProb = probs[0];
        for (int i = 1; i < nVocab; i++) {
            if (probs[i] > bestProb) {
                bestProb = probs[i];
                best = i;
            }
        }
        return best;
    }
    
    // ========================================================
    // 核心生成逻辑
    // ========================================================
    
    std::string generateTokens(
        const std::string& prompt,
        TokenCallback callback,
        void* userData,
        int maxTokens
    ) {
        if (!ctx || !model) {
            LOGE("Context not initialized");
            return "";
        }
        
        if (generating) {
            LOGE("Already generating");
            return "";
        }
        
        std::lock_guard<std::mutex> lock(mutex);
        generating = true;
        shouldStop = false;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 清除KV缓存
        llama_kv_cache_clear(ctx);
        outputTokens.clear();
        currentPos = 0;
        
        // 构建完整prompt
        std::string fullPrompt = buildPrompt(prompt);
        
        // Tokenize输入
        inputTokens = tokenizeText(fullPrompt, true);
        if (inputTokens.empty()) {
            LOGE("Failed to tokenize prompt");
            generating = false;
            return "";
        }
        
        LOGI("Processing %zu input tokens", inputTokens.size());
        
        // 处理输入tokens（批处理）
        int32_t nProcessed = 0;
        while (nProcessed < static_cast<int32_t>(inputTokens.size()) && !shouldStop) {
            int32_t n = std::min(params.nBatch, 
                                 static_cast<int32_t>(inputTokens.size()) - nProcessed);
            
            // 创建batch
            struct llama_batch batch = llama_batch_from_tokens(
                std::vector<llama_token>(inputTokens.begin() + nProcessed,
                                        inputTokens.begin() + nProcessed + n),
                currentPos
            );
            
            // 解码
            int result = llama_decode(ctx, batch);
            
            // 释放batch
            llama_batch_free_custom(batch);
            
            if (result != 0) {
                LOGE("Failed to decode input batch at position %d", nProcessed);
                generating = false;
                return "";
            }
            
            nProcessed += n;
            currentPos += n;
        }
        
        LOGI("Input processed, starting generation");
        
        // 开始生成
        std::string output;
        output.reserve(maxTokens * 4);
        
        // 获取EOS token
        llama_token eosToken = llama_vocab_eos(model);
        if (eosToken == -1) {
            // 某些模型可能没有EOS，使用默认值
            eosToken = 2; // 常见的EOS token ID
        }
        
        // 生成循环
        for (int i = 0; i < maxTokens && !shouldStop; i++) {
            // 采样下一个token
            llama_token nextToken = sampleNextToken();
            
            if (nextToken < 0) {
                LOGE("Sampling failed at position %d", i);
                break;
            }
            
            // 检查EOS
            if (nextToken == eosToken) {
                LOGI("EOS token reached at position %d", i);
                break;
            }
            
            // 转换为文本
            std::string piece = tokenToString(nextToken);
            if (!piece.empty()) {
                output += piece;
                outputTokens.push_back(nextToken);
                
                // 流式回调
                if (callback) {
                    callback(piece.c_str(), userData);
                }
            }
            
            // 更新上下文
            struct llama_batch batch = llama_batch_get_one(nextToken, currentPos);
            int result = llama_decode(ctx, batch);
            
            if (result != 0) {
                LOGW("Decode failed at token %d, stopping generation", i);
                break;
            }
            
            currentPos++;
        }
        
        // 性能统计
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        float tokensPerSecond = duration.count() > 0 ? 
            static_cast<float>(outputTokens.size()) * 1000.0f / duration.count() : 0.0f;
        
        LOGI("Generation complete: %zu tokens in %ld ms (%.2f tokens/sec)", 
             outputTokens.size(), duration.count(), tokensPerSecond);
        
        generating = false;
        return output;
    }
    
    // ========================================================
    // 构建提示词
    // ========================================================
    
    std::string buildPrompt(const std::string& userPrompt) {
        // 根据模型类型构建不同的提示格式
        // 这里使用通用的chat格式
        
        std::ostringstream ss;
        
        // 检测模型类型（从文件名推断）
        std::string modelNameLower = modelInfo.name;
        std::transform(modelNameLower.begin(), modelNameLower.end(), 
                      modelNameLower.begin(), ::tolower);
        
        if (modelNameLower.find("llama-3") != std::string::npos ||
            modelNameLower.find("llama3") != std::string::npos) {
            // Llama 3 格式
            ss << "<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n\n"
               << DEFAULT_SYSTEM_PROMPT 
               << "<|eot_id|><|start_header_id|>user<|end_header_id|>\n\n"
               << userPrompt 
               << "<|eot_id|><|start_header_id|>assistant<|end_header_id|>\n\n";
        }
        else if (modelNameLower.find("chatglm") != std::string::npos) {
            // ChatGLM 格式
            ss << "[Round 0]\n\n问：" << userPrompt << "\n\n答：";
        }
        else if (modelNameLower.find("qwen") != std::string::npos ||
                 modelNameLower.find("通义") != std::string::npos) {
            // Qwen 格式
            ss << "<|im_start|>system\n" << DEFAULT_SYSTEM_PROMPT << "<|im_end|>\n"
               << "<|im_start|>user\n" << userPrompt << "<|im_end|>\n"
               << "<|im_start|>assistant\n";
        }
        else if (modelNameLower.find("mistral") != std::string::npos ||
                 modelNameLower.find("mixtral") != std::string::npos) {
            // Mistral 格式
            ss << "[INST] " << DEFAULT_SYSTEM_PROMPT << "\n\n" << userPrompt << " [/INST]";
        }
        else {
            // 默认通用格式
            ss << "System: " << DEFAULT_SYSTEM_PROMPT << "\n\n"
               << "User: " << userPrompt << "\n\n"
               << "Assistant: ";
        }
        
        return ss.str();
    }
};

// ============================================================
// LlamaWrapper 单例实现
// ============================================================

LlamaWrapper& LlamaWrapper::getInstance() {
    static LlamaWrapper instance;
    return instance;
}

LlamaWrapper::LlamaWrapper() : pImpl_(std::make_unique<Impl>()) {
    // 设置默认参数
    pImpl_->params = GenerationParams();
    
    // 检测CPU核心数并设置线程数
    unsigned int cpuCores = std::thread::hardware_concurrency();
    if (cpuCores > 0) {
        pImpl_->params.nThreads = std::min(cpuCores, static_cast<unsigned int>(8));
    }
    
    LOGI("LlamaWrapper created with %d threads", pImpl_->params.nThreads);
}

LlamaWrapper::~LlamaWrapper() {
    if (pImpl_) {
        pImpl_->cleanup();
    }
}

// ============================================================
// 初始化与释放
// ============================================================

bool LlamaWrapper::initialize(const char* modelPath, const GenerationParams& params) {
    if (pImpl_->initialized) {
        LOGW("Already initialized, releasing previous resources");
        pImpl_->cleanup();
    }
    
    // 合并参数（使用用户提供的参数或默认值）
    if (params.nThreads > 0) {
        pImpl_->params = params;
    } else {
        GenerationParams defaultParams = pImpl_->params;
        if (params.maxTokens != 512) defaultParams.maxTokens = params.maxTokens;
        if (params.temperature != 0.7f) defaultParams.temperature = params.temperature;
        if (params.topP != 0.9f) defaultParams.topP = params.topP;
        if (params.topK != 40) defaultParams.topK = params.topK;
        if (params.nCtx != 2048) defaultParams.nCtx = params.nCtx;
        pImpl_->params = defaultParams;
    }
    
    bool success = pImpl_->loadModel(modelPath, nullptr, nullptr);
    pImpl_->initialized = success;
    
    return success;
}

bool LlamaWrapper::initializeWithProgress(
    const char* modelPath,
    ProgressCallback callback,
    void* userData,
    const GenerationParams& params
) {
    if (pImpl_->initialized) {
        LOGW("Already initialized, releasing previous resources");
        pImpl_->cleanup();
    }
    
    // 使用提供的参数或默认值
    if (params.nThreads > 0) {
        pImpl_->params = params;
    }
    
    bool success = pImpl_->loadModel(modelPath, callback, userData);
    pImpl_->initialized = success;
    
    return success;
}

void LlamaWrapper::release() {
    if (pImpl_) {
        pImpl_->cleanup();
    }
}

bool LlamaWrapper::isInitialized() const {
    return pImpl_ && pImpl_->initialized;
}

// ============================================================
// 生成接口
// ============================================================

std::string LlamaWrapper::generate(const std::string& prompt, const GenerationParams& params) {
    if (!isInitialized()) {
        LOGE("Not initialized, cannot generate");
        return "";
    }
    
    // 使用提供的参数或默认参数
    GenerationParams genParams = (params.nThreads > 0) ? params : pImpl_->params;
    
    return pImpl_->generateTokens(prompt, nullptr, nullptr, genParams.maxTokens);
}

void LlamaWrapper::generateStream(
    const std::string& prompt,
    TokenCallback callback,
    void* userData,
    const GenerationParams& params
) {
    if (!isInitialized()) {
        LOGE("Not initialized, cannot generate stream");
        if (callback) callback("", userData); // 发送空字符串表示错误
        return;
    }
    
    // 使用提供的参数或默认参数
    GenerationParams genParams = (params.nThreads > 0) ? params : pImpl_->params;
    
    pImpl_->generateTokens(prompt, callback, userData, genParams.maxTokens);
}

void LlamaWrapper::stopGeneration() {
    if (pImpl_) {
        LOGI("Stopping generation");
        pImpl_->shouldStop = true;
    }
}

bool LlamaWrapper::isGenerating() const {
    return pImpl_ && pImpl_->generating;
}

// ============================================================
// 模型信息
// ============================================================

ModelInfo LlamaWrapper::getModelInfo() const {
    return pImpl_ ? pImpl_->modelInfo : ModelInfo();
}

std::string LlamaWrapper::getLastToken() const {
    if (!pImpl_ || pImpl_->outputTokens.empty()) return "";
    return pImpl_->tokenToString(pImpl_->outputTokens.back());
}

int LlamaWrapper::getTokenCount() const {
    return pImpl_ ? static_cast<int>(pImpl_->outputTokens.size()) : 0;
}

// ============================================================
// 上下文管理
// ============================================================

void LlamaWrapper::clearContext() {
    if (pImpl_ && pImpl_->ctx) {
        LOGI("Clearing KV cache");
        llama_kv_cache_clear(pImpl_->ctx);
        pImpl_->outputTokens.clear();
        pImpl_->inputTokens.clear();
        pImpl_->currentPos = 0;
    }
}

int LlamaWrapper::getContextSize() const {
    return pImpl_ && pImpl_->ctx ? static_cast<int>(llama_n_ctx(pImpl_->ctx)) : 0;
}

int LlamaWrapper::getRemainingContext() const {
    if (!pImpl_ || !pImpl_->ctx) return 0;
    int total = static_cast<int>(llama_n_ctx(pImpl_->ctx));
    int used = static_cast<int>(pImpl_->inputTokens.size()) + 
               static_cast<int>(pImpl_->outputTokens.size());
    return total - used;
}

// ============================================================
// Tokenization
// ============================================================

std::vector<int> LlamaWrapper::tokenize(const std::string& text) {
    if (!pImpl_) return {};
    auto tokens = pImpl_->tokenizeText(text, false);
    return std::vector<int>(tokens.begin(), tokens.end());
}

std::string LlamaWrapper::detokenize(const std::vector<int>& tokens) {
    if (!pImpl_) return "";
    std::vector<llama_token> llamaTokens(tokens.begin(), tokens.end());
    return pImpl_->detokenizeTokens(llamaTokens);
}

int LlamaWrapper::getTokenCount(const std::string& text) {
    return static_cast<int>(tokenize(text).size());
}

// ============================================================
// 参数设置
// ============================================================

void LlamaWrapper::setParams(const GenerationParams& params) {
    if (pImpl_) {
        pImpl_->params = params;
        LOGI("Params updated: temp=%.2f, top_p=%.2f, top_k=%d, max_tokens=%d",
             params.temperature, params.topP, params.topK, params.maxTokens);
    }
}

GenerationParams LlamaWrapper::getParams() const {
    return pImpl_ ? pImpl_->params : GenerationParams();
}

} // namespace localai

// ============================================================
// C接口实现（供JNI调用）
// ============================================================

extern "C" {

using namespace localai;

/**
 * 从文件初始化模型
 */
bool llama_init_from_file(const char* modelPath) {
    return LlamaWrapper::getInstance().initialize(modelPath);
}

/**
 * 带进度回调的初始化
 */
bool llama_init_with_progress(
    const char* modelPath,
    ProgressCallback callback,
    void* userData
) {
    return LlamaWrapper::getInstance().initializeWithProgress(modelPath, callback, userData);
}

/**
 * 同步生成
 */
int llama_generate(
    const char* prompt,
    char* outputBuffer,
    int bufferSize,
    int maxTokens
) {
    if (!outputBuffer || bufferSize <= 0) return -1;
    
    GenerationParams params;
    params.maxTokens = maxTokens > 0 ? maxTokens : 512;
    
    std::string result = LlamaWrapper::getInstance().generate(prompt, params);
    
    if (result.empty()) {
        outputBuffer[0] = '\0';
        return 0;
    }
    
    int copyLen = std::min(static_cast<int>(result.size()), bufferSize - 1);
    std::memcpy(outputBuffer, result.c_str(), copyLen);
    outputBuffer[copyLen] = '\0';
    
    return copyLen;
}

/**
 * 流式生成
 */
bool llama_generate_stream(
    const char* prompt,
    TokenCallback callback,
    void* userData,
    int maxTokens
) {
    if (!callback) return false;
    
    GenerationParams params;
    params.maxTokens = maxTokens > 0 ? maxTokens : 512;
    
    LlamaWrapper::getInstance().generateStream(prompt, callback, userData, params);
    return true;
}

/**
 * 停止生成
 */
void llama_stop_generation() {
    LlamaWrapper::getInstance().stopGeneration();
}

/**
 * 释放资源
 */
void llama_free() {
    LlamaWrapper::getInstance().release();
}

/**
 * 检查是否已初始化
 */
bool llama_is_initialized() {
    return LlamaWrapper::getInstance().isInitialized();
}

/**
 * 检查是否正在生成
 */
bool llama_is_generating() {
    return LlamaWrapper::getInstance().isGenerating();
}

/**
 * 获取模型信息字符串
 */
const char* llama_get_model_info() {
    static std::string cachedInfo;
    
    ModelInfo info = LlamaWrapper::getInstance().getModelInfo();
    
    std::ostringstream ss;
    ss << "Model: " << info.name << "\n"
       << "Architecture: " << info.architecture << "\n"
       << "Quantization: " << info.quantization << "\n"
       << "Context: " << info.contextLength << " tokens\n"
       << "Vocab size: " << info.vocabSize << "\n"
       << "Embedding dim: " << info.embeddingLength << "\n"
       << "Layers: " << info.layerCount << "\n"
       << "Memory: " << std::fixed << std::setprecision(1) 
       << (info.memoryRequired / (1024.0 * 1024.0)) << " MB";
    
    cachedInfo = ss.str();
    return cachedInfo.c_str();
}

/**
 * 清除上下文
 */
void llama_clear_context() {
    LlamaWrapper::getInstance().clearContext();
}

/**
 * 获取上下文大小
 */
int llama_get_context_size() {
    return LlamaWrapper::getInstance().getContextSize();
}

/**
 * 获取剩余上下文空间
 */
int llama_get_remaining_context() {
    return LlamaWrapper::getInstance().getRemainingContext();
}

/**
 * 设置采样参数
 */
void llama_set_params(
    float temperature,
    float topP,
    int topK,
    float repeatPenalty,
    int nThreads
) {
    GenerationParams params = LlamaWrapper::getInstance().getParams();
    
    if (temperature >= 0.0f) params.temperature = temperature;
    if (topP >= 0.0f && topP <= 1.0f) params.topP = topP;
    if (topK > 0) params.topK = topK;
    if (repeatPenalty >= 1.0f) params.repeatPenalty = repeatPenalty;
    if (nThreads > 0) params.nThreads = nThreads;
    
    LlamaWrapper::getInstance().setParams(params);
}

/**
 * Tokenize文本
 */
int llama_tokenize_text(
    const char* text,
    int* tokenBuffer,
    int bufferSize
) {
    auto tokens = LlamaWrapper::getInstance().tokenize(std::string(text));
    
    if (tokenBuffer && bufferSize > 0) {
        int copyCount = std::min(static_cast<int>(tokens.size()), bufferSize);
        for (int i = 0; i < copyCount; i++) {
            tokenBuffer[i] = tokens[i];
        }
        return copyCount;
    }
    
    return static_cast<int>(tokens.size());
}

/**
 * Detokenize tokens
 */
int llama_detokenize_tokens(
    const int* tokens,
    int tokenCount,
    char* outputBuffer,
    int bufferSize
) {
    if (!tokens || tokenCount <= 0 || !outputBuffer || bufferSize <= 0) {
        return -1;
    }
    
    std::vector<int> tokenVec(tokens, tokens + tokenCount);
    std::string result = LlamaWrapper::getInstance().detokenize(tokenVec);
    
    int copyLen = std::min(static_cast<int>(result.size()), bufferSize - 1);
    std::memcpy(outputBuffer, result.c_str(), copyLen);
    outputBuffer[copyLen] = '\0';
    
    return copyLen;
}

/**
 * 获取token数量
 */
int llama_get_token_count(const char* text) {
    return LlamaWrapper::getInstance().getTokenCount(std::string(text));
}

} // extern "C"