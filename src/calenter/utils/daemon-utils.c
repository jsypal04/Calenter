#include <errno.h>
#include <fcntl.h>
#include <libnotify/notify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug.h"
#include "daemon-utils.h"

void handle_daemon_fifo(Config config) {
    if (config.enable_notifications) {
        start_notification_daemon(config);
    }
    else {
        int fifo_fd = open(DAEMON_FIFO, O_WRONLY);
        if (fifo_fd != -1) {
            char buf[2] = "k";
            write(fifo_fd, buf, 2);
            close(fifo_fd);
        }
    }
}

void start_notification_daemon(Config config) {
    debug_log("Starting notification daemon...\n");
    FILE* fp = popen("pidof calenter-notification-daemon", "r");
    char pid_buff[16];

    if (fgets(pid_buff, sizeof(pid_buff), fp) != NULL) {
        debug_log("Daemon already running\n");
        pclose(fp);
        return;
    }

    char proc_dir[1024] = "\0";
    ssize_t len = readlink("/proc/self/exe", proc_dir, sizeof(proc_dir) - 1);
    if (len == -1) {
        debug_log("Failed to start notification daemon.\n");
        return;
    }

    if (mkfifo(DAEMON_FIFO, 0666) < 0) {
       if (errno != EEXIST) {
           debug_log("Failed to create FIFO. Daemon not started.\n");
           return;
       }
    }

    debug_log("proc_dir: %s\n", proc_dir);
    char noti_path[2048] = "\0";
    strncpy(noti_path, proc_dir, 1024);
    strcpy(noti_path + strlen(proc_dir), "-notification-daemon");
    debug_log("noti_path: %s\n", noti_path);

    int pid = fork();
    if (pid == 0) {
        if (setsid() < 0) exit(EXIT_FAILURE);
        umask(0);
        chdir("/");

        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        int retval = execl(noti_path, "calenter-notification-daemon", NULL);
        if (retval == -1) {
            NotifyNotification* noti = notify_notification_new(
                "Daemon Error",
                "Failed to start the notification daemon",
                ""
            );
            notify_notification_show(noti, NULL);
            g_object_unref(G_OBJECT(noti));
            notify_uninit();
            _exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        int fifo_fd = open(DAEMON_FIFO, O_WRONLY);
        char buf[10] = "\0";
        sprintf(buf, "t%d", config.notify_time);
        write(fifo_fd, buf, sizeof(buf));
        close(fifo_fd);

        debug_log("started noti daemon\n");
    }

}
