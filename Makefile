CC = gcc
CFLAGS = -std=c11 -g -Wall -o
SRCDIR = ./src
OUTDIR = ./bin
TARGET = raven
CLIBS =

# main
default:
	$(CC) $(CFLAGS) $(OUTDIR)/$(TARGET) $(SRCDIR)/*.c $(CLIBS)

run:
	@$(OUTDIR)/$(TARGET)

clean:
	$(RM) $(OUTDIR)/*


# lexer tests
LEXER_TEST = tests/test_lexer.c
LEXER_TEST_TARGET = lex_test
LEXER_TEST_RELATED = src/alloc.c \
					src/salloc.c \
					src/lexer.c  \
					src/error.c  \
					src/list.c   \
					src/strutil.c

# compile the lexer test and run it
lextest:
	@$(CC) $(CFLAGS) $(OUTDIR)/$(LEXER_TEST_TARGET) $(LEXER_TEST_RELATED) $(LEXER_TEST) $(CLIBS)
	@$(OUTDIR)/$(LEXER_TEST_TARGET)

# compile the lexer test only
clextest:
	@$(CC) $(CFLAGS) $(OUTDIR)/$(LEXER_TEST_TARGET) $(LEXER_TEST_RELATED) $(LEXER_TEST) $(CLIBS)
