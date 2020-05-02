#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <uv.h>
#include <duktape.h>
#include <duk_console.h>
#include <duk_module_node.h>
#include "pynogil.h"

#define DEBUG
static int exit_status;
static duk_idx_t uv_handle_idx;

duk_ret_t cb_resolve_module(duk_context *ctx) {
    /*
     *  Entry stack: [ requested_id parent_id ]
     */
    const char *requested_id = duk_get_string(ctx, 0);
    const char *parent_id = duk_get_string(ctx, 1);  /* calling module */
    const char *resolved_id;

    /* Arrive at the canonical module ID somehow. */
    duk_push_sprintf(ctx, "%s.js", requested_id);
    resolved_id = duk_get_string(ctx, -1);

    #ifdef DEBUG
    printf("DEBUG resolve_cb: requested_id: '%s', parent_id: '%s', resolved_id: '%s'\n", requested_id, parent_id, resolved_id);
    #endif

    return 1;  /*nrets*/
}

duk_ret_t cb_load_module(duk_context *ctx) {
    /*
     *  Entry stack: [ resolved_id exports module ]
     */
    const char *module_id;
    const char *filename;
    char *module_source = NULL;

    // get uv_handle from global namespace
    // FIXME: needs to be more secure
    (void) duk_get_global_string(ctx, "__uv_handle");
    const char *__uv_handle = duk_get_string(ctx, -1);
    void *_uv_handle_p;
    sscanf(__uv_handle, "%p", &_uv_handle_p);
    uv_handle_t *uv_handle = _uv_handle_p;
    
    #ifdef DEBUG
    printf("DEBUG cb_load_module uv_handle: %p\n", uv_handle);
    #endif

    uv_loop_t *loop = uv_handle_get_loop(uv_handle);

    module_id = duk_require_string(ctx, 0);
    duk_get_prop_string(ctx, 2, "filename");
    filename = duk_require_string(ctx, -1);

    #ifdef DEBUG
    printf("DEBUG load_cb: module_id:'%s', filename:'%s'\n", module_id, filename);
    #endif

    uv_buf_t buf = read_from_path(loop, module_id);
    module_source = buf.base;

    if (!module_source) {
        (void) duk_type_error(ctx, "cannot find module: '%s'", module_id);
        goto cleanup;
    }

    // FIXME: what happens with module_source memory allocated ?!
    // duk_push_string(ctx, module_source);
    duk_push_lstring(ctx, buf.base, buf.len);

    cleanup:
        ;

    return 1;  /*nrets*/
}

char *compile_py2js(uv_handle_t *uv_handle, char *python_code) {
    #ifdef DEBUG
    printf("DEBUG python_code:\n");
    printf("%s", python_code);
    #endif

    return NULL;
}

static void my_fatal(void *udata, const char *msg) {
    (void) udata;  /* ignored in this case, silence warning */

    /* Note that 'msg' may be NULL. */
    fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
    fflush(stderr);
    // abort();
}

int exec_js_code(uv_handle_t *uv_handle, char *js_code) {
    // init
    // duk_context *ctx = duk_create_heap_default();
    duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, my_fatal);
    
    if (!ctx) {
        printf("error, could not create duktape context\n");
        return 1; // sys exit
    }

    /* 
     * modeule-node
     * After initializing the Duktape heap or when creating a new
     * thread with a new global environment:
     */
    duk_push_object(ctx);
    duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "resolve");
    duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "load");
    duk_module_node_init(ctx);

    /*
     * console
     */
    duk_console_init(ctx, DUK_CONSOLE_PROXY_WRAPPER /*flags*/);

    // put uv_handle in global namespace
    // FIXME: needs to be more secure
    duk_push_sprintf(ctx, "%p", uv_handle);
    (void) duk_put_global_string(ctx, "__uv_handle");
    
    #ifdef DEBUG
    printf("DEBUG exec_js_code uv_handle: %p\n", uv_handle);
    #endif

    /*
     * compile python to javascript
     */
    duk_eval_string(ctx, "const r = 10; console.log(r); r;");
    duk_eval_string(ctx, "const a = require('./a');");
    duk_eval_string(ctx, "console.log(a);");
    duk_eval_string(ctx, "const lodash = require('./lodash.min');");
    duk_eval_string(ctx, "console.log(lodash.range(10));");
    duk_eval_string(ctx, "console.log(Number.isInteger(1));");

    // duk_eval_string(ctx,
    //     "var __BRYTHON__ = {};"
    //     "__BRYTHON__.brython = null;"
    //     "__BRYTHON__.isNone = true;"
    // );
    
    // duk_eval_string(ctx, "$compile_python = function(module_contents,module) {};");
    // duk_eval_string(ctx,
    //     "global = {};"
    //     "document = {};"
    //     "document.getElementsByTagName = function () { return [{src: ''}] };"
    //     "window = {};"
    //     "window.location = {href: ''};"
    //     "window.navigator = {};"
    //     "window.confirm = function () { return true; };"
    //     "window.console = console;"
    //     "document.$py_src = {};"
    //     "document.$debug = 0;"
    
    //     "self = {};"
    //     "__BRYTHON__ = {};"
    //     "__BRYTHON__.$py_module_path = {};"
    //     "__BRYTHON__.$py_module_path['__main__'] = './';"
    //     "__BRYTHON__.$py_module_alias = {};"
    //     "__BRYTHON__.$py_next_hash = -Math.pow(2,53);"
    //     "__BRYTHON__.exception_stack = [];"
    //     "__BRYTHON__.scope = {};"
    //     "__BRYTHON__.modules = {};"
    //     "__BRYTHON__.isNode = true;"
    
    //     "__BRYTHON__.$py_module_path = __BRYTHON__.$py_module_path || {};"
    //     "__BRYTHON__.$py_module_alias = __BRYTHON__.$py_module_alias || {};"
    //     "__BRYTHON__.exception_stack = __BRYTHON__.exception_stack || [];"
    //     "__BRYTHON__.scope = __BRYTHON__.scope || {};"
    //     "__BRYTHON__.imported = __BRYTHON__.imported || {};"
    //     "__BRYTHON__.modules = __BRYTHON__.modules || {};"
    //     // "__BRYTHON__.compile_python = $compile_python;"
    
    //     "__BRYTHON__.debug = 0;"
    //     "__BRYTHON__.$options = {};"
    //     "__BRYTHON__.$options.debug = 0;"
    // );
    // duk_eval_string(ctx, "const py2js = require('./brython/www/src/py2js');");
    // duk_eval_string(ctx, "console.log(py2js);");
    // duk_eval_string(ctx, "console.log(__BRYTHON__);");
    // duk_eval_string(ctx, "var root = __BRYTHON__.py2js('print(123)', '__main__');");
    // duk_eval_string(ctx, "console.log(root);");
    // duk_eval_string(ctx, "const brython = require('./brython_dist_babel');");
    // duk_eval_string(ctx, "console.log(brython);");
    
    /* pop eval result */
    duk_pop(ctx);

    //  cleanup
    duk_destroy_heap(ctx);
    return 0; // sys exit
}

void exec_python_code(uv_handle_t *uv_handle, uv_buf_t buf) {
    // compile
    char *python_code = buf.base;
    char *js_code = compile_py2js(uv_handle, python_code);

    // exec
    exit_status = exec_js_code(uv_handle, js_code);

    // cleanup:
    if (js_code) free(js_code);
}

uv_buf_t read_from_path(uv_loop_t *loop, const char *path) {
    char *buffer = NULL;

    // size of main file
    uv_fs_t stat_req;
    uint64_t size;
    uv_fs_stat(loop, &stat_req, path, NULL);

    if (stat_req.result < 0) {
        printf("error, can't open file '%s' (stat)\n", path);
        goto cleanup;
    }

    size = stat_req.statbuf.st_size;

    #ifdef DEBUG
    printf("DEBUG size: %lu\n", size);
    #endif

    // open main file
    uv_fs_t open_req;
    uv_fs_open(loop, &open_req, path, O_RDONLY, 0, NULL); // sync

    if (open_req.result < 0) {
        printf("error, can't open file '%s' (open)\n", path);
        goto cleanup;
    }

    // read main file
    uv_fs_t read_req;
    buffer = (char*) calloc(size, 1);
    uv_buf_t buf = uv_buf_init(buffer, size);
    uv_fs_read(loop, &read_req, open_req.result, &buf, 1, -1, NULL);

    if (read_req.result < 0) {
        printf("error, can't read file '%s'\n", path);
        goto cleanup;
    }

    // close main file
    uv_fs_t close_req;
    uv_fs_close(loop, &close_req, open_req.result, NULL);

    if (close_req.result < 0) {
        printf("error, can't close file '%s'\n", path);
        goto cleanup;
    }

    cleanup:
        if (uv_fs_get_type(&stat_req) == UV_FS_STAT) uv_fs_req_cleanup(&stat_req);
        if (uv_fs_get_type(&open_req) == UV_FS_OPEN) uv_fs_req_cleanup(&open_req);
        if (uv_fs_get_type(&read_req) == UV_FS_READ) uv_fs_req_cleanup(&read_req);
        if (uv_fs_get_type(&close_req) == UV_FS_CLOSE) uv_fs_req_cleanup(&close_req);

    // make sure to free buffer when not needed anymore
    return buf;
}

void async_exec_python_code(uv_async_t *async) {
    uv_handle_t *handle = (uv_handle_t*) async;
    uv_loop_t *loop = uv_handle_get_loop(handle);

    // main path
    const char *main_path = (const char*)uv_handle_get_data(handle);
    
    #ifdef DEBUG
    printf("DEBUG main_path: '%s'\n", main_path);
    #endif

    // read main file
    uv_buf_t buf = read_from_path(loop, main_path);
    char *python_code = buf.base;

    if (!python_code) {
        printf("error, can't open/read file '%s'\n", main_path);
        goto cleanup;
    }
    
    // exec main file
    exec_python_code(handle, buf);

    cleanup:
        if (python_code) free(python_code);
        uv_close(handle, NULL);
}

int main(int argc, char **argv) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    // exec python code
    uv_async_t exec_python_code_handle;
    uv_async_init(loop, &exec_python_code_handle, async_exec_python_code);
    uv_handle_set_data((uv_handle_t*) &exec_python_code_handle, (void*) argv[1]);
    uv_async_send(&exec_python_code_handle);

    // run event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // cleanup
    uv_loop_close(loop);
    free(loop);
    return exit_status;
}
