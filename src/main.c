#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#define COLOR_RESET     "\x1b[0m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"
#define COLOR_RED       "\x1b[31m"

#define LOGI(fmt, ...) fprintf(stderr, COLOR_GREEN "[INFO] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) fprintf(stderr, COLOR_YELLOW "[WARNING] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) fprintf(stderr, COLOR_RED "[ERROR] " fmt COLOR_RESET "\n", ##__VA_ARGS__)

struct termios config;

void print_line_state(int fd, const char* label) {
    int status;
    ioctl(fd, TIOCMGET, &status);
    LOGI("%s: RTS=%d DTR=%d", label,
         (status & TIOCM_RTS) ? 1 : 0,
         (status & TIOCM_DTR) ? 1 : 0);
}

void log_termios(const struct termios* t) {
    speed_t ispeed = cfgetispeed(t);
    speed_t ospeed = cfgetospeed(t);

    const char* speed_str;
    switch(ospeed) {
        case B9600:     speed_str = "9600"; break;
        case B19200:    speed_str = "19200"; break;
        case B38400:    speed_str = "38400"; break;
        case B57600:    speed_str = "57600"; break;
        case B115200:   speed_str = "115200"; break;
        case B230400:   speed_str = "230400"; break;
        default:        speed_str = "unknown"; break;
    }

    int data_bits = (t->c_cflag & CSIZE) == CS8 ? 8 :
                    (t->c_cflag & CSIZE) == CS7 ? 7 :
                    (t->c_cflag & CSIZE) == CS6 ? 6 : 5;

    char parity = (t->c_cflag & PARENB)
                    ? ((t->c_cflag & PARODD) ? 'O' : 'E') : 'N';

    int stop_bits = (t->c_cflag & CSTOPB) ? 2 : 1;

    LOGI("termios: in=%s out=%s %d%c%d cflag=0x%lx iflag=0x%lx oflag=0x%lx lflag=0x%lx "
         "raw=%s rtscts=%s crtscts_ixon=%s vmin=%d vtime=%d",
         speed_str, speed_str, data_bits, parity, stop_bits,
         (unsigned long)t->c_cflag, (unsigned long)t->c_iflag,
         (unsigned long)t->c_oflag, (unsigned long)t->c_lflag,
         (t->c_lflag & (ICANON | ECHO | ISIG)) ? "no" : "yes",
         (t->c_cflag & CRTSCTS) ? "yes" : "no",
         (t->c_iflag & (IXON | IXOFF)) ? "yes" : "no",
         t->c_cc[VMIN], t->c_cc[VTIME]);
}

void mod_termios(int fd, struct termios* cfg) {
    //log_termios(cfg);

    /* 
     * Input flags - Turn off input processing
     *
     * convert break to nill byte, no CR to NL translation,
     * no input parity check, don't strip high bit off,
     * no XON/XOFF software flow controll.
     */
    cfg->c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);

    /* 
     * Output flags - Turn off output processing
     *
     * no CR to NL translation, no NL to CR-NL translation,
     * no NL to CR translation, no column 0 CR suppresion,
     * no Ctrl-D suppression, no fill chars, no case mapping,
     * no local output processing
     */
    cfg->c_oflag = 0;

    /* 
     * No line processing
     *
     * echo off, echo new line off, canonical mode off,
     * extended input processing off, signal chars off
     */
    cfg->c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    /*
     * Turn off char processing
     *
     * clear current char size mask, no parity checking,
     * no output procesing, force 8 bit input
     */
    cfg->c_cflag &= ~(CSIZE | PARENB);
    cfg->c_cflag |= CS8;

    /* One input byte is enough to return from read()
     * Inter-char timer off
     */
    cfg->c_cc[VMIN] = 1;
    cfg->c_cc[VTIME] = 0;

    /* Com speed(simple version using pre-def consts) */
    if(cfsetispeed(cfg, B115200) < 0 || cfsetospeed(cfg, B115200) < 0) {
        LOGE("Failed to set communication speed");
    }
    
    /* Apply configiration */
    if(tcsetattr(fd, TCSAFLUSH, cfg) < 0) {
        LOGE("Failed to apply configuration");
    }
    //log_termios(cfg);
}

void bridge(int tty_fd) {
    char c;
    fd_set readfds;

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(tty_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = (tty_fd > STDIN_FILENO ? tty_fd : STDIN_FILENO) + 1;

        int r = select(maxfd, &readfds, NULL, NULL, NULL); // NULL timeout = block indef.
        if(r < 0) {
            LOGE("select() failed");
            break;
        }

        if(FD_ISSET(tty_fd, &readfds)) {
            if(read(tty_fd, &c, 1) > 0) {
                write(STDOUT_FILENO, &c, 1);
                if(c == 'q') break; // exit cond. (serial side)
            }
        }

        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            if(read(STDIN_FILENO, &c, 1) > 0) {
                if(c == 'q') break;
                write(tty_fd, &c, 1);
            }
        }
    }
}

void set_rts_dtr_lines(int fd, int rts, int dtr) {
    int stat;
    ioctl(fd, TIOCMGET, &stat);
    if(rts) stat |= TIOCM_RTS; else stat &= ~TIOCM_RTS;
    if(dtr) stat |= TIOCM_DTR; else stat &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &stat); // single atomic(ish) call
}

void trig_dld_rts_dtr(int fd) {

    // 1. Assert EN (RTS) -> chip held in reset
    set_rts_dtr_lines(fd, 1, 0);
    print_line_state(fd, "assert EN");
    usleep(50000);
    
    // 2. Release EN (RTS) FIRST -> chip exits reset, samples GPIO0 = LOW -> download
    set_rts_dtr_lines(fd, 0, 1);
    print_line_state(fd, "release EN");
    usleep(50000); // Wait for bootstrap sampling

    // 3. Release GPIO0 (DTR) LAST -> back to HIGH
    set_rts_dtr_lines(fd, 0, 0);
    print_line_state(fd, "release DTR");
}


int main(void) {
    int fd = 0;
    const char* device = "/dev/ttyUSB0";
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd == -1) {
        LOGE("Failed to open port \n");
        return -1;
    }
    LOGI("Connected to %s", device);

    if(!isatty(fd)) {
        LOGW("Device is not a TTY");
    }

    if(tcgetattr(fd, &config) < 0)
    {
        LOGE("Unable to retrieve termios config from %s", device);
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags& ~O_NDELAY);

    mod_termios(fd, &config);

    trig_dld_rts_dtr(fd);
    bridge(fd);

    close(fd);
    return 0;
} 
