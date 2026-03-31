# GitHub Actions 构建配置

本目录包含 LocalAI Chat 项目的自动化构建工作流。

## 工作流说明

### 1. build.yml - 日常构建
**触发条件：**
- 推送到 `main`、`master` 或 `develop` 分支
- 发起 Pull Request
- 手动触发 (workflow_dispatch)

**输出：**
- Debug APK（30天保留）
- Release APK（90天保留，仅限 main/master 分支）
- 构建报告（失败时上传）

### 2. build-release.yml - 发布构建
**触发条件：**
- 推送版本标签（如 `v1.0.0`）
- 手动触发并指定版本号

**功能：**
- 自动签名 Release APK（需要配置 secrets）
- 创建 GitHub Release 并上传 APK
- 生成发布说明

## 配置签名（可选）

如需自动签名 APK，在仓库 Settings → Secrets 中添加：

| Secret | 说明 |
|--------|------|
| `KEYSTORE_BASE64` | Base64 编码的签名密钥库文件 |
| `KEYSTORE_PASSWORD` | 密钥库密码 |
| `KEY_ALIAS` | 密钥别名 |
| `KEY_PASSWORD` | 密钥密码 |

### 生成 Base64 密钥库

```bash
# 生成签名密钥（如尚未创建）
keytool -genkey -v -keystore localai.keystore -alias release -keyalg RSA -keysize 2048 -validity 10000

# 转换为 Base64
cat localai.keystore | base64
```

## 手动下载构建产物

1. 进入 GitHub 仓库 → Actions 标签
2. 选择最近成功的工作流运行
3. 页面底部 Artifacts 区域下载 APK

## 构建环境

- **OS:** Ubuntu Latest
- **JDK:** Temurin 17
- **Android SDK:** API 34
- **NDK:** r25c
- **Gradle:** 8.x (wrapper 指定版本)

## 常见问题

### Q: 构建失败提示 NDK 找不到？
A: `build-release.yml` 使用 `nttld/setup-ndk` 自动安装 NDK。如需特定版本，修改 `ndk-version` 参数。

### Q: 如何加速构建？
A: 工作流已配置 Gradle 缓存，首次构建较慢，后续会复用缓存。

### Q: APK 安装时提示"解析包时出现问题"？
A: 
- 确保 Android 版本 >= 7.0 (API 24)
- 设备架构需为 ARM64
- 检查是否允许"未知来源"安装

## 相关文档

- [项目 README](../../README.md)
- [快速开始](../../QUICKSTART.md)
- [使用指南](../../USAGE_GUIDE.md)
