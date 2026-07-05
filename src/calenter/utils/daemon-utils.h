#ifndef DAEMON_UTILS_H
#define DAEMON_UTILS_H

#include "config.h"

#define DAEMON_FIFO "/tmp/calenter-notification-daemon.fifo"


void handle_daemon_fifo(Config config);

void start_notification_daemon(Config config);

#endif
