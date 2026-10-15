#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main() {
    char buf[64];
    int is_on = 0;
    
    // Start with amp off (silent sleep)
    system("/usr/local/bin/aw8738_ctrl off");

    while (1) {
        int fd = open("/proc/asound/card0/pcm0p/sub0/status", O_RDONLY);
        if (fd >= 0) {
            int n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) {
                buf[n] = '\0';
                if (strncmp(buf, "state: RUNNING", 14) == 0) {
                    if (!is_on) {
                        system("/usr/local/bin/aw8738_ctrl on");
                        is_on = 1;
                    }
                } else if (strncmp(buf, "closed", 6) == 0) {
                    if (is_on) {
                        system("/usr/local/bin/aw8738_ctrl off");
                        is_on = 0;
                    }
                }
            }
        }
        usleep(50000); // 50ms poll = instant 50ms response to sound!
    }
    return 0;
}
