#include "protocol.h"
#include "serial.h"
#include "log.h"

#define END         0300    /* End of packet */
#define ESC         0333    /* Byte stuffing */
#define ESC_END     0334    /* ESC ESC_END means END data byte */
#define ESC_ESC     0335    /* ESC ESC_ESC means ESC data byte */
#define SERIAL_TIMEOUT 100    /* In MS */

int slip_encode(int fd, const unsigned char* payload, size_t len) {
    serial_send_byte(fd, END);

    for(size_t i = 0; i < len; i++) {
        if(payload[i] == END) {
            serial_send_byte(fd, ESC);
            serial_send_byte(fd, ESC_END);
        }
        else if(payload[i] == ESC) {
            serial_send_byte(fd, ESC);
            serial_send_byte(fd, ESC_ESC);
        }
        else {
            serial_send_byte(fd, payload[i]);
        }
    }
    serial_send_byte(fd, END);

    return 0;
}

int slip_decode(int fd, unsigned char* buf, size_t max_len) {
    unsigned char c;
    size_t out_len = 0;

    /* Wait for leading END byte that starts frame. */
    if(serial_recv_byte_timeout(fd, &c, SERIAL_TIMEOUT) < 0) return -1;
    if(c != END) {
        LOGE("Expected SLIP frame start (0XC0) got 0x%02x", c);
        return -1;
    }

    while(1) {
        if(serial_recv_byte_timeout(fd, &c, SERIAL_TIMEOUT) < 0) return -1;
        if(c == END) {
            // Frame end
            return (int)out_len;
        }
        else if (c == ESC) {
            // Escaped byte follows
            unsigned char esc;
            if(serial_recv_byte_timeout(fd, &esc, SERIAL_TIMEOUT) < 0) return -1;
            
            if(esc == ESC_END) {
                c = END;
            }
            else if( esc == ESC_ESC) {
                c = ESC;
            }
            else {
                LOGE("Invalid SLIP escape sequence: -xDB -x%02x", esc);
                return -1;
            }
        }
        // else: c is a leteral data byte, store

        if(out_len >= max_len) {
            LOGE("slip_decode: output buffer full, frame too large");
            return -1;
        }
        buf[out_len++] = c;
    }
    return 0;
}
