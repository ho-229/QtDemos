# QtDemos

English | [简体中文](./README.CN.md)

* This is a small project about `Qt5`. More controls will be added in the future. Welcome to try it out and leave your comments.

| Name | Description              |
| ---- | ---------------- |
| [CustomWidgetDemos](./CustomWidgetDemos) | Custom animation controls and examples |
| [MultithreadedDownloader](./MultithreadedDownloader) | Multi-threaded Downloader |
| [QmlDemo](./QmlDemo) | QML Learning project |
| [QmlFireworks](./QmlFireworks) | QML Fireworks (Particle System) Demo |
| [VideoPlayer](./VideoPlayer) | Video Player (Use `FFmpeg` and `OpenGL`) |

## Development Environment

* Tool Kit : `Qt 5.15.2`.
* Complier : `Microsoft Visual C++ 2019` , `GCC 10`.

## Build

* Require `Qt5` and `FFmpeg`.  

  * Install Qt5.  

  * Install FFmpeg.
    * Windows

      Download [FFmpeg(shared libraries)](https://github.com/BtbN/FFmpeg-Builds/releases) and add the path to your system environment variable `FFMPEG_PATH`.

    * Linux

      ```shell
      sudo pacman -S ffmpeg                     # Arch
      sudo apt install ffmpeg libavfilter-dev   # Debian
      ```

* Clone this repository and build from source.

  ```shell
  git clone https://github.com/ho-229/QtDemos.git
       # or https://gitee.com/ho229/QtDemos.git
  cd QtDemos
  mkdir build
  cd build
  qmake ..
  make -j
  ```
