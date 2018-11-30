CC = gcc
CFLAGS = -std=c11 -g -o
SRCDIR = ./src
OUTDIR = ./bin
TARGET = l
CLIBS =

default:
	$(CC) $(CFLAGS) $(OUTDIR)/$(TARGET) $(SRCDIR)/*.c $(CLIBS)

run:
	$(OUTDIR)/$(TARGET)

clean:
	$(RM) $(OUTDIR)/*
