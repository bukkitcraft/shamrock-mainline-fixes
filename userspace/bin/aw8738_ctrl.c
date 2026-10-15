#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
    int fd;
    if (argc < 2) return 1;

    // Ensure exported
    if (access("/sys/class/gpio/gpio550", F_OK) != 0) {
        fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd >= 0) { write(fd, "550", 3); close(fd); usleep(20000); }
    }
    fd = open("/sys/class/gpio/gpio550/direction", O_WRONLY);
    if (fd >= 0) { write(fd, "out", 3); close(fd); }

    fd = open("/sys/class/gpio/gpio550/value", O_WRONLY);
    if (fd < 0) return 2;

    if (strcmp(argv[1], "on") == 0) {
        // Mode 2: reset low, then 2 pulses, stay high
        write(fd, "0", 1);
        usleep(10000); // 10ms reset
        for (int i = 0; i < 2; i++) {
            write(fd, "1", 1);
            usleep(2);
            write(fd, "0", 1);
            usleep(2);
        }
        write(fd, "1", 1); // Stay high to enable amplifier
    } else {
        // Off / Sleep: pull low for > 1ms
        write(fd, "0", 1);
    }
    close(fd);
    return 0;
}
