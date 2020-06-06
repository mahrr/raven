#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "vm.h"

static void usage() {
    fputs("Usage: raven [file-path]\n", stdout);
    exit(EXIT_FAILURE);
}

static void repl() {
    VM vm;
    init_vm(&vm);

    char buf[256];
    for (;;) {
        fputs("> ", stdout);
        if (!fgets(buf, 256, stdin) || feof(stdin)) break;
        interpret(&vm, buf, "stdin");
    }
    
    free_vm(&vm);
}

static char *scan_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Fatal: error openning '%s' (%s)\n", path,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *buf = (char *)malloc(size + 1);
    if (buf == NULL) {
        fprintf(stderr, "Fatal: not enough memory to scan '%s'\n", path);
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(buf, sizeof (char), size, file);
    if (bytes_read < size || ferror(file)) {
        fprintf(stderr, "Fatal: error reading '%s' (%s)\n", path,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(file);
    
    buf[size] = '\0';
    return buf;
}

static void execute_file(const char *path) {
    VM vm;
    init_vm(&vm);

    char *source = scan_file(path);
    InterpretResult result = interpret(&vm, source, path);
    free(source);

    free_vm(&vm);
    
    if (result == INTERPRET_RUNTIME_ERROR) exit(EXIT_FAILURE);
    if (result == INTERPRET_COMPILE_ERROR) exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        execute_file(argv[1]);
    } else {
        usage();
    }
    
    return EXIT_SUCCESS;
}
