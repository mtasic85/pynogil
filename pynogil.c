#include "pynogil.h"

static duk_ret_t native_print(duk_context *ctx) {
  printf("%s", duk_to_string(ctx, 0));
  return 0;  /* no return value (= undefined) */
}


static void _duk_fatal(void *udata, const char *msg) {
    (void) udata;  /* ignored in this case, silence warning */

    /* Note that 'msg' may be NULL. */
    fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
    fflush(stderr);
    abort();
}

duk_ret_t _cb_duk_resolve_module(duk_context *duk_ctx) {
    /*
     *  Entry stack: [ requested_id parent_id ]
     */
    const char *requested_id = duk_get_string(duk_ctx, 0);
    const char *parent_id = duk_get_string(duk_ctx, 1);  /* calling module */
    const char *resolved_id;

    /* Arrive at the canonical module ID somehow. */
    size_t requested_id_len = strlen(requested_id);
    
    // FIXME: check requested_id ?!
    if (strncmp((requested_id + requested_id_len - 3), ".js", 3) == 0) {
        // PYNG_LOG_DEBUG("with .js");
        duk_push_sprintf(duk_ctx, "%s", requested_id);
    } else {
        // PYNG_LOG_DEBUG("without .js");
        duk_push_sprintf(duk_ctx, "%s.js", requested_id);
    }

    resolved_id = duk_get_string(duk_ctx, -1);
    PYNG_LOG_DEBUG("_cb_duk_resolve_module requested_id: '%s', parent_id: '%s', resolved_id: '%s'", requested_id, parent_id, resolved_id);
    return 1;  /*nrets*/
}

duk_ret_t _cb_duk_load_module(duk_context *duk_ctx) {
    /*
     *  Entry stack: [ resolved_id exports module ]
     */
    const char *module_id;
    const char *filename;
    char *module_source = NULL;

    // get pyng_ctx from global namespace
    // FIXME: needs to be more secure
    (void) duk_get_global_string(duk_ctx, "__pyng_ctx");
    const char *_pyng_ctx_s = duk_get_string(duk_ctx, -1);
    void *_pyng_ctx_p;
    sscanf(_pyng_ctx_s, "%p", &_pyng_ctx_p);
    pyng_ctx_t *ctx = (pyng_ctx_t*) _pyng_ctx_p;
    PYNG_LOG_DEBUG("_cb_duk_load_module __pyng_ctx: %p", ctx);

    module_id = duk_require_string(duk_ctx, 0);
    duk_get_prop_string(duk_ctx, 2, "filename");
    filename = duk_require_string(duk_ctx, -1);
    PYNG_LOG_DEBUG("_cb_duk_load_module module_id: '%s', filename: '%s'", module_id, filename);
    
    // read file to buf
    uv_buf_t buf = pyng_ctx_read_file(ctx, module_id);

    if (buf.base == NULL) {
        (void) duk_type_error(duk_ctx, "cannot find module: '%s'", module_id);
        goto cleanup;
    }

    // FIXME: what happens with module_source memory allocated ?!
    duk_push_lstring(duk_ctx, buf.base, buf.len);

    cleanup:
        ;

    return 1;  /*nrets*/
}

/*
 * pyng_ctx_*
 */
pyng_ctx_t *pyng_ctx_new(void) {
    // ctx
    pyng_ctx_t *ctx = NULL;
    
    // uv_loop
    uv_loop_t *uv_loop = (uv_loop_t*) malloc(sizeof(uv_loop_t));

    if (!uv_loop) {
        PYNG_ERROR("could not create uv_loop");
        return NULL;
    }

    PYNG_LOG_DEBUG("uv_loop created %p", uv_loop);
    uv_loop_init(uv_loop);

    // duk_ctx
    // duk_context *duk_ctx = duk_create_heap_default();
    duk_context *duk_ctx = duk_create_heap(NULL, NULL, NULL, NULL, _duk_fatal);

    if (!duk_ctx) {
        PYNG_ERROR("could not create duk_ctx");
        _pyng_ctx_del_uv_loop(ctx);
        return NULL;
    }

    /*
     * extras
     */
    duk_push_c_function(duk_ctx, native_print, 1 /*nargs*/);
    duk_put_global_string(duk_ctx, "_native_print");

    /* 
     * modeule-node
     * After initializing the Duktape heap or when creating a new
     * thread with a new global environment:
     */
    duk_push_object(duk_ctx);
    duk_push_c_function(duk_ctx, _cb_duk_resolve_module, DUK_VARARGS);
    duk_put_prop_string(duk_ctx, -2, "resolve");
    duk_push_c_function(duk_ctx, _cb_duk_load_module, DUK_VARARGS);
    duk_put_prop_string(duk_ctx, -2, "load");
    duk_module_node_init(duk_ctx);

    /*
     * console
     */
    duk_console_init(duk_ctx, DUK_CONSOLE_PROXY_WRAPPER /*flags*/);

    /*
     * poll
     */
    poll_register(duk_ctx);

    /*
     * pyng_ctx
     */
    ctx = (pyng_ctx_t*) malloc(sizeof(pyng_ctx_t));
    // put pyng_ctx in global namespace
    // FIXME: needs to be more secure
    duk_push_sprintf(duk_ctx, "%p", ctx);
    (void) duk_put_global_string(duk_ctx, "__pyng_ctx");
    PYNG_LOG_DEBUG("pyng_ctx_new __pyng_ctx %p", ctx);
    PYNG_LOG_DEBUG("duk_ctx created %p", duk_ctx);

    // ctx
    ctx->uv_loop = uv_loop;
    ctx->duk_ctx = duk_ctx;
    return ctx;
}

void _pyng_ctx_del_uv_loop(pyng_ctx_t *ctx) {
    PYNG_LOG_DEBUG("uv_loop destroyed %p", ctx->uv_loop);
    uv_loop_close(ctx->uv_loop);
    free(ctx->uv_loop);
    ctx->uv_loop = NULL;
}

void _pyng_ctx_del_duk_ctx(pyng_ctx_t *ctx) {
    PYNG_LOG_DEBUG("duk_ctx destroyed %p", ctx->duk_ctx);
    duk_destroy_heap(ctx->duk_ctx);
    ctx->duk_ctx = NULL;
}

void pyng_ctx_del(pyng_ctx_t *ctx) {
    // duk_ctx
    _pyng_ctx_del_duk_ctx(ctx);

    // uv_loop
    _pyng_ctx_del_uv_loop(ctx);
    
    // ctx
    free(ctx);
}

int pyng_ctx_run_file(pyng_ctx_t *ctx, const char *path) {
    /*
     * return values:
     * == 0 - ok
     * < 0 - error
    */
    int result;

    // read file to buf
    PYNG_LOG_DEBUG("path: '%s'", path);
    uv_buf_t main_module_buf = pyng_ctx_read_file(ctx, path);
    
    if (main_module_buf.base == NULL) {
        PYNG_ERROR("can't open main module file '%s'", path);
        return -1;
    }

    // eval buf
    result = pyng_ctx_eval_buf(ctx, main_module_buf);

    // The user is responsible for freeing base after the uv_buf_t is done.
    free(main_module_buf.base);
    return result;
}

uv_buf_t pyng_ctx_read_file(pyng_ctx_t *ctx, const char *path) {
    uv_buf_t buf; // returned as result
    buf.base = NULL;
    buf.len = 0;
    
    // size of main file
    uv_fs_t stat_req;
    uint64_t size;
    uv_fs_stat(ctx->uv_loop, &stat_req, path, NULL);

    if (stat_req.result < 0) {
        PYNG_LOG_ERROR("can't open file '%s' (stat)", path);
        goto cleanup;
    }

    size = stat_req.statbuf.st_size;
    PYNG_LOG_DEBUG("size: %lu", size);

    // open main file
    uv_fs_t open_req;
    uv_fs_open(ctx->uv_loop, &open_req, path, O_RDONLY, 0, NULL); // sync

    if (open_req.result < 0) {
        PYNG_LOG_ERROR("can't open file '%s' (open)", path);
        goto cleanup;
    }

    // read main file
    uv_fs_t read_req;
    // The user is responsible for freeing base after the uv_buf_t is done. Return struct passed by value.
    char *buffer = (char*) calloc(size, 1);
    buf = uv_buf_init(buffer, size);
    uv_fs_read(ctx->uv_loop, &read_req, open_req.result, &buf, 1, -1, NULL);

    if (read_req.result < 0) {
        PYNG_LOG_ERROR("can't read file '%s'", path);
        goto cleanup;
    }

    // close main file
    uv_fs_t close_req;
    uv_fs_close(ctx->uv_loop, &close_req, open_req.result, NULL);

    if (close_req.result < 0) {
        PYNG_LOG_ERROR("can't close file '%s'", path);
        goto cleanup;
    }

    cleanup:
        if (uv_fs_get_type(&stat_req) == UV_FS_STAT)
            uv_fs_req_cleanup(&stat_req);
        
        if (uv_fs_get_type(&open_req) == UV_FS_OPEN)
            uv_fs_req_cleanup(&open_req);
        
        if (uv_fs_get_type(&read_req) == UV_FS_READ)
            uv_fs_req_cleanup(&read_req);
        
        if (uv_fs_get_type(&close_req) == UV_FS_CLOSE)
            uv_fs_req_cleanup(&close_req);

    // make sure to free buffer when not needed anymore
    return buf;
}

int pyng_ctx_eval_buf(pyng_ctx_t *ctx, uv_buf_t buf) {
    duk_context *duk_ctx = ctx->duk_ctx;

    /*
     * compile python to javascript
     */
    // duk_eval_string(duk_ctx, "const r = 10; console.log(r); r;");
    // duk_eval_string(duk_ctx, "const a = require('./a.js');");
    // duk_eval_string(duk_ctx, "console.log(a);");
    
    // duk_eval_string(duk_ctx, "const lodash = require('./lodash.js');");
    // duk_eval_string(duk_ctx, "console.log(lodash.range(10));");
    // duk_eval_string(duk_ctx, "console.log(Number.isInteger(1));");
    
    // duk_eval_string(duk_ctx, "const eventloop = require('./dummy_eventloop.js');");
    duk_eval_string(duk_ctx, "const eventloop = require('./ecma_eventloop.js');");
    duk_eval_string(duk_ctx, "const setTimeout = eventloop.setTimeout;");
    duk_eval_string(duk_ctx, "const clearTimeout = eventloop.clearTimeout;");
    duk_eval_string(duk_ctx, "const setInterval = eventloop.setInterval;");
    duk_eval_string(duk_ctx, "const clearInterval = eventloop.clearInterval;");
        
    duk_eval_string(duk_ctx, "const micropython = require('./micropython.js');");
    duk_eval_string(duk_ctx, "EventLoop.run();");

    duk_eval_string(duk_ctx, "micropython._mp_js_init(64 * 1024);");
    // duk_eval_string(duk_ctx, "micropython._mp_js_do_str(\"print('hello world')\n\");");
    duk_eval_string(duk_ctx, "micropython._mp_js_do_str('1 + 2');");
    // duk_eval_string(duk_ctx, "micropython._mp_js_do_str('print(1 + 2)');");
    // duk_eval_string(duk_ctx, "EventLoop.run();");
    
    /* pop eval result */
    duk_pop(duk_ctx);
    return 0;
}
