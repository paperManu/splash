/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @image_ffmpeg.h
 * The Image_FFmpeg class
 */

#ifndef SPLASH_IMAGE_FFMPEG_H
#define SPLASH_IMAGE_FFMPEG_H

#include "./config.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./image/image.h"
#if HAVE_PORTAUDIO
#include "./sound/speaker.h"
#endif

namespace Splash
{

class Image_FFmpeg : public Image
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Image_FFmpeg(RootObject* root);

    /**
     * \brief Destructor
     */
    ~Image_FFmpeg() final;

    /**
     * No copy, but some move constructors
     */
    Image_FFmpeg(const Image_FFmpeg&) = delete;
    Image_FFmpeg& operator=(const Image_FFmpeg&) = delete;

    /**
     * \brief Set the path to read from
     * \param filename File to read
     * \bool Return true if all went well
     */
    bool read(const std::string& filename) final;

  private:
    std::thread _readLoopThread;
    std::atomic_bool _continueRead{false};
    std::atomic_bool _loopOnVideo{true};

    std::thread _videoDisplayThread;
    struct TimedFrame
    {
        std::unique_ptr<ImageBuffer> frame{};
        uint64_t timing{0ull}; // in us
    };
    std::deque<TimedFrame> _timedFrames;

    // Frame size history, used to keep the frame buffer smaller than _maximumBufferSize
    std::vector<int64_t> _framesSize{};
    int64_t _maximumBufferSize{(int64_t)1 << 29};

    std::mutex _videoQueueMutex;
    std::mutex _videoSeekMutex;
    std::mutex _videoEndMutex;
    std::future<void> _seekFuture;

    std::atomic_bool _timeJump{false};

    bool _intraOnly{false};
    int64_t _startTime{0};
    int64_t _currentTime{0};
    int64_t _elapsedTime{0};
    float _shiftTime{0};
    float _seekTime{0};
    bool _paused{false};
    uint64_t _trimStart{0ull}; //!< Start trimming time
    uint64_t _trimEnd{0ull};   //!< End trimming time

    std::mutex _clockMutex;
    bool _useClock{false};
    int64_t _clockTime{-1};

    AVFormatContext* _avContext{nullptr};
    double _videoTimeBase{0.033};
    int _videoStreamIndex{-1};
    std::string _videoFormat{""}; //!< Holds the current video format information

#if HAVE_PORTAUDIO
    std::unique_ptr<Speaker> _speaker;
    int _audioStreamIndex{-1};
    double _audioTimeBase{0.001};
    bool _planar{false};
    std::string _audioDeviceOutput{""};
    bool _audioDeviceOutputUpdated{false};

    std::thread _audioThread{};
    struct TimedAudioFrame
    {
        ResizableArray<uint8_t> frame{};
        int64_t timing{0ull}; // in us
    };
    std::deque<TimedAudioFrame> _audioQueue{};
    std::mutex _audioMutex{};
#endif

    /**
     * \brief Convert a codec tag to a fourcc
     * \param tag Tag to convert
     * \return Return the tag as a string
     */
    std::string tagToFourCC(unsigned int tag);

    /**
     * \brief Free everything related to FFmpeg
     */
    void freeFFmpegObjects();

    /**
     * Get the media duration
     */
    float getMediaDuration() const;

    /**
     * \brief Base init for the class
     */
    void init();

    /**
     * \brief File read loop
     */
    void readLoop();

    /**
     * \brief Seek in the video
     * \param seconds Desired position
     */
    void seek(float seconds, bool clearQueues = true);

    /**
     * Seek asynchronously
     * \param seconds Desired position
     */
    void seek_async(float seconds, bool clearQueues = true);

    /**
     * Set the audio output
     * \return Return true if all went well
     */
    bool setupAudioOutput(AVCodecContext* audioCodecContext);

    /**
     * Audio loop
     */
    void audioLoop();

    /**
     * Add more media info
     */
    void updateMoreMediaInfo(Values& mediaInfo) final;

    /**
     * \brief Video display loop
     */
    void videoDisplayLoop();

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_IMAGE_FFMPEG_H
