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

#define DEBUG
static int exit_status;
static duk_idx_t uv_handle_idx;

duk_ret_t cb_resolve_module(duk_context *ctx);
duk_ret_t cb_load_module(duk_context *ctx);
char *compile_py2js(uv_handle_t *uv_handle, char *python_code);
static void my_fatal(void *udata, const char *msg);
int exec_js_code(uv_handle_t *uv_handle, char *js_code);
void exec_python_code(uv_handle_t *uv_handle, uv_buf_t buf);
uv_buf_t read_from_path(uv_loop_t *loop, const char *path);
void async_exec_python_code(uv_async_t *async);

#endif