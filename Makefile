CC = gcc
CFLAGS = -std=c99 -g -Wall -DPRINT_AST -o
PFLAGS = -std=c99 -g -Wall -pg -O2 -o 
OFLAGS = -std=c99 -march=native -g -Wall -O2 -o
SRCDIR = ./src
OUTDIR = ./bin
TARGET = raven
CLIBS =

# debug
default:
	@$(CC) $(CFLAGS) $(OUTDIR)/$(TARGET) $(SRCDIR)/*.c $(CLIBS)

# profiling
profile:
	@$(CC) $(PFLAGS) $(OUTDIR)/$(TARGET) $(SRCDIR)/*.c $(CLIBS)

# release
opt:
	@$(CC) $(OFLAGS) $(OUTDIR)/$(TARGET) $(SRCDIR)/*.c $(CLIBS)


run:
	@$(OUTDIR)/$(TARGET)

clean:
	$(RM) $(OUTDIR)/*


# lexer tests
LEX_TEST = tests/test_lexer.c
LEX_TEST_TARGET = lex_test
LEX_TEST_RELATED = src/alloc.c  \
				   src/salloc.c \
				   src/lexer.c  \
				   src/error.c  \
				   src/list.c   \
				   src/debug.c  \
				   src/strutil.c

LEX_INCLUDE = -I./src
LEX_PRINT = -DPRINT_TOKENS

# compile the lexer test and run it
lextest:
	@$(CC) $(CFLAGS) $(OUTDIR)/$(LEX_TEST_TARGET) \
	 $(LEX_TEST_RELATED) $(LEX_TEST) $(LEX_INCLUDE) $(CLIBS)
	@$(OUTDIR)/$(LEX_TEST_TARGET)

# compile the lexer test with PRINT_TOKENS macro defined and run it
plextest:
	@$(CC) $(CFLAGS) $(OUTDIR)/$(LEX_TEST_TARGET) \
	 $(LEX_PRINT) $(LEX_TEST_RELATED) $(LEX_TEST) \
	 $(LEX_INCLUDE) $(CLIBS)
	@$(OUTDIR)/$(LEX_TEST_TARGET)

# compile the lexer test only
clextest:
	@$(CC) $(CFLAGS) $(OUTDIR)/$(LEX_TEST_TARGET) \
	 $(LEX_TEST_RELATED) $(LEX_TEST) $(LEX_INCLUDE) $(CLIBS)
