#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
#include <WinSock2.h>
#define poll WSAPoll
#pragma comment(lib, "ws2_32")
#else
#include <poll.h>
#endif

#include <time.h>

#include <duktape.h>

void poll_register(duk_context *ctx);
