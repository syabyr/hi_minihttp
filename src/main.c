#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#include "hidemo.h"
#include "server.h"

#include "config/app_config.h"

#include "rtsp/rtspservice.h"
#include "rtsp/rtputils.h"
#include "rtsp/ringfifo.h"

#include "chipid.h"

#include "http_post.h"

#include "night.h"

// void Usage(char *sPrgNm) {
//     printf("Usage : %s <path to sensor config ini>\n", sPrgNm);
//     printf("   ex: %s ./configs/imx222_1080p_line.ini\n", sPrgNm);
// }

int main(int argc, char *argv[]) {
    chip_id();
    isp_version();

    if (parse_app_config("./minihttp.ini") != CONFIG_OK) {
        printf("Can't load app config './minihttp.ini'\n");
        return EXIT_FAILURE;
    }

    start_server();

    memset(&state, 0, sizeof(struct SDKState));

    int s32MainFd;
    if (app_config.rtsp_enable) {
        ringmalloc(1280*720);
        printf("RTSP server START, listen for client connecting...\n");
        PrefsInit();
        signal(SIGINT, IntHandl);
        s32MainFd = tcp_listen(SERVER_RTSP_PORT_DEFAULT);
        if (ScheduleInit() == ERR_FATAL) {
            fprintf(stderr,"Fatal: Can't start scheduler %s, %i \nServer is aborting.\n", __FILE__, __LINE__);
            return 0;
        }
        RTP_port_pool_init(RTP_DEFAULT_PORT);
    }

    if(start_sdk(&state) == EXIT_FAILURE) keepRunning = 0; // return EXIT_FAILURE;
    // TODO when return EXIT_FAILURE need to deinitialize sdk correctly

    if (app_config.night_mode_enable) start_monitor_light_sensor();
    if (app_config.http_post_enable) start_http_post_send();
    if (app_config.rtsp_enable) {
        struct timespec ts = { 2, 0 };
        while (keepRunning) {
            nanosleep(&ts, NULL);
            EventLoop(s32MainFd);
        }
        ringfree();
        printf("RTSP server quit!\n");
    } else {
        while (keepRunning) sleep(1);
    }

    stop_sdk(&state);
    stop_server();

    printf("Shutdown main thread\n");
    return EXIT_SUCCESS;
}
