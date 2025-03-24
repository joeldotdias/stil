#include "ast.h"
#include <string.h>

SymbolList *symbol_list_init() {
    SymbolList *list = stil_malloc(sizeof *list);
    list->symbols = stil_malloc(10 * sizeof(Symbol *));
    list->count = 0;
    list->cap = 0;
    return list;
}

void symbol_list_push(SymbolList *list, Symbol *symbol) {
    /* for(size_t i = 0; i < list->count; i++) {
        if(strcmp(list->symbols[i]->label, symbol->label) == 0) {
            return;
        }
    } */

    if(list->count >= list->cap) {
        size_t new_cap = list->cap += 2;
        Symbol **new_symbols =
            stil_realloc(list->symbols, new_cap * sizeof(Symbol *));
        list->symbols = new_symbols;
        list->cap = new_cap;
    }
    list->symbols[list->count] = symbol;
    list->count++;
}

void symbol_list_show(const SymbolList *list) {
    for(size_t i = 0; i < list->count; i++) {
        printf("SYMBOL %zu: %s\n", i, list->symbols[i]->label);
    }
}

ASTNodeList *astnode_list_init() {
    ASTNodeList *list = stil_malloc(sizeof *list);
    list->nodes = stil_malloc(10 * sizeof(ASTNode *));
    list->count = 0;
    list->cap = 0;
    return list;
}

void astnode_list_push(ASTNodeList *list, ASTNode *node) {
    if(list->count >= list->cap) {
        size_t new_capacity = list->cap += 2;
        ASTNode **new_nodes =
            stil_realloc(list->nodes, new_capacity * sizeof(ASTNode *));
        list->nodes = new_nodes;
        list->cap = new_capacity;
    }

    list->nodes[list->count] = node;
    list->count++;
};

void astnode_list_show(const ASTNodeList *list) {
    for(size_t i = 0; i < list->count; i++) {
        astnode_dbg_indented(list->nodes[i], 0);
    }
}

STUnitList *st_unit_list_init() {
    STUnitList *list = stil_malloc(sizeof *list);
    list->units = stil_malloc(10 * sizeof(STUnit *));
    list->count = 0;
    list->cap = 0;
    return list;
}

void st_unit_list_push(STUnitList *list, STUnit *unit) {
    if(list->count >= list->cap) {
        size_t new_capacity = list->cap += 2;
        STUnit **new_units =
            stil_realloc(list->units, new_capacity * sizeof(STUnit *));
        list->units = new_units;
        list->cap = new_capacity;
    }

    list->units[list->count] = unit;
    list->count++;
}
