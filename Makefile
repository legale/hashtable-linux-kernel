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
SRC_LIB = hashtable.c deque.c
SRC_BIN = main.c
SRC = $(SRC_LIB) $(SRC_BIN)

all: $(NAME) static shared

$(NAME): $(SRC)
		mkdir -p build
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(LIB) $^ -o build/$(NAME)

staticlib:
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_LIB) -c -o $(BD)/lib$(LIBNAME).a
		#$(AR) rcs build/lib$(LIBNAME).a build/lib$(LIBNAME).o

sharedlib:
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_LIB) -shared -fPIC -o $(BD)/lib$(LIBNAME).so

shared: sharedlib
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_BIN) -L./build -Wl,-Bdynamic -l$(LIBNAME) -o $(BD)/$(NAME)_shared

static: staticlib
		$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDDIRS) $(SRC_BIN) -Wl,-Bstatic -l$(LIBNAME) -Wl,-Bdynamic -o $(BD)/$(NAME)_static

clean:
		rm -rf $(BD)/*