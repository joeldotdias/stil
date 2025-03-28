#ifndef LEXER_H
#define LEXER_H

#include "arena.h"
#include "ht.h"
#include "shared.h"
#include <stdbool.h>

typedef struct _Lexer {
    const char *whole;
    const char *rest;
    const char *source;
    kw_ht *kw_lookup;
    size_t pos;
    size_t source_len;
    int n_errors;
} Lexer;

typedef enum _TokenKind {
    TOKEN_PROPERTY_EXTERNAL,
    TOKEN_PROPERTY_BY_REF,
    TOKEN_PROPERTY_CONSTANT,
    TOKEN_PROPERTY_SIZED,
    TOKEN_KEYWORD_PROGRAM,
    TOKEN_KEYWORD_CLASS,
    TOKEN_KEYWORD_END_CLASS,
    TOKEN_KEYWORD_EXTENDS,
    TOKEN_KEYWORD_IMPLEMENTS,
    TOKEN_KEYWORD_INTERFACE,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_REAL,
    TOKEN_KEYWORD_END_INTERFACE,
    TOKEN_KEYWORD_PROPERTY,
    TOKEN_KEYWORD_END_PROPERTY,
    TOKEN_KEYWORD_VAR_INPUT,
    TOKEN_KEYWORD_VAR_OUTPUT,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_VAR_CONFIG,
    TOKEN_KEYWORD_ABSTRACT,
    TOKEN_KEYWORD_FINAL,
    TOKEN_KEYWORD_METHOD,
    TOKEN_KEYWORD_CONSTANT,
    TOKEN_KEYWORD_RETAIN,
    TOKEN_KEYWORD_NON_RETAIN,
    TOKEN_KEYWORD_VAR_TEMP,
    TOKEN_KEYWORD_END_METHOD,
    TOKEN_KEYWORD_ACCESS_PUBLIC,
    TOKEN_KEYWORD_ACCESS_PRIVATE,
    TOKEN_KEYWORD_ACCESS_INTERNAL,
    TOKEN_KEYWORD_ACCESS_PROTECTED,
    TOKEN_KEYWORD_OVERRIDE,
    TOKEN_KEYWORD_VAR_GLOBAL,
    TOKEN_KEYWORD_VAR_IN_OUT,
    TOKEN_KEYWORD_VAR_EXTERNAL,
    TOKEN_KEYWORD_END_VAR,
    TOKEN_KEYWORD_END_PROGRAM,
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_KEYWORD_END_FUNCTION,
    TOKEN_KEYWORD_FUNCTION_BLOCK,
    TOKEN_KEYWORD_END_FUNCTION_BLOCK,
    TOKEN_KEYWORD_TYPE,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_END_TYPE,
    TOKEN_KEYWORD_END_STRUCT,
    TOKEN_KEYWORD_ACTIONS,
    TOKEN_KEYWORD_ACTION,
    TOKEN_KEYWORD_END_ACTION,
    TOKEN_KEYWORD_END_ACTIONS,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_KEYWORD_OUTPUT_ASSIGNMENT,
    TOKEN_KEYWORD_REFERENCE_ASSIGNMENT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LSQUARE,
    TOKEN_RSQUARE,
    TOKEN_COMMA,
    TOKEN_DOT_DOT_DOT,
    TOKEN_DOT_DOT,
    TOKEN_DOT,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_THEN,
    TOKEN_KEYWORD_ELSE_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_END_IF,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_TO,
    TOKEN_KEYWORD_BY,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_END_FOR,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_END_WHILE,
    TOKEN_KEYWORD_REPEAT,
    TOKEN_KEYWORD_UNTIL,
    TOKEN_KEYWORD_END_REPEAT,
    TOKEN_KEYWORD_CASE,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_EXIT,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_POINTER,
    TOKEN_KEYWORD_REF,
    TOKEN_KEYWORD_REFERENCE_TO,
    TOKEN_KEYWORD_ARRAY,
    TOKEN_KEYWORD_STRING,
    TOKEN_KEYWORD_WIDE_STRING,
    TOKEN_KEYWORD_OF,
    TOKEN_KEYWORD_AT,
    TOKEN_KEYWORD_END_CASE,
    TOKEN_OPERATOR_PLUS,
    TOKEN_OPERATOR_MINUS,
    TOKEN_OPERATOR_MULTIPLICATION,
    TOKEN_OPERATOR_EXPONENT,
    TOKEN_OPERATOR_DIVISION,
    TOKEN_OPERATOR_EQ,
    TOKEN_OPERATOR_NOT_EQ,
    TOKEN_OPERATOR_LESS_THAN,
    TOKEN_OPERATOR_GREATER_THAN,
    TOKEN_OPERATOR_LESS_THAN_EQ,
    TOKEN_OPERATOR_GREATER_THAN_EQ,
    TOKEN_OPERATOR_AMP,
    TOKEN_OPERATOR_DEREF,
    TOKEN_OPERATOR_MODULO,
    TOKEN_OPERATOR_AND,
    TOKEN_OPERATOR_OR,
    TOKEN_OPERATOR_XOR,
    TOKEN_OPERATOR_NOT,
    TOKEN_IDENT,
    TOKEN_LITERAL_INTEGER_HEX,
    TOKEN_LITERAL_INTEGER_OCT,
    TOKEN_LITERAL_INTEGER_BIN,
    TOKEN_LITERAL_INTEGER,
    TOKEN_LITERAL_NULL,
    TOKEN_LITERAL_TRUE,
    TOKEN_LITERAL_FALSE,
    TOKEN_LITERAL_DATE,
    TOKEN_LITERAL_DATE_AND_TIME,
    TOKEN_LITERAL_TIME_OF_DAY,
    TOKEN_LITERAL_TIME,
    TOKEN_DIRECT_ACCESS,
    TOKEN_HARDWARE_ACCESS,
    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_REAL,
    TOKEN_LITERAL_WIDE_STRING,
    TOKEN_TYPE_CAST_PREFIX,
    TOKEN_EOF,
    TOKEN_ILLEGAL,
} TokenKind;

typedef enum _HardwareAcessType {
    HARDWARE_ACCESS_INPUT,
    HARDWARE_ACCESS_OUTPUT,
    HARDWARE_ACCESS_MEMORY,
    HARDWARE_ACCESS_GLOBAL
} HardwareAccessType;

typedef enum _DirectAccessType {
    DIRECT_ACCESS_BIT,
    DIRECT_ACCESS_BYTE,
    DIRECT_ACCESS_WORD,
    DIRECT_ACCESS_DWORD,
    DIRECT_ACCESS_LWORD,
    DIRECT_ACCESS_TEMPLATE
} DirectAccessType;

typedef struct _Token {
    TokenKind kind;
    size_t offset;
    char *string_val;
    /* union {
        int int_val;
        double float_val;
        bool boolean_val;

        char *string_val; // will also be used for idents and typecasts
        wchar_t *wide_string_val;

        struct {
            int year, month, day;
            int hour, minute, second;
            long long nanoseconds;
        } datetime_val;

        struct {
            HardwareAccessType hardware_type;
            DirectAccessType direct_type;
            int offset;
        } hardware_access;

        struct {
            DirectAccessType direct_type;
            int offset;
        } direct_access;
    }; */
} Token;

/* Lexer *lexer_init(const char *filepath, Arena *arena);
Token *lexer_next_tok(Lexer *lexer, Arena *arena); */
Lexer *lexer_init(const char *filepath);
Token *lexer_next_tok(Lexer *lexer);
// void token_show(Token *token);
char *tok_dbg(Token *token);
void report(Lexer *lexer, size_t offset, size_t len, const char *message);

typedef enum _Started {
    ST_String,
    ST_WideString,
    ST_Number,
    ST_Keyword,
    ST_Ident_or_Keyword,
    ST_FSlash,
    ST_BlockComment,
    ST_None,
} Started;

#endif
