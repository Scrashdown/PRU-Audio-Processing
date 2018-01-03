#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ringbuffer.h"


int main(void) {
    printf("\nStarting ringbuffer testing program!\n");

    printf("TEST: A newly created buffer has length 0: ");
    const size_t nelem = 10;
    const size_t blocksize = 6 * 4;
    ringbuffer_t * ringbuf = ringbuf_create(nelem, blocksize);
    if (ringbuf == NULL) {
        fprintf(stderr, "\nERROR: ringbuffer could not be created for test.\n");
        return 1;
    }
    const size_t length = ringbuf_len(ringbuf);
    if (length == 0) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected length is %zu but found %zu.\n", 0, length);
    }

    printf("TEST: A buffer has maxLength nelem * blocksize : ");
    if (ringbuf -> maxLength == nelem * blocksize) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected maxLength is %zu but found %zu\n", ringbuf -> maxLength, nelem * blocksize);
    }

    printf("TEST: Pushing N bytes of data to an empty buffer increases its length by N: ");
    const size_t data_len = 2 * blocksize;
    uint8_t data[data_len];
    for (size_t i = 0; i < data_len; ++i) {
        data[i] = i;
    }
    ringbuf_push(ringbuf, data, blocksize, 2);
    const size_t length2 = ringbuf_len(ringbuf);
    if (length2 == data_len) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected buffer length %zu but found %zu.\n", data_len, length2);
    }

    printf("TEST: Popping N bytes of data after pushing N bytes gives out the same data, and length 0: ");
    uint8_t outdata[data_len];
    size_t read = ringbuf_pop(ringbuf, outdata, blocksize, 2);
    const size_t length3 = ringbuf_len(ringbuf);
    int success = length3 == 0 ? 1 : 0;
    success = success && (read == 2);
    for (size_t i = 0; i < data_len; ++i) {
        if (data[i] != outdata[i]) {
            success = 0;
        }
    }
    if (success != 0) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected length %zu but found %zu\n, expected to read %zu samples but read %zu instead.", 0, length3, 2, read);
        printf("    Expected data : [ ");
            for (size_t i = 0; i < data_len; ++i) {
                printf("%u ", (char) data[i]);
            }
        printf("]\n");
        printf("    Actual data : [ ");
            for (size_t i = 0; i < data_len; ++i) {
                printf("%u ", (char) outdata[i]);
            }
        printf("]\n");
    }
    
    printf("TEST: Popping data on an empty buffer does not return any data: ");
    read = ringbuf_pop(ringbuf, outdata, blocksize, 1);
    if (read == 0) {
        printf("Success!\n");
    } else {
        printf("Failure! %zu blocks were returned.\n", read);
    }

    printf("TEST: Pushing maxLength bytes to an empty buffer does not cause an overflow: ");
    uint8_t data2[blocksize * nelem];
    size_t written = ringbuf_push(ringbuf, data2, blocksize, nelem);
    if (written == nelem) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected %zu blocks to be written but found %zu.\n", nelem, written);
    }

    printf("TEST: Pushing some data to a full buffer causes an overflow: ");
    written = ringbuf_push(ringbuf, data2, blocksize, 1);
    if (written == 0) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected %zu to be written but found %zu.\n", 0, written);
    }

    printf("TEST: Pushing data to an already overflowed buffer triggers a new overflow: ");
    written = ringbuf_push(ringbuf, data2, blocksize, 1);
    if (written == 0) {
        printf("Success!\n");
    } else {
        printf("Failure! Expected %zu to be written but found %zu.\n", 0, written);
    }

    return 0;
}