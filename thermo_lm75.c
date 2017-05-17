#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define TEMP_REG    0x00 // 2bytes to store the measured temperature data
#define CONF_REG    0x01  // 1byte to set the devic operating condition
#define THYST_REG   0x02 // 2bytes to store the hysteresis T limit
#define TOS_REG     0x03 // 2bytes to store the overtemperature shutdown limit

static long int i2c_slave_addr = 0x48;
static long int i2c_bus_nr = 1;
static const char *fifo_client = "/var/run/imsg/rx_pipe";
static const char *fifo_server = "/var/run/thermo_lm75/rx_pipe";
static int devfd;

static void removefifo()
{
    unlink(fifo_server);
    exit(0);
}

void start_server()
{
    unsigned char buf[32];
    unsigned short shortbuf;
    int n;
    float tempr;
    FILE *srvfile;
    int clientfd, dummyfd;
    struct sigaction sa_act;

    signal(SIGPIPE, SIG_IGN);
    sa_act.sa_handler = removefifo;
    sigemptyset(&sa_act.sa_mask);
    sa_act.sa_flags = 0;
    sigaction(SIGINT, &sa_act, NULL);
    sigaction(SIGTERM, &sa_act, NULL);

    if ((srvfile = fopen(fifo_server, "r")) == NULL) {
        syslog(LOG_ERR, "open server fifo %s: %s", fifo_server, strerror(errno));
        exit(1);
    }
    if ((dummyfd = open(fifo_server, O_WRONLY)) < 0) { //make sure we don't close on eof
        syslog(LOG_ERR, "open dummyfd %s: %s", fifo_server, strerror(errno));
        exit(1);
    }

    for (;;) {
        if (fgets(buf, sizeof(buf), srvfile) == NULL) {
            syslog(LOG_WARNING, "error reading client request, discarding\n");
            continue;
        }
        if ((clientfd = open(fifo_client, O_WRONLY)) < 0) {
            syslog(LOG_WARNING, "open client fifo %s: %s", fifo_client, strerror(errno));
            continue;
        }

        /* only one command for now */
        if (!strncmp(buf, "TEMPERATURE\n", sizeof(buf))) {
            shortbuf = TEMP_REG;
            if (write(devfd, &shortbuf, 1) != 1) {
                syslog(LOG_WARNING, "error writing to i2c device(%d), continue\n", devfd);
                continue;
            }
            if (read(devfd, &shortbuf, 2) != 2) {
                syslog(LOG_WARNING, "error reading from i2c device(%d), continue\n", devfd);
                continue;
            }

            shortbuf = ((shortbuf >> 8) | (shortbuf << 8));
            tempr = ((signed short)shortbuf >> 5) * 0.125; // only 11 msb bits

            snprintf(buf, sizeof(buf), "TEMPERATURE %.2f\n", tempr);

            if ((n = write(clientfd, buf, strlen(buf))) != strlen(buf)) {
                syslog(LOG_WARNING, "error writing response to client, discarding\n");
                continue;
            }

            close(clientfd);

        } else {
            syslog(LOG_WARNING, "undefined method request from client: %s\n", buf);
            close(clientfd);
        }
    }
}

void daemon_init(const char *pname, int facility)
{
    int i;
    pid_t pid;

    if ((pid = fork()) < 0) {
        perror("fork1");
        exit(1);
    }
    if (pid != 0)
        exit(0);

    setsid();
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        perror("signal set SIGHUP");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        perror("fork2");
        exit(1);
    }
    if (pid != 0)
        exit(0);

    chdir("/");
    umask(0);
    
    for (i = 0; i < 64; ++i)
        if (i != devfd)
            close(i);

    openlog(pname, LOG_PID, facility);
}

int main(int argc, char **argv)
{
    int opt; 
    char *err, devname[32];
    unsigned short buf;
    int fd;

    while ((opt = getopt(argc, argv, "a:b:s:c:")) != -1) {
        switch (opt) {
        case 'a':
            i2c_slave_addr = (optarg[0] == '0' && (optarg[1] == 'x'|| optarg[1] == 'X')) ? \
                        strtol(optarg, &err, 16) : strtol(optarg, &err, 10);
            break;
        case 'b':
            i2c_bus_nr = strtol(optarg, &err, 10);
            break;
        case 's':
            fifo_server = strdup(optarg);
            break;
        case 'c':
            fifo_client = strdup(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-s i2c_slave_addr] [-b i2c_bus_nr] " \
                    "[-s fifo_server] [-c fifo_client]\n", argv[0]);
            exit(1);
        }
        if (*err) {
            fprintf(stderr, "%s\n", "Argument parsing error");
            exit(1);
        }
    }
    
    snprintf(devname, sizeof(devname), "/dev/i2c-%ld", i2c_bus_nr);

    if ((devfd = open(devname, O_RDWR)) < 0) {
        fprintf(stderr, "open %s: %s\n", devname, strerror(errno));
        exit(1);
    }

    if (ioctl(devfd, I2C_SLAVE, i2c_slave_addr) < 0) {
        fprintf(stderr, "unable to use i2c slave address %lx: %s\n", i2c_slave_addr, strerror(errno));
        exit(1);
    }

    if (mkfifo(fifo_server, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        fprintf(stderr, "mkfifo %s: %s\n", fifo_server, strerror(errno));
        exit(1);
    }

    /* just check for permissions before forking */
    if ((fd = open(fifo_server, O_RDONLY | O_NONBLOCK)) < 0) {
        fprintf(stderr, "open %s: %s\n", fifo_server, strerror(errno));
        exit(1);
    }
    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    daemon_init(argv[0], 0);
    start_server();
}
