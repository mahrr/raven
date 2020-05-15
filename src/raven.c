#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "vm.h"

int main(void) {
    VM vm;
    char buff[128];
    
    init_vm(&vm);
    for (;;) {
        printf("> ");
        if (!fgets(buff, 128, stdin) || feof(stdin)) break;
        interpret(&vm, buff);
    }

    free_vm(&vm);
    return 0;
}
