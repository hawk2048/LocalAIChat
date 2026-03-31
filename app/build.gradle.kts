plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.localai.chat"
    compileSdk = 34
    ndkVersion = "25.1.8937393"

    defaultConfig {
        applicationId = "com.localai.chat"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        vectorDrawables {
            useSupportLibrary = true
        }
        
        externalNativeBuild {
            cmake {
                cppFlags += listOf(
                    "-std=c++17",
                    "-fexceptions",
                    "-frtti"
                )
                cFlags += listOf(
                    "-std=c11",
                    "-Wall",
                    "-Wextra"
                )
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_TOOLCHAIN=clang",
                    "-DANDROID_ARM_MODE=arm",
                    "-DCMAKE_BUILD_TYPE=Release"
                )
                // 启用 llama.cpp 支持
                targets += "localai"
            }
        }
        
        ndk {
            // 支持的CPU架构
            // arm64-v8a: 64位ARM设备（推荐，性能最佳）
            // armeabi-v7a: 32位ARM设备（兼容性）
            // x86_64: 模拟器和部分平板
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }
    }

    buildTypes {
        debug {
            isDebuggable = true
            isMinifyEnabled = false
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Debug"
                    cFlags += "-g"
                    cppFlags += "-g"
                }
            }
        }
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }
    }
    
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    
    kotlinOptions {
        jvmTarget = "17"
        freeCompilerArgs += listOf(
            "-opt-in=kotlin.RequiresOptIn",
            "-opt-in=androidx.compose.material3.ExperimentalMaterial3Api"
        )
    }
    
    buildFeatures {
        compose = true
        buildConfig = true
    }
    
    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.4"
    }
    
    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
            excludes += "/META-INF/DEPENDENCIES"
        }
        jniLibs {
            // 避免重复打包原生库
            useLegacyPackaging = true
        }
    }
    
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    
    // Split APKs by ABI to reduce size
    splits {
        abi {
            isEnable = true
            reset()
            include("arm64-v8a", "armeabi-v7a", "x86_64")
            isUniversalApk = true
        }
    }
}

dependencies {
    // ============================================================
    // Core Android
    // ============================================================
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.7.0")
    implementation("androidx.activity:activity-compose:1.8.2")
    
    // ============================================================
    // Jetpack Compose
    // ============================================================
    implementation(platform("androidx.compose:compose-bom:2023.10.01"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.compose.foundation:foundation")
    
    // ============================================================
    // Navigation
    // ============================================================
    implementation("androidx.navigation:navigation-compose:2.7.6")
    
    // ============================================================
    // Lifecycle & ViewModel
    // ============================================================
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.7.0")
    
    // ============================================================
    // Coroutines (异步处理)
    // ============================================================
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3")
    
    // ============================================================
    // DataStore (本地存储)
    // ============================================================
    implementation("androidx.datastore:datastore-preferences:1.0.0")
    
    // ============================================================
    // Networking (模型下载)
    // ============================================================
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
    implementation("com.squareup.okhttp3:okhttp-sse:4.12.0")
    implementation("com.squareup.okio:okio:3.7.0")
    
    // ============================================================
    // JSON Parsing
    // ============================================================
    implementation("com.google.code.gson:gson:2.10.1")
    
    // ============================================================
    // Security (模型文件加密，可选)
    // ============================================================
    implementation("androidx.security:security-crypto:1.1.0-alpha06")
    
    // ============================================================
    // App Startup (优化启动性能)
    // ============================================================
    implementation("androidx.startup:startup-runtime:1.1.1")
    
    // ============================================================
    // Testing
    // ============================================================
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.7.3")
    testImplementation("io.mockk:mockk:1.13.8")
    
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    androidTestImplementation(platform("androidx.compose:compose-bom:2023.10.01"))
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")
    
    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}

// ============================================================
// 自定义任务：下载预训练模型
// ============================================================
tasks.register("downloadModel") {
    group = "build"
    description = "Download a sample GGUF model for testing"
    
    doLast {
        val modelDir = file("src/main/assets/models")
        modelDir.mkdirs()
        
        // 可以在这里配置下载链接
        println("请手动下载GGUF模型文件并放入: ${modelDir.absolutePath}")
        println("推荐模型:")
        println("  - Phi-2 Q4: https://huggingface.co/microsoft/phi-2-gguf")
        println("  - TinyLlama Q4: https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v0.1-GGUF")
        println("  - Qwen2.5-1.5B: https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF")
    }
}
