#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "exec.h"
#include "functab.h"
#include "semantic.h"
#include "symtab.h"
#include "tac.h"

extern int yyparse(void);
extern FILE* yyin;
extern StmtList* g_program_ast;

int main(int argc, char* argv[]) {
    int sem_errors;
    EvalVal run_result;

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Error: unable to open %s\n", argv[1]);
            return 1;
        }
    }

    if (yyparse() != 0 || !g_program_ast) {
        if (yyin) {
            fclose(yyin);
        }
        return 1;
    }

    sem_errors = semantic_check(g_program_ast);
    if (sem_errors > 0) {
        ast_free_stmt_list(g_program_ast);
        g_program_ast = NULL;
        symtab_clear();
        functab_clear();
        if (yyin) {
            fclose(yyin);
        }
        return 2;
    }

    tac_generate(g_program_ast);

    run_result = exec_program(g_program_ast);
    if (run_result.signal == EXEC_RETURN) {
        value_free(&run_result.value);
    }

    printf("Parsing + Semantic OK.\n");

    ast_free_stmt_list(g_program_ast);
    g_program_ast = NULL;
    symtab_clear();
    functab_clear();

    if (yyin) {
        fclose(yyin);
    }

    return 0;
}
