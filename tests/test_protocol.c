#include "protocol.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>


void test_slip_roundtrip(void) {
    int fds[2];
    pipe(fds); // fds[0] = read end, fds[1] = write end

    unsigned char payload[] = { 0x07, 0x07, 0x12, 0x20, 0xC0, 0xDB, 0x55 };
    slip_encode(fds[1], payload, sizeof(payload));

    unsigned char decoded[64];
    int len = slip_decode(fds[0], decoded, sizeof(decoded));

    assert(len == (int)sizeof(payload));
    assert(memcmp(decoded, payload, len) == 0);
    printf("test_slip_roundtrip: PASS\n");
}

int main(void) {
    printf("===== TEST BLOCK START =====\n");

    test_slip_roundtrip();

    printf("===== TEST BLOCK END   =====\n\n");


    return 0;
}
