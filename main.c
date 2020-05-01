#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <uv.h>
#include <duktape.h>

uv_fs_t open_req;
uv_fs_t read_req;

static char buffer[1024];
static uv_buf_t iov;
static int exit_status;

static duk_ret_t native_print(duk_context *ctx) {
    duk_push_string(ctx, " ");
    duk_insert(ctx, 0);
    duk_join(ctx, duk_get_top(ctx) - 1);
    printf("%s\n", duk_safe_to_string(ctx, -1));
    return 0;
}

static duk_ret_t native_adder(duk_context *ctx) {
    int i;
    int n = duk_get_top(ctx);  /* #args */
    double res = 0.0;

    for (i = 0; i < n; i++) {
        res += duk_to_number(ctx, i);
    }

    duk_push_number(ctx, res);
    return 1;  /* one return value */
}

char* compile_py2js(char* python_code) {
    printf("%s", python_code);
    return NULL;
}

int exec_js_code(char* js_code) {
    duk_context *ctx = duk_create_heap_default();

    // (void) argc; (void) argv;  /* suppress warning */

    duk_push_c_function(ctx, native_print, DUK_VARARGS);
    duk_put_global_string(ctx, "print");
    duk_push_c_function(ctx, native_adder, DUK_VARARGS);
    duk_put_global_string(ctx, "adder");

    duk_eval_string(ctx, "print('Hello world!');");

    duk_eval_string(ctx, "print('2+3=' + adder(2, 3));");
    duk_pop(ctx);  /* pop eval result */

    duk_destroy_heap(ctx);
    return 0;
}

void exec_python_code(char* python_code) {
    // compile
    char* js_code = compile_py2js(python_code);

    // exec
    exit_status = exec_js_code(js_code);

    // cleanup
    if (js_code) {
        free(js_code);
    }
}

void on_read(uv_fs_t* req) {
    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
    } else if (req->result == 0) {
        uv_fs_t close_req;
        // synchronous
        uv_fs_close(req->loop, &close_req, open_req.result, NULL);
    } else if (req->result > 0) {
        char* python_code = iov.base;
        exec_python_code(python_code);
    }
}

void on_open(uv_fs_t* req) {
    // The request passed to the callback is the same as the one the call setup
    // function was passed.
    assert(req == &open_req);
    
    if (req->result >= 0) {
        iov = uv_buf_init(buffer, sizeof(buffer));
        uv_fs_read(req->loop, &read_req, req->result, &iov, 1, -1, on_read);
    } else {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
    }
}

int main(int argc, char** argv) {
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    // open file
    uv_fs_open(loop, &open_req, argv[1], O_RDONLY, 0, on_open);

    // run event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // cleanup
    uv_fs_req_cleanup(&open_req);
    uv_fs_req_cleanup(&read_req);
    uv_loop_close(loop);
    free(loop);
    return exit_status;
}
