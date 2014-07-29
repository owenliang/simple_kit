/*
 * Copyright (C) 2014-2015  liangdong <liangdong01@baidu.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "sio.h"

static char cmdline_quit = 0;

static void stdin_echo(struct sio_fd *sfd)
{
	char input[64];
	int ret = read(sfd->fd, input, sizeof(input));
	if (ret > 0) {
		printf("%.*s", ret, input);
	} else if (ret == 0) {
		printf("stdin_echo=read:EOF\n");
		cmdline_quit = 1;
	} else {
		printf("stdin_echo=read:-1\n");
	}
}

static void stdin_callback(struct sio *sio, struct sio_fd *sfd, enum sio_event event, void *arg)
{
    switch (event) {
    case SIO_READ:
        stdin_echo(sfd);
        break;
    case SIO_WRITE:
        fprintf(stderr, "stdin_callback=write\n");
        break;
    case SIO_ERROR:
        fprintf(stderr, "stdin_callback=error\n");
        break;
    default:
        fprintf(stderr, "stdin_callback=unknown\n");
        break;
    }
}

static void timer_callback(struct sio *sio, struct sio_timer *timer, void *arg)
{
    fprintf(stderr, "-------------------[timer]2 seconds passed------------------\n");
    sio_start_timer(sio, timer, 2000, timer_callback, NULL);
}

static void sio_cmdline_quit_handler(int signo)
{
	cmdline_quit = 1;
}

static void sio_cmdline_signal()
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sio_cmdline_quit_handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
	sio_cmdline_signal();

    struct sio *sio = sio_new();
    if (!sio) {
        fprintf(stderr, "sio_new=NULL\n");
        return -1;
    }

    struct sio_fd *sfd = sio_add(sio, 0, stdin_callback, NULL);
    if (!sfd) {
        fprintf(stderr, "sio_add=NULL\n");
        return -1;
    }
    sio_watch_read(sio, sfd);

    struct sio_timer timer;
    sio_start_timer(sio, &timer, 2000, timer_callback, NULL);

    while (!cmdline_quit)
        sio_run(sio, 1000);

    sio_del(sio, sfd);
    sio_stop_timer(sio, &timer);
    sio_free(sio);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
