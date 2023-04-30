/**
 * @brief Static Configurations
 * @anchor Ho 229
 * @date 2023/4/21
 */

#ifndef CONFIG_H
#define CONFIG_H

#define VIDEO_CACHE_SIZE 32
#define AUDIO_CACHE_SIZE 128
#define SUBTITLE_CACHE_SIZE 32

// Audio playback delay, it's a experience value
#define AUDIO_DELAY 0.3

// FFmpegDecoder::decode() will be called asynchronously
// when the time difference between the current frame and the last cached frame
// is less than MIN_DECODED_DURATION, in seconds
#define MIN_DECODED_DURATION 0.3

// Allowed time difference between the video PTS and the audio PTS, in seconds
#define ALLOW_DIFF 0.045         // 45ms

#endif // CONFIG_H
