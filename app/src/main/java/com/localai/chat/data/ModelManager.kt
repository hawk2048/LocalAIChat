package com.localai.chat.data

import android.content.Context
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

object ModelManager {
    private const val TAG = "ModelManager"
    private const val MODELS_DIR = "models"
    
    data class ModelInfo(
        val name: String,
        val path: String,
        val size: Long,
        val format: String  // "gguf", "ggml"
    )
    
    fun getModelsDir(context: Context): File {
        val dir = File(context.filesDir, MODELS_DIR)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        return dir
    }
    
    fun getDefaultModelPath(context: Context): String? {
        val modelsDir = getModelsDir(context)
        val modelFiles = modelsDir.listFiles { file ->
            file.extension in listOf("gguf", "ggml", "bin")
        }
        
        return modelFiles?.firstOrNull()?.absolutePath
    }
    
    fun listModels(context: Context): List<ModelInfo> {
        val modelsDir = getModelsDir(context)
        val modelFiles = modelsDir.listFiles { file ->
            file.extension in listOf("gguf", "ggml", "bin")
        } ?: return emptyList()
        
        return modelFiles.map { file ->
            ModelInfo(
                name = file.nameWithoutExtension,
                path = file.absolutePath,
                size = file.length(),
                format = file.extension
            )
        }
    }
    
    suspend fun deleteModel(context: Context, modelPath: String): Boolean = withContext(Dispatchers.IO) {
        try {
            val file = File(modelPath)
            if (file.exists()) {
                val deleted = file.delete()
                Log.i(TAG, "Model deleted: $modelPath, success: $deleted")
                deleted
            } else {
                false
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to delete model", e)
            false
        }
    }
    
    fun formatSize(bytes: Long): String {
        return when {
            bytes < 1024 -> "$bytes B"
            bytes < 1024 * 1024 -> "${bytes / 1024} KB"
            bytes < 1024 * 1024 * 1024 -> String.format("%.1f MB", bytes / (1024.0 * 1024))
            else -> String.format("%.1f GB", bytes / (1024.0 * 1024 * 1024))
        }
    }
}
