CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
DEBUG_FLAGS = -ggdb -DDEBUG -O0
RELEASE_FLAGS = -march=native -DNDEBUG -O2
PROFILE_FLAGS = $(RELEASE_FLAGS) -pg
LDLIBS =

RM = rm -f
MKDIR = mkdir -p

OBJS = raven.o vm.o chunk.o value.o compiler.o lexer.o debug.o mem.o

dev: bin/raven
release: clean bin/raven
profile: clean bin/raven

dev: CFLAGS += $(DEBUG_FLAGS)
release: CFLAGS += $(RELEASE_FLAGS)
profile: CFLAGS += $(PROFILE_FLAGS)

bin/raven: $(OBJS:%.o=src/%.o)
	@$(MKDIR) bin/
	$(CC) -o $@ $^ $(LDLIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: run clean

run:
	@./bin/raven

clean:
	$(RM) src/*.o
	$(RM) bin/raven
