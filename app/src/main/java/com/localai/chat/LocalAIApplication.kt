package com.localai.chat

import android.app.Application
import android.util.Log
import com.localai.chat.data.ModelManager
import com.localai.chat.engine.LLMEngine
import com.localai.chat.utils.PreferenceManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

class LocalAIApplication : Application() {
    
    private val applicationScope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    
    companion object {
        private const val TAG = "LocalAIApplication"
        
        @Volatile
        private var instance: LocalAIApplication? = null
        
        fun getInstance(): LocalAIApplication {
            return instance ?: throw IllegalStateException("Application not initialized")
        }
    }
    
    override fun onCreate() {
        super.onCreate()
        instance = this
        
        Log.d(TAG, "LocalAI Application starting...")
        
        PreferenceManager.init(this)
        loadNativeLibraries()
        
        applicationScope.launch {
            initializeLLMEngine()
        }
    }
    
    private fun loadNativeLibraries() {
        try {
            System.loadLibrary("localai")
            Log.i(TAG, "Native library loaded successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native library", e)
        }
    }
    
    private suspend fun initializeLLMEngine() {
        try {
            val modelPath = ModelManager.getDefaultModelPath(this)
            if (modelPath != null) {
                LLMEngine.initialize(modelPath)
                Log.i(TAG, "LLM Engine initialized with model: $modelPath")
            } else {
                Log.w(TAG, "No model found, please download model first")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize LLM Engine", e)
        }
    }
    
    override fun onTerminate() {
        super.onTerminate()
        LLMEngine.release()
    }
}
