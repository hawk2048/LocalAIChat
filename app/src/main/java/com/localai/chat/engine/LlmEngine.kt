/**
 * LlmEngine - 本地LLM推理引擎
 * 
 * 提供完整的本地LLM推理能力，包括：
 * - 模型加载与释放
 * - 同步/流式文本生成
 * - Tokenization
 * - 上下文管理
 * - 参数配置
 * 
 * 使用示例：
 * ```kotlin
 * // 初始化
 * val engine = LlmEngine()
 * engine.initialize(modelPath)
 * 
 * // 同步生成
 * val response = engine.generate("Hello, world!")
 * 
 * // 流式生成
 * engine.generateStream(prompt, object : StreamCallback {
 *     override fun onToken(token: String) { print(token) }
 *     override fun onComplete() { println() }
 * })
 * 
 * // 释放资源
 * engine.release()
 * ```
 */
package com.localai.chat.engine

import android.util.Log

/**
 * 本地LLM推理引擎
 * 
 * 单例模式实现，确保全局只有一个模型实例在运行。
 * 所有操作都是线程安全的。
 */
class LlmEngine private constructor() {
    
    companion object {
        private const val TAG = "LlmEngine"
        
        @Volatile
        private var instance: LlmEngine? = null
        
        /**
         * 获取单例实例
         */
        fun getInstance(): LlmEngine {
            return instance ?: synchronized(this) {
                instance ?: LlmEngine().also { instance = it }
            }
        }
        
        init {
            try {
                System.loadLibrary("llmengine")
                Log.i(TAG, "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
            }
        }
    }
    
    // 当前生成参数
    private var currentParams: GenerationParams = GenerationParams()
    
    // 是否已初始化
    private var isInitializedFlag: Boolean = false
    
    // ============================================================
    // Native 方法声明
    // ============================================================
    
    // 模型管理
    private external fun nativeInit(modelPath: String): Boolean
    private external fun nativeInitWithProgress(modelPath: String, callback: ProgressCallback): Boolean
    private external fun nativeRelease()
    
    // 文本生成
    private external fun nativeGenerate(prompt: String, params: GenerationParams): String
    private external fun nativeGenerateStream(prompt: String, callback: StreamCallback, params: GenerationParams)
    private external fun nativeStopStream()
    private external fun nativeIsGenerating(): Boolean
    
    // 模型信息
    private external fun nativeGetModelInfo(): ModelInfo?
    
    // Tokenization
    private external fun nativeTokenize(text: String): IntArray
    private external fun nativeDetokenize(tokens: IntArray): String
    private external fun nativeGetTokenCount(text: String): Int
    
    // 上下文管理
    private external fun nativeClearContext()
    private external fun nativeGetContextSize(): Int
    private external fun nativeGetRemainingContext(): Int
    
    // 参数
    private external fun nativeSetParams(params: GenerationParams)
    
    // 工具
    private external fun nativeGetBackendInfo(): String
    
    // ============================================================
    // 公共API
    // ============================================================
    
    /**
     * 初始化引擎（简单版本）
     * 
     * @param modelPath GGUF模型文件路径
     * @return 是否成功
     */
    fun initialize(modelPath: String): Boolean {
        Log.i(TAG, "Initializing engine with model: $modelPath")
        
        if (isInitializedFlag) {
            Log.w(TAG, "Engine already initialized, releasing first")
            release()
        }
        
        val success = nativeInit(modelPath)
        isInitializedFlag = success
        
        if (success) {
            Log.i(TAG, "Engine initialized successfully")
        } else {
            Log.e(TAG, "Engine initialization failed")
        }
        
        return success
    }
    
    /**
     * 初始化引擎（带进度回调）
     * 
     * @param modelPath GGUF模型文件路径
     * @param callback 进度回调接口
     * @return 是否成功
     */
    fun initializeWithProgress(modelPath: String, callback: ProgressCallback): Boolean {
        Log.i(TAG, "Initializing engine with progress callback")
        
        if (isInitializedFlag) {
            Log.w(TAG, "Engine already initialized, releasing first")
            release()
        }
        
        val success = nativeInitWithProgress(modelPath, callback)
        isInitializedFlag = success
        
        return success
    }
    
    /**
     * 释放引擎资源
     */
    fun release() {
        Log.i(TAG, "Releasing engine resources")
        nativeRelease()
        isInitializedFlag = false
    }
    
    /**
     * 检查是否已初始化
     */
    fun isInitialized(): Boolean = isInitializedFlag
    
    /**
     * 同步生成文本
     * 
     * @param prompt 输入提示
     * @param params 生成参数（可选，使用当前设置）
     * @return 生成的文本
     */
    fun generate(prompt: String, params: GenerationParams? = null): String {
        if (!isInitializedFlag) {
            Log.e(TAG, "Engine not initialized")
            return ""
        }
        
        val useParams = params ?: currentParams
        Log.d(TAG, "Generating for prompt (${prompt.length} chars)")
        
        return nativeGenerate(prompt, useParams)
    }
    
    /**
     * 流式生成文本
     * 
     * @param prompt 输入提示
     * @param callback Token回调接口
     * @param params 生成参数（可选）
     */
    fun generateStream(
        prompt: String,
        callback: StreamCallback,
        params: GenerationParams? = null
    ) {
        if (!isInitializedFlag) {
            Log.e(TAG, "Engine not initialized")
            callback.onComplete()
            return
        }
        
        val useParams = params ?: currentParams
        Log.d(TAG, "Starting stream generation for prompt (${prompt.length} chars)")
        
        nativeGenerateStream(prompt, callback, useParams)
    }
    
    /**
     * 停止当前生成
     */
    fun stopGeneration() {
        Log.i(TAG, "Stopping generation")
        nativeStopStream()
    }
    
    /**
     * 检查是否正在生成
     */
    fun isGenerating(): Boolean = nativeIsGenerating()
    
    /**
     * 获取模型信息
     */
    fun getModelInfo(): ModelInfo? {
        if (!isInitializedFlag) return null
        return nativeGetModelInfo()
    }
    
    /**
     * Tokenize文本
     * 
     * @return token ID数组
     */
    fun tokenize(text: String): IntArray {
        if (!isInitializedFlag) return intArrayOf()
        return nativeTokenize(text)
    }
    
    /**
     * Detokenize tokens
     * 
     * @return 文本字符串
     */
    fun detokenize(tokens: IntArray): String {
        if (!isInitializedFlag) return ""
        return nativeDetokenize(tokens)
    }
    
    /**
     * 获取文本的token数量
     */
    fun getTokenCount(text: String): Int {
        if (!isInitializedFlag) return 0
        return nativeGetTokenCount(text)
    }
    
    /**
     * 清除上下文（KV缓存）
     */
    fun clearContext() {
        if (!isInitializedFlag) return
        Log.i(TAG, "Clearing context")
        nativeClearContext()
    }
    
    /**
     * 获取上下文大小（已使用的token数）
     */
    fun getContextSize(): Int {
        if (!isInitializedFlag) return 0
        return nativeGetContextSize()
    }
    
    /**
     * 获取剩余上下文空间
     */
    fun getRemainingContext(): Int {
        if (!isInitializedFlag) return 0
        return nativeGetRemainingContext()
    }
    
    /**
     * 设置生成参数
     */
    fun setParams(params: GenerationParams) {
        currentParams = params
        if (isInitializedFlag) {
            nativeSetParams(params)
        }
        Log.d(TAG, "Parameters updated: $params")
    }
    
    /**
     * 获取当前生成参数
     */
    fun getParams(): GenerationParams = currentParams
    
    /**
     * 获取后端信息
     */
    fun getBackendInfo(): String {
        return nativeGetBackendInfo()
    }
    
    // ============================================================
    // 便捷方法
    // ============================================================
    
    /**
     * 聊天模式生成
     * 
     * 自动构建对话格式的prompt
     * 
     * @param userMessage 用户消息
     * @param systemPrompt 系统提示（可选）
     * @param history 历史对话（可选）
     * @param params 生成参数（可选）
     * @return 助手回复
     */
    fun chat(
        userMessage: String,
        systemPrompt: String? = null,
        history: List<Pair<String, String>>? = null,
        params: GenerationParams? = null
    ): String {
        val prompt = buildPrompt(
            userMessage = userMessage,
            systemPrompt = systemPrompt,
            history = history
        )
        return generate(prompt, params)
    }
    
    /**
     * 聊天模式流式生成
     */
    fun chatStream(
        userMessage: String,
        callback: StreamCallback,
        systemPrompt: String? = null,
        history: List<Pair<String, String>>? = null,
        params: GenerationParams? = null
    ) {
        val prompt = buildPrompt(
            userMessage = userMessage,
            systemPrompt = systemPrompt,
            history = history
        )
        generateStream(prompt, callback, params)
    }
    
    /**
     * 构建对话prompt
     */
    private fun buildPrompt(
        userMessage: String,
        systemPrompt: String?,
        history: List<Pair<String, String>>?
    ): String {
        val sb = StringBuilder()
        
        // 系统提示
        if (!systemPrompt.isNullOrBlank()) {
            sb.append("<|system|>\n")
            sb.append(systemPrompt)
            sb.append("\n<|end|>\n")
        }
        
        // 历史对话
        history?.forEach { (user, assistant) ->
            sb.append("<|user|>\n")
            sb.append(user)
            sb.append("\n<|end|>\n")
            sb.append("<|assistant|>\n")
            sb.append(assistant)
            sb.append("\n<|end|>\n")
        }
        
        // 当前用户消息
        sb.append("<|user|>\n")
        sb.append(userMessage)
        sb.append("\n<|end|>\n")
        sb.append("<|assistant|>\n")
        
        return sb.toString()
    }
    
    /**
     * 基于模型架构构建prompt（使用LlamaWrapper的buildPrompt方法）
     */
    fun buildPromptForModel(
        userMessage: String,
        systemPrompt: String?,
        history: List<Pair<String, String>>?,
        modelType: String
    ): String {
        // 这个方法可以在native层实现更复杂的格式化
        // 目前使用简单的聊天模板
        return buildPrompt(userMessage, systemPrompt, history)
    }
}
