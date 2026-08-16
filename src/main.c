#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define COLOR_RESET     "\x1b[0m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"
#define COLOR_RED       "\x1b[31m"

#define LOGI(fmt, ...) fprintf(stderr, COLOR_GREEN "[INFO] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) fprintf(stderr, COLOR_YELLOW "[WARNING] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) fprintf(stderr, COLOR_RED "[ERROR] " fmt COLOR_RESET "\n", ##__VA_ARGS__)

struct termios config;

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
    log_termios(cfg);

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
    if(cfsetispeed(cfg, B9600) < 0 || cfsetospeed(cfg, B9600) < 0) {
        LOGE("Failed to set communication speed");
    }
    
    /* Apply configiration */
    if(tcsetattr(fd, TCSAFLUSH, cfg) < 0) {
        LOGE("Failed to apply configuration");
    }
    log_termios(cfg);
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
    mod_termios(fd, &config);

    close(fd);
    return 0;
} 
