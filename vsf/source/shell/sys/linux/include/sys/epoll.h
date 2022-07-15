#ifndef __VSF_LINUX_EPOLL_H__
#define __VSF_LINUX_EPOLL_H__

#include "shell/sys/linux/vsf_linux_cfg.h"

#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   include "../sys/types.h"
#   include "../fcntl.h"
#   include "../poll.h"
#else
#   include <sys/types.h>
#   include <fcntl.h>
#   include <poll.h>
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EPOLL_CLOEXEC       O_CLOEXEC

#define EPOLL_CTL_ADD       0
#define EPOLL_CTL_MOD       1
#define EPOLL_CTL_DEL       2

#define EPOLLIN             POLLIN
#define EPOLLOUT            POLLOUT
#define EPOLLHUP            POLLHUP
#define EPOLLERR            POLLERR
#define EPOLLPRI            POLLPRI

#define EPOLLET             (1 << 16)
#define EPOLLONESHOT        (1 << 17)
#define EPOLLWAKEUP         (1 << 18)
#define EPOLLEXCLUSIVE      (1 << 19)

typedef union epoll_data {
    void        *ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;
    epoll_data_t data;
};

#if VSF_LINUX_CFG_WRAPPER == ENABLED
#define epoll_create        VSF_LINUX_WRAPPER(epoll_create)
#define epoll_create1       VSF_LINUX_WRAPPER(epoll_create1)
#define epoll_ctl           VSF_LINUX_WRAPPER(epoll_ctl)
#define epoll_wait          VSF_LINUX_WRAPPER(epoll_wait)
#define epoll_pwait         VSF_LINUX_WRAPPER(epoll_pwait)
#define epoll_pwait2        VSF_LINUX_WRAPPER(epoll_pwait2)
#endif

int epoll_create(int size);
int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout);
int epoll_pwait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout,
                      const sigset_t *sigmask);
int epoll_pwait2(int epfd, struct epoll_event *events,
                      int maxevents, const struct timespec *timeout,
                      const sigset_t *sigmask);

#ifdef __cplusplus
}
#endif

#endif
