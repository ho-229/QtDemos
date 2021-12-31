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

* Tool Kit : `Qt 5.15.2`
* Complier : `Microsoft Visual C++ 2019` , `GCC 10`

## Build

* Require `Qt5`, `FFmpeg` and `SDL2`.  

  Install Qt5.  

  Install FFmpeg and SDL2.
  * Windows

    Download [FFmpeg](https://github.com/BtbN/FFmpeg-Builds/releases) and add the library path to your system environment variable `FFMPEG_PATH`.

    Download [SDL2 devel](http://www.libsdl.org/download-2.0.php) and add it the library path your system environment variable `SDL_PATH`.

  * Linux

    Install `FFmpeg` and `SDL2` than build from source.

    ```shell
    sudo pacman -S ffmpeg sdl2                            # Arch
    sudo apt install ffmpeg libavfilter-dev libsdl2-dev   # Debian
    ```

* Clone this repository and build from source.

  ```shell
  git clone https://github.com/ho229v3666/QtDemos.git
       # or https://gitee.com/ho229/QtDemos.git
  cd QtDemos
  mkdir build
  cd build
  qmake ..
  make -j
     ```

## Author

* QQ : 2189684957
* E-mail : <ho229v3666@gmail.com>
