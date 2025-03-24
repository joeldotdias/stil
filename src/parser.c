#include "parser.h"
#include <string.h>

#define FAILED_EXPECTATION(expected)                                           \
    stil_fatal("EXPECTED %s | RECEIVED %s", expected,                          \
               tok_dbg(parser->curr_token))

/* helpers */
static Token *consume_token_and_clone(Parser *parser, TokenKind expected);
static bool consume_token(Parser *parser, TokenKind expected);
static ASTNode *str_from_ident(Token *ident);
static bool fail_tok(Token *token);
static void parser_advance(Parser *parser);

/* Parser *parser_init(Lexer *lexer, Arena *arena) { */
Parser *parser_init(Lexer *lexer) {
    /* Parser *parser = arena_alloc(arena, sizeof *parser); */
    Parser *parser = stil_malloc(sizeof *parser);
    parser->lexer = lexer;
    /* parser->curr_token = lexer_next_tok(lexer, arena); */
    parser->curr_token = lexer_next_tok(lexer);
    parser->peeked = lexer_next_tok(lexer);
    /* parser->peeked = lexer_next_tok(lexer, arena); */

    return parser;
}

static inline ASTNode *make_node(NodeKind kind) {
    ASTNode *node = stil_malloc(sizeof *node);
    node->kind = kind;
    return node;
}

TypeDecl type_from_token(Token *token) {
    switch(token->kind) {
        case TOKEN_KEYWORD_STRING:
            return TYPE_STRING;
        case TOKEN_KEYWORD_INT:
            return TYPE_INT;
        case TOKEN_KEYWORD_REAL:
            return TYPE_REAL;
        default:
            return NO_TYPE;
    }
}

TokenKind fail_for_unit(StUnitType ty) {
    switch(ty) {
        case STUNIT_PROGRAM:
            return TOKEN_KEYWORD_END_PROGRAM;
        case STUNIT_ACTION:
            return TOKEN_KEYWORD_END_ACTION;
        case STUNIT_CLASS:
            return TOKEN_KEYWORD_END_CLASS;
        case NO_STUNIT:
            stil_warn("Not a st unit type");
            return -1;
            break;
    }
}

Symbol *parse_symbol(Parser *parser) {
    Token *ident = consume_token_and_clone(parser, TOKEN_IDENT);
    if(!ident) {
        stil_fatal("Expected IDENT got %s", tok_dbg(parser->curr_token));
    }
    Symbol *symbol = stil_malloc(sizeof *symbol);
    symbol->label = strdup(ident->string_val);
    return symbol;
}

ASTNode *parse_expr(Parser *parser) {
    ASTNode *node = NULL;

    switch(parser->curr_token->kind) {
        case TOKEN_LITERAL_INTEGER:
            {
                node = make_node(ASTNODE_INT_LITERAL);
                IntLiteral *int_literal = stil_malloc(sizeof *int_literal);
                int_literal->int_val = atoi(parser->curr_token->string_val);
                node->int_literal = *int_literal;
                break;
            }
        case TOKEN_LITERAL_REAL:
            {
                node = make_node(ASTNODE_REAL_LITERAL);
                RealLiteral *real_literal = stil_malloc(sizeof *real_literal);
                real_literal->real_val =
                    strtod(parser->curr_token->string_val, NULL);
                node->real_literal = *real_literal;
                break;
            }
        case TOKEN_LITERAL_STRING:
            {
                node = make_node(ASTNODE_STR_LITERAL);
                StrLiteral *str_literal = stil_malloc(sizeof *str_literal);
                str_literal->str_val = strdup(parser->curr_token->string_val);
                node->str_literal = *str_literal;
                break;
            }
        default:
            stil_warn("UNIMPLEMENTED");
            break;
    }

    if(node) {
        parser_advance(parser);
    }
    return node;
}

ASTNode *parse_var_decl(Parser *parser) {
    ASTNode *node = make_node(ASNTNODE_VAR_DECLARATION);
    VarDeclaration *var_decl = stil_malloc(sizeof *var_decl);
    var_decl->labels = symbol_list_init();
    var_decl->value = NULL;

    while(true) {
        Symbol *symbol = parse_symbol(parser);
        symbol_list_push(var_decl->labels, symbol);

        if(consume_token(parser, TOKEN_COLON)) {
            break;
        } else if(consume_token(parser, TOKEN_COMMA)) {
            continue;
        } else {
            stil_fatal("Expected ASSIGN or COMMA");
        }
    }

    var_decl->type = type_from_token(parser->curr_token);
    parser_advance(parser);

    if(consume_token(parser, TOKEN_ASSIGN)) {
        var_decl->value = parse_expr(parser);
    }

    if(!consume_token(parser, TOKEN_SEMICOLON)) {
        FAILED_EXPECTATION(TOKEN_SEMICOLON);
    }

    node->var_decl = *var_decl;
    return node;
}

VarBlockType block_type_from_token(Token *token) {
    switch(token->kind) {
        case TOKEN_KEYWORD_VAR:
            return VARBLOCK_LOCAL;
        case TOKEN_KEYWORD_VAR_TEMP:
            return VARBLOCK_TEMP;
        case TOKEN_KEYWORD_VAR_INPUT:
            return VARBLOCK_INPUT;
        case TOKEN_KEYWORD_VAR_GLOBAL:
            return VARBLOCK_GLOBAL;
        case TOKEN_KEYWORD_VAR_IN_OUT:
            return VARBLOCK_IN_OUT;
        default:
            return NO_VARBLOCK;
    }
}

ASTNode *parse_declaration_block(Parser *parser) {
    Token *block_type_tok = parser->curr_token;
    VarBlockType block_type = block_type_from_token(block_type_tok);
    parser_advance(parser);

    ASTNode *node = make_node(ASNTNODE_VAR_DECLARATION_BLOCK);
    VarBlock *var_block = stil_malloc(sizeof *var_block);
    var_block->block_type = block_type;
    var_block->var_decls = astnode_list_init();

    while(!consume_token(parser, TOKEN_KEYWORD_END_VAR)) {
        ASTNode *var_decl = parse_var_decl(parser);
        astnode_list_push(var_block->var_decls, var_decl);
    }

    node->var_block = *var_block;
    return node;
}

StUnitType unit_type_from_token(Token *token) {
    switch(token->kind) {
        case TOKEN_KEYWORD_PROGRAM:
            return STUNIT_PROGRAM;
        case TOKEN_KEYWORD_ACTION:
            return STUNIT_ACTION;
        case TOKEN_KEYWORD_CLASS:
            return STUNIT_CLASS;
        default:
            return NO_STUNIT;
    }
}

ASTNode *parse_statement(Parser *parser) {
    ASTNode *node = make_node(ASTNODE_ASSIGNMENT_STMT);
    Assignment *asgmt = stil_malloc(sizeof *asgmt);

    Symbol *name = parse_symbol(parser);
    asgmt->name = name;

    if(!consume_token(parser, TOKEN_ASSIGN)) {
        stil_fatal("Expected ASSIGN | Got %s", tok_dbg(parser->curr_token));
    }

    ASTNode *value = parse_expr(parser);
    asgmt->value = value;

    if(!consume_token(parser, TOKEN_SEMICOLON)) {
        stil_fatal("Expected SEMICOLON | Got %s", tok_dbg(parser->curr_token));
    }

    node->asgmt = *asgmt;
    return node;
}

STUnit *parse_st_unit(Parser *parser) {
    StUnitType unit_type = unit_type_from_token(parser->curr_token);
    parser_advance(parser);

    STUnit *unit = stil_malloc(sizeof *unit);
    unit->unit_type = unit_type;
    unit->variable_blocks = astnode_list_init();
    unit->statements = astnode_list_init();

    Symbol *unit_name = parse_symbol(parser);
    unit->name = unit_name;
    ASTNode *node = NULL;
    TokenKind end_tok = fail_for_unit(unit_type);

    while(!consume_token(parser, end_tok)) {
        if(parser->curr_token->kind == TOKEN_EOF) {
            stil_fatal("Reached end of file");
        }

        switch(parser->curr_token->kind) {
            case TOKEN_KEYWORD_VAR:
                node = parse_declaration_block(parser);
                astnode_list_push(unit->variable_blocks, node);
                break;

            case TOKEN_IDENT:
                node = parse_statement(parser);
                astnode_list_push(unit->statements, node);
                break;

            default:
                stil_fatal("Unexpected %s", tok_dbg(parser->curr_token));
        }
    }

    /* node->st_unit = *unit; */
    return unit;
}

ASTNode *parse(Parser *parser) { return parse_declaration_block(parser); }

CompilationUnit *parse_compilation_unit(Parser *parser) {
    CompilationUnit *comp_unit = stil_malloc(sizeof *comp_unit);
    /* comp_unit->st_units = astnode_list_init(); */
    comp_unit->st_units = st_unit_list_init();

    STUnit *unit = NULL;

    while(parser->curr_token->kind != TOKEN_EOF) {
        switch(parser->curr_token->kind) {
            case TOKEN_KEYWORD_PROGRAM:
            case TOKEN_KEYWORD_ACTION:
                /* node = parse_program(parser); */
                unit = parse_st_unit(parser);
                /* print_st_unit(unit, 0); */
                break;

            default:
                report(parser->lexer, parser->curr_token->offset, 1,
                       "Unexpected token");
        }

        /* astnode_list_push(comp_unit->s, node); */

        stil_info("parsed unit %zu", comp_unit->st_units->count);
        st_unit_list_push(comp_unit->st_units, unit);
        stil_info("pushed unit %zu", comp_unit->st_units->count);

        if(!parser->curr_token) {
            break;
        }
    }

    return comp_unit;
}

static Token *consume_token_and_clone(Parser *parser, TokenKind expected) {
    if(parser->curr_token->kind != expected) {
        return NULL;
    }

    Token *curr_token = stil_malloc(sizeof *curr_token);
    *curr_token = *(parser->curr_token);
    if(parser->curr_token->string_val) {
        curr_token->string_val = strdup(parser->curr_token->string_val);
    }

    parser_advance(parser);
    return curr_token;
}

static bool consume_token(Parser *parser, TokenKind expected) {
    if(parser->curr_token->kind != expected) {
        return false;
    }
    parser_advance(parser);
    return true;
}

static bool fail_tok(Token *token) {
    switch(token->kind) {
        case TOKEN_KEYWORD_END_CLASS:
        case TOKEN_KEYWORD_ELSE_IF:
        case TOKEN_KEYWORD_ELSE:
        case TOKEN_KEYWORD_END_CASE:
        case TOKEN_KEYWORD_END_ACTION:
        case TOKEN_KEYWORD_END_PROGRAM:
        case TOKEN_EOF:
            return true;
        default:
            return false;
    }
}

static void parser_advance(Parser *parser) {
    parser->curr_token = parser->peeked;
    parser->peeked = lexer_next_tok(parser->lexer);
}
