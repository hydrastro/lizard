CC = gcc
AR = ar
CFLAGS = -c -std=c89 -lgmp -pg
CFLAGS += -O2 -march=native -mtune=native
CFLAGS += -Wall -Wextra -Werror -pedantic -pedantic-errors
CFLAGS += -fno-common -Wl,--gc-sections -Wredundant-decls -Wno-unused-parameter
CFLAGS += -fstack-protector-strong #-fPIE
CFLAGS += -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security
CFLAGS += -fstack-clash-protection -z noexecstack -z relro -z now
CFLAGS += -Wl,-z,relro,-z,now -Wl,-pie #-fpie
CFLAGS += -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual -Wdeclaration-after-statement
CLFAGS += -Wfloat-equal -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wmissing-prototypes -Wnested-externs
CFLAGS += -Wpointer-arith -Wredundant-decls -Wsequence-point -Wstrict-prototypes
CFLAGS += -Wswitch -Wundef -Wunreachable-code -Wwrite-strings -Wconversion

CFLAGS_DEBUG = -g -fno-omit-frame-pointer -fsanitize=address,undefined -fsanitize=leak

LDFLAGS = -shared

PREFIX ?= /usr/local
LIB_DIR = ./
OUT_LIB_DIR = $(PREFIX)/lib
OUT_INCLUDE_DIR = $(PREFIX)/include/lib

#SRC_FILES = $(wildcard $(LIB_DIR)/*.c)
#SRC_FILES = lizard.c
SRC_FILES = lizard.c env.c mem.c parser.c primitives.c tokenizer.c
OBJ_FILES = $(SRC_FILES:.c=.o)

all: liblizard.a liblizard.so

liblizard.a: $(OBJ_FILES)
	$(AR) rcs $@ $^

liblizard.so: $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

install: all
	mkdir -p $(OUT_LIB_DIR)
	mkdir -p $(OUT_INCLUDE_DIR)
	cp liblizard.a liblizard.so $(OUT_LIB_DIR)/
	cp $(LIB_DIR)/*.h $(OUT_INCLUDE_DIR)/
	cp lizard.h $(PREFIX)/include/

uninstall:
	rm -f $(OUT_LIB_DIR)/liblizard.a $(OUT_LIB_DIR)/liblizard.so
	rm -rf $(OUT_INCLUDE_DIR)
	rm -f $(PREFIX)/include/lizard.h

clean:
	rm -f $(OBJ_FILES) $(OBJ_FILES_SAFE) liblizard.a liblizard.so
