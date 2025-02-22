#include "lexer.h"
#include <time.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        /* stil_fatal("Usage: stil <filename>"); */
        stil_warn("No file arg provided. Using sample file");
    }

    const char *filepath = argc < 2 ? "testdata/class_method.st" : argv[1];

    Lexer *lexer = lexer_init(filepath);
    clock_t start = clock();
    Token *t = lexer_next_tok(lexer);
    while(t) {
        /* token_show(t); */
        t = lexer_next_tok(lexer);
    }
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Execution time: %f seconds\n", time_spent);

    return 0;
}
