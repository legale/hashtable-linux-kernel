#name of output file
NAME = ht
#build dir
BD = ./build

#Linker flags
LDLIBS +=
LDDIRS += -L$(BD)

#Compiler flags
CFLAGS += -Wall -Wextra -O2 -Wno-unused-parameter
I += -I./ -I/usr/include -I$(UNITY_ROOT)/src

#Compiler
CC = gcc -ggdb
AR = ar

LIBNAME = hashtable
SRC_LIB := hashtable.c deque.c mactable.c
SRC_BIN := main.c
SRC := $(SRC_LIB) $(SRC_BIN)
OBJS := $(SRC:%.c=$(BD)/%.o)
OBJS_LIB := $(SRC_LIB:%.c=$(BD)/%.o)

# Test setup
UNITY_ROOT = ./unity
TEST_SRC = test/test_create_hashtable.c
UNITY_SRC = $(UNITY_ROOT)/src/unity.c
UNITY_OBJ = $(UNITY_SRC:%.c=$(BD)/%.o)
TEST_OBJS = $(TEST_SRC:%.c=$(BD)/%.o)
TEST_EXEC = $(BD)/run_tests

all: $(NAME) static shared test
.PHONY: all clean test

build_dir:
	mkdir -p $(BD)
	mkdir -p $(BD)/test
	mkdir -p $(BD)/unity/src

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -o $(BD)/$(NAME)

$(BD)/%.o: %.c build_dir
	$(CC) $(CFLAGS) $(I) -I$(UNITY_ROOT) $(LDDIRS) $(LDLIBS) -c $< -o $@

staticlib: $(OBJS_LIB)
	$(AR) rcs $(BD)/lib$(LIBNAME).a $^

sharedlib:
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_LIB) -shared -fPIC -o $(BD)/lib$(LIBNAME).so

shared: sharedlib
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_BIN) -L$(BD) -Wl,-Bdynamic -l$(LIBNAME) -o $(BD)/$(NAME)_shared

static: staticlib
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(SRC_BIN) -L$(BD) -Wl,-Bstatic -l$(LIBNAME) -Wl,-Bdynamic -o $(BD)/$(NAME)_static

test: $(TEST_EXEC)
	$(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJS) $(OBJS_LIB) $(UNITY_OBJ)
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -o $@

clean:
	rm -rf $(BD)/*
