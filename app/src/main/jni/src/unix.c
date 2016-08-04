#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

int initPipe(int *fd, const char *fname) {
    if (!fname)
        return -1;

    if (*fd > 0)
        return *fd;

    //unlink(fname);

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("open unix socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, fname);

    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        LOGE("connect unix socket: %s", fname);
        close(sock);
        return -1;
    }

    *fd = sock;
    return *fd;
}

void closePipe(int *fd) {
    if (*fd > 0) {
        close(*fd);
        *fd = -1;
    }
}

