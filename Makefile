# Compiler setup
CC = gcc
CC_MUSL = musl-gcc

AR = ar
CFLAGS += -Wall -Wextra -O3 -Wno-unused-parameter -ffunction-sections -fdata-sections
ifdef LEAKCHECK
CFLAGS += -DLEAKCHECK
endif
ifdef JEMALLOC
LDLIBS += -ljemalloc
endif

I += -I./ -I/usr/include -I$(UNITY_ROOT)/src
I_MUSL = -I./ -I$(shell musl-gcc -print-file-name=include) -I$(UNITY_ROOT)/src
LDLIBS +=
LDDIRS += -L$(BD)

# profiling flags
CFLAGS_PERF += -g

# Output name and build directory
NAME = ht
BD = ./build

# Library and executable setup
LIBNAME = hashtable
SRC_LIB := hashtable.c deque.c assoc_array.c mock_mem_functions.c
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
.PHONY:  all

all: $(NAME) build_dir static static_full shared test

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

static_full: staticlib
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(SRC_BIN) -L$(BD) -l$(LIBNAME) -static -o $(BD)/$(NAME)_staticfull 

# Executable linking with the shared library
shared: sharedlib
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $(SRC_BIN) -L$(BD) -Wl,-Bdynamic -l$(LIBNAME) -o $(BD)/$(NAME)_shared


# code test coverage report generation
coverage: test
	@echo "Generating gcov reports..."
	@for src_file in $(SRC_LIB); do \
		gcov -n -o $(BD) $$src_file | grep -A 1 "File '$$src_file'"; \
	done

	@echo "Generating coverage report..."
	@lcov --capture --directory $(BD) --output-file coverage.info
	@genhtml coverage.info --output-directory coverage	

# Compile and run tests
test: CFLAGS += -fprofile-arcs -ftest-coverage
test: $(TEST_EXECS)
	@for test_exec in $^ ; do \
		echo Running $$test_exec ; \
		$$test_exec ; \
	done

# Rule to make each test executable
$(BD)/%: $(BD)/%.o $(OBJS_LIB) $(UNITY_OBJ)
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -o $@


# Добавляем новый рецепт для сборки с musl
musl: CC = $(CC_MUSL)
musl: I = $(I_MUSL)
musl: all

# Пример для сборки тестов с musl
musl_test: CC = $(CC_MUSL)
musl_test: I = $(I_MUSL)
musl_test: test


perf: CFLAGS += $(CFLAGS_PERF)
perf: all
	@echo "Running performance profiling..."
	@sudo perf record $(BD)/$(NAME)_staticfull 22 0.5 0.000001
	@sudo chmod 666 perf.data*
	@perf report > perf_report.txt
	@echo "Performance report saved to perf_report.txt"

musl_perf: CC = $(CC_MUSL)
musl_perf: I = $(I_MUSL)
musl_perf: CFLAGS += $(CFLAGS_PERF)
musl_perf: all
	@echo "Running performance profiling..."
	@sudo perf record $(BD)/$(NAME)_staticfull 22 0.5 0.000001
	@sudo chmod 666 perf.data*
	@perf report > perf_report_musl.txt
	@echo "Performance report saved to perf_report_musl.txt"


measure_time: static_full
	@echo "Measuring execution time..."
	@for i in $$(seq 1 10); do \
		$(BD)/$(NAME)_staticfull 22 0.4 0.000001 | grep "time passed:"; \
	done

musl_measure_time: CC = $(CC_MUSL)
musl_measure_time: I = $(I_MUSL)
musl_measure_time: static_full
	@echo "Measuring execution time..."
	@for i in $$(seq 1 10); do \
		$(BD)/$(NAME)_staticfull 22 0.4 0.000001 | grep "time passed:"; \
	done


# Clean up build directory and coverage data
clean:
	rm -rf $(BD)/*
	rm -f *.gcda *.gcno *.info
	rm -rf coverage
	rm -rf perf.data*

