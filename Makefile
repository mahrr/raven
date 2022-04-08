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
OBJDIR = build/object

debug: build/debug/raven
release: build/release/raven
release_symbols: build/relase_symbols/raven
re: clean dev

debug: CFLAGS += $(DEBUG_FLAGS)
release: CFLAGS += $(RELEASE_FLAGS)
release_symbols: CFLAGS += $(RELEASE_SYMBOLS_FLAGS)

build/debug/raven: $(OBJS:%.o=$(OBJDIR)/debug/%.o)
	@$(MKDIR) $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)

build/release/raven: $(OBJS:%.o=$(OBJDIR)/release/%.o)
	@$(MKDIR) $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)

build/relase_symbols/raven: $(OBJS:%.o=$(OBJDIR)/relase_symbols/%.o)
	@$(MKDIR) $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)

.SECONDARY:
$(OBJDIR)/debug/%.o: $(SRCDIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.SECONDARY:
$(OBJDIR)/release/%.o: $(SRCDIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.SECONDARY:
$(OBJDIR)/relase_symbols/%.o: $(SRCDIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: run clean

run:
	@./build/debug/raven

clean:
	$(RM) -rf build/*
