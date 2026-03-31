package com.localai.chat.ui

import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.Send
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.localai.chat.data.Message
import com.localai.chat.viewmodel.ChatViewModel
import com.localai.chat.engine.GenerationParams

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    viewModel: ChatViewModel = viewModel()
) {
    val messages by viewModel.messages.collectAsState()
    val isGenerating by viewModel.isGenerating.collectAsState()
    val isEngineReady by viewModel.isEngineReady.collectAsState()
    val modelInfo by viewModel.modelInfo.collectAsState()
    val isLoading by viewModel.isLoading.collectAsState()
    val loadingProgress by viewModel.loadingProgress.collectAsState()
    val statusMessage by viewModel.statusMessage.collectAsState()
    val generationParams by viewModel.generationParams.collectAsState()
    
    var inputText by remember { mutableStateOf("") }
    var showSettings by remember { mutableStateOf(false) }
    var showModelMenu by remember { mutableStateOf(false) }
    
    val context = LocalContext.current
    
    // 文件选择器
    val modelPicker = rememberLauncherForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        uri?.let {
            // 获取文件路径
            val path = getFilePathFromUri(context, it)
            path?.let { modelPath ->
                viewModel.loadModel(modelPath)
            }
        }
    }
    
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text("LocalAI Chat")
                        if (modelInfo != null) {
                            Text(
                                text = modelInfo!!.name,
                                style = MaterialTheme.typography.labelSmall,
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis
                            )
                        }
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                ),
                actions = {
                    // 模型状态指示器
                    if (isEngineReady) {
                        Icon(
                            imageVector = Icons.Default.CheckCircle,
                            contentDescription = "模型已加载",
                            tint = MaterialTheme.colorScheme.primary,
                            modifier = Modifier.padding(end = 8.dp)
                        )
                    }
                    
                    // 加载模型按钮
                    IconButton(onClick = { showModelMenu = true }) {
                        Icon(Icons.Default.FolderOpen, "加载模型")
                    }
                    
                    DropdownMenu(
                        expanded = showModelMenu,
                        onDismissRequest = { showModelMenu = false }
                    ) {
                        DropdownMenuItem(
                            text = { Text("从文件选择模型") },
                            onClick = {
                                showModelMenu = false
                                modelPicker.launch(arrayOf("*/*"))
                            }
                        )
                        
                        if (isEngineReady) {
                            DropdownMenuItem(
                                text = { Text("卸载当前模型") },
                                onClick = {
                                    showModelMenu = false
                                    viewModel.unloadModel()
                                }
                            )
                        }
                    }
                    
                    // 清除按钮
                    IconButton(onClick = { viewModel.clearHistory() }) {
                        Icon(Icons.Default.Delete, "清除历史")
                    }
                    
                    // 设置按钮
                    IconButton(onClick = { showSettings = true }) {
                        Icon(Icons.Default.Settings, "设置")
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            // 状态消息栏
            if (statusMessage != null || isLoading) {
                Surface(
                    modifier = Modifier.fillMaxWidth(),
                    color = MaterialTheme.colorScheme.secondaryContainer,
                    tonalElevation = 2.dp
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 16.dp, vertical = 8.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        if (isLoading) {
                            CircularProgressIndicator(
                                progress = { loadingProgress / 100f },
                                modifier = Modifier.size(24.dp),
                                strokeWidth = 2.dp
                            )
                            Spacer(modifier = Modifier.width(12.dp))
                            Text(
                                text = "${statusMessage ?: "加载中..."} (${"%.1f".format(loadingProgress)}%)",
                                style = MaterialTheme.typography.bodyMedium
                            )
                        } else {
                            Text(
                                text = statusMessage!!,
                                style = MaterialTheme.typography.bodyMedium
                            )
                        }
                    }
                }
            }
            
            // 欢迎提示（无消息时）
            if (messages.isEmpty() && !isEngineReady && !isLoading) {
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth(),
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        Icon(
                            imageVector = Icons.Default.Chat,
                            contentDescription = null,
                            modifier = Modifier.size(64.dp),
                            tint = MaterialTheme.colorScheme.primary.copy(alpha = 0.5f)
                        )
                        Text(
                            text = "欢迎使用 LocalAI Chat",
                            style = MaterialTheme.typography.headlineSmall
                        )
                        Text(
                            text = "点击右上角文件夹图标加载 GGUF 模型",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Button(
                            onClick = { modelPicker.launch(arrayOf("*/*")) }
                        ) {
                            Icon(Icons.Default.FolderOpen, contentDescription = null)
                            Spacer(modifier = Modifier.width(8.dp))
                            Text("选择模型文件")
                        }
                    }
                }
            } else {
                // 消息列表
                LazyColumn(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .padding(horizontal = 16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    contentPadding = PaddingValues(vertical = 16.dp)
                ) {
                    items(messages, key = { it.id }) { message ->
                        MessageBubble(message = message)
                    }
                    
                    // 生成中指示器
                    if (isGenerating) {
                        item {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.Start
                            ) {
                                Surface(
                                    shape = RoundedCornerShape(16.dp),
                                    color = MaterialTheme.colorScheme.surfaceVariant
                                ) {
                                    Row(
                                        modifier = Modifier.padding(12.dp),
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        CircularProgressIndicator(
                                            modifier = Modifier.size(16.dp),
                                            strokeWidth = 2.dp
                                        )
                                        Spacer(modifier = Modifier.width(8.dp))
                                        Text("正在生成...", style = MaterialTheme.typography.bodyMedium)
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // 输入区域
            Surface(
                modifier = Modifier.fillMaxWidth(),
                tonalElevation = 2.dp
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    OutlinedTextField(
                        value = inputText,
                        onValueChange = { inputText = it },
                        modifier = Modifier.weight(1f),
                        placeholder = { 
                            Text(if (isEngineReady) "输入消息..." else "请先加载模型") 
                        },
                        enabled = isEngineReady && !isGenerating,
                        maxLines = 4,
                        shape = RoundedCornerShape(24.dp)
                    )
                    
                    if (isGenerating) {
                        // 停止按钮
                        FloatingActionButton(
                            onClick = { viewModel.stopGeneration() },
                            containerColor = MaterialTheme.colorScheme.errorContainer
                        ) {
                            Icon(Icons.Default.Stop, "停止生成")
                        }
                    } else {
                        // 发送按钮
                        FloatingActionButton(
                            onClick = {
                                if (inputText.isNotBlank() && isEngineReady) {
                                    viewModel.sendMessage(inputText)
                                    inputText = ""
                                }
                            },
                            elevation = FloatingActionButtonDefaults.elevation(
                                defaultElevation = 2.dp
                            )
                        ) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.Send,
                                contentDescription = "发送"
                            )
                        }
                    }
                }
            }
        }
        
        // 设置对话框
        if (showSettings) {
            SettingsDialog(
                params = generationParams,
                onDismiss = { showSettings = false },
                onSave = { newParams ->
                    viewModel.updateGenerationParams(newParams)
                    showSettings = false
                }
            )
        }
    }
}

@Composable
fun MessageBubble(message: Message) {
    val isUser = message.role == "user"
    val isSystem = message.role == "system"
    
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = if (isUser) Arrangement.End else Arrangement.Start
    ) {
        Surface(
            shape = RoundedCornerShape(16.dp),
            color = when {
                isUser -> MaterialTheme.colorScheme.primaryContainer
                isSystem -> MaterialTheme.colorScheme.tertiaryContainer
                else -> MaterialTheme.colorScheme.surfaceVariant
            },
            modifier = Modifier.fillMaxWidth(0.85f)
        ) {
            Column(
                modifier = Modifier.padding(12.dp)
            ) {
                Text(
                    text = when {
                        isUser -> "你"
                        isSystem -> "系统"
                        else -> "AI"
                    },
                    style = MaterialTheme.typography.labelSmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = message.content,
                    style = MaterialTheme.typography.bodyLarge
                )
            }
        }
    }
}

@Composable
fun SettingsDialog(
    params: GenerationParams,
    onDismiss: () -> Unit,
    onSave: (GenerationParams) -> Unit
) {
    var maxTokens by remember { mutableStateOf(params.maxTokens) }
    var temperature by remember { mutableStateOf(params.temperature) }
    var topP by remember { mutableStateOf(params.topP) }
    var topK by remember { mutableStateOf(params.topK) }
    var repeatPenalty by remember { mutableStateOf(params.repeatPenalty) }
    var nThreads by remember { mutableStateOf(params.nThreads) }
    
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("生成参数设置") },
        text = {
            Column(
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                // Max Tokens
                Column {
                    Text("最大Token数: $maxTokens", style = MaterialTheme.typography.bodyMedium)
                    Slider(
                        value = maxTokens.toFloat(),
                        onValueChange = { maxTokens = it.toInt() },
                        valueRange = 64f..4096f,
                        steps = 20
                    )
                }
                
                // Temperature
                Column {
                    Text("温度: ${"%.2f".format(temperature)}", style = MaterialTheme.typography.bodyMedium)
                    Slider(
                        value = temperature,
                        onValueChange = { temperature = it },
                        valueRange = 0.1f..2.0f
                    )
                }
                
                // Top P
                Column {
                    Text("Top-P: ${"%.2f".format(topP)}", style = MaterialTheme.typography.bodyMedium)
                    Slider(
                        value = topP,
                        onValueChange = { topP = it },
                        valueRange = 0.1f..1.0f
                    )
                }
                
                // Top K
                Column {
                    Text("Top-K: $topK", style = MaterialTheme.typography.bodyMedium)
                    Slider(
                        value = topK.toFloat(),
                        onValueChange = { topK = it.toInt() },
                        valueRange = 1f..100f,
                        steps = 20
                    )
                }
                
                // Repeat Penalty
                Column {
                    Text("重复惩罚: ${"%.2f".format(repeatPenalty)}", style = MaterialTheme.typography.bodyMedium)
                    Slider(
                        value = repeatPenalty,
                        onValueChange = { repeatPenalty = it },
                        valueRange = 1.0f..2.0f
                    )
                }
                
                // Threads
                Column {
                    Text("线程数: $nThreads", style = MaterialTheme.typography.bodyMedium)
                    Slider(
                        value = nThreads.toFloat(),
                        onValueChange = { nThreads = it.toInt() },
                        valueRange = 1f..8f,
                        steps = 7
                    )
                }
            }
        },
        confirmButton = {
            TextButton(
                onClick = {
                    onSave(GenerationParams(
                        maxTokens = maxTokens,
                        temperature = temperature,
                        topP = topP,
                        topK = topK,
                        repeatPenalty = repeatPenalty,
                        nThreads = nThreads
                    ))
                }
            ) {
                Text("保存")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text("取消")
            }
        }
    )
}

/**
 * 从Uri获取文件路径
 */
private fun getFilePathFromUri(context: Context, uri: Uri): String? {
    return try {
        // 对于本地文件，直接返回路径
        when (uri.scheme) {
            "file" -> uri.path
            "content" -> {
                // 复制到缓存目录
                val fileName = getFileName(context, uri)
                val cacheFile = java.io.File(context.cacheDir, fileName)
                context.contentResolver.openInputStream(uri)?.use { input ->
                    cacheFile.outputStream().use { output ->
                        input.copyTo(output)
                    }
                }
                cacheFile.absolutePath
            }
            else -> null
        }
    } catch (e: Exception) {
        android.util.Log.e("MainScreen", "Error getting file path from uri", e)
        null
    }
}

private fun getFileName(context: Context, uri: Uri): String {
    var name = "model.gguf"
    context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
        val nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
        if (cursor.moveToFirst() && nameIndex >= 0) {
            name = cursor.getString(nameIndex)
        }
    }
    return name
}
