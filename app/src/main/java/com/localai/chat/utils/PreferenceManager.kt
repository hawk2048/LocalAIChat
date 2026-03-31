package com.localai.chat.utils

import android.content.Context
import android.content.SharedPreferences

object PreferenceManager {
    private const val PREFS_NAME = "localai_prefs"
    
    private lateinit var prefs: SharedPreferences
    
    fun init(context: Context) {
        prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    }
    
    var currentModelPath: String?
        get() = prefs.getString("current_model_path", null)
        set(value) = prefs.edit().putString("current_model_path", value).apply()
    
    var darkMode: Boolean
        get() = prefs.getBoolean("dark_mode", false)
        set(value) = prefs.edit().putBoolean("dark_mode", value).apply()
    
    var maxTokens: Int
        get() = prefs.getInt("max_tokens", 512)
        set(value) = prefs.edit().putInt("max_tokens", value).apply()
    
    var temperature: Float
        get() = prefs.getFloat("temperature", 0.7f)
        set(value) = prefs.edit().putFloat("temperature", value).apply()
    
    fun clear() {
        prefs.edit().clear().apply()
    }
}
