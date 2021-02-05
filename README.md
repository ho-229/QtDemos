# QtDemos
### 项目说明
* 一个关于 `Qt5` 的项目实践，未来会添加一些控件和 `Video Player`。欢迎大家留言评论，参考学习和测试。
* 开发环境：`Qt 5.14.2 MSVC 2017 (Windows)`，`Qt 5.15.2 GCC (Linux)`
* | 名称 | 描述              |
  | ---- | ---------------- |
  | [CustomWidgetDemos](./CustomWidgetDemos) | 一些动画控件和使用示例 |
  | [MultithreadedDownloader](./MultithreadedDownloader) | 多线程下载器 |
---------
### 如何编译
* 需要 `Qt5` 环境
```shell
sudo pacman -S qt5
```
* 克隆到仓库到本地并编译项目
```shell
git clone https://gitee.com/ho229/QtDemos.git
cd QtDemos
mkdir build
cd build
qmake ..
make -j
```

### 关于作者
* QQ：2189684957
* E-mail：<2189684957@qq.com>