#include "pynogil.h"

int main(int argc, char **argv) {
    pyng_ctx_t *ctx = pyng_ctx_new();
    int result = pyng_ctx_run_file(ctx, argv[1]);
    pyng_ctx_del(ctx);
    return 0;
}
