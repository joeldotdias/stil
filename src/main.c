#include "lexer.h"

int main(int argc, char **argv) {
    if(argc < 2) {
        /* stil_fatal("Usage: stil <filename>"); */
        stil_warn("No file arg provided. Using sample file");
    }

    const char *filepath = argc < 2 ? "testdata/class_method.st" : argv[1];

    Lexer *lexer = lexer_init(filepath);
    Token *t = lexer_next_tok(lexer);
    while(t) {
        token_show(t);
        t = lexer_next_tok(lexer);
    }

    return 0;
}
