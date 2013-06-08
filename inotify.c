#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void errDoIt(int errnoflag, int err, const char* fmt, va_list ap) {
    static const size_t MAXLINE = 16384;

    char buf[MAXLINE];
    size_t len;

    vsnprintf(buf, MAXLINE, fmt, ap);
    len = strlen(buf);

    if (errnoflag) {
        snprintf(buf + len, MAXLINE - len, ": %s", strerror(err));
        len = strlen(buf);
    }

    strncat(buf, "\n", MAXLINE - len);

    fflush(stdout);
    fputs(buf, stderr);
    fflush(NULL);
}

void errSys(const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    errDoIt(1, errno, fmt, ap);

    va_end(ap);
    exit(11);
}

void errQuit(const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    errDoIt(0, errno, fmt, ap);

    va_end(ap);
    exit(22);
}

void usageExit(const char* progName) {
    printf("usage: %s [dir_to_watch]\n", progName);
    exit(1);
}

int isDir(const char* path) {
    struct stat s;

    if (stat(path, &s) < 0) {
        errSys("could not stat %s", path);
        return -1;
    }

    return S_ISDIR(s.st_mode);
}

void printInotifyEvent(struct inotify_event* event) {
    printf("    wd =%2d; ", event->wd);

    if (event->cookie > 0) {
        printf("cookie =%4d", event->cookie);
    }

    printf("mask = ");

#define PRINT_MASK(key) \
    if (event->mask & key) printf(#key " ");

    PRINT_MASK(IN_ACCESS);
    PRINT_MASK(IN_ATTRIB);
    PRINT_MASK(IN_CLOSE_NOWRITE);
    PRINT_MASK(IN_CLOSE_WRITE);
    PRINT_MASK(IN_CREATE);
    PRINT_MASK(IN_DELETE);
    PRINT_MASK(IN_DELETE_SELF);
    PRINT_MASK(IN_IGNORED);
    PRINT_MASK(IN_ISDIR);
    PRINT_MASK(IN_MODIFY);
    PRINT_MASK(IN_MOVE_SELF);
    PRINT_MASK(IN_MOVED_FROM);
    PRINT_MASK(IN_MOVED_TO);
    PRINT_MASK(IN_OPEN);
    PRINT_MASK(IN_Q_OVERFLOW);
    PRINT_MASK(IN_UNMOUNT);

    printf("\n");

    if (event->len > 0) {
        printf("      name = %s\n", event->name);
    }
}

int displayInotifyEvent(int inotify, char* buf, size_t bufSize) {
    struct inotify_event* event;
    ssize_t bytes;

r:
    if ((bytes = read(inotify, buf, bufSize - 1)) < 0) {
        if (errno == EINTR) {
            goto r;
        } else if (bytes == 0) {
            errQuit("inotify read returned 0");
        } else {
            errSys("could not read from inotify fd");
        }
    }

    for (char* p = buf; p < buf + bytes; p += sizeof(*event) + event->len) {
        event = (struct inotify_event*) p;
        printInotifyEvent(event);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    static const size_t NAME_MAX = 8096;
#define INOTIFY_BUF_SIZE 10 * (sizeof(struct inotify_event) + NAME_MAX + 1)

    if (argc > 2) {
        usageExit(argv[0]);
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usageExit(argv[0]);
        }
    }

    const char* path = argc == 2 ? argv[1] : ".";

    if (!isDir(path)) {
        errQuit("%s is not a directory", path);
    }

    int queue = inotify_init();
    if (queue < 0) {
        errSys("could not init inotify");
    }

    int wfd = inotify_add_watch(queue, path, IN_ALL_EVENTS | IN_ONLYDIR | IN_CLOSE);
    if (wfd < 0) {
        errSys("could not inotify add watch %s", path);
    }

    char buf[INOTIFY_BUF_SIZE];

    while (1) {
        if (displayInotifyEvent(queue, buf, INOTIFY_BUF_SIZE) < 0) {
            errQuit("could not fill inotify event");
        }
    }

    if (close(queue) < 0) {
        errSys("could not close queue");
    }
    exit(0);
}
