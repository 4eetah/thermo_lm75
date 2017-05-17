#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

static const char *fifo_client = "/var/run/imsg/rx_pipe";
static const char *fifo_server = "/var/run/thermo_lm75/rx_pipe";
static int serverfd, clientfd;

#define error(fmt, ...) \
    do { fprintf(stderr, "errno(%s): " fmt "\n", strerror(errno), ##__VA_ARGS__); exit(1); } \
    while(0)

static void removefifo()
{
    unlink(fifo_client);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <request>\n", argv[0]);
        exit(1);
    }

    unsigned char buf[64];
    FILE *fclient;

    if (mkfifo(fifo_client, S_IRUSR | S_IWUSR | S_IWGRP) < 0 && errno != EEXIST)
        error("mkfifo %s", fifo_client);

    if (atexit(removefifo) != 0)
        error("atexit");

    if ((serverfd = open(fifo_server, O_WRONLY)) < 0)
        error("open %s", fifo_server);

    snprintf(buf, sizeof(buf), "%s\n", argv[1]);

    if (write(serverfd, buf, strlen(buf)) != strlen(buf))
        error("write serverfd(%d)", serverfd);

    if ((clientfd = open(fifo_client, O_RDONLY)) < 0)
        error("open %s", fifo_client);

    if ((fclient = fdopen(clientfd, "r")) == NULL)
        error("fdopen %s failed", fifo_client);

    if (fgets(buf, sizeof(buf), fclient) == NULL)
        error("fgets failed");

    fputs(buf, stdout);
}
