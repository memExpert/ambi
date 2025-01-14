#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"

int main(int argc, char **argv)
{
    bool amend = false;
    const char *coloropt = "white";
    bool brief = false;
    int delay = 0;

    char *arg;
    int option;
    struct optparse options;

    optparse_init(&options, argv);
    while ((option = optparse(&options, "abc:d::")) != -1) {
        switch (option) {
        case 'a':
            amend = true;
            break;
        case 'b':
            brief = true;
            break;
        case 'c':
            coloropt = options.optarg;
            break;
        case 'd':
            delay = options.optarg ? atoi(options.optarg) : 1;
            break;
        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
            exit(EXIT_FAILURE);
        }
    }

    /* Print remaining arguments. */
    while ((arg = optparse_arg(&options)))
        printf("%s\n", arg);



    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Can't connect to X-server\n");
        return 1;
    }

    Window root = DefaultRootWindow(display);


    int x = 200, y = 200;

    XColor color;

    Visual *visual = DefaultVisual(display, 0);
    Colormap colormap = DefaultColormap(display, 0);

    XShmSegmentInfo shminfo;
    XImage *image = XShmCreateImage(display, visual, 24, ZPixmap, 0, &shminfo, 1, 1);

    if (!image) {
        fprintf(stderr, "Err: can't create XImage\n");
        XCloseDisplay(display);
        return 1;
    }

    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
    if (shminfo.shmid < 0) {
        fprintf(stderr, "err: can't create shm\n");
        XDestroyImage(image);
        XCloseDisplay(display);
        return 1;
    }

    shminfo.shmaddr = (char *) shmat(shminfo.shmid, 0, 0);
    if ((long)shminfo.shmaddr == -1) {
        fprintf(stderr, "Err: can't link shm\n");
        XDestroyImage(image);
        XCloseDisplay(display);
        return 1;
    }
    image->data = shminfo.shmaddr;

    XShmAttach(display, &shminfo);
    while (1) {
        XShmGetImage(display, root, image, x, y, AllPlanes);

        unsigned long pixel = XGetPixel(image, 0, 0);
        color.pixel = pixel;
        XQueryColor(display, colormap, &color);

//        printf("Pixel (%d, %d): R=%d, G=%d, B=%d\n", x, y, color.red / 256, color.green / 256, color.blue / 256);

        usleep(10 * 1000);
    }

    XShmDetach(display, &shminfo);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);
    XDestroyImage(image);
    XCloseDisplay(display);

    return 0;
}