/*
 * Tests whether the Datapath driver handles correctly connecting/disconnecting
 * the video input off a driver card, when buffers are grabber into a USERPTR and
 * more than a single input buffer is used.
 *
 * As of driver version 7.19.0.24078, running this code and connecting/disconnecting
 * the video input results in a freeze of the capture, more or less randomly.
 * The same code runs flawlessly with driver version 7.17.14.304 (which sadly does not
 * work with kernel 4.15 and newer).
 *
 * To compile it:
 * $ g++ ${FILENAME} -o datapath
 *
 * Then run it:
 * $ ./datapath
 */

#include <linux/videodev2.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define BUFFER_SIZE 4096 * 4096 * 3
#define BUFFER_COUNT 3

int main()
{
    int deviceFd;
    if ((deviceFd = open("/dev/video0", O_RDWR | O_NONBLOCK)) < 0)
    {
        printf("Error opening\n");
        return 1;
    }

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = 2048;
    format.fmt.pix.height = 2048;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(deviceFd, VIDIOC_S_FMT, &format) < 0)
    {
        printf("Error setting format\n");
        return 1;
    }

    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_USERPTR;
    bufrequest.count = BUFFER_COUNT;

    if (ioctl(deviceFd, VIDIOC_REQBUFS, &bufrequest) < 0)
    {
        printf("Error reqbufs\n");
        return 1;
    }

    struct v4l2_buffer bufferinfo;
    auto buffers = std::vector<std::vector<uint8_t>>((size_t)BUFFER_COUNT);
    for (size_t i = 0; i < BUFFER_COUNT; ++i)
    {
        buffers.emplace_back((size_t)BUFFER_SIZE);
        auto& buffer = buffers[buffers.size() - 1];

        memset(&bufferinfo, 0, sizeof(bufferinfo));
        bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferinfo.memory = V4L2_MEMORY_USERPTR;
        bufferinfo.index = i;
        bufferinfo.m.userptr = reinterpret_cast<unsigned long>(buffer.data());
        bufferinfo.length = buffer.size();

        // Put the buffer in the incoming queue.
        if (ioctl(deviceFd, VIDIOC_QBUF, &bufferinfo) < 0)
        {
            printf("Error queuing\n");
            return 1;
        }
    }

    // Activate streaming
    int type = bufferinfo.type;
    if (ioctl(deviceFd, VIDIOC_STREAMON, &type) < 0)
    {
        printf("Error streamon\n");
        return 1;
    }

    int frameIndex = 0;
    while (true)
    {
        struct pollfd fd;
        fd.fd = deviceFd;
        fd.events = POLLIN | POLLPRI;
        fd.revents = 0;

        if (poll(&fd, 1, 50) > 0)
        {
            if (fd.revents & (POLLIN | POLLPRI))
            {
                // Dequeue the buffer.
                memset(&bufferinfo, 0, sizeof(bufferinfo));
                bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                bufferinfo.memory = V4L2_MEMORY_USERPTR;

                while (ioctl(deviceFd, VIDIOC_DQBUF, &bufferinfo) < 0)
                {
                    if (errno != EINTR)
                    {
                        printf("Error dequeuing\n");
                        return 1;
                    }
                }

                printf("Grabbed frame %i from buffer %i\n", frameIndex, bufferinfo.index);
                ++frameIndex;

                // Queue the next one.
                bufferinfo.length = BUFFER_SIZE;
                if (ioctl(deviceFd, VIDIOC_QBUF, &bufferinfo) < 0)
                {
                    printf("Error requeuing\n");
                    return 1;
                }
            }
        }
    }

    // Deactivate streaming
    if (ioctl(deviceFd, VIDIOC_STREAMOFF, &type) < 0)
    {
        printf("Error streamoff\n");
        return 1;
    }
}
