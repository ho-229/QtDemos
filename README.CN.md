# QtDemos
[English](./README.md) | 简体中文

![windows](https://img.shields.io/badge/Windows-passing-green.svg?style=flat-square)
![linux](https://img.shields.io/badge/Linux-unknow-red.svg?style=flat-square)


* 这是一个关于 `Qt5` 的项目实践，未来会添加一些控件。欢迎大家留言评论，参考学习和测试。

| 名称 | 描述              |
| ---- | ---------------- |
| [CustomWidgetDemos](./CustomWidgetDemos) | 自定义动画控件和使用示例 |
| [MultithreadedDownloader](./MultithreadedDownloader) | 多线程下载器 |
| [QmlDemo](./QmlDemo) | QML 学习项目 |
| [QmlFireworks](./QmlFireworks) | QML  烟花 (粒子系统) Demo |
| [VideoPlayer](./VideoPlayer) | 视频播放器 (使用了 `FFmpeg` 和 `OpenGL`) |

## 开发环境
* 工具集 : `Qt 5.15.2`
* 编译器 : `Microsoft Visual C++ 2019` , `GCC 10`

## 如何编译
* 需要 `Qt5` 环境
```shell
sudo pacman -S qt5  # Arch Linux
```
* 克隆仓库到本地并编译项目
```shell
git clone https://github.com/ho229v3666/QtDemos.git
     # or https://gitee.com/ho229/QtDemos.git
cd QtDemos
mkdir build
cd build
qmake ..
make -j
```
## 关于作者
* QQ : 2189684957
* E-mail : <ho229v3666@gmail.com>