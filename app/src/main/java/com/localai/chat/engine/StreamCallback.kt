/**
 * StreamCallback - 流式生成回调接口
 * 
 * 用于接收流式生成的token
 */
package com.localai.chat.engine

/**
 * 流式生成回调接口
 * 
 * 在流式生成过程中，每个token会通过onToken回调，
 * 生成完成后调用onComplete。
 */
interface StreamCallback {
    /**
     * 收到一个新生成的token
     * 
     * @param token 生成的token文本
     */
    fun onToken(token: String)
    
    /**
     * 生成完成
     * 
     * 在所有token生成完毕后调用，无论是因为达到maxTokens、
     * 遇到EOS token，还是被用户中断。
     */
    fun onComplete()
}

/**
 * StreamCallback适配器
 * 
 * 提供空实现，方便子类只重写需要的方法
 */
open class StreamCallbackAdapter : StreamCallback {
    override fun onToken(token: String) {}
    override fun onComplete() {}
}

/**
 * 收集所有token的回调实现
 */
class CollectingStreamCallback : StreamCallback {
    private val stringBuilder = StringBuilder()
    
    override fun onToken(token: String) {
        stringBuilder.append(token)
    }
    
    override fun onComplete() {
        // 不做任何事
    }
    
    /**
     * 获取收集的完整文本
     */
    fun getText(): String = stringBuilder.toString()
    
    /**
     * 清空收集的文本
     */
    fun clear() {
        stringBuilder.clear()
    }
}

/**
 * 打印回调实现（调试用）
 */
class PrintingStreamCallback : StreamCallback {
    override fun onToken(token: String) {
        print(token)
    }
    
    override fun onComplete() {
        println() // 换行
    }
}
