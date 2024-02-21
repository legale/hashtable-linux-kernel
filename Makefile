# Compiler setup
CC = gcc -ggdb
AR = ar
CFLAGS += -Wall -Wextra -O2 -Wno-unused-parameter
ifdef LEAKCHECK
CFLAGS += -DLEAKCHECK
endif
I += -I./ -I/usr/include -I$(UNITY_ROOT)/src
LDLIBS +=
LDDIRS += -L$(BD)

# Output name and build directory
NAME = ht
BD = ./build

# Library and executable setup
LIBNAME = hashtable
SRC_LIB := hashtable.c deque.c assoc_array.c
SRC_BIN := main.c
ifdef LEAKCHECK
SRC_BIN += leak_detector_c.c
endif
SRC := $(SRC_LIB) $(SRC_BIN)
OBJS := $(SRC:%.c=$(BD)/%.o)
OBJS_LIB := $(SRC_LIB:%.c=$(BD)/%.o)

# Test setup
UNITY_ROOT = ./unity
TEST_SRCS := test/test_deque.c test/test_hashtable.c test/test_assoc_array.c test/test_assoc_array_net_data.c
UNITY_SRC := $(UNITY_ROOT)/src/unity.c
UNITY_OBJ := $(UNITY_SRC:%.c=$(BD)/%.o)
TEST_OBJS := $(TEST_SRCS:%.c=$(BD)/%.o)
TEST_EXECS := $(TEST_SRCS:%.c=$(BD)/%)

# Phony targets for standard make commands
.PHONY: all clean test static shared staticlib sharedlib build_dir

all: $(NAME) static shared test

# Create build directories
build_dir:
	@mkdir -p $(BD)
	@mkdir -p $(BD)/test
	@mkdir -p $(BD)/unity/src

# Main executable
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -o $(BD)/$(NAME)

# Compile .c to .o
$(BD)/%.o: %.c build_dir
	$(CC) $(CFLAGS) $(I) -I$(UNITY_ROOT) $(LDDIRS) $(LDLIBS) -c $< -o $@

# Static library
staticlib: $(OBJS_LIB)
	$(AR) rcs $(BD)/lib$(LIBNAME).a $^

# Shared library
sharedlib:
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_LIB) -shared -fPIC -o $(BD)/lib$(LIBNAME).so

# Executable linking with the static library
static: staticlib
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(SRC_BIN) -L$(BD) -Wl,-Bstatic -l$(LIBNAME) -Wl,-Bdynamic -o $(BD)/$(NAME)_static

# Executable linking with the shared library
shared: sharedlib
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_BIN) -L$(BD) -Wl,-Bdynamic -l$(LIBNAME) -o $(BD)/$(NAME)_shared

# Compile and run tests
test: $(TEST_EXECS)
	@for test_exec in $^ ; do \
		echo Running $$test_exec ; \
		$$test_exec ; \
	done

# Rule to make each test executable
$(BD)/%: $(BD)/%.o $(OBJS_LIB) $(UNITY_OBJ)
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -o $@

# Clean up build directory
clean:
	rm -rf $(BD)/*
