CC = gcc
CFLAGS = -std=c11 -g -o
SRCDIR = ./src
OUTDIR = ./bin
TARGET = l
CLIBS =

# main
default:
	$(CC) $(CFLAGS) $(OUTDIR)/$(TARGET) $(SRCDIR)/*.c $(CLIBS)

run:
	$(OUTDIR)/$(TARGET)

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

lex_test:
	$(CC) $(CFLAGS) $(OUTDIR)/$(LEXER_TEST_TARGET) $(LEXER_TEST_RELATED) $(LEXER_TEST) $(CLIBS)

run_ltest: 
	$(OUTDIR)/$(LEXER_TEST_TARGET)
