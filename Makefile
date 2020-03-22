CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic
DEBUG_FLAGS = -ggdb -O0
RELEASE_FLAGS = -march=native -O2
PROFILE_FLAGS = $(RELEASE_FLAGS) -pg
LDLIBS =

RM = rm -f
MKDIR = mkdir -p

OBJS = raven.o lexer.o

debug: bin/raven
release: clean bin/raven
profile: clean bin/raven

debug: CFLAGS += $(DEBUG_FLAGS)
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


