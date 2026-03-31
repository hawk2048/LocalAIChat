package com.localai.chat.data

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

private val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "chat_data")

class ChatRepository(private val context: Context) {
    
    private val gson = Gson()
    private val messagesKey = stringPreferencesKey("messages")
    
    fun getMessages(): Flow<List<Message>> {
        return context.dataStore.data.map { preferences ->
            val json = preferences[messagesKey] ?: "[]"
            try {
                val type = object : TypeToken<List<Message>>() {}.type
                gson.fromJson(json, type) ?: emptyList()
            } catch (e: Exception) {
                emptyList()
            }
        }
    }
    
    suspend fun addMessage(message: Message) {
        context.dataStore.edit { preferences ->
            val currentMessages = preferences[messagesKey]?.let { json ->
                try {
                    val type = object : TypeToken<List<Message>>() {}.type
                    gson.fromJson<List<Message>>(json, type) ?: emptyList()
                } catch (e: Exception) {
                    emptyList()
                }
            } ?: emptyList()
            
            val updatedMessages = currentMessages + message
            preferences[messagesKey] = gson.toJson(updatedMessages)
        }
    }
    
    suspend fun clearAll() {
        context.dataStore.edit { preferences ->
            preferences[messagesKey] = "[]"
        }
    }
}
