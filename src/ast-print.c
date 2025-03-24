#include "ast.h"

char *type_dbg(TypeDecl ty) {
    switch(ty) {
        case TYPE_INT:
            return "INT";
        case TYPE_REAL:
            return "REAL";
        case TYPE_STRING:
            return "STRING";
        case TYPE_BOOL:
            return "BOOL";
        case NO_RETURN_TYPE:
            return "";

        case NO_TYPE:
            stil_warn("Not a type");
            return "";
    }
}

char *var_block_type_dbg(VarBlockType ty) {
    switch(ty) {
        case VARBLOCK_LOCAL:
            return "LOCAL";
        case VARBLOCK_TEMP:
            return "TEMP";
        case VARBLOCK_GLOBAL:
            return "GLOBAL";
        case VARBLOCK_INPUT:
            return "INPUT";
        case VARBLOCK_OUTPUT:
            return "OUTPUT";
        case VARBLOCK_IN_OUT:
            return "IN OUT";
        case VARBLOCK_EXTERNAL:
            return "EXTERNAL";
        case NO_VARBLOCK:
            stil_warn("Not a variable block type");
            return "";
    }
}

char *st_unit_type_dbg(StUnitType ty) {
    switch(ty) {
        case STUNIT_PROGRAM:
            return "PROGRAM";
        case STUNIT_ACTION:
            return "ACTION";
        case STUNIT_CLASS:
            return "CLASS";
        case NO_STUNIT:
            stil_warn("Not a st unit type");
            return "";
            break;
    }
}

void ast_dump(ASTNode *root) {
    stil_info("======AST======");
    astnode_dbg(root);
}

void print_st_unit(STUnit *unit, size_t indent) {
    INDENTED(indent, "%s:", st_unit_type_dbg(unit->unit_type));
    INDENTED(indent + 1, "NAME: %s", unit->name->label);

    if(unit->variable_blocks->count > 0) {
        INDENTED(indent + 1, "VARIABLE_DECLARATIONS:");
        for(size_t i = 0; i < unit->variable_blocks->count; i++) {
            astnode_dbg_indented(unit->variable_blocks->nodes[i], indent + 2);
        }
    }

    if(unit->statements->count > 0) {
        INDENTED(indent + 1, "BODY:");
        for(size_t i = 0; i < unit->statements->count; i++) {
            astnode_dbg_indented(unit->statements->nodes[i], indent + 2);
        }
    }
}

void comp_unit_dump(CompilationUnit *comp_unit) {
    stil_info("======AST======");
    for(size_t i = 0; i < comp_unit->st_units->count; i++) {
        print_st_unit(comp_unit->st_units->units[i], 0);
    }
}

void print_var_decl(VarDeclaration *decl, size_t indent) {
    INDENTED(indent, "VAR DECLARATION:");
    INDENTED_NONEW(indent + 1, "Symbols: ");
    for(size_t i = 0; i < decl->labels->count; i++) {
        if(i == 0) {
            printf("%s", decl->labels->symbols[i]->label);
        } else {
            printf(", %s", decl->labels->symbols[i]->label);
        }
    }
    printf("\n");
    INDENTED(indent + 1, "TYPE: %s", type_dbg(decl->type));
    if(decl->value) {
        INDENTED(indent + 1, "VALUE:");
        astnode_dbg_indented(decl->value, indent + 2);
    }
}

void print_var_decl_block(VarBlock *block, size_t indent) {
    INDENTED(indent, "VAR DECLARATION BLOCK (%s):",
             var_block_type_dbg(block->block_type));
    for(size_t i = 0; i < block->var_decls->count; i++) {
        print_var_decl(&block->var_decls->nodes[i]->var_decl, indent + 1);
    }
}

void print_assignment(Assignment *asgmt, size_t indent) {
    INDENTED(indent, "ASSIGNMENT:");
    INDENTED(indent + 1, "LHS: %s", asgmt->name->label);
    INDENTED(indent + 1, "RHS:");
    astnode_dbg_indented(asgmt->value, indent + 2);
}

void print_int_literal(IntLiteral *int_literal, size_t indent) {
    INDENTED(indent, "INT LITERAL: %d", int_literal->int_val);
}

void print_real_literal(RealLiteral *real_literal, size_t indent) {
    INDENTED(indent, "REAL LITERAL: %f", real_literal->real_val);
}

void print_str_literal(StrLiteral *str_literal, size_t indent) {
    INDENTED(indent, "STR LITERAL: %s", str_literal->str_val);
}

void astnode_dbg_indented(ASTNode *node, size_t indent) {
    if(!node) {
        return;
    }

    switch(node->kind) {
        case ASNTNODE_VAR_DECLARATION_BLOCK:
            print_var_decl_block(&node->var_block, indent);
            break;
        case ASNTNODE_VAR_DECLARATION:
            print_var_decl(&node->var_decl, indent);
            break;
        case ASTNODE_IF_STMT:
        case ASTNODE_COND_THEN_BLOCK:
        case ASTNODE_UNARY_EXPR:
        case ASTNODE_BINARY_EXPR:
            break;
        case ASTNODE_INT_LITERAL:
            print_int_literal(&node->int_literal, indent);
            break;
        case ASTNODE_REAL_LITERAL:
            print_real_literal(&node->real_literal, indent);
            break;
        case ASTNODE_STR_LITERAL:
            print_str_literal(&node->str_literal, indent);
            break;
        case ASTNODE_BOOL_LITERAL:
        case ASTNODE_SYMBOL:
            break;

        case ASTNODE_CHUNK:
            stil_info("UNIMPLEMENTED");
            break;
        case ASTNODE_PROGRAM:
            /* print_st_unit(&node->st_unit, indent); */
            break;

        case ASTNODE_ASSIGNMENT_STMT:
            print_assignment(&node->asgmt, indent);
            break;
    }
}
