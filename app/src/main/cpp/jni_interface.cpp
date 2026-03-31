/**
 * jni_interface.cpp - JNI桥接层
 * 
 * 提供Java/Kotlin与C++之间的接口转换
 * 支持同步生成、流式生成、模型管理等功能
 */

#include "llama_wrapper.h"
#include "third_party/llama.h"

#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

#define LOG_TAG "JNI_Interface"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using namespace localai;

// ============================================================
// 全局状态管理
// ============================================================

namespace {
    // 全局JVM指针（用于回调）
    JavaVM* g_jvm = nullptr;
    
    // 流式生成回调类引用
    jclass g_streamCallbackClass = nullptr;
    jmethodID g_onTokenMethod = nullptr;
    jmethodID g_onCompleteMethod = nullptr;
    
    // 进度回调类引用
    jclass g_progressCallbackClass = nullptr;
    jmethodID g_onProgressMethod = nullptr;
    
    // 模型信息类
    jclass g_modelInfoClass = nullptr;
    jmethodID g_modelInfoConstructor = nullptr;
    
    // 生成参数类
    jclass g_generationParamsClass = nullptr;
    jfieldID g_maxTokensField = nullptr;
    jfieldID g_temperatureField = nullptr;
    jfieldID g_topPField = nullptr;
    jfieldID g_topKField = nullptr;
    jfieldID g_repeatPenaltyField = nullptr;
    jfieldID g_repeatLastNField = nullptr;
    jfieldID g_nThreadsField = nullptr;
    jfieldID g_nCtxField = nullptr;
    jfieldID g_nBatchField = nullptr;
    jfieldID g_useMmapField = nullptr;
    jfieldID g_useMlockField = nullptr;
    
    // 缓存字符串类
    jclass g_stringClass = nullptr;
    
    // 单例锁
    std::mutex g_mutex;
    
    // 流式上下文（用于多线程回调）
    struct StreamContext {
        jobject callback;
        jmethodID onTokenMethod;
        bool isActive;
    };
    std::shared_ptr<StreamContext> g_streamContext;
}

// ============================================================
// 辅助函数
// ============================================================

/**
 * 获取JNIEnv（支持多线程）
 */
static JNIEnv* getJNIEnv() {
    if (!g_jvm) return nullptr;
    
    JNIEnv* env = nullptr;
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    if (status == JNI_EDETACHED) {
        // 当前线程未附加到JVM
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("Failed to attach thread to JVM");
            return nullptr;
        }
    } else if (status != JNI_OK) {
        LOGE("Failed to get JNIEnv: %d", status);
        return nullptr;
    }
    
    return env;
}

/**
 * JNI字符串转C++字符串
 */
static std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    
    return result;
}

/**
 * C++字符串转JNI字符串
 */
static jstring stringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

/**
 * 从Java对象读取GenerationParams
 */
static GenerationParams getGenerationParamsFromJava(JNIEnv* env, jobject paramsObj) {
    GenerationParams params;
    
    if (!paramsObj || !g_generationParamsClass) {
        return params; // 返回默认值
    }
    
    params.maxTokens = env->GetIntField(paramsObj, g_maxTokensField);
    params.temperature = env->GetFloatField(paramsObj, g_temperatureField);
    params.topP = env->GetFloatField(paramsObj, g_topPField);
    params.topK = env->GetIntField(paramsObj, g_topKField);
    params.repeatPenalty = env->GetFloatField(paramsObj, g_repeatPenaltyField);
    params.repeatLastN = env->GetIntField(paramsObj, g_repeatLastNField);
    params.nThreads = env->GetIntField(paramsObj, g_nThreadsField);
    params.nCtx = env->GetIntField(paramsObj, g_nCtxField);
    params.nBatch = env->GetIntField(paramsObj, g_nBatchField);
    params.useMmap = env->GetBooleanField(paramsObj, g_useMmapField) == JNI_TRUE;
    params.useMlock = env->GetBooleanField(paramsObj, g_useMlockField) == JNI_TRUE;
    
    return params;
}

/**
 * 创建Java ModelInfo对象
 */
static jobject createModelInfoJava(JNIEnv* env, const ModelInfo& info) {
    if (!g_modelInfoClass || !g_modelInfoConstructor) {
        return nullptr;
    }
    
    jstring name = stringToJstring(env, info.name);
    jstring architecture = stringToJstring(env, info.architecture);
    jstring quantization = stringToJstring(env, info.quantization);
    
    jobject result = env->NewObject(
        g_modelInfoClass,
        g_modelInfoConstructor,
        name,
        architecture,
        quantization,
        info.contextLength,
        info.embeddingLength,
        info.vocabSize,
        info.layerCount,
        info.headCount,
        (jlong)info.fileSize,
        (jlong)info.memoryRequired
    );
    
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(architecture);
    env->DeleteLocalRef(quantization);
    
    return result;
}

/**
 * 流式生成Token回调包装
 */
static void streamTokenCallback(const std::string& token, void* userData) {
    JNIEnv* env = getJNIEnv();
    if (!env || !g_streamContext || !g_streamContext->isActive) {
        return;
    }
    
    // 检查token是否为空（表示结束）
    if (token.empty()) {
        if (g_streamContext->callback && g_onCompleteMethod) {
            env->CallVoidMethod(g_streamContext->callback, g_onCompleteMethod);
        }
        g_streamContext->isActive = false;
        return;
    }
    
    // 调用Java回调
    if (g_streamContext->callback && g_streamContext->onTokenMethod) {
        jstring jToken = stringToJstring(env, token);
        env->CallVoidMethod(g_streamContext->callback, g_streamContext->onTokenMethod, jToken);
        env->DeleteLocalRef(jToken);
    }
}

/**
 * 进度回调包装
 */
static bool progressCallback(float progress, void* userData) {
    JNIEnv* env = getJNIEnv();
    if (!env) return true;
    
    jobject callback = static_cast<jobject>(userData);
    if (callback && g_onProgressMethod) {
        env->CallVoidMethod(callback, g_onProgressMethod, progress);
    }
    
    return true; // 继续加载
}

// ============================================================
// JNI生命周期管理
// ============================================================

/**
 * 库加载时初始化
 */
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    LOGI("JNI_OnLoad called");
    
    std::lock_guard<std::mutex> lock(g_mutex);
    g_jvm = vm;
    
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get JNIEnv in JNI_OnLoad");
        return JNI_ERR;
    }
    
    // 缓存常用类和方法ID
    g_stringClass = (jclass)env->NewGlobalRef(env->FindClass("java/lang/String"));
    
    // StreamCallback类
    jclass streamCallbackClass = env->FindClass(
        "com/localai/chat/engine/StreamCallback");
    if (streamCallbackClass) {
        g_streamCallbackClass = (jclass)env->NewGlobalRef(streamCallbackClass);
        g_onTokenMethod = env->GetMethodID(streamCallbackClass, "onToken", "(Ljava/lang/String;)V");
        g_onCompleteMethod = env->GetMethodID(streamCallbackClass, "onComplete", "()V");
    }
    
    // ProgressCallback类
    jclass progressCallbackClass = env->FindClass(
        "com/localai/chat/engine/ProgressCallback");
    if (progressCallbackClass) {
        g_progressCallbackClass = (jclass)env->NewGlobalRef(progressCallbackClass);
        g_onProgressMethod = env->GetMethodID(progressCallbackClass, "onProgress", "(F)V");
    }
    
    // ModelInfo类
    jclass modelInfoClass = env->FindClass(
        "com/localai/chat/engine/ModelInfo");
    if (modelInfoClass) {
        g_modelInfoClass = (jclass)env->NewGlobalRef(modelInfoClass);
        // 构造函数: (String, String, String, int, int, int, int, int, long, long)
        g_modelInfoConstructor = env->GetMethodID(
            modelInfoClass,
            "<init>",
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIIIIIJJ)V"
        );
    }
    
    // GenerationParams类
    jclass generationParamsClass = env->FindClass(
        "com/localai/chat/engine/GenerationParams");
    if (generationParamsClass) {
        g_generationParamsClass = (jclass)env->NewGlobalRef(generationParamsClass);
        g_maxTokensField = env->GetFieldID(generationParamsClass, "maxTokens", "I");
        g_temperatureField = env->GetFieldID(generationParamsClass, "temperature", "F");
        g_topPField = env->GetFieldID(generationParamsClass, "topP", "F");
        g_topKField = env->GetFieldID(generationParamsClass, "topK", "I");
        g_repeatPenaltyField = env->GetFieldID(generationParamsClass, "repeatPenalty", "F");
        g_repeatLastNField = env->GetFieldID(generationParamsClass, "repeatLastN", "I");
        g_nThreadsField = env->GetFieldID(generationParamsClass, "nThreads", "I");
        g_nCtxField = env->GetFieldID(generationParamsClass, "nCtx", "I");
        g_nBatchField = env->GetFieldID(generationParamsClass, "nBatch", "I");
        g_useMmapField = env->GetFieldID(generationParamsClass, "useMmap", "Z");
        g_useMlockField = env->GetFieldID(generationParamsClass, "useMlock", "Z");
    }
    
    // 初始化流式上下文
    g_streamContext = std::make_shared<StreamContext>();
    g_streamContext->callback = nullptr;
    g_streamContext->onTokenMethod = nullptr;
    g_streamContext->isActive = false;
    
    LOGI("JNI_OnLoad completed successfully");
    return JNI_VERSION_1_6;
}

/**
 * 库卸载时清理
 */
JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* /*reserved*/) {
    LOGI("JNI_OnUnload called");
    
    std::lock_guard<std::mutex> lock(g_mutex);
    
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        // 释放全局引用
        if (g_stringClass) env->DeleteGlobalRef(g_stringClass);
        if (g_streamCallbackClass) env->DeleteGlobalRef(g_streamCallbackClass);
        if (g_progressCallbackClass) env->DeleteGlobalRef(g_progressCallbackClass);
        if (g_modelInfoClass) env->DeleteGlobalRef(g_modelInfoClass);
        if (g_generationParamsClass) env->DeleteGlobalRef(g_generationParamsClass);
    }
    
    g_jvm = nullptr;
    g_streamContext.reset();
}

// ============================================================
// 核心功能：模型初始化
// ============================================================

extern "C" {

/**
 * 初始化引擎（简单版本）
 * 
 * @param modelPath GGUF模型文件路径
 * @return 是否成功
 */
JNIEXPORT jboolean JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeInit(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring modelPath
) {
    if (!modelPath) {
        LOGE("Model path is null");
        return JNI_FALSE;
    }
    
    std::string path = jstringToString(env, modelPath);
    LOGI("Initializing model: %s", path.c_str());
    
    bool success = LlamaWrapper::getInstance().initialize(path);
    
    LOGI("Model initialization %s", success ? "succeeded" : "failed");
    return success ? JNI_TRUE : JNI_FALSE;
}

/**
 * 初始化引擎（带进度回调）
 * 
 * @param modelPath GGUF模型文件路径
 * @param callback 进度回调接口
 * @return 是否成功
 */
JNIEXPORT jboolean JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeInitWithProgress(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring modelPath,
    jobject callback
) {
    if (!modelPath) {
        LOGE("Model path is null");
        return JNI_FALSE;
    }
    
    std::string path = jstringToString(env, modelPath);
    LOGI("Initializing model with progress: %s", path.c_str());
    
    // 创建全局引用以便回调使用
    jobject callbackRef = nullptr;
    if (callback) {
        callbackRef = env->NewGlobalRef(callback);
    }
    
    bool success = LlamaWrapper::getInstance().initializeWithProgress(
        path,
        callback ? progressCallback : nullptr,
        callbackRef
    );
    
    // 释放全局引用
    if (callbackRef) {
        env->DeleteGlobalRef(callbackRef);
    }
    
    return success ? JNI_TRUE : JNI_FALSE;
}

/**
 * 释放引擎资源
 */
JNIEXPORT void JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeRelease(
    JNIEnv* /*env*/,
    jobject /*thiz*/
) {
    LOGI("Releasing engine resources");
    LlamaWrapper::getInstance().release();
}

// ============================================================
// 核心功能：文本生成
// ============================================================

/**
 * 同步生成文本
 * 
 * @param prompt 输入提示
 * @param params 生成参数
 * @return 生成的文本
 */
JNIEXPORT jstring JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGenerate(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring prompt,
    jobject params
) {
    if (!prompt) {
        LOGE("Prompt is null");
        return stringToJstring(env, "");
    }
    
    if (!LlamaWrapper::getInstance().isInitialized()) {
        LOGE("Engine not initialized");
        return stringToJstring(env, "");
    }
    
    std::string promptStr = jstringToString(env, prompt);
    GenerationParams genParams = getGenerationParamsFromJava(env, params);
    
    LOGI("Generating response for prompt (%zu chars, max_tokens=%d)",
         promptStr.size(), genParams.maxTokens);
    
    std::string result = LlamaWrapper::getInstance().generate(promptStr, genParams);
    
    LOGI("Generation complete, output length: %zu", result.size());
    return stringToJstring(env, result);
}

/**
 * 流式生成文本
 * 
 * @param prompt 输入提示
 * @param callback Token回调接口
 * @param params 生成参数
 */
JNIEXPORT void JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGenerateStream(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring prompt,
    jobject callback,
    jobject params
) {
    if (!prompt || !callback) {
        LOGE("Prompt or callback is null");
        return;
    }
    
    if (!LlamaWrapper::getInstance().isInitialized()) {
        LOGE("Engine not initialized");
        return;
    }
    
    std::string promptStr = jstringToString(env, prompt);
    GenerationParams genParams = getGenerationParamsFromJava(env, params);
    
    LOGI("Starting stream generation for prompt (%zu chars)", promptStr.size());
    
    // 设置流式上下文
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_streamContext->callback) {
            env->DeleteGlobalRef(g_streamContext->callback);
        }
        g_streamContext->callback = env->NewGlobalRef(callback);
        g_streamContext->onTokenMethod = g_onTokenMethod;
        g_streamContext->isActive = true;
    }
    
    // 执行生成
    LlamaWrapper::getInstance().generateStream(
        promptStr,
        streamTokenCallback,
        nullptr,
        genParams
    );
    
    // 清理
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_streamContext->callback) {
            env->DeleteGlobalRef(g_streamContext->callback);
            g_streamContext->callback = nullptr;
        }
        g_streamContext->isActive = false;
    }
    
    LOGI("Stream generation completed");
}

/**
 * 停止当前生成
 */
JNIEXPORT void JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeStopStream(
    JNIEnv* /*env*/,
    jobject /*thiz*/
) {
    LOGI("Stopping stream generation");
    LlamaWrapper::getInstance().stopGeneration();
}

/**
 * 检查是否正在生成
 */
JNIEXPORT jboolean JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeIsGenerating(
    JNIEnv* /*env*/,
    jobject /*thiz*/
) {
    return LlamaWrapper::getInstance().isGenerating() ? JNI_TRUE : JNI_FALSE;
}

// ============================================================
// 核心功能：模型信息
// ============================================================

/**
 * 获取模型信息
 */
JNIEXPORT jobject JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGetModelInfo(
    JNIEnv* env,
    jobject /*thiz*/
) {
    if (!LlamaWrapper::getInstance().isInitialized()) {
        return nullptr;
    }
    
    ModelInfo info = LlamaWrapper::getInstance().getModelInfo();
    return createModelInfoJava(env, info);
}

// ============================================================
// 核心功能：Tokenization
// ============================================================

/**
 * Tokenize文本
 * 
 * @return token ID数组
 */
JNIEXPORT jintArray JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeTokenize(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring text
) {
    if (!text) {
        return env->NewIntArray(0);
    }
    
    std::string textStr = jstringToString(env, text);
    std::vector<int> tokens = LlamaWrapper::getInstance().tokenize(textStr);
    
    jintArray result = env->NewIntArray(static_cast<jsize>(tokens.size()));
    if (!result) return nullptr;
    
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(tokens.size()), tokens.data());
    return result;
}

/**
 * Detokenize tokens
 * 
 * @param tokens token ID数组
 * @return 文本字符串
 */
JNIEXPORT jstring JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeDetokenize(
    JNIEnv* env,
    jobject /*thiz*/,
    jintArray tokens
) {
    if (!tokens) {
        return stringToJstring(env, "");
    }
    
    jsize length = env->GetArrayLength(tokens);
    jint* elements = env->GetIntArrayElements(tokens, nullptr);
    
    std::vector<int> tokenVec(elements, elements + length);
    env->ReleaseIntArrayElements(tokens, elements, JNI_ABORT);
    
    std::string result = LlamaWrapper::getInstance().detokenize(tokenVec);
    return stringToJstring(env, result);
}

/**
 * 获取token数量
 */
JNIEXPORT jint JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGetTokenCount(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring text
) {
    if (!text) return 0;
    
    std::string textStr = jstringToString(env, text);
    return LlamaWrapper::getInstance().getTokenCount(textStr);
}

// ============================================================
// 核心功能：上下文管理
// ============================================================

/**
 * 清除上下文（KV缓存）
 */
JNIEXPORT void JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeClearContext(
    JNIEnv* /*env*/,
    jobject /*thiz*/
) {
    LOGI("Clearing context");
    LlamaWrapper::getInstance().clearContext();
}

/**
 * 获取上下文大小
 */
JNIEXPORT jint JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGetContextSize(
    JNIEnv* /*env*/,
    jobject /*thiz*/
) {
    return LlamaWrapper::getInstance().getContextSize();
}

/**
 * 获取剩余上下文空间
 */
JNIEXPORT jint JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGetRemainingContext(
    JNIEnv* /*env*/,
    jobject /*thiz*/
) {
    return LlamaWrapper::getInstance().getRemainingContext();
}

// ============================================================
// 核心功能：参数设置
// ============================================================

/**
 * 设置生成参数
 */
JNIEXPORT void JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeSetParams(
    JNIEnv* env,
    jobject /*thiz*/,
    jobject params
) {
    if (!params) return;
    
    GenerationParams genParams = getGenerationParamsFromJava(env, params);
    LlamaWrapper::getInstance().setParams(genParams);
    
    LOGI("Parameters updated: temp=%.2f, top_p=%.2f, top_k=%d",
         genParams.temperature, genParams.topP, genParams.topK);
}

/**
 * 获取当前生成参数
 */
JNIEXPORT jobject JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGetParams(
    JNIEnv* env,
    jobject /*thiz*/
) {
    // TODO: 实现获取参数并创建Java对象
    return nullptr;
}

// ============================================================
// 工具函数
// ============================================================

/**
 * 获取内存估算
 */
JNIEXPORT jlong JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeEstimateMemory(
    JNIEnv* env,
    jobject /*thiz*/,
    jstring modelPath
) {
    if (!modelPath) return 0;
    
    std::string path = jstringToString(env, modelPath);
    // TODO: 实现内存估算
    return 0;
}

/**
 * 获取后端信息
 */
JNIEXPORT jstring JNICALL
Java_com_localai_chat_engine_LlmEngine_nativeGetBackendInfo(
    JNIEnv* env,
    jobject /*thiz*/
) {
    std::string info = "llama.cpp ";
    info += std::to_string(LLAMA_BUILD_NUMBER);
    info += " (";
    info += llama_print_system_info();
    info += ")";
    
    return stringToJstring(env, info);
}

} // extern "C"
