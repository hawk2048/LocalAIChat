package com.localai.chat.data

/**
 * 聊天消息数据模型
 */
data class ChatMessage(
    val id: Long = System.currentTimeMillis(),
    val content: String,
    val isUser: Boolean,
    val timestamp: Long = System.currentTimeMillis()
)

/**
 * 聊天会话
 */
data class ChatSession(
    val id: Long,
    val title: String,
    val messages: List<ChatMessage>,
    val createdAt: Long = System.currentTimeMillis(),
    val updatedAt: Long = System.currentTimeMillis()
)

/**
 * 模型信息
 */
data class ModelInfo(
    val name: String,
    val path: String,
    val size: Long,
    val downloaded: Boolean = false
)
