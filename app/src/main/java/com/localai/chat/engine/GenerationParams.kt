/**
 * GenerationParams - 生成参数配置类
 * 
 * 控制文本生成的各项参数
 */
package com.localai.chat.engine

/**
 * 生成参数
 * 
 * @property maxTokens 最大生成token数
 * @property temperature 温度参数 (0.0-2.0)，越高越随机
 * @property topP Top-p (nucleus) 采样阈值 (0.0-1.0)
 * @property topK Top-k 采样，保留前k个候选token
 * @property repeatPenalty 重复惩罚 (1.0-2.0)
 * @property repeatLastN 重复惩罚的上下文窗口大小
 * @property nThreads 推理线程数
 * @property nCtx 上下文长度
 * @property nBatch 批处理大小
 * @property useMmap 是否使用内存映射
 * @property useMlock 是否锁定内存
 */
data class GenerationParams(
    var maxTokens: Int = 512,
    var temperature: Float = 0.7f,
    var topP: Float = 0.9f,
    var topK: Int = 40,
    var repeatPenalty: Float = 1.1f,
    var repeatLastN: Int = 64,
    var nThreads: Int = 4,
    var nCtx: Int = 2048,
    var nBatch: Int = 512,
    var useMmap: Boolean = true,
    var useMlock: Boolean = false
) {
    companion object {
        /**
         * 预设配置：创意写作
         */
        fun creative() = GenerationParams(
            maxTokens = 1024,
            temperature = 1.0f,
            topP = 0.95f,
            topK = 60,
            repeatPenalty = 1.15f
        )
        
        /**
         * 预设配置：平衡模式
         */
        fun balanced() = GenerationParams(
            maxTokens = 512,
            temperature = 0.7f,
            topP = 0.9f,
            topK = 40,
            repeatPenalty = 1.1f
        )
        
        /**
         * 预设配置：精确模式（代码、事实问答）
         */
        fun precise() = GenerationParams(
            maxTokens = 512,
            temperature = 0.3f,
            topP = 0.8f,
            topK = 20,
            repeatPenalty = 1.05f
        )
        
        /**
         * 预设配置：快速响应
         */
        fun fast() = GenerationParams(
            maxTokens = 256,
            temperature = 0.5f,
            topP = 0.85f,
            topK = 30,
            nThreads = 8 // 使用更多线程加速
        )
    }
    
    /**
     * 验证参数是否在有效范围内
     */
    fun validate(): List<String> {
        val errors = mutableListOf<String>()
        
        if (maxTokens <= 0) {
            errors.add("maxTokens must be positive")
        }
        if (maxTokens > 8192) {
            errors.add("maxTokens should not exceed 8192")
        }
        
        if (temperature < 0.0f || temperature > 2.0f) {
            errors.add("temperature must be between 0.0 and 2.0")
        }
        
        if (topP < 0.0f || topP > 1.0f) {
            errors.add("topP must be between 0.0 and 1.0")
        }
        
        if (topK < 0) {
            errors.add("topK must be non-negative")
        }
        
        if (repeatPenalty < 1.0f || repeatPenalty > 2.0f) {
            errors.add("repeatPenalty must be between 1.0 and 2.0")
        }
        
        if (nThreads < 1 || nThreads > 16) {
            errors.add("nThreads must be between 1 and 16")
        }
        
        if (nCtx < 512 || nCtx > 32768) {
            errors.add("nCtx must be between 512 and 32768")
        }
        
        return errors
    }
    
    /**
     * 应用验证修正
     */
    fun applyCorrections(): GenerationParams {
        maxTokens = maxTokens.coerceIn(1, 8192)
        temperature = temperature.coerceIn(0.0f, 2.0f)
        topP = topP.coerceIn(0.0f, 1.0f)
        topK = topK.coerceIn(0, 1000)
        repeatPenalty = repeatPenalty.coerceIn(1.0f, 2.0f)
        nThreads = nThreads.coerceIn(1, 16)
        nCtx = nCtx.coerceIn(512, 32768)
        return this
    }
    
    override fun toString(): String {
        return "GenerationParams(maxTokens=$maxTokens, temp=$temperature, topP=$topP, topK=$topK, repeatPen=$repeatPenalty)"
    }
}
