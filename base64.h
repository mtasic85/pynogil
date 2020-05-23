/*
 * https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
 *
 * NOTE:
 *  doesn't do any error-checking while decoding
 *  non base 64 encoded data will get processed
 */
#ifndef PYNOGIL_BASE64_H
#define PYNOGIL_BASE64_H

#include <stdint.h>
#include <stdlib.h>

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);

void build_decoding_table(void);
void base64_cleanup(void);

#endif