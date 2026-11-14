---
title: 版本变更记录
sort: 5
lang: zh
---
# 变更日志
本文件记录所有重要变更。

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
并采用[语义化版本](https://semver.org/)。

## [v1.1.10.0] - 2026-11-09

**对应核心固件版本：T1_v1.1.10.0**

### 新添加的功能
- 新增 IMU 惯导状态、外挂算力板供电控制、停止音频播放以及系统状态迁移（重启与关机）接口。
- 新增灯语逻辑说明。
- 新增 T1机型 的 Emotion 表情枚举。

### 对现有功能的变更
- 更新`接口说明.md`中关节限位参数。
- 更新`接口说明.md`中录音相关说明，补充 8 通道录音回采通道描述。
- 更新`接口说明.md`中语音合成接口的文本字符规范与 CLI 引号嵌套说明。
- 调整`接口说明.md`目录结构。

### 已经移除的功能
- 移除 `ShutdownTask`，由 `MigrateSystemState` 替代。
- 移除四足坐下动作与 T1 不再支持的动作枚举。
- 移除 `BIPED_RECOVERY`、`BIPED_STAND_DEFAULT`、`BIPED_LOCOMOTION_DEFAULT`、`BIPED_LOCOMOTION_TERRAIN`、`BIPED_LOCOMOTION_ROTATE_LOCAL`、`BIPED_LOCOMOTION_ANIMATION` 动作：`接口说明.md`的动作枚举表、运控状态机图及示例代码中的动作切换规则不再包含上述动作。
- 移除 `AudioInfo` 中的 PCM 相关引用。
- 移除示例目录中的 `build.sh`。

### 对 bug 的修复
- 修复文档中锚点链接跳转错误的问题。
- 修复文档格式与示例代码错误。
- 修复进行 arm 交叉编译时opencv报错问题。

## [v1.0.0.0] - 2026-10-10

T1 SDK 接口文档与示例代码首个正式版本。
