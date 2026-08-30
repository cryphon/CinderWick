#include "serial.h"

#include "log.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>

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




int set_config(int fd, struct termios* cfg) {
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
    if(cfsetispeed(cfg, B115200) < 0 || cfsetospeed(cfg, B115200) < 0) {
        LOGE("Failed to set communication speed");
        return -1;
    }
    
    /* Apply configiration */
    if(tcsetattr(fd, TCSAFLUSH, cfg) < 0) {
        LOGE("Failed to apply configuration");
        return -1;
    }
    log_termios(cfg);
    return 0;
}

int serial_open(const char* device, speed_t baud) {
    int fd = 0;
    struct termios config;
    
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if(fd == -1) {
        return -1;
    }
    LOGI("Connected to %s", device);

    if(!isatty(fd)) {
        LOGE("Device is not a TTY");
        goto serial_on_err;
    }

    if(tcgetattr(fd, &config) < 0) {
        LOGE("Unable to retrieve termios config from %s", device);
        goto serial_on_err;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags& ~O_NDELAY);

    if(set_config(fd, &config) < 0) {
        LOGE("Terminating serial opening, failed to set config");
        goto serial_on_err;
    }
    return fd;

serial_on_err:
    close(fd);
    return -1;
}

int serial_close(int fd) {
    if(close(fd) < 0) {
        LOGE("Failed to close serial connection");
        return -1;
    }
    return 0;
}

int serial_send_byte(int fd, unsigned char c) {
    ssize_t res = write(fd, &c, 1);
    if(res < 0) {
        LOGE("Failed to write byte to serial");
        return -1;
    }
    return 0;
}

int serial_recv_byte(int fd, unsigned char* out) {
    ssize_t res = read(fd, out, 1);
    if(res < 0) {
        LOGE("Failed to read byte from serial");
        return -1;
    }
    return 0;
}
