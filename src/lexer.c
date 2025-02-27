#include "lexer.h"
#include <ctype.h>
#include <string.h>

static void fillup_keywords(kw_ht *table);
static inline void advance(Lexer *l);
static inline bool is_st_ident_ch(char c);
static inline char *str_to_upper(char *s);

Lexer *lexer_init(const char *filepath, Arena *arena) {
    Lexer *lexer = arena_alloc(arena, sizeof *lexer);

    FILE *fd = fopen(filepath, "r");
    if(!fd) {
        stil_fatal("Couldn't open file %s", filepath);
    }

    fseek(fd, 0, SEEK_END);
    size_t len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buffer = arena_alloc(arena, len + 1);
    size_t bytes_read = fread(buffer, sizeof buffer[0], len, fd);
    if(bytes_read != len) {
        stil_fatal("Couldn't read from file %s", filepath);
    }
    buffer[bytes_read] = '\0';

    lexer->whole = strdup(buffer);
    lexer->rest = lexer->whole;
    lexer->kw_lookup = ht_init();
    fillup_keywords(lexer->kw_lookup);

    lexer->pos = 0;
    lexer->source_len = bytes_read;
    fclose(fd);

    return lexer;
}

static Token *make_sym_token(TokenKind kind, size_t offset, Arena *arena) {
    Token *tok = (Token *)arena_alloc(arena, sizeof *tok);
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
#define just_tok(tok_kind) make_sym_token(tok_kind, curr_at, arena);

Token *lexer_next_tok(Lexer *lexer, Arena *arena) {
    Token *tok = NULL;
    Started started = ST_None;

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
            case '#':
                started = ST_Ident_or_Keyword;
                break;

            default:
                if(isdigit(curr)) {
                    started = ST_Number;
                } else if(isalpha(curr)) {
                    started = ST_Ident_or_Keyword;
                } else {
                    tok = arena_alloc(arena, sizeof *tok);
                    tok->kind = TOKEN_ILLEGAL;
                    tok->string_val = arena_alloc(arena, 2);
                    tok->string_val[0] = curr;
                    tok->string_val[1] = '\0';
                    return tok;
                }
        }

        switch(started) {
            case ST_String:
                {
                    const char *end = strchr(lexer->rest + 1, '\'');
                    if(!end) {
                        stil_warn(
                            "Got ' but string literal is not properly closed");
                        continue;
                    }

                    size_t s_len = (end - lexer->rest);
                    tok = arena_alloc(arena, sizeof *tok);
                    tok->kind = TOKEN_LITERAL_STRING;
                    tok->offset = curr_at;
                    tok->string_val = arena_alloc(arena, s_len + 1);
                    memcpy(tok->string_val, c_onwards + 1, s_len);
                    tok->string_val[s_len] = '\0';

                    lexer->pos += s_len;
                    lexer->rest = end + 1;

                    return tok;
                }
            case ST_WideString:
                break;
            case ST_Number:
                {
                    bool got_dot = false;
                    const char *first_non_digit = c_onwards;
                    while(*first_non_digit &&
                          (isdigit((unsigned char)*first_non_digit) ||
                           (*first_non_digit == '.' && !got_dot))) {
                        if(*first_non_digit == '.') {
                            got_dot = true;
                        }
                        first_non_digit++;
                    }

                    size_t first_non_digit_index = first_non_digit - c_onwards;
                    size_t literal_len = first_non_digit_index;

                    // ** Truncate if there is more than one dot **
                    const char *dot1 = strchr(c_onwards, '.');
                    if(dot1) {
                        const char *dot2 = strchr(dot1 + 1, '.');
                        if(dot2) {
                            literal_len =
                                dot1 - c_onwards; // Stop at the first dot
                            got_dot = false;      // Treat as an integer
                        }
                    }

                    Token *tok = arena_alloc(arena, sizeof *tok);
                    tok->offset = curr_at;
                    char num_buf[literal_len + 1];
                    snprintf(num_buf, literal_len + 1, "%s", c_onwards);

                    if(got_dot) {
                        tok->kind = TOKEN_LITERAL_REAL;
                        tok->float_val = strtod(num_buf, NULL);
                    } else {
                        tok->kind = TOKEN_LITERAL_INTEGER;
                        tok->int_val = atoi(num_buf);
                    }

                    size_t extra_bytes = literal_len - 1;
                    lexer->pos += extra_bytes;
                    lexer->rest += extra_bytes;

                    return tok;
                }
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

                    tok = arena_alloc(arena, sizeof *tok);
                    tok->offset = curr_at;

                    int kw =
                        ht_get(lexer->kw_lookup, str_to_upper(strdup(lexeme)));
                    if(kw == -1) {
                        tok->kind = TOKEN_IDENT;
                        tok->string_val = strdup(lexeme);
                    } else {
                        tok->kind = (TokenKind)kw;
                    }

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

static void fillup_keywords(kw_ht *table) {
    ht_set(table, "PROGRAM", TOKEN_KEYWORD_PROGRAM);
    ht_set(table, "CLASS", TOKEN_KEYWORD_CLASS);
    ht_set(table, "END_CLASS", TOKEN_KEYWORD_END_CLASS);
    ht_set(table, "ENDCLASS", TOKEN_KEYWORD_END_CLASS);
    ht_set(table, "EXTENDS", TOKEN_KEYWORD_EXTENDS);
    ht_set(table, "IMPLEMENTS", TOKEN_KEYWORD_IMPLEMENTS);
    ht_set(table, "INTERFACE", TOKEN_KEYWORD_INTERFACE);
    ht_set(table, "END_INTERFACE", TOKEN_KEYWORD_END_INTERFACE);
    ht_set(table, "ENDINTERFACE", TOKEN_KEYWORD_END_INTERFACE);
    ht_set(table, "PROPERTY", TOKEN_KEYWORD_PROPERTY);
    ht_set(table, "END_PROPERTY", TOKEN_KEYWORD_END_PROPERTY);
    ht_set(table, "ENDPROPERTY", TOKEN_KEYWORD_END_PROPERTY);
    ht_set(table, "VAR_INPUT", TOKEN_KEYWORD_VAR_INPUT);
    ht_set(table, "VARINPUT", TOKEN_KEYWORD_VAR_INPUT);
    ht_set(table, "VAR_OUTPUT", TOKEN_KEYWORD_VAR_OUTPUT);
    ht_set(table, "VAROUTPUT", TOKEN_KEYWORD_VAR_OUTPUT);
    ht_set(table, "VAR", TOKEN_KEYWORD_VAR);
    ht_set(table, "VAR_CONFIG", TOKEN_KEYWORD_VAR_CONFIG);
    ht_set(table, "ABSTRACT", TOKEN_KEYWORD_ABSTRACT);
    ht_set(table, "FINAL", TOKEN_KEYWORD_FINAL);
    ht_set(table, "METHOD", TOKEN_KEYWORD_METHOD);
    ht_set(table, "CONSTANT", TOKEN_KEYWORD_CONSTANT);
    ht_set(table, "RETAIN", TOKEN_KEYWORD_RETAIN);
    ht_set(table, "NON_RETAIN", TOKEN_KEYWORD_NON_RETAIN);
    ht_set(table, "NONRETAIN", TOKEN_KEYWORD_NON_RETAIN);
    ht_set(table, "VAR_TEMP", TOKEN_KEYWORD_VAR_TEMP);
    ht_set(table, "VARTEMP", TOKEN_KEYWORD_VAR_TEMP);
    ht_set(table, "END_METHOD", TOKEN_KEYWORD_END_METHOD);
    ht_set(table, "ENDMETHOD", TOKEN_KEYWORD_END_METHOD);
    ht_set(table, "PUBLIC", TOKEN_KEYWORD_ACCESS_PUBLIC);
    ht_set(table, "PRIVATE", TOKEN_KEYWORD_ACCESS_PRIVATE);
    ht_set(table, "INTERNAL", TOKEN_KEYWORD_ACCESS_INTERNAL);
    ht_set(table, "PROTECTED", TOKEN_KEYWORD_ACCESS_PROTECTED);
    ht_set(table, "OVERRIDE", TOKEN_KEYWORD_OVERRIDE);
    ht_set(table, "VAR_GLOBAL", TOKEN_KEYWORD_VAR_GLOBAL);
    ht_set(table, "VARGLOBAL", TOKEN_KEYWORD_VAR_GLOBAL);
    ht_set(table, "VAR_IN_OUT", TOKEN_KEYWORD_VAR_IN_OUT);
    ht_set(table, "VARINOUT", TOKEN_KEYWORD_VAR_IN_OUT);
    ht_set(table, "VAR_EXTERNAL", TOKEN_KEYWORD_VAR_EXTERNAL);
    ht_set(table, "END_VAR", TOKEN_KEYWORD_END_VAR);
    ht_set(table, "ENDVAR", TOKEN_KEYWORD_END_VAR);
    ht_set(table, "END_PROGRAM", TOKEN_KEYWORD_END_PROGRAM);
    ht_set(table, "ENDPROGRAM", TOKEN_KEYWORD_END_PROGRAM);
    ht_set(table, "FUNCTION", TOKEN_KEYWORD_FUNCTION);
    ht_set(table, "END_FUNCTION", TOKEN_KEYWORD_END_FUNCTION);
    ht_set(table, "ENDFUNCTION", TOKEN_KEYWORD_END_FUNCTION);
    ht_set(table, "FUNCTION_BLOCK", TOKEN_KEYWORD_FUNCTION_BLOCK);
    ht_set(table, "FUNCTIONBLOCK", TOKEN_KEYWORD_FUNCTION_BLOCK);
    ht_set(table, "END_FUNCTION_BLOCK", TOKEN_KEYWORD_END_FUNCTION_BLOCK);
    ht_set(table, "ENDFUNCTIONBLOCK", TOKEN_KEYWORD_END_FUNCTION_BLOCK);
    ht_set(table, "TYPE", TOKEN_KEYWORD_TYPE);
    ht_set(table, "STRUCT", TOKEN_KEYWORD_STRUCT);
    ht_set(table, "END_TYPE", TOKEN_KEYWORD_END_TYPE);
    ht_set(table, "ENDTYPE", TOKEN_KEYWORD_END_TYPE);
    ht_set(table, "END_STRUCT", TOKEN_KEYWORD_END_STRUCT);
    ht_set(table, "ENDSTRUCT", TOKEN_KEYWORD_END_STRUCT);
    ht_set(table, "ACTIONS", TOKEN_KEYWORD_ACTIONS);
    ht_set(table, "ACTION", TOKEN_KEYWORD_ACTION);
    ht_set(table, "END_ACTION", TOKEN_KEYWORD_END_ACTION);
    ht_set(table, "ENDACTION", TOKEN_KEYWORD_END_ACTION);
    ht_set(table, "END_ACTIONS", TOKEN_KEYWORD_END_ACTIONS);
    ht_set(table, "ENDACTIONS", TOKEN_KEYWORD_END_ACTIONS);
    ht_set(table, "IF", TOKEN_KEYWORD_IF);
    ht_set(table, "THEN", TOKEN_KEYWORD_THEN);
    ht_set(table, "ELSIF", TOKEN_KEYWORD_ELSE_IF);
    ht_set(table, "ELSE", TOKEN_KEYWORD_ELSE);
    ht_set(table, "END_IF", TOKEN_KEYWORD_END_IF);
    ht_set(table, "ENDIF", TOKEN_KEYWORD_END_IF);
    ht_set(table, "FOR", TOKEN_KEYWORD_FOR);
    ht_set(table, "TO", TOKEN_KEYWORD_TO);
    ht_set(table, "BY", TOKEN_KEYWORD_BY);
    ht_set(table, "DO", TOKEN_KEYWORD_DO);
    ht_set(table, "END_FOR", TOKEN_KEYWORD_END_FOR);
    ht_set(table, "ENDFOR", TOKEN_KEYWORD_END_FOR);
    ht_set(table, "WHILE", TOKEN_KEYWORD_WHILE);
    ht_set(table, "END_WHILE", TOKEN_KEYWORD_END_WHILE);
    ht_set(table, "ENDWHILE", TOKEN_KEYWORD_END_WHILE);
    ht_set(table, "REPEAT", TOKEN_KEYWORD_REPEAT);
    ht_set(table, "UNTIL", TOKEN_KEYWORD_UNTIL);
    ht_set(table, "END_REPEAT", TOKEN_KEYWORD_END_REPEAT);
    ht_set(table, "ENDREPEAT", TOKEN_KEYWORD_END_REPEAT);
    ht_set(table, "CASE", TOKEN_KEYWORD_CASE);
    ht_set(table, "RETURN", TOKEN_KEYWORD_RETURN);
    ht_set(table, "EXIT", TOKEN_KEYWORD_EXIT);
    ht_set(table, "CONTINUE", TOKEN_KEYWORD_CONTINUE);
    ht_set(table, "POINTER", TOKEN_KEYWORD_POINTER);
    ht_set(table, "REF_TO", TOKEN_KEYWORD_REFERENCE_TO);
    ht_set(table, "REFTO", TOKEN_KEYWORD_REFERENCE_TO);
    ht_set(table, "ARRAY", TOKEN_KEYWORD_ARRAY);
    ht_set(table, "STRING", TOKEN_KEYWORD_STRING);
    ht_set(table, "WSTRING", TOKEN_KEYWORD_WIDE_STRING);
    ht_set(table, "OF", TOKEN_KEYWORD_OF);
    ht_set(table, "AT", TOKEN_KEYWORD_AT);
    ht_set(table, "END_CASE", TOKEN_KEYWORD_END_CASE);
    ht_set(table, "ENDCASE", TOKEN_KEYWORD_END_CASE);

    ht_set(table, "MOD", TOKEN_OPERATOR_MODULO);
    ht_set(table, "AND", TOKEN_OPERATOR_AND);
    ht_set(table, "OR", TOKEN_OPERATOR_OR);
    ht_set(table, "XOR", TOKEN_OPERATOR_XOR);
    ht_set(table, "NOT", TOKEN_OPERATOR_NOT);
}

static inline void advance(Lexer *l) {
    l->pos++;
    l->rest++;
}

static inline bool is_st_ident_ch(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static char *str_to_upper(char *s) {
    char *orig = s;
    while(*s) {
        *s = toupper((unsigned char)*s);
        s++;
    }
    return orig;
}

#define tok_dbg(kind, fmt, ...)                                                \
    case kind:                                                                 \
        stil_info(fmt, ##__VA_ARGS__);                                         \
        break

void token_show(Token *token) {
    switch(token->kind) {
        tok_dbg(TOKEN_KEYWORD_PROGRAM, "PROGRAM");
        tok_dbg(TOKEN_KEYWORD_CLASS, "CLASS");
        tok_dbg(TOKEN_KEYWORD_END_CLASS, "END_CLASS");
        tok_dbg(TOKEN_KEYWORD_EXTENDS, "EXTENDS");
        tok_dbg(TOKEN_KEYWORD_IMPLEMENTS, "IMPLEMENTS");
        tok_dbg(TOKEN_KEYWORD_INTERFACE, "INTERFACE");
        tok_dbg(TOKEN_KEYWORD_END_INTERFACE, "END_INTERFACE");
        tok_dbg(TOKEN_KEYWORD_PROPERTY, "PROPERTY");
        tok_dbg(TOKEN_KEYWORD_END_PROPERTY, "END_PROPERTY");
        tok_dbg(TOKEN_KEYWORD_VAR_INPUT, "VAR_INPUT");
        tok_dbg(TOKEN_KEYWORD_VAR_OUTPUT, "VAR_OUTPUT");
        tok_dbg(TOKEN_KEYWORD_VAR, "VAR");
        tok_dbg(TOKEN_KEYWORD_VAR_CONFIG, "VAR_CONFIG");
        tok_dbg(TOKEN_KEYWORD_ABSTRACT, "ABSTRACT");
        tok_dbg(TOKEN_KEYWORD_FINAL, "FINAL");
        tok_dbg(TOKEN_KEYWORD_METHOD, "METHOD");
        tok_dbg(TOKEN_KEYWORD_CONSTANT, "CONSTANT");
        tok_dbg(TOKEN_KEYWORD_RETAIN, "RETAIN");
        tok_dbg(TOKEN_KEYWORD_NON_RETAIN, "NON_RETAIN");
        tok_dbg(TOKEN_KEYWORD_VAR_TEMP, "VAR_TEMP");
        tok_dbg(TOKEN_KEYWORD_END_METHOD, "END_METHOD");
        tok_dbg(TOKEN_KEYWORD_ACCESS_PUBLIC, "ACCESS_PUBLIC");
        tok_dbg(TOKEN_KEYWORD_ACCESS_PRIVATE, "ACCESS_PRIVATE");
        tok_dbg(TOKEN_KEYWORD_ACCESS_INTERNAL, "ACCESS_INTERNAL");
        tok_dbg(TOKEN_KEYWORD_ACCESS_PROTECTED, "ACCESS_PROTECTED");
        tok_dbg(TOKEN_KEYWORD_OVERRIDE, "OVERRIDE");
        tok_dbg(TOKEN_KEYWORD_VAR_GLOBAL, "VAR_GLOBAL");
        tok_dbg(TOKEN_KEYWORD_VAR_IN_OUT, "VAR_IN_OUT");
        tok_dbg(TOKEN_KEYWORD_VAR_EXTERNAL, "VAR_EXTERNAL");
        tok_dbg(TOKEN_KEYWORD_END_VAR, "END_VAR");
        tok_dbg(TOKEN_KEYWORD_END_PROGRAM, "END_PROGRAM");
        tok_dbg(TOKEN_KEYWORD_FUNCTION, "FUNCTION");
        tok_dbg(TOKEN_KEYWORD_END_FUNCTION, "END_FUNCTION");
        tok_dbg(TOKEN_KEYWORD_FUNCTION_BLOCK, "FUNCTION_BLOCK");
        tok_dbg(TOKEN_KEYWORD_END_FUNCTION_BLOCK, "END_FUNCTION_BLOCK");
        tok_dbg(TOKEN_KEYWORD_TYPE, "TYPE");
        tok_dbg(TOKEN_KEYWORD_STRUCT, "STRUCT");
        tok_dbg(TOKEN_KEYWORD_END_TYPE, "END_TYPE");
        tok_dbg(TOKEN_KEYWORD_END_STRUCT, "END_STRUCT");
        tok_dbg(TOKEN_KEYWORD_ACTIONS, "ACTIONS");
        tok_dbg(TOKEN_KEYWORD_ACTION, "ACTION");
        tok_dbg(TOKEN_KEYWORD_END_ACTION, "END_ACTION");
        tok_dbg(TOKEN_KEYWORD_END_ACTIONS, "END_ACTIONS");
        tok_dbg(TOKEN_KEYWORD_COLON, "COLON");
        tok_dbg(TOKEN_KEYWORD_SEMICOLON, "SEMICOLON");
        tok_dbg(TOKEN_KEYWORD_ASSIGN, "ASSIGN");
        tok_dbg(TOKEN_KEYWORD_OUTPUT_ASSIGNMENT, "OUTPUT_ASSIGNMENT");
        tok_dbg(TOKEN_KEYWORD_REFERENCE_ASSIGNMENT, "REFERENCE_ASSIGNMENT");
        tok_dbg(TOKEN_KEYWORD_LPAREN, "LPAREN");
        tok_dbg(TOKEN_KEYWORD_RPAREN, "RPAREN");
        tok_dbg(TOKEN_KEYWORD_LSQUARE, "LSQUARE");
        tok_dbg(TOKEN_KEYWORD_RSQUARE, "RSQUARE");
        tok_dbg(TOKEN_KEYWORD_COMMA, "COMMA");
        tok_dbg(TOKEN_KEYWORD_DOT_DOT_DOT, "DOT_DOT_DOT");
        tok_dbg(TOKEN_KEYWORD_DOT_DOT, "DOT_DOT");
        tok_dbg(TOKEN_KEYWORD_DOT, "DOT");
        tok_dbg(TOKEN_KEYWORD_IF, "IF");
        tok_dbg(TOKEN_KEYWORD_THEN, "THEN");
        tok_dbg(TOKEN_KEYWORD_ELSE_IF, "ELSE_IF");
        tok_dbg(TOKEN_KEYWORD_ELSE, "ELSE");
        tok_dbg(TOKEN_KEYWORD_END_IF, "END_IF");
        tok_dbg(TOKEN_KEYWORD_FOR, "FOR");
        tok_dbg(TOKEN_KEYWORD_TO, "TO");
        tok_dbg(TOKEN_KEYWORD_BY, "BY");
        tok_dbg(TOKEN_KEYWORD_DO, "DO");
        tok_dbg(TOKEN_KEYWORD_END_FOR, "END_FOR");
        tok_dbg(TOKEN_KEYWORD_WHILE, "WHILE");
        tok_dbg(TOKEN_KEYWORD_END_WHILE, "END_WHILE");
        tok_dbg(TOKEN_KEYWORD_REPEAT, "REPEAT");
        tok_dbg(TOKEN_KEYWORD_UNTIL, "UNTIL");
        tok_dbg(TOKEN_KEYWORD_END_REPEAT, "END_REPEAT");
        tok_dbg(TOKEN_KEYWORD_CASE, "CASE");
        tok_dbg(TOKEN_KEYWORD_RETURN, "RETURN");
        tok_dbg(TOKEN_KEYWORD_EXIT, "EXIT");
        tok_dbg(TOKEN_KEYWORD_CONTINUE, "CONTINUE");
        tok_dbg(TOKEN_KEYWORD_POINTER, "POINTER");
        tok_dbg(TOKEN_KEYWORD_REF, "REF");
        tok_dbg(TOKEN_KEYWORD_REFERENCE_TO, "REFERENCE_TO");
        tok_dbg(TOKEN_KEYWORD_ARRAY, "ARRAY");
        tok_dbg(TOKEN_KEYWORD_STRING, "STRING");
        tok_dbg(TOKEN_KEYWORD_WIDE_STRING, "WIDE_STRING");
        tok_dbg(TOKEN_KEYWORD_OF, "OF");
        tok_dbg(TOKEN_KEYWORD_AT, "AT");
        tok_dbg(TOKEN_KEYWORD_END_CASE, "END_CASE");

        tok_dbg(TOKEN_OPERATOR_PLUS, "PLUS");
        tok_dbg(TOKEN_OPERATOR_MINUS, "MINUS");
        tok_dbg(TOKEN_OPERATOR_MULTIPLICATION, "MULTIPLICATION");
        tok_dbg(TOKEN_OPERATOR_EXPONENT, "EXPONENT");
        tok_dbg(TOKEN_OPERATOR_DIVISION, "DIVISION");
        tok_dbg(TOKEN_OPERATOR_EQ, "EQ");
        tok_dbg(TOKEN_OPERATOR_NOT_EQ, "NOT_EQ");
        tok_dbg(TOKEN_OPERATOR_LESS_THAN, "LESS_THAN");
        tok_dbg(TOKEN_OPERATOR_GREATER_THAN, "GREATER_THAN");
        tok_dbg(TOKEN_OPERATOR_LESS_THAN_EQ, "LESS_THAN_EQ");
        tok_dbg(TOKEN_OPERATOR_GREATER_THAN_EQ, "GREATER_THAN_EQ");
        tok_dbg(TOKEN_OPERATOR_AMP, "AMP");
        tok_dbg(TOKEN_OPERATOR_DEREF, "DEREF");
        tok_dbg(TOKEN_OPERATOR_MODULO, "MODULO");
        tok_dbg(TOKEN_OPERATOR_AND, "AND");
        tok_dbg(TOKEN_OPERATOR_OR, "OR");
        tok_dbg(TOKEN_OPERATOR_XOR, "XOR");
        tok_dbg(TOKEN_OPERATOR_NOT, "NOT");

        tok_dbg(TOKEN_IDENT, "IDENT %s", token->string_val);
        tok_dbg(TOKEN_LITERAL_STRING, "STR LITERAL '%s'", token->string_val);
        tok_dbg(TOKEN_EOF, "EOF");
        tok_dbg(TOKEN_ILLEGAL, "ILLEGAL %s", token->string_val);

        tok_dbg(TOKEN_LITERAL_INTEGER, "INT LITERAL %d", token->int_val);
        tok_dbg(TOKEN_LITERAL_REAL, "REAL LITERAL %f", token->float_val);

        case TOKEN_PROPERTY_EXTERNAL:
        case TOKEN_PROPERTY_BY_REF:
        case TOKEN_PROPERTY_CONSTANT:
        case TOKEN_PROPERTY_SIZED:
        case TOKEN_LITERAL_INTEGER_HEX:
        case TOKEN_LITERAL_INTEGER_OCT:
        case TOKEN_LITERAL_INTEGER_BIN:
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
    }
}

#undef tok_dbg
