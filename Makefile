CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror
DEBUG_FLAGS = -ggdb -DDEBUG -O0
RELEASE_FLAGS = -march=native -DNDEBUG -O2
RELEASE_SYMBOLS_FLAGS = $(RELEASE_FLAGS) -ggdb
LDLIBS = -lm

RM = rm -f
MKDIR = mkdir -p

OBJS = raven.o vm.o chunk.o table.o object.o value.o compiler.o \
	   lexer.o debug.o mem.o

SRCDIR = src
BINDIR = build

debug: build/debug/raven
release: build/release/raven
release_symbols: build/relase_symbols/raven
re: clean debug

debug: CFLAGS += $(DEBUG_FLAGS)
release: CFLAGS += $(RELEASE_FLAGS)
release_symbols: CFLAGS += $(RELEASE_SYMBOLS_FLAGS)

build/debug/raven: $(OBJS:%.o=build/debug/%.o)
	@$(MKDIR) $(dir $@)
	@$(CC) -o $@ $^ $(LDLIBS)

build/release/raven: $(OBJS:%.o=build/release/%.o)
	@$(MKDIR) $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)

build/relase_symbols/raven: $(OBJS:%.o=build/relase_symbols/%.o)
	@$(MKDIR) $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)

.SECONDARY:
build/debug/%.o: $(SRCDIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.SECONDARY:
build/release/%.o: $(SRCDIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.SECONDARY:
build/relase_symbols/%.o: $(SRCDIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: run clean

run: build/debug/raven
	@./build/debug/raven

clean:
	$(RM) -rf build/*
