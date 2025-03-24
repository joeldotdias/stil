#include "arena.h"
#include "lexer.h"
#include "parser.h"
#include <time.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        /* stil_fatal("Usage: stil <filename>"); */
        stil_warn("No file arg provided. Using sample file");
    }

    /* const char *filepath = argc < 2 ? "testdata/class_method.st" : argv[1];
     */
    const char *filepath = argc < 2 ? "testdata/simple_program.st" : argv[1];

    Arena arena = arena_init(64 * 1024 * 1024);
    /* Lexer *lexer = lexer_init(filepath, &arena); */
    Lexer *lexer = lexer_init(filepath);
    Parser *parser = parser_init(lexer);
    /* ASTNode *root = parse(parser); */
    clock_t start = clock();
    CompilationUnit *comp_unit = parse_compilation_unit(parser);

    /* Token *t = lexer_next_tok(lexer, &arena); */
    /* Token *t = lexer_next_tok(lexer);
    while(t) {
        stil_info("%s", tok_dbg(t));
        // t = lexer_next_tok(lexer, &arena);
        t = lexer_next_tok(lexer);
    } */

    if(lexer->n_errors > 0) {
        stil_fatal("Couldn't compile due to %d errors.", lexer->n_errors);
    }
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    comp_unit_dump(comp_unit);
    /* ast_dump(root); */
    printf("Execution time: %f seconds\n", time_spent);

    arena_deinit(&arena);

    return 0;
}
