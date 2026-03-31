package com.localai.chat.viewmodel

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.localai.chat.data.Message
import com.localai.chat.data.ChatRepository
import com.localai.chat.engine.LlmEngine
import com.localai.chat.engine.GenerationParams
import com.localai.chat.engine.StreamCallback
import com.localai.chat.engine.ModelInfo
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import java.util.UUID

/**
 * 聊天界面ViewModel
 * 
 * 管理聊天消息状态和LLM引擎交互
 */
class ChatViewModel(application: Application) : AndroidViewModel(application) {
    
    companion object {
        private const val TAG = "ChatViewModel"
        private const val DEFAULT_SYSTEM_PROMPT = "你是一个有帮助的AI助手。"
    }
    
    private val repository = ChatRepository(application)
    private val engine = LlmEngine.getInstance()
    
    // 消息列表
    private val _messages = MutableStateFlow<List<Message>>(emptyList())
    val messages: StateFlow<List<Message>> = _messages.asStateFlow()
    
    // 生成状态
    private val _isGenerating = MutableStateFlow(false)
    val isGenerating: StateFlow<Boolean> = _isGenerating.asStateFlow()
    
    // 引擎状态
    private val _isEngineReady = MutableStateFlow(false)
    val isEngineReady: StateFlow<Boolean> = _isEngineReady.asStateFlow()
    
    // 模型信息
    private val _modelInfo = MutableStateFlow<ModelInfo?>(null)
    val modelInfo: StateFlow<ModelInfo?> = _modelInfo.asStateFlow()
    
    // 当前模型路径
    private val _currentModelPath = MutableStateFlow<String?>(null)
    val currentModelPath: StateFlow<String?> = _currentModelPath.asStateFlow()
    
    // 生成参数
    private val _generationParams = MutableStateFlow(GenerationParams())
    val generationParams: StateFlow<GenerationParams> = _generationParams.asStateFlow()
    
    // 加载状态
    private val _isLoading = MutableStateFlow(false)
    val isLoading: StateFlow<Boolean> = _isLoading.asStateFlow()
    
    // 加载进度
    private val _loadingProgress = MutableStateFlow(0f)
    val loadingProgress: StateFlow<Float> = _loadingProgress.asStateFlow()
    
    // 状态消息
    private val _statusMessage = MutableStateFlow<String?>(null)
    val statusMessage: StateFlow<String?> = _statusMessage.asStateFlow()
    
    // 历史对话（用于构建上下文）
    private val conversationHistory = mutableListOf<Pair<String, String>>()
    
    private var generationJob: Job? = null
    
    init {
        loadChatHistory()
    }
    
    /**
     * 加载聊天历史
     */
    private fun loadChatHistory() {
        viewModelScope.launch {
            repository.getMessages().collect { history ->
                _messages.value = history
                // 重建对话历史
                conversationHistory.clear()
                history.chunked(2).forEach { chunk ->
                    if (chunk.size == 2 && 
                        chunk[0].role == "user" && 
                        chunk[1].role == "assistant") {
                        conversationHistory.add(chunk[0].content to chunk[1].content)
                    }
                }
            }
        }
    }
    
    /**
     * 加载模型
     */
    fun loadModel(modelPath: String) {
        if (_isLoading.value) {
            Log.w(TAG, "Already loading a model")
            return
        }
        
        viewModelScope.launch {
            _isLoading.value = true
            _statusMessage.value = "正在加载模型..."
            _loadingProgress.value = 0f
            
            try {
                val success = engine.initializeWithProgress(modelPath, object : com.localai.chat.engine.ProgressCallback {
                    override fun onProgress(progress: Float, message: String?) {
                        _loadingProgress.value = progress * 100
                        _statusMessage.value = message ?: "加载中... ${"%.1f".format(progress * 100)}%"
                    }
                    
                    override fun onComplete() {
                        _statusMessage.value = "模型加载完成"
                    }
                    
                    override fun onError(error: String?) {
                        _statusMessage.value = "加载失败: ${error ?: "未知错误"}"
                    }
                })
                
                if (success) {
                    _isEngineReady.value = true
                    _currentModelPath.value = modelPath
                    _modelInfo.value = engine.getModelInfo()
                    _statusMessage.value = "模型加载成功: ${_modelInfo.value?.name ?: "未知模型"}"
                    Log.i(TAG, "Model loaded: ${_modelInfo.value}")
                } else {
                    _statusMessage.value = "模型加载失败"
                    Log.e(TAG, "Model loading failed")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error loading model", e)
                _statusMessage.value = "加载错误: ${e.message}"
            } finally {
                _isLoading.value = false
            }
        }
    }
    
    /**
     * 卸载模型
     */
    fun unloadModel() {
        engine.release()
        _isEngineReady.value = false
        _currentModelPath.value = null
        _modelInfo.value = null
        _statusMessage.value = "模型已卸载"
        conversationHistory.clear()
        Log.i(TAG, "Model unloaded")
    }
    
    /**
     * 发送消息
     */
    fun sendMessage(content: String) {
        if (content.isBlank()) return
        
        val userMessage = Message(
            id = UUID.randomUUID().toString(),
            role = "user",
            content = content,
            timestamp = System.currentTimeMillis()
        )
        
        viewModelScope.launch {
            repository.addMessage(userMessage)
        }
        
        generateResponse(content)
    }
    
    /**
     * 生成AI回复
     */
    private fun generateResponse(userMessage: String) {
        if (!_isEngineReady.value) {
            addSystemMessage("模型未加载，请先加载模型")
            return
        }
        
        if (_isGenerating.value) {
            Log.w(TAG, "Already generating, stopping previous generation")
            stopGeneration()
        }
        
        _isGenerating.value = true
        
        val assistantMessageId = UUID.randomUUID().toString()
        var assistantMessage = Message(
            id = assistantMessageId,
            role = "assistant",
            content = "",
            timestamp = System.currentTimeMillis()
        )
        
        // 先添加空的assistant消息占位
        val currentMessages = _messages.value.toMutableList()
        currentMessages.add(assistantMessage)
        _messages.value = currentMessages
        
        var fullResponse = StringBuilder()
        
        generationJob = viewModelScope.launch {
            try {
                engine.chatStream(
                    userMessage = userMessage,
                    callback = object : StreamCallback {
                        override fun onToken(token: String) {
                            fullResponse.append(token)
                            updateAssistantMessage(assistantMessageId, fullResponse.toString())
                        }
                        
                        override fun onComplete() {
                            Log.d(TAG, "Stream completed, total ${fullResponse.length} chars")
                            viewModelScope.launch {
                                // 保存最终消息
                                val finalMessage = assistantMessage.copy(
                                    content = fullResponse.toString()
                                )
                                repository.addMessage(finalMessage)
                                
                                // 更新对话历史
                                conversationHistory.add(userMessage to fullResponse.toString())
                            }
                        }
                        
                        override fun onError(error: String?) {
                            Log.e(TAG, "Stream error: $error")
                            viewModelScope.launch {
                                addSystemMessage("生成错误: ${error ?: "未知错误"}")
                            }
                        }
                    },
                    systemPrompt = DEFAULT_SYSTEM_PROMPT,
                    history = conversationHistory.toList().dropLast(1), // 排除当前正在生成的对话
                    params = _generationParams.value
                )
            } catch (e: Exception) {
                Log.e(TAG, "Generation failed", e)
                addSystemMessage("生成失败: ${e.message}")
            } finally {
                _isGenerating.value = false
            }
        }
    }
    
    /**
     * 更新assistant消息内容
     */
    private fun updateAssistantMessage(messageId: String, newContent: String) {
        val currentMessages = _messages.value.toMutableList()
        val index = currentMessages.indexOfFirst { it.id == messageId }
        
        if (index >= 0) {
            currentMessages[index] = currentMessages[index].copy(content = newContent)
            _messages.value = currentMessages
        }
    }
    
    /**
     * 添加系统消息
     */
    private fun addSystemMessage(content: String) {
        val systemMessage = Message(
            id = UUID.randomUUID().toString(),
            role = "system",
            content = content,
            timestamp = System.currentTimeMillis()
        )
        
        viewModelScope.launch {
            repository.addMessage(systemMessage)
        }
    }
    
    /**
     * 停止生成
     */
    fun stopGeneration() {
        engine.stopGeneration()
        generationJob?.cancel()
        _isGenerating.value = false
        Log.i(TAG, "Generation stopped")
    }
    
    /**
     * 清除聊天历史
     */
    fun clearHistory() {
        viewModelScope.launch {
            repository.clearAll()
            conversationHistory.clear()
            engine.clearContext()
            Log.i(TAG, "History cleared")
        }
    }
    
    /**
     * 清除上下文（保留历史记录）
     */
    fun clearContext() {
        engine.clearContext()
        conversationHistory.clear()
        _statusMessage.value = "上下文已清除"
        Log.i(TAG, "Context cleared")
    }
    
    /**
     * 更新生成参数
     */
    fun updateGenerationParams(params: GenerationParams) {
        _generationParams.value = params
        engine.setParams(params)
        Log.d(TAG, "Generation params updated: $params")
    }
    
    /**
     * 获取上下文使用情况
     */
    fun getContextUsage(): Pair<Int, Int> {
        val used = engine.getContextSize()
        val remaining = engine.getRemainingContext()
        return Pair(used, used + remaining)
    }
    
    override fun onCleared() {
        super.onCleared()
        generationJob?.cancel()
        // 注意：不在这里释放引擎，因为可能其他地方还在使用
        // 如果需要，可以在Application的onTerminate中释放
    }
}
