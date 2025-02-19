#include "lexer.h"
#include <ctype.h>
#include <string.h>

static inline void advance(Lexer *l);
static inline bool is_st_ident_ch(char c);
static TokenKind try_kw_or_ident(const char *lexeme);

Lexer *lexer_init(const char *filepath) {
    Lexer *lexer = stil_malloc(sizeof *lexer);

    FILE *fd = fopen(filepath, "r");
    if(!fd) {
        stil_fatal("Couldn't open file %s", filepath);
    }

    fseek(fd, 0, SEEK_END);
    size_t len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buffer = stil_malloc(len + 1);
    size_t bytes_read = fread(buffer, sizeof buffer[0], len, fd);
    if(bytes_read != len) {
        stil_fatal("Couldn't read from file %s", filepath);
    }
    buffer[bytes_read] = '\0';

    lexer->whole = strdup(buffer);
    lexer->rest = lexer->whole;
    lexer->pos = 0;
    lexer->source_len = bytes_read;
    fclose(fd);
    stil_free(buffer);

    return lexer;
}

static Token *make_sym_token(TokenKind kind, size_t offset) {
    Token *tok = stil_malloc(sizeof *tok);
    tok->kind = kind;
    tok->offset = offset;

    return tok;
}

static char peek_n(Lexer *l, size_t n) {
    if(l->pos + n >= l->source_len) {
        return '\0';
    }
    return l->rest[n - 1];
}

#define peek(lexer)        peek_n(lexer, 1)
#define just_tok(tok_kind) make_sym_token(tok_kind, curr_at);

Token *lexer_next_tok(Lexer *lexer) {
    Token *tok = NULL;
    Started started;

    while(lexer->pos < lexer->source_len - 1) {
        char curr = *lexer->rest;
        size_t curr_at = lexer->pos;
        const char *c_onwards = lexer->rest;
        advance(lexer);

        started = ST_None;
        if(isspace(curr)) {
            continue;
        }

        switch(curr) {
            case ':':
                if(peek(lexer) == '=') {
                    advance(lexer);
                    return just_tok(TOKEN_KEYWORD_ASSIGN)
                } else {
                    return just_tok(TOKEN_KEYWORD_COLON);
                }
            case ';':
                return just_tok(TOKEN_KEYWORD_SEMICOLON);
            case '(':
                return just_tok(TOKEN_KEYWORD_LPAREN);
            case ')':
                return just_tok(TOKEN_KEYWORD_RPAREN);
            case '[':
                return just_tok(TOKEN_KEYWORD_LSQUARE);
            case ']':
                return just_tok(TOKEN_KEYWORD_RSQUARE);
            case ',':
                return just_tok(TOKEN_KEYWORD_COMMA);
            case '.':
                if(peek(lexer) == '.') {
                    advance(lexer);
                    if(peek_n(lexer, 2) == '.') {
                        advance(lexer);
                        return just_tok(TOKEN_KEYWORD_DOT_DOT_DOT);
                    }
                    return just_tok(TOKEN_KEYWORD_DOT_DOT);
                } else {
                    return just_tok(TOKEN_KEYWORD_DOT);
                }

            case '+':
                return just_tok(TOKEN_OPERATOR_PLUS);
            case '-':
                return just_tok(TOKEN_OPERATOR_MINUS);
            case '*':
                if(peek(lexer) == '*') {
                    advance(lexer);
                    return just_tok(TOKEN_OPERATOR_EXPONENT);
                } else {
                    return just_tok(TOKEN_OPERATOR_MULTIPLICATION);
                }
            case '/':
                return just_tok(TOKEN_OPERATOR_DIVISION);
            case '=':
                return just_tok(TOKEN_OPERATOR_EQ);
            case '<':
                if(peek(lexer) == '>') {
                    advance(lexer);
                    return just_tok(TOKEN_OPERATOR_NOT_EQ);
                } else if(peek(lexer) == '=') {
                    advance(lexer);
                    return just_tok(TOKEN_OPERATOR_LESS_THAN_EQ);
                } else {
                    return just_tok(TOKEN_OPERATOR_LESS_THAN);
                }
            case '>':
                if(peek(lexer) == '=') {
                    advance(lexer);
                    return just_tok(TOKEN_OPERATOR_GREATER_THAN_EQ);
                } else {
                    return just_tok(TOKEN_OPERATOR_GREATER_THAN);
                }
            case '&':
                return just_tok(TOKEN_OPERATOR_AMP);
            case '^':
                return just_tok(TOKEN_OPERATOR_DEREF);

            case '\0':
                return just_tok(TOKEN_EOF);

            case '\'':
                started = ST_String;
                break;

            case '"':
                started = ST_WideString;
                break;

            case '@':
            case '{':
                started = ST_Ident_or_Keyword;
                break;

            default:
                if(isdigit(curr)) {
                    started = ST_Number;
                } else if(isalpha(curr)) {
                    started = ST_Ident_or_Keyword;
                } else {
                    tok = stil_malloc(sizeof *tok);
                    tok->kind = TOKEN_ILLEGAL;
                    tok->string_val = stil_malloc(2);
                    tok->string_val[0] = curr;
                    tok->string_val[1] = '\0';
                    return tok;
                }
        }

        switch(started) {
            case ST_String:
                {
                    const char *end = strchr(lexer->rest + 1, '\'');
                    if(end == NULL) {
                        stil_warn("Got ' but string literal is not properly closed");
                        continue;
                    }

                    size_t s_len = (end - lexer->rest);
                    tok = stil_malloc(sizeof *tok);
                    tok->kind = TOKEN_LITERAL_STRING;
                    tok->offset = curr_at;
                    tok->string_val = stil_malloc(s_len + 1);
                    memcpy(tok->string_val, c_onwards + 1, s_len);
                    tok->string_val[s_len] = '\0';

                    lexer->pos += s_len;
                    lexer->rest = end + 1;

                    return tok;
                }
            case ST_WideString:
                break;
            case ST_Number:
                break;
            case ST_Keyword:
                break;
            case ST_Ident_or_Keyword:
                {
                    size_t remaining_len = 0;
                    while(lexer->pos + remaining_len < lexer->source_len) {
                        char c = lexer->rest[remaining_len];
                        if(!is_st_ident_ch(c)) {
                            break;
                        }
                        remaining_len++;
                    }

                    size_t total_len = remaining_len + 1;
                    char *lexeme = stil_malloc(total_len + 1);
                    memcpy(lexeme, c_onwards, total_len);
                    lexeme[total_len] = '\0';

                    tok = stil_malloc(sizeof *tok);
                    tok->offset = curr_at;

                    tok->kind = try_kw_or_ident(lexeme);
                    if(tok->kind == TOKEN_IDENT) {
                        tok->string_val = strdup(lexeme);
                    }

                    /* tok->kind = TOKEN_IDENT;
                    tok->string_val = strdup(ident); */

                    lexer->pos += total_len - 1;
                    lexer->rest += remaining_len;
                    free(lexeme);
                    return tok;
                }
                break;
            case ST_FSlash:
                break;
            case ST_BlockComment:
                break;
            case ST_None:
                stil_info("%c", curr);
                break;
        }
    }

    return NULL;
}

static inline void advance(Lexer *l) {
    l->pos++;
    l->rest++;
}

static inline bool is_st_ident_ch(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '_';
}

static TokenKind try_kw_or_ident(const char *lexeme) {
    if(strcasecmp(lexeme, "PROGRAM") == 0)
        return TOKEN_KEYWORD_PROGRAM;
    if(strcasecmp(lexeme, "CLASS") == 0)
        return TOKEN_KEYWORD_CLASS;
    if(strcasecmp(lexeme, "END_CLASS") == 0 || strcasecmp(lexeme, "ENDCLASS") == 0)
        return TOKEN_KEYWORD_END_CLASS;
    if(strcasecmp(lexeme, "EXTENDS") == 0)
        return TOKEN_KEYWORD_EXTENDS;
    if(strcasecmp(lexeme, "IMPLEMENTS") == 0)
        return TOKEN_KEYWORD_IMPLEMENTS;
    if(strcasecmp(lexeme, "INTERFACE") == 0)
        return TOKEN_KEYWORD_INTERFACE;
    if(strcasecmp(lexeme, "END_INTERFACE") == 0 ||
       strcasecmp(lexeme, "ENDINTERFACE") == 0)
        return TOKEN_KEYWORD_END_INTERFACE;
    if(strcasecmp(lexeme, "PROPERTY") == 0)
        return TOKEN_KEYWORD_PROPERTY;
    if(strcasecmp(lexeme, "END_PROPERTY") == 0 || strcasecmp(lexeme, "ENDPROPERTY") == 0)
        return TOKEN_KEYWORD_END_PROPERTY;
    if(strcasecmp(lexeme, "VAR_INPUT") == 0 || strcasecmp(lexeme, "VARINPUT") == 0)
        return TOKEN_KEYWORD_VAR_INPUT;
    if(strcasecmp(lexeme, "VAR_OUTPUT") == 0 || strcasecmp(lexeme, "VAROUTPUT") == 0)
        return TOKEN_KEYWORD_VAR_OUTPUT;
    if(strcasecmp(lexeme, "VAR") == 0)
        return TOKEN_KEYWORD_VAR;
    if(strcasecmp(lexeme, "VAR_CONFIG") == 0)
        return TOKEN_KEYWORD_VAR_CONFIG;
    if(strcasecmp(lexeme, "ABSTRACT") == 0)
        return TOKEN_KEYWORD_ABSTRACT;
    if(strcasecmp(lexeme, "FINAL") == 0)
        return TOKEN_KEYWORD_FINAL;
    if(strcasecmp(lexeme, "METHOD") == 0)
        return TOKEN_KEYWORD_METHOD;
    if(strcasecmp(lexeme, "CONSTANT") == 0)
        return TOKEN_KEYWORD_CONSTANT;
    if(strcasecmp(lexeme, "RETAIN") == 0)
        return TOKEN_KEYWORD_RETAIN;
    if(strcasecmp(lexeme, "NON_RETAIN") == 0 || strcasecmp(lexeme, "NONRETAIN") == 0)
        return TOKEN_KEYWORD_NON_RETAIN;
    if(strcasecmp(lexeme, "VAR_TEMP") == 0 || strcasecmp(lexeme, "VARTEMP") == 0)
        return TOKEN_KEYWORD_VAR_TEMP;
    if(strcasecmp(lexeme, "END_METHOD") == 0 || strcasecmp(lexeme, "ENDMETHOD") == 0)
        return TOKEN_KEYWORD_END_METHOD;
    if(strcasecmp(lexeme, "PUBLIC") == 0)
        return TOKEN_KEYWORD_ACCESS_PUBLIC;
    if(strcasecmp(lexeme, "PRIVATE") == 0)
        return TOKEN_KEYWORD_ACCESS_PRIVATE;
    if(strcasecmp(lexeme, "INTERNAL") == 0)
        return TOKEN_KEYWORD_ACCESS_INTERNAL;
    if(strcasecmp(lexeme, "PROTECTED") == 0)
        return TOKEN_KEYWORD_ACCESS_PROTECTED;
    if(strcasecmp(lexeme, "OVERRIDE") == 0)
        return TOKEN_KEYWORD_OVERRIDE;
    if(strcasecmp(lexeme, "VAR_GLOBAL") == 0 || strcasecmp(lexeme, "VARGLOBAL") == 0)
        return TOKEN_KEYWORD_VAR_GLOBAL;
    if(strcasecmp(lexeme, "VAR_IN_OUT") == 0 || strcasecmp(lexeme, "VARINOUT") == 0)
        return TOKEN_KEYWORD_VAR_IN_OUT;
    if(strcasecmp(lexeme, "VAR_EXTERNAL") == 0)
        return TOKEN_KEYWORD_VAR_EXTERNAL;
    if(strcasecmp(lexeme, "END_VAR") == 0 || strcasecmp(lexeme, "ENDVAR") == 0)
        return TOKEN_KEYWORD_END_VAR;
    if(strcasecmp(lexeme, "END_PROGRAM") == 0 || strcasecmp(lexeme, "ENDPROGRAM") == 0)
        return TOKEN_KEYWORD_END_PROGRAM;
    if(strcasecmp(lexeme, "FUNCTION") == 0)
        return TOKEN_KEYWORD_FUNCTION;
    if(strcasecmp(lexeme, "END_FUNCTION") == 0 || strcasecmp(lexeme, "ENDFUNCTION") == 0)
        return TOKEN_KEYWORD_END_FUNCTION;
    if(strcasecmp(lexeme, "FUNCTION_BLOCK") == 0 ||
       strcasecmp(lexeme, "FUNCTIONBLOCK") == 0)
        return TOKEN_KEYWORD_FUNCTION_BLOCK;
    if(strcasecmp(lexeme, "END_FUNCTION_BLOCK") == 0 ||
       strcasecmp(lexeme, "ENDFUNCTIONBLOCK") == 0)
        return TOKEN_KEYWORD_END_FUNCTION_BLOCK;
    if(strcasecmp(lexeme, "TYPE") == 0)
        return TOKEN_KEYWORD_TYPE;
    if(strcasecmp(lexeme, "STRUCT") == 0)
        return TOKEN_KEYWORD_STRUCT;
    if(strcasecmp(lexeme, "END_TYPE") == 0 || strcasecmp(lexeme, "ENDTYPE") == 0)
        return TOKEN_KEYWORD_END_TYPE;
    if(strcasecmp(lexeme, "END_STRUCT") == 0 || strcasecmp(lexeme, "ENDSTRUCT") == 0)
        return TOKEN_KEYWORD_END_STRUCT;
    if(strcasecmp(lexeme, "ACTIONS") == 0)
        return TOKEN_KEYWORD_ACTIONS;
    if(strcasecmp(lexeme, "ACTION") == 0)
        return TOKEN_KEYWORD_ACTION;
    if(strcasecmp(lexeme, "END_ACTION") == 0 || strcasecmp(lexeme, "ENDACTION") == 0)
        return TOKEN_KEYWORD_END_ACTION;
    if(strcasecmp(lexeme, "END_ACTIONS") == 0 || strcasecmp(lexeme, "ENDACTIONS") == 0)
        return TOKEN_KEYWORD_END_ACTIONS;
    if(strcasecmp(lexeme, "IF") == 0)
        return TOKEN_KEYWORD_IF;
    if(strcasecmp(lexeme, "THEN") == 0)
        return TOKEN_KEYWORD_THEN;
    if(strcasecmp(lexeme, "ELSIF") == 0)
        return TOKEN_KEYWORD_ELSE_IF;
    if(strcasecmp(lexeme, "ELSE") == 0)
        return TOKEN_KEYWORD_ELSE;
    if(strcasecmp(lexeme, "END_IF") == 0 || strcasecmp(lexeme, "ENDIF") == 0)
        return TOKEN_KEYWORD_END_IF;
    if(strcasecmp(lexeme, "FOR") == 0)
        return TOKEN_KEYWORD_FOR;
    if(strcasecmp(lexeme, "TO") == 0)
        return TOKEN_KEYWORD_TO;
    if(strcasecmp(lexeme, "BY") == 0)
        return TOKEN_KEYWORD_BY;
    if(strcasecmp(lexeme, "DO") == 0)
        return TOKEN_KEYWORD_DO;
    if(strcasecmp(lexeme, "END_FOR") == 0 || strcasecmp(lexeme, "ENDFOR") == 0)
        return TOKEN_KEYWORD_END_FOR;
    if(strcasecmp(lexeme, "WHILE") == 0)
        return TOKEN_KEYWORD_WHILE;
    if(strcasecmp(lexeme, "END_WHILE") == 0 || strcasecmp(lexeme, "ENDWHILE") == 0)
        return TOKEN_KEYWORD_END_WHILE;
    if(strcasecmp(lexeme, "REPEAT") == 0)
        return TOKEN_KEYWORD_REPEAT;
    if(strcasecmp(lexeme, "UNTIL") == 0)
        return TOKEN_KEYWORD_UNTIL;
    if(strcasecmp(lexeme, "END_REPEAT") == 0 || strcasecmp(lexeme, "ENDREPEAT") == 0)
        return TOKEN_KEYWORD_END_REPEAT;
    if(strcasecmp(lexeme, "CASE") == 0)
        return TOKEN_KEYWORD_CASE;
    if(strcasecmp(lexeme, "RETURN") == 0)
        return TOKEN_KEYWORD_RETURN;
    if(strcasecmp(lexeme, "EXIT") == 0)
        return TOKEN_KEYWORD_EXIT;
    if(strcasecmp(lexeme, "CONTINUE") == 0)
        return TOKEN_KEYWORD_CONTINUE;
    if(strcasecmp(lexeme, "POINTER") == 0)
        return TOKEN_KEYWORD_POINTER;
    if(strcasecmp(lexeme, "REF_TO") == 0 || strcasecmp(lexeme, "REFTO") == 0)
        return TOKEN_KEYWORD_REFERENCE_TO;
    if(strcasecmp(lexeme, "ARRAY") == 0)
        return TOKEN_KEYWORD_ARRAY;
    if(strcasecmp(lexeme, "STRING") == 0)
        return TOKEN_KEYWORD_STRING;
    if(strcasecmp(lexeme, "WSTRING") == 0)
        return TOKEN_KEYWORD_WIDE_STRING;
    if(strcasecmp(lexeme, "OF") == 0)
        return TOKEN_KEYWORD_OF;
    if(strcasecmp(lexeme, "AT") == 0)
        return TOKEN_KEYWORD_AT;
    if(strcasecmp(lexeme, "END_CASE") == 0 || strcasecmp(lexeme, "ENDCASE") == 0)
        return TOKEN_KEYWORD_END_CASE;

    if(strcasecmp(lexeme, "MOD") == 0)
        return TOKEN_OPERATOR_MODULO;
    if(strcasecmp(lexeme, "AND") == 0)
        return TOKEN_OPERATOR_AND;
    if(strcasecmp(lexeme, "OR") == 0)
        return TOKEN_OPERATOR_OR;
    if(strcasecmp(lexeme, "XOR") == 0)
        return TOKEN_OPERATOR_XOR;
    if(strcasecmp(lexeme, "NOT") == 0)
        return TOKEN_OPERATOR_NOT;

    return TOKEN_IDENT;
}

void token_show(Token *token) {
    switch(token->kind) {
        case TOKEN_KEYWORD_PROGRAM:
            stil_info("PROGRAM");
            break;
        case TOKEN_KEYWORD_CLASS:
            stil_info("CLASS");
            break;
        case TOKEN_KEYWORD_END_CLASS:
            stil_info("END_CLASS");
            break;
        case TOKEN_KEYWORD_EXTENDS:
            stil_info("EXTENDS");
            break;
        case TOKEN_KEYWORD_IMPLEMENTS:
            stil_info("IMPLEMENTS");
            break;
        case TOKEN_KEYWORD_INTERFACE:
            stil_info("INTERFACE");
            break;
        case TOKEN_KEYWORD_END_INTERFACE:
            stil_info("END_INTERFACE");
            break;
        case TOKEN_KEYWORD_PROPERTY:
            stil_info("PROPERTY");
            break;
        case TOKEN_KEYWORD_END_PROPERTY:
            stil_info("END_PROPERTY");
            break;
        case TOKEN_KEYWORD_VAR_INPUT:
            stil_info("VAR_INPUT");
            break;
        case TOKEN_KEYWORD_VAR_OUTPUT:
            stil_info("VAR_OUTPUT");
            break;
        case TOKEN_KEYWORD_VAR:
            stil_info("VAR");
            break;
        case TOKEN_KEYWORD_VAR_CONFIG:
            stil_info("VAR_CONFIG");
            break;
        case TOKEN_KEYWORD_ABSTRACT:
            stil_info("ABSTRACT");
            break;
        case TOKEN_KEYWORD_FINAL:
            stil_info("FINAL");
            break;
        case TOKEN_KEYWORD_METHOD:
            stil_info("METHOD");
            break;
        case TOKEN_KEYWORD_CONSTANT:
            stil_info("CONSTANT");
            break;
        case TOKEN_KEYWORD_RETAIN:
            stil_info("RETAIN");
            break;
        case TOKEN_KEYWORD_NON_RETAIN:
            stil_info("NON_RETAIN");
            break;
        case TOKEN_KEYWORD_VAR_TEMP:
            stil_info("VAR_TEMP");
            break;
        case TOKEN_KEYWORD_END_METHOD:
            stil_info("END_METHOD");
            break;
        case TOKEN_KEYWORD_ACCESS_PUBLIC:
            stil_info("ACCESS_PUBLIC");
            break;
        case TOKEN_KEYWORD_ACCESS_PRIVATE:
            stil_info("ACCESS_PRIVATE");
            break;
        case TOKEN_KEYWORD_ACCESS_INTERNAL:
            stil_info("ACCESS_INTERNAL");
            break;
        case TOKEN_KEYWORD_ACCESS_PROTECTED:
            stil_info("ACCESS_PROTECTED");
            break;
        case TOKEN_KEYWORD_OVERRIDE:
            stil_info("OVERRIDE");
            break;
        case TOKEN_KEYWORD_VAR_GLOBAL:
            stil_info("VAR_GLOBAL");
            break;
        case TOKEN_KEYWORD_VAR_IN_OUT:
            stil_info("VAR_IN_OUT");
            break;
        case TOKEN_KEYWORD_VAR_EXTERNAL:
            stil_info("VAR_EXTERNAL");
            break;
        case TOKEN_KEYWORD_END_VAR:
            stil_info("END_VAR");
            break;
        case TOKEN_KEYWORD_END_PROGRAM:
            stil_info("END_PROGRAM");
            break;
        case TOKEN_KEYWORD_FUNCTION:
            stil_info("FUNCTION");
            break;
        case TOKEN_KEYWORD_END_FUNCTION:
            stil_info("END_FUNCTION");
            break;
        case TOKEN_KEYWORD_FUNCTION_BLOCK:
            stil_info("FUNCTION_BLOCK");
            break;
        case TOKEN_KEYWORD_END_FUNCTION_BLOCK:
            stil_info("END_FUNCTION_BLOCK");
            break;
        case TOKEN_KEYWORD_TYPE:
            stil_info("TYPE");
            break;
        case TOKEN_KEYWORD_STRUCT:
            stil_info("STRUCT");
            break;
        case TOKEN_KEYWORD_END_TYPE:
            stil_info("END_TYPE");
            break;
        case TOKEN_KEYWORD_END_STRUCT:
            stil_info("END_STRUCT");
            break;
        case TOKEN_KEYWORD_ACTIONS:
            stil_info("ACTIONS");
            break;
        case TOKEN_KEYWORD_ACTION:
            stil_info("ACTION");
            break;
        case TOKEN_KEYWORD_END_ACTION:
            stil_info("END_ACTION");
            break;
        case TOKEN_KEYWORD_END_ACTIONS:
            stil_info("END_ACTIONS");
            break;
        case TOKEN_KEYWORD_COLON:
            stil_info("COLON");
            break;
        case TOKEN_KEYWORD_SEMICOLON:
            stil_info("SEMICOLON");
            break;
        case TOKEN_KEYWORD_ASSIGN:
            stil_info("ASSIGN");
            break;
        case TOKEN_KEYWORD_OUTPUT_ASSIGNMENT:
            stil_info("OUTPUT_ASSIGNMENT");
            break;
        case TOKEN_KEYWORD_REFERENCE_ASSIGNMENT:
            stil_info("REFERENCE_ASSIGNMENT");
            break;
        case TOKEN_KEYWORD_LPAREN:
            stil_info("LPAREN");
            break;
        case TOKEN_KEYWORD_RPAREN:
            stil_info("RPAREN");
            break;
        case TOKEN_KEYWORD_LSQUARE:
            stil_info("LSQUARE");
            break;
        case TOKEN_KEYWORD_RSQUARE:
            stil_info("RSQUARE");
            break;
        case TOKEN_KEYWORD_COMMA:
            stil_info("COMMA");
            break;
        case TOKEN_KEYWORD_DOT_DOT_DOT:
            stil_info("DOT_DOT_DOT");
            break;
        case TOKEN_KEYWORD_DOT_DOT:
            stil_info("DOT_DOT");
            break;
        case TOKEN_KEYWORD_DOT:
            stil_info("DOT");
            break;
        case TOKEN_KEYWORD_IF:
            stil_info("IF");
            break;
        case TOKEN_KEYWORD_THEN:
            stil_info("THEN");
            break;
        case TOKEN_KEYWORD_ELSE_IF:
            stil_info("ELSE_IF");
            break;
        case TOKEN_KEYWORD_ELSE:
            stil_info("ELSE");
            break;
        case TOKEN_KEYWORD_END_IF:
            stil_info("END_IF");
            break;
        case TOKEN_KEYWORD_FOR:
            stil_info("FOR");
            break;
        case TOKEN_KEYWORD_TO:
            stil_info("TO");
            break;
        case TOKEN_KEYWORD_BY:
            stil_info("BY");
            break;
        case TOKEN_KEYWORD_DO:
            stil_info("DO");
            break;
        case TOKEN_KEYWORD_END_FOR:
            stil_info("END_FOR");
            break;
        case TOKEN_KEYWORD_WHILE:
            stil_info("WHILE");
            break;
        case TOKEN_KEYWORD_END_WHILE:
            stil_info("END_WHILE");
            break;
        case TOKEN_KEYWORD_REPEAT:
            stil_info("REPEAT");
            break;
        case TOKEN_KEYWORD_UNTIL:
            stil_info("UNTIL");
            break;
        case TOKEN_KEYWORD_END_REPEAT:
            stil_info("END_REPEAT");
            break;
        case TOKEN_KEYWORD_CASE:
            stil_info("CASE");
            break;
        case TOKEN_KEYWORD_RETURN:
            stil_info("RETURN");
            break;
        case TOKEN_KEYWORD_EXIT:
            stil_info("EXIT");
            break;
        case TOKEN_KEYWORD_CONTINUE:
            stil_info("CONTINUE");
            break;
        case TOKEN_KEYWORD_POINTER:
            stil_info("POINTER");
            break;
        case TOKEN_KEYWORD_REF:
            stil_info("REF");
            break;
        case TOKEN_KEYWORD_REFERENCE_TO:
            stil_info("REFERENCE_TO");
            break;
        case TOKEN_KEYWORD_ARRAY:
            stil_info("ARRAY");
            break;
        case TOKEN_KEYWORD_STRING:
            stil_info("STRING");
            break;
        case TOKEN_KEYWORD_WIDE_STRING:
            stil_info("WIDE_STRING");
            break;
        case TOKEN_KEYWORD_OF:
            stil_info("OF");
            break;
        case TOKEN_KEYWORD_AT:
            stil_info("AT");
            break;
        case TOKEN_KEYWORD_END_CASE:
            stil_info("END_CASE");
            break;

        case TOKEN_OPERATOR_PLUS:
            stil_info("PLUS");
            break;
        case TOKEN_OPERATOR_MINUS:
            stil_info("MINUS");
            break;
        case TOKEN_OPERATOR_MULTIPLICATION:
            stil_info("MULTIPLICATION");
            break;
        case TOKEN_OPERATOR_EXPONENT:
            stil_info("EXPONENT");
            break;
        case TOKEN_OPERATOR_DIVISION:
            stil_info("DIVISION");
            break;
        case TOKEN_OPERATOR_EQ:
            stil_info("EQ");
            break;
        case TOKEN_OPERATOR_NOT_EQ:
            stil_info("NOT_EQ");
            break;
        case TOKEN_OPERATOR_LESS_THAN:
            stil_info("LESS_THAN");
            break;
        case TOKEN_OPERATOR_GREATER_THAN:
            stil_info("GREATER_THAN");
            break;
        case TOKEN_OPERATOR_LESS_THAN_EQ:
            stil_info("LESS_THAN_EQ");
            break;
        case TOKEN_OPERATOR_GREATER_THAN_EQ:
            stil_info("GREATER_THAN_EQ");
            break;
        case TOKEN_OPERATOR_AMP:
            stil_info("AMP");
            break;
        case TOKEN_OPERATOR_DEREF:
            stil_info("DEREF");
            break;
        case TOKEN_OPERATOR_MODULO:
            stil_info("MODULO");
            break;
        case TOKEN_OPERATOR_AND:
            stil_info("AND");
            break;
        case TOKEN_OPERATOR_OR:
            stil_info("OR");
            break;
        case TOKEN_OPERATOR_XOR:
            stil_info("XOR");
            break;
        case TOKEN_OPERATOR_NOT:
            stil_info("NOT");
            break;

        case TOKEN_PROPERTY_EXTERNAL:
        case TOKEN_PROPERTY_BY_REF:
        case TOKEN_PROPERTY_CONSTANT:
        case TOKEN_PROPERTY_SIZED:
        case TOKEN_LITERAL_INTEGER_HEX:
        case TOKEN_LITERAL_INTEGER_OCT:
        case TOKEN_LITERAL_INTEGER_BIN:
        case TOKEN_LITERAL_INTEGER:
        case TOKEN_LITERAL_NULL:
        case TOKEN_LITERAL_TRUE:
        case TOKEN_LITERAL_FALSE:
        case TOKEN_LITERAL_DATE:
        case TOKEN_LITERAL_DATE_AND_TIME:
        case TOKEN_LITERAL_TIME_OF_DAY:
        case TOKEN_LITERAL_TIME:
        case TOKEN_DIRECT_ACCESS:
        case TOKEN_HARDWARE_ACCESS:
        case TOKEN_LITERAL_WIDE_STRING:
        case TOKEN_TYPE_CAST_PREFIX:
            stil_warn("UNIMPLEMENTED");
            break;

        case TOKEN_IDENT:
            stil_info("IDENT %s", token->string_val);
            break;
        case TOKEN_LITERAL_STRING:
            stil_info("STR LITERAL '%s'", token->string_val);
            break;
        case TOKEN_EOF:
            stil_info("EOF");
            break;
        case TOKEN_ILLEGAL:
            stil_warn("ILLEGAL %s", token->string_val);
            break;
    }
}
