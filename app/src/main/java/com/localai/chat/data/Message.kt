package com.localai.chat.data

data class Message(
    val id: String,
    val role: String,  // "user", "assistant", "system"
    val content: String,
    val timestamp: Long
)
