CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic
DEBUG_FLAGS = -ggdb -DDEBUG -O0
RELEASE_FLAGS = -march=native -O2
PROFILE_FLAGS = $(RELEASE_FLAGS) -pg
LDLIBS = -lm

RM = rm -f
MKDIR = mkdir -p

debug: bin/raven

test: bin/test_lexer begin_test

release profile: clean bin/raven

OBJS = raven.o eval.o builtin.o object.o   	\
       env.o resolver.o parser.o lexer.o	\
       error.o token.o table.o hashing.o	\
       list.o strutil.o debug.o

bin/raven: $(OBJS:%.o=src/%.o)
	@$(MKDIR) bin/
	$(CC) -o $@ $^ $(LDLIBS)

OBJS = lexer.o error.o token.o debug.o strutil.o

bin/test_lexer: tests/test_lexer.o $(OBJS:%.o=src/%.o)
	@$(MKDIR) bin/
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

debug test: CFLAGS += $(DEBUG_FLAGS)
test: CFLAGS += -I./src
release: CFLAGS += $(RELEASE_FLAGS)
profile: CFLAGS += $(PROFILE_FLAGS)

.PHONY: run begin_test clean

run:
	@./bin/raven

begin_test:
	@./bin/test_lexer

clean:
	$(RM) src/*.o
	$(RM) bin/test_lexer
	$(RM) bin/raven
