#ifndef PYNOGIL_H
#define PYNOGIL_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <uv.h>
#include <duktape.h>
#include <duk_console.h>
#include <duk_module_node.h>

#include "duk_poll.h"

#define DEBUG // FIXME: remove
#define PYNG_DEBUG

#ifdef PYNG_DEBUG
#define PYNG_LOG_DEBUG(fmt, ...) printf("DEBUG " fmt "\n", ##__VA_ARGS__)
#else
#define PYNG_LOG_DEBUG(fmt, ...) ;
#endif

#ifdef PYNG_DEBUG
#define PYNG_LOG_ERROR(fmt, ...) printf("ERROR " fmt "\n", ##__VA_ARGS__)
#else
#define PYNG_LOG_ERROR(fmt, ...) ;
#endif

#define PYNG_ERROR(fmt, ...) printf("error: " fmt "\n", ##__VA_ARGS__)

/*
 * extras
 */
/*static duk_ret_t native_print(duk_context *ctx);*/

/*
 * duk_module_node
 */
/*static void _duk_fatal(void *udata, const char *msg);*/
duk_ret_t _cb_duk_resolve_module(duk_context *ctx);
duk_ret_t _cb_duk_load_module(duk_context *ctx);

/*
 * pyng_ctx_*
 */
typedef struct pyng_ctx_t {
    uv_loop_t *uv_loop;
    duk_context *duk_ctx;
} pyng_ctx_t;

pyng_ctx_t *pyng_ctx_new(void);
void _pyng_ctx_del_uv_loop(pyng_ctx_t *ctx);
void _pyng_ctx_del_duk_ctx(pyng_ctx_t *ctx);
void pyng_ctx_del(pyng_ctx_t *ctx);
int pyng_ctx_run_file(pyng_ctx_t *ctx, const char *path);
uv_buf_t pyng_ctx_read_file(pyng_ctx_t *ctx, const char *path);
int pyng_ctx_eval_buf(pyng_ctx_t *ctx, uv_buf_t buf);

#endif
