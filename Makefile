#name of output file
NAME = ht
#build dir
BD = ./build

#Linker flags
LDLIBS +=
LDDIRS += -L$(BD)

#Compiler flags
# -w to remove warnings
CFLAGS += -Wall -O2
I += -I./ -I/usr/include
LIB += # -lpcap

#Compiler
CC = gcc -ggdb
AR = ar

#SRC=$(wildcard *.c)
LIBNAME = hashtable
SRC_LIB := hashtable.c deque.c
SRC_BIN := main.c
SRC := $(SRC_LIB) $(SRC_BIN)
OBJS := $(SRC:%.c=$(BD)/%.o)
OBJS_LIB := $(SRC_LIB:%.c=$(BD)/%.o)

all: $(NAME) static shared
.PHONY: all clean

build_dir:
	mkdir -p $(BD)

$(NAME): $(OBJS) 
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(LIB) $^ -o build/$(NAME)

$(BD)/%.o: %.c build_dir
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $< -c -o $@


staticlib: $(OBJS_LIB)
		$(AR) rcs $(BD)/lib$(LIBNAME).a $^ 

sharedlib:
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_LIB) -shared -fPIC -o $(BD)/lib$(LIBNAME).so

shared: sharedlib
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_BIN) -L./build -Wl,-Bdynamic -l$(LIBNAME) -o $(BD)/$(NAME)_shared

static: staticlib
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDDIRS) $(SRC_BIN) -Wl,-Bstatic -l$(LIBNAME) -Wl,-Bdynamic -o $(BD)/$(NAME)_static

clean:
		rm -rf $(BD)/*