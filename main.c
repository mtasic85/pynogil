#include "pynogil.h"

int main(int argc, char **argv) {
    pyng_ctx_t *ctx = pyng_ctx_new();
    int result;
    
    if (argc > 1) {
        result = pyng_ctx_run_file(ctx, argv[1]);
    } else {
        result = pyng_ctx_repl(ctx);
    }

    pyng_ctx_del(ctx);
    return 0;
}
