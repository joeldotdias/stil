#include "arena.h"
#include "lexer.h"
#include <time.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        /* stil_fatal("Usage: stil <filename>"); */
        stil_warn("No file arg provided. Using sample file");
    }

    const char *filepath = argc < 2 ? "testdata/class_method.st" : argv[1];

    Arena arena = arena_init(64 * 1024 * 1024);
    Lexer *lexer = lexer_init(filepath, &arena);
    clock_t start = clock();
    Token *t = lexer_next_tok(lexer, &arena);
    while(t) {
        token_show(t);
        t = lexer_next_tok(lexer, &arena);
    }
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Execution time: %f seconds\n", time_spent);

    arena_deinit(&arena);

    return 0;
}
