# Video Player
* A video player based on `Qt`, using `FFmpeg` for decoding, `SDL2` for playing audio and `OpenGL` for rendering.

* Screenshot
![image](./screenshot/run.png)

## Build & Run
* Windows

    Copy the [DLL files](./ffmpeg/win64/bin) to the release directory before running the executable.

    Download the [SDL2 devel](http://www.libsdl.org/download-2.0.php) to your computer and add it to your system environment variable `SDL_PATH`.

* Linux

    Install `FFmpeg` and build from source.
    ```shell
    sudo pacman -S ffmpeg       # Arch
    sudo apt install ffmpeg     # Debian
    ```
