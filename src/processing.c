
#include "headers.h"
#include "processing_fb_uvc.h"
#include "processing_image_uvc.h"
#include "processing_v4l2_uvc.h"
#include "processing_stdin_uvc.h"
#include "processing_actions.h"

static bool terminate = 0;

void term(int signum)
{
    printf("\nTERMINATE: Signal %s\n", (signum == SIGTERM) ? "SIGTERM" : "SIGINT");
    terminate = true;
}

void processing_loop(struct processing *processing)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));

    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    processing->terminate = &terminate;

    if (processing->target.type == ENDPOINT_UVC)
    {
        switch (processing->source.type)
        {
        case ENDPOINT_FB:
            processing_loop_fb_uvc(processing);
            break;

        case ENDPOINT_IMAGE:
            processing_loop_image_uvc(processing);
            break;

        case ENDPOINT_V4L2:
            processing_loop_v4l2_uvc(processing);
            break;

        case ENDPOINT_STDIN:
            processing_loop_stdin_uvc(processing);
            break;

        default:
            printf("PROCESSING: ERROR - Missing loop for UVC endpoint\n");
            break;
        }
    }
    else
    {
        printf("PROCESSING: ERROR - Missing loop\n");
    }
}
