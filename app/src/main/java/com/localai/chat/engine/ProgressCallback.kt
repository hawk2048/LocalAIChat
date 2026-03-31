/**
 * ProgressCallback - 进度回调接口
 * 
 * 用于模型加载过程中的进度反馈
 */
package com.localai.chat.engine

/**
 * 进度回调接口
 * 
 * 在模型加载过程中，定期调用onProgress报告加载进度。
 */
interface ProgressCallback {
    /**
     * 报告加载进度
     * 
     * @param progress 进度值 (0.0 - 1.0)
     */
    fun onProgress(progress: Float)
}

/**
 * ProgressCallback适配器
 */
open class ProgressCallbackAdapter : ProgressCallback {
    override fun onProgress(progress: Float) {}
}

/**
 * 百分比进度回调
 * 
 * 只在百分比变化时触发回调
 */
abstract class PercentageProgressCallback : ProgressCallback {
    private var lastPercentage = -1
    
    override fun onProgress(progress: Float) {
        val percentage = (progress * 100).toInt()
        if (percentage != lastPercentage) {
            lastPercentage = percentage
            onPercentageChanged(percentage)
        }
    }
    
    /**
     * 百分比变化时调用
     * 
     * @param percentage 当前进度百分比 (0-100)
     */
    abstract fun onPercentageChanged(percentage: Int)
}
