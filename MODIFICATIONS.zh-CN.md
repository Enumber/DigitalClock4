[English](MODIFICATIONS.md) · **中文**

# 修改说明（Linux 维护版）

本仓库是 [Digital Clock 4](https://github.com/Kolcha/DigitalClock4)（作者 Nick Korotysh，
GPL-3.0-or-later）的 **Linux 向维护版**。上游曾支持 Linux，但从未提供安装包；4.7.9
（2020-12）之后项目已归档。本仓库让程序能在 Linux 上编译、安装与日常使用。

**本维护版维护者：** **ENum**（GitHub: [Enumber](https://github.com/Enumber)）。

修改内容仍以 GPL-3.0-or-later 许可；原版权声明保留。

## 构建修复
- 空格路径下 `lrelease`、英文 `.ts` 桩、`<QWidget>` 头文件、QHotkey qmake 与第三方入库。

## 翻译
- 修复插件翻译资源加载；补全简体中文；修正快捷键与误译。语言仍跟随系统 locale。

## 与官方 Windows 版对齐
- 正确应用图标；附带皮肤/纹理；关机插件；朗读时钟；X11 全屏检测；任务栏图标开关。

## 缺陷修复与打包
- 自启动路径修正；**移除**自动更新联网检查；`$ORIGIN` RUNPATH；交互式 `install.sh`。
