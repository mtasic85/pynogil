#include "pynogil.h"

duk_ret_t _duk_print(duk_context *ctx) {
  printf("%s", duk_to_string(ctx, 0));
  return 0;  /* no return value (= undefined) */
}

void _duk_fatal(void *udata, const char *msg) {
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
    duk_push_c_function(duk_ctx, _duk_print, 1 /*nargs*/);
    duk_put_global_string(duk_ctx, "_duk_print");

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
    PYNG_LOG_DEBUG("python module path: '%s'", path);
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

int _pyng_ctx_setup_polyfills(pyng_ctx_t *ctx) {
    /* duktape: context */
    duk_context *duk_ctx = ctx->duk_ctx;

    /*
     * Uint8Array.copyWithin polyfill
     */
    duk_eval_string(duk_ctx, "require('./polyfills/Uint8Array.copyWithin.js');");

    /*
     * eventloop
     */
    duk_eval_string(duk_ctx, "const eventloop = require('./polyfills/ecma_eventloop.js');");
    duk_eval_string(duk_ctx, "const setTimeout = eventloop.setTimeout;");
    duk_eval_string(duk_ctx, "const clearTimeout = eventloop.clearTimeout;");
    duk_eval_string(duk_ctx, "const setInterval = eventloop.setInterval;");
    duk_eval_string(duk_ctx, "const clearInterval = eventloop.clearInterval;");

    /*
     * promise-polyfill
     */
    /* required for `promise-polyfill` */
    duk_eval_string(duk_ctx, "const self = this;");
    /* inserts `Promise` into `self` */
    duk_eval_string(duk_ctx, "require('./polyfills/promise-polyfill.js');");

    /*
     * base64 polyfill (atob, btoa)
     */
    duk_eval_string(duk_ctx, "require('./polyfills/base64.js');");
    
    /*
     * Node's Buffer polyfill
     */
    duk_eval_string(duk_ctx, "Buffer.alloc = function Buffer_alloc(size) { return new Buffer(size); };");

    /*
     * Node's process polyfill
     */
    duk_eval_string(duk_ctx, "const process = {};");
    duk_eval_string(duk_ctx, "process.argv = ['pynogil'];"); /* FIXME: pass propper argv */
    duk_eval_string(duk_ctx, "process.stdin = {};");
    duk_eval_string(duk_ctx, "process.stdin.fd = 0;");
    duk_eval_string(duk_ctx, "process.stdin.setRawMode = function setRawMode(v) {};");
    duk_eval_string(duk_ctx, "process.stdout = {};");
    duk_eval_string(duk_ctx, "process.stdout.on = function(eventName, cb) {};");
    duk_eval_string(duk_ctx, "process.stdout.write = function(data) { _duk_print(data); };");
    duk_eval_string(duk_ctx, "process.on = function(eventName, cb) {};");
    duk_eval_string(duk_ctx, "process.exit = function(exitCode) { process.exitCode = exitCode; };");
    duk_eval_string(duk_ctx, "process.versions = {};");
    duk_eval_string(duk_ctx, "process.versions.node = 'duktape';"); /* FIXME: pass duktape version */
    duk_eval_string(duk_ctx, "process.exitCode = 0;");

    return 0;
}

int _pyng_ctx_setup_micropython(pyng_ctx_t *ctx) {
    /* duktape: context */
    duk_context *duk_ctx = ctx->duk_ctx;

    /*
     * micropython
     */
    /* requred by `micropython` */
    duk_eval_string(duk_ctx, "var mp_js_init = null;");                             
    duk_eval_string(duk_ctx, "var mp_js_init_repl = null;");
    duk_eval_string(duk_ctx, "var mp_js_do_str = null;");
    duk_eval_string(duk_ctx, "var mp_js_process_char = null;");
    duk_eval_string(duk_ctx, "var mp_hal_get_interrupt_char = null;");
    duk_eval_string(duk_ctx, "var mp_keyboard_interrupt = null;");
    /* micropython` */
    duk_eval_string(duk_ctx, "require('./micropython/ports/javascript/build/micropython.js');");
    /* requred by `micropython` */
    duk_eval_string(duk_ctx, "EventLoop.run();");

    return 0;
}

int pyng_ctx_eval_buf(pyng_ctx_t *ctx, uv_buf_t buf) {
    duk_context *duk_ctx = ctx->duk_ctx;
    _pyng_ctx_setup_polyfills(ctx);
    _pyng_ctx_setup_micropython(ctx);

    /* create python code, and embed python string into javascript string to be evaluated */
    // char *pyeval_code_pre = "mp_js_do_str(atob('";
    // size_t pyeval_base64_code_len = 0;
    // char *pyeval_base64_code = base64_encode(buf.base, buf.len, &pyeval_base64_code_len);
    // char *pyeval_code_post = "'));";
    // size_t pyeval_code_len = strlen(pyeval_code_pre) + pyeval_base64_code_len + strlen(pyeval_code_post);
    // char *pyeval_code = (char*)calloc(pyeval_code_len, sizeof(char));
    // strncpy(pyeval_code, pyeval_code_pre, strlen(pyeval_code_pre));
    // strncpy(pyeval_code + strlen(pyeval_code_pre), pyeval_base64_code, pyeval_base64_code_len);
    // strncpy(pyeval_code + strlen(pyeval_code_pre) + pyeval_base64_code_len, pyeval_code_post, strlen(pyeval_code_post));
    
    /* evaluate python code */
    duk_eval_string(duk_ctx, "mp_js_init(1024 * 1024);");
    // duk_eval_lstring(duk_ctx, pyeval_code, pyeval_code_len);
    // duk_eval_string(duk_ctx, "EventLoop.run();");

    /* free python code */
    // free(pyeval_code);
    // free(pyeval_base64_code);
    
    /* duktape: pop eval result */
    duk_pop(duk_ctx);
    return 0;
}

int pyng_ctx_repl(pyng_ctx_t *ctx) {
    duk_context *duk_ctx = ctx->duk_ctx;
    _pyng_ctx_setup_polyfills(ctx);
    _pyng_ctx_setup_micropython(ctx);
    
    return 0;
}
