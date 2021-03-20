
#include "headers.h"

#include "uvc_events.h"
#include "processing_actions.h"
#include "image_endpoint.h"

static void image_uvc_video_process(struct processing *processing)
{
    struct endpoint_image *image = &processing->source.image;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct events *events = &processing->events;
    struct v4l2_buffer uvc_buffer;

    if (!uvc->is_streaming || events->stream == STREAM_OFF)
    {
        return;
    }

    memset(&uvc_buffer, 0, sizeof(struct v4l2_buffer));
    uvc_buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    uvc_buffer.memory = V4L2_MEMORY_USERPTR;
    if (ioctl(uvc->fd, VIDIOC_DQBUF, &uvc_buffer) < 0)
    {
        printf("UVC: Unable to dequeue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    memcpy(uvc->mem[uvc_buffer.index].start, image->data, image->size);
    uvc_buffer.bytesused = image->size;

    if (ioctl(uvc->fd, VIDIOC_QBUF, &uvc_buffer) < 0)
    {
        printf("UVC: Unable to queue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    uvc->qbuf_count++;

    if (settings->show_fps)
    {
        uvc->buffers_processed++;
    }
}

void processing_loop_image_uvc(struct processing *processing)
{
    struct endpoint_image *image = &processing->source.image;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;

    int activity;
    struct timeval tv;
    double next_frame_time = 0;
    double now;
    fd_set fdsu, fdsi;

    printf("PROCESSING: IMAGE %s -> UVC %s\n", image->image_path, uvc->device_name);

    while (!*(processing->terminate))
    {
        FD_ZERO(&fdsu);
        FD_ZERO(&fdsi);
        FD_SET(uvc->fd, &fdsu);
        FD_SET(image->inotify_fd, &fdsi);

        fd_set efds = fdsu;
        fd_set dfds = fdsu;

        nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        activity = select(max(image->inotify_fd, uvc->fd) + 1, &fdsi, &dfds, &efds, &tv);

        if (activity == -1)
        {
            printf("PROCESSING: Select error %d, %s\n", errno, strerror(errno));
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }

        if (activity == 0)
        {
            printf("PROCESSING: Select timeout\n");
            break;
        }

        if (FD_ISSET(image->inotify_fd, &fdsi))
        {
            image_load(processing);
        }

        if (FD_ISSET(uvc->fd, &efds))
        {
            uvc_events_process(processing);
        }

        now = processing_now();

        if (FD_ISSET(uvc->fd, &dfds))
        {
            if (now >= next_frame_time)
            {
                image_uvc_video_process(processing);
                next_frame_time = now + settings->frame_interval;
            }
        }

        processing_internals(processing);
    }
}