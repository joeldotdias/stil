#ifndef AST_H
#define AST_H

#include "shared.h"

#define INDENTED(depth, format, ...)                                           \
    do {                                                                       \
        for(size_t i = 0; i < (depth) * 2; i++) {                              \
            printf(" ");                                                       \
        }                                                                      \
        printf(format "\n", ##__VA_ARGS__);                                    \
    } while(0)

#define INDENTED_NONEW(depth, format, ...)                                     \
    do {                                                                       \
        for(size_t i = 0; i < (depth) * 2; i++) {                              \
            printf(" ");                                                       \
        }                                                                      \
        printf(format, ##__VA_ARGS__);                                         \
    } while(0)

typedef enum {
    ASTNODE_CHUNK,

    ASTNODE_PROGRAM,

    ASNTNODE_VAR_DECLARATION_BLOCK,
    ASNTNODE_VAR_DECLARATION,

    ASTNODE_ASSIGNMENT_STMT,
    ASTNODE_IF_STMT,
    ASTNODE_COND_THEN_BLOCK,

    ASTNODE_UNARY_EXPR,
    ASTNODE_BINARY_EXPR,
    ASTNODE_INT_LITERAL,
    ASTNODE_REAL_LITERAL,
    ASTNODE_STR_LITERAL,
    ASTNODE_BOOL_LITERAL,

    ASTNODE_SYMBOL,
} NodeKind;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_EQ,
    OP_NE,

    OP_AND,
    OP_OR,
    OP_XOR,

    NO_INFIX,
} InfixOperator;

typedef enum {
    OP_NEG,
    OP_NOT,

    NO_PREFIX,
} PrefixOperator;

typedef enum _TypeDecl {
    TYPE_INT,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_BOOL,

    // special case for ST Units other than functions
    // its main purpose is to differentiate between invalid type
    // and structures that have no type
    NO_RETURN_TYPE,

    NO_TYPE,
} TypeDecl;

typedef enum _LinkageType {
    LINKAGE_INTERNAL,
    LINKAGE_EXTERNAL,
    LINKAGE_BUILTIN,
} LinkageType;

typedef enum _VarBlockType {
    VARBLOCK_LOCAL,
    VARBLOCK_TEMP,
    VARBLOCK_GLOBAL,
    VARBLOCK_INPUT,
    VARBLOCK_OUTPUT,
    VARBLOCK_IN_OUT,
    VARBLOCK_EXTERNAL,

    NO_VARBLOCK,
} VarBlockType;

typedef enum _StUnitType {
    STUNIT_PROGRAM,
    STUNIT_ACTION,
    STUNIT_CLASS,

    NO_STUNIT,
} StUnitType;

typedef struct _ASTNode ASTNode;
typedef struct _STUnit STUnit;

typedef struct _Symbol {
    char *label;
} Symbol;

typedef struct _SymbolList {
    Symbol **symbols;
    size_t count, cap;
} SymbolList;

typedef struct _ASTNodeList {
    ASTNode **nodes;
    size_t count, cap;
} ASTNodeList;

struct _STUnit {
    StUnitType unit_type;
    Symbol *name;
    ASTNodeList *variable_blocks; // have to be of type VarBlock
    ASTNodeList *statements;
    TypeDecl ReturnType; // set to NO_RETURN_TYPE if not a function
};

typedef struct _STUnitList {
    STUnit **units;
    size_t count, cap;
} STUnitList;

typedef struct _CompilationUnit {
    // global_vars;
    // var_config;
    /* These must be organization units, i.e.
     *      Program
     *      Function
     *      Action
     **/
    // ASTNodeList *nodes;
    STUnitList *st_units;

} CompilationUnit;

typedef struct _VarBlock {
    ASTNodeList *var_decls; // --> each of type ASNTNODE_VAR_DECLARATION
    VarBlockType block_type;
} VarBlock;

typedef struct _VarDeclaration {
    SymbolList *labels;
    TypeDecl type;
    ASTNode *value;
} VarDeclaration;

typedef struct _Function {
    Symbol *name;
    ASTNodeList *body;
    TypeDecl return_type;
} Function;

typedef struct _Assignment {
    Symbol *name;
    ASTNode *value;
} Assignment;

typedef struct _Precedence {
    int left_bind;
    int right_bind;
} Precedence;

typedef struct _IntLiteral {
    int int_val;
} IntLiteral;

typedef struct _RealLiteral {
    double real_val;
} RealLiteral;

typedef struct _StrLiteral {
    char *str_val;
} StrLiteral;

struct _ASTNode {
    NodeKind kind;

    union {
        STUnit st_unit;
        VarBlock var_block;
        VarDeclaration var_decl;
        Assignment asgmt;
        Symbol symbol;
        IntLiteral int_literal;
        RealLiteral real_literal;
        StrLiteral str_literal;
    };
};

#define astnode_dbg(node) astnode_dbg_indented(node, 0);

void astnode_dbg_indented(ASTNode *node, size_t indent);
void ast_dump(ASTNode *root);
void comp_unit_dump(CompilationUnit *comp_unit);
void print_st_unit(STUnit *unit, size_t indent);

SymbolList *symbol_list_init();
void symbol_list_push(SymbolList *list, Symbol *symbol);
void symbol_list_show(const SymbolList *list);
ASTNodeList *astnode_list_init();
void astnode_list_push(ASTNodeList *list, ASTNode *node);
void astnode_list_show(const ASTNodeList *list);
STUnitList *st_unit_list_init();
void st_unit_list_push(STUnitList *list, STUnit *unit);
// void st_unit_list_show(const STUnitList *list);

#endif
