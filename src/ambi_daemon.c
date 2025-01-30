#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <libusb-1.0/libusb.h>

#include "daemon.h"





static const char *help_str =
        " Daemon name:  " DAEMON_NAME          "\n"
        " Daemon  ver:  " DAEMON_VERSION_STR   "\n\n"
#ifdef  DEBUG
        " Build  mode:  debug\n"
#else
        " Build  mode:  release\n"
#endif
        " Build  date:  " __DATE__ "\n"
        " Build  time:  " __TIME__ "\n"
#if  CHANGED_FILES == 0
        " Build  hash:  "  COMMIT_HASH "\n\n"
#else
        " Build  hash:  *" COMMIT_HASH "\n\n"
#endif
        "Options:                      description:\n\n"
        "       --no_chdir             Don't change the directory to '/'\n"
        "       --no_fork              Don't do fork\n"
        "       --no_close             Don't close standart IO files\n"
        "       --pid_file [value]     Set pid file name\n"
        "       --log_file [value]     Set log file name\n"
        "       --cmd_pipe [value]     Set CMD Pipe name\n"
        "  -v,  --version              Display daemon version\n"
        "  -h,  --help                 Display this help\n\n";



// indexes for long_opt function
enum
{
    cmd_opt_help    = 'h',
    cmd_opt_version = 'v',

    //daemon options
    cmd_opt_no_chdir,
    cmd_opt_no_fork,
    cmd_opt_no_close,
    cmd_opt_pid_file,
    cmd_opt_log_file,
    cmd_opt_cmd_pipe
};



static const char *short_opts = "hv";


static const struct option long_opts[] =
{
    { "version",      no_argument,       NULL, cmd_opt_version  },
    { "help",         no_argument,       NULL, cmd_opt_help     },

    //daemon options
    { "no_chdir",     no_argument,       NULL, cmd_opt_no_chdir },
    { "no_fork",      no_argument,       NULL, cmd_opt_no_fork  },
    { "no_close",     no_argument,       NULL, cmd_opt_no_close },
    { "pid_file",     required_argument, NULL, cmd_opt_pid_file },
    { "log_file",     required_argument, NULL, cmd_opt_log_file },
    { "cmd_pipe",     required_argument, NULL, cmd_opt_cmd_pipe },

    { NULL,           no_argument,       NULL,  0 }
};



pthread_t  pthread_cmd_pipe;




void daemon_exit_handler(int sig)
{

    //Here we release resources


    unlink(daemon_info.pid_file);
    unlink(daemon_info.cmd_pipe);

    _exit(EXIT_FAILURE);
}



void init_signals(void)
{
    set_sig_handler(SIGINT,  daemon_exit_handler); //for Ctrl-C in terminal for debug (in debug mode)
    set_sig_handler(SIGTERM, daemon_exit_handler);

    set_sig_handler(SIGCHLD, SIG_IGN); // ignore child
    set_sig_handler(SIGTSTP, SIG_IGN); // ignore tty signals
    set_sig_handler(SIGTTOU, SIG_IGN);
    set_sig_handler(SIGTTIN, SIG_IGN);
    set_sig_handler(SIGHUP,  SIG_IGN);
}



void processing_cmd(int argc, char *argv[])
{
    int opt;

    // We use the processing_cmd function for processing the command line and
    // for commands from the DAEMON_CMD_PIPE_NAME
    // For this we use the getopt_long function several times
    // to work properly, we must reset the optind
    optind = 0;


    while( (opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1 )
    {
        switch( opt )
        {

            case cmd_opt_help:
                        puts(help_str);
                        exit_if_not_daemonized(EXIT_SUCCESS);
                        break;

            case cmd_opt_version:
                        puts(DAEMON_NAME "  version  " DAEMON_VERSION_STR "\n");
                        exit_if_not_daemonized(EXIT_SUCCESS);
                        break;


                 //daemon options
            case cmd_opt_no_chdir:
                        daemon_info.no_chdir = 1;
                        break;

            case cmd_opt_no_fork:
                        daemon_info.no_fork = 1;
                        break;

            case cmd_opt_no_close:
                        daemon_info.no_close_stdio = 1;
                        break;

            case cmd_opt_pid_file:
                        daemon_info.pid_file = optarg;
                        break;

            case cmd_opt_log_file:
                        daemon_info.log_file = optarg;
                        break;

            case cmd_opt_cmd_pipe:
                        daemon_info.cmd_pipe = optarg;
                        break;

            default:
                        puts("for more detail see help\n\n");
                        exit_if_not_daemonized(EXIT_FAILURE);
                        break;
        }
    }
}



void* cmd_pipe_thread(void *thread_arg)
{
    int   fd;
    int   argc;
    char *arg;
    char **argv;
    char *cmd_pipe_buf;



    pthread_detach(pthread_self());
    unlink(daemon_info.cmd_pipe);


    argv = (char **)malloc(PIPE_BUF*sizeof(char *));
    if( !argv )
        daemon_error_exit("Can't get mem for argv (CMD_PIPE): %m\n");


    cmd_pipe_buf = (char *)malloc(PIPE_BUF);
    if( !cmd_pipe_buf )
        daemon_error_exit("Can't get mem for cmd_pipe_buf: %m\n");


    if( mkfifo(daemon_info.cmd_pipe, 0622) != 0 )
        daemon_error_exit("Can't create CMD_PIPE: %m\n");


    fd = open(daemon_info.cmd_pipe, O_RDWR);
    if( fd == -1 )
        daemon_error_exit("Can't open CMD_PIPE: %m\n");



    while(1)
    {
        memset(cmd_pipe_buf, 0, PIPE_BUF);


        if( read(fd, cmd_pipe_buf, PIPE_BUF) == -1 )             // wait for command from DAEMON_CMD_PIPE_NAME
            daemon_error_exit("read CMD_PIPE return -1: %m\n");


        argc = 1;                                                // see getopt_long function
        arg  = strtok(cmd_pipe_buf, " \t\n");


        while( (arg != NULL)  && (argc < PIPE_BUF) )
        {
            argv[argc++] = arg;
            arg          = strtok(NULL, " \t\n");
        }


        if( argc > 1 )
          processing_cmd(argc, argv);
    }


    return NULL;
}



void init(void *data)
{
    init_signals();


    if( pthread_create(&pthread_cmd_pipe, NULL, cmd_pipe_thread, NULL) != 0 )
       daemon_error_exit("Can't create thread_cmd_pipe: %m\n");


    //Here is your code to initialize
}

#define debug_x_len (2560)
#define debug_y_len (1440)
#define X_LED 42
#define Y_LED 24
#define ws2812_len  (X_LED + X_LED + Y_LED + Y_LED)
typedef struct {
    u_int8_t R;
    u_int8_t G;
    u_int8_t B;
} LED_RGB;

int main(int argc, char *argv[])
{
    processing_cmd(argc, argv);
    daemonize2(init, NULL);

    libusb_context *ctx = NULL;
    libusb_device_handle *argb_controller_handle;
    int transferred, res;
    u_int8_t data[64];
    
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Не удалось открыть дисплей\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Получаем параметры экрана
    XWindowAttributes attr;
    XGetWindowAttributes(display, root, &attr);
    int width = attr.width;
    int height = attr.height;
    int depth = attr.depth; // Глубина цвета

    printf("Размер экрана: %dx%d, Глубина: %d бит\n", width, height, depth);


    // Настройка XShm
    XShmSegmentInfo shminfo;
    XImage *image = XShmCreateImage(display, DefaultVisual(display, screen), depth, ZPixmap, NULL, &shminfo, width, height);
    if (!image) {
        fprintf(stderr, "Не удалось создать XImage\n");
        XCloseDisplay(display);
        return 1;
    }

    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
    if (shminfo.shmid < 0) {
        perror("shmget");
        XDestroyImage(image);
        XCloseDisplay(display);
        return 1;
    }

    shminfo.shmaddr = (char *)shmat(shminfo.shmid, NULL, 0);
    image->data = shminfo.shmaddr;
    shminfo.readOnly = False;


    if (!XShmAttach(display, &shminfo)) {
        fprintf(stderr, "Не удалось прикрепить сегмент разделяемой памяти\n");
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, NULL);
        XDestroyImage(image);
        XCloseDisplay(display);
        return 1;
    }

    LED_RGB ledBuf[ws2812_len] = {0,};


//    ssize_t devCount = libusb_get_device_list(NULL, list);
//    if(devCount < 0) return NULL;
//    for(ssize_t i = 0; i < devCount; i++) {
//        libusb_device *device = list[i];
//        struct libusb_device_descriptor *desc;
//        int r = libusb_get_device_descriptor(device, &desc);
//    	if (r < 0) {
//    		argb_controller = NULL;
//    	}
//        if(desc->idProduct == atoi("0483") && desc->idVendor == atoi("5740")){
//            argb_controller = list[i];
//        }
//    }
    libusb_init(&ctx);
    
    argb_controller_handle = libusb_open_device_with_vid_pid(ctx, 0x0483, 0x5740);
    if(argb_controller_handle != NULL) {
        libusb_set_configuration(argb_controller_handle, 1);
        libusb_claim_interface(argb_controller_handle, 1);
        libusb_detach_kernel_driver(argb_controller_handle, 1);
    } else {
        printf("Устройство не найдено\n");
    }
    while( !daemon_info.terminated )
    {
        //argb_controller = findDevice("0483", "5740"); /* TODO: must be start options */

        //if(argb_controller != NULL) {
        if(argb_controller_handle != NULL) {
            size_t xUp    = 0 + 10,                yUp    = 0 + 10;
            size_t xDown  = debug_x_len - 10, yDown  = debug_y_len - 10;
            size_t xLeft  = 0 + 10,                yLeft  = debug_y_len - 10;
            size_t xRight = debug_x_len - 10, yRight = 0 + 10;

            XShmGetImage(display, root, image, 0, 0, AllPlanes);

            u_int32_t red_mask = image->red_mask;
            u_int32_t green_mask = image->green_mask;
            u_int32_t blue_mask = image->blue_mask;
            for(size_t i = 0; i < X_LED; i++) { /* up */
                u_int32_t up_pixel = XGetPixel(image, xUp, yUp);
                ledBuf[i].R = (up_pixel & red_mask) >> 16;
                ledBuf[i].G = (up_pixel & green_mask) >> 8;
                ledBuf[i].B = (up_pixel & blue_mask);
                xUp = (debug_x_len / X_LED) * i;
            }
            for(size_t i = 0; i < Y_LED; i++) { /* right */
                u_int32_t right_pixel = XGetPixel(image, xRight, yRight);
                ledBuf[X_LED + i].R = (right_pixel & red_mask) >> 16;
                ledBuf[X_LED + i].G = (right_pixel & green_mask) >> 8;
                ledBuf[X_LED + i].B = (right_pixel & blue_mask);
                yRight = (debug_y_len / Y_LED) * i;
            }
            for(size_t i = 0; i < X_LED; i++) { /* Down */
                u_int32_t down_pixel = XGetPixel(image, xDown, yDown);
                ledBuf[X_LED + Y_LED + i].R = (down_pixel & red_mask) >> 16;
                ledBuf[X_LED + Y_LED + i].G = (down_pixel & green_mask) >> 8;
                ledBuf[X_LED + Y_LED + i].B = (down_pixel & blue_mask);
                xDown = (debug_x_len / X_LED) * (X_LED - i);
            }
            for(size_t i = 0; i < Y_LED; i++) { /*left*/
                u_int32_t left_pixel = XGetPixel(image, xLeft, yLeft);
                ledBuf[X_LED + Y_LED + X_LED + i].R = (left_pixel & red_mask) >> 16;
                ledBuf[X_LED + Y_LED + X_LED + i].G = (left_pixel & green_mask) >> 8;
                ledBuf[X_LED + Y_LED + X_LED + i].B = (left_pixel & blue_mask);
                yLeft = (debug_y_len / Y_LED) * (Y_LED - i);
            }
          /* (ws2812_len / 20) + 1 = 7 */
            for(u_int32_t i = 0; i < 7; i++) {
                if(i < 6) {
                    data[3] = i;
                    memcpy(data + 4, &ledBuf[i * 20], 20 * sizeof(LED_RGB));
                } else {
                    for(size_t j = 0; j < sizeof(data); j++) {
                        data[j] = 0;
                    }
                    data[3] = i;
                    memcpy(data + 4, &ledBuf[120], 12 * sizeof(LED_RGB));
                }
                
                res = libusb_bulk_transfer(argb_controller_handle, 0x01, data, sizeof(data), &transferred, 1000);
                if(res != 0) {
                    printf("Transfer fault %d\n", res);
                }
                usleep(500);
                //sleep(1);
            }
            //usleep(150000);
            //sleep(1);
        } else {
            printf("No device\n");
            argb_controller_handle = libusb_open_device_with_vid_pid(ctx, 0x0483, 0x5740);
            if(argb_controller_handle != NULL) {
                libusb_claim_interface(argb_controller_handle, 1);
            }
            sleep(1);
        }
    

            //for(size_t i = 0; i < ws2812_len; i++) {
            //    char temp[10];
            //    if(i < X_LED) {
            //        strcpy(temp, "upper ");
            //    } else if (i < X_LED + Y_LED) {
            //        strcpy(temp, "right ");
            //    } else if (i < X_LED + Y_LED + X_LED) {
            //        strcpy(temp, "down ");
            //    } else {
            //        strcpy(temp, "left ");
            //    }
            //    printf("%s [%ld]: %d %d %d \n", temp, i, ledBuf[i].R, ledBuf[i].G, ledBuf[i].B);
            //}
//            usleep(100);
        //} else {
        //    sleep(3);
        //}
    }

    libusb_release_interface(argb_controller_handle, 1);
    libusb_close(argb_controller_handle);
    libusb_exit(ctx);


    XShmDetach(display, &shminfo);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, NULL);
    XDestroyImage(image);
    XCloseDisplay(display);

    return EXIT_SUCCESS; // good job (we interrupted (finished) main loop)
}