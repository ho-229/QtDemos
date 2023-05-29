/**
 * @brief Static Configurations
 * @anchor Ho 229
 * @date 2023/4/21
 */

#ifndef CONFIG_H
#define CONFIG_H

#define VIDEO_CACHE_SIZE 256
#define AUDIO_CACHE_SIZE 256
#define SUBTITLE_CACHE_SIZE 64

// Audio playback delay, it's a experience value
#define AUDIO_DELAY 0.25

// FFmpegDecoder::decode() will be called asynchronously
// when the time difference between the current frame and the last cached frame
// is less than MIN_DECODED_DURATION, in seconds
#define MIN_DECODED_DURATION 0.25

#define MAX_DECODED_DURATION MIN_DECODED_DURATION * 2

#endif // CONFIG_H
