#include "pynogil.h"

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
    return 0;
}
