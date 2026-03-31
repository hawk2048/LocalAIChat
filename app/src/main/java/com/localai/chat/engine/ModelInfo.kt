/**
 * ModelInfo - 模型信息数据类
 * 
 * 包含已加载模型的元数据信息
 */
package com.localai.chat.engine

data class ModelInfo(
    val name: String,
    val architecture: String,
    val quantization: String,
    val contextLength: Int,
    val embeddingLength: Int,
    val vocabSize: Int,
    val layerCount: Int,
    val headCount: Int,
    val fileSize: Long,
    val memoryRequired: Long
) {
    /**
     * 获取人类可读的文件大小
     */
    fun getReadableFileSize(): String {
        return formatSize(fileSize)
    }
    
    /**
     * 获取人类可读的内存需求
     */
    fun getReadableMemoryRequired(): String {
        return formatSize(memoryRequired)
    }
    
    private fun formatSize(bytes: Long): String {
        return when {
            bytes < 1024 -> "$bytes B"
            bytes < 1024 * 1024 -> String.format("%.1f KB", bytes / 1024.0)
            bytes < 1024 * 1024 * 1024 -> String.format("%.1f MB", bytes / (1024.0 * 1024))
            else -> String.format("%.2f GB", bytes / (1024.0 * 1024 * 1024))
        }
    }
    
    /**
     * 获取量化类型描述
     */
    fun getQuantizationDescription(): String {
        return when {
            quantization.contains("Q4_0", ignoreCase = true) -> "4-bit量化 (最快，最低内存)"
            quantization.contains("Q4_K_M", ignoreCase = true) -> "4-bit K-量化 (平衡速度和质量)"
            quantization.contains("Q5_0", ignoreCase = true) -> "5-bit量化"
            quantization.contains("Q5_K_M", ignoreCase = true) -> "5-bit K-量化"
            quantization.contains("Q6_K", ignoreCase = true) -> "6-bit量化"
            quantization.contains("Q8_0", ignoreCase = true) -> "8-bit量化 (较高质量)"
            quantization.contains("F16", ignoreCase = true) -> "半精度浮点 (高质量)"
            quantization.contains("F32", ignoreCase = true) -> "全精度浮点 (最高质量)"
            else -> quantization
        }
    }
    
    override fun toString(): String {
        return """
            ModelInfo(
                name=$name,
                architecture=$architecture,
                quantization=$quantization,
                contextLength=$contextLength,
                vocabSize=$vocabSize,
                layers=$layerCount,
                heads=$headCount,
                fileSize=${getReadableFileSize()},
                memoryRequired=${getReadableMemoryRequired()}
            )
        """.trimIndent()
    }
}
