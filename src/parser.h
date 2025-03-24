#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct _Parser {
    Lexer *lexer;
    Token *curr_token;
    Token *peeked;
} Parser;

typedef struct _Chunk {
} Chunk;

typedef struct _Class {
} Class;

// Parser *init_parser(Lexer *lexer, Arena *arena);
Parser *parser_init(Lexer *lexer);
CompilationUnit *parse_compilation_unit(Parser *parser);
ASTNode *parse(Parser *parser);

#endif
