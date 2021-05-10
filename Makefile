AR := ar
CC ?= cc
AS := as

CFLAGS := -g -O0 -std=c99 -Wall -Wextra
ARFLAGS := rcs

PROJ_NAME := coro

INCDIR := include
SRCDIR := src
OBJDIR := objs
LIBDIR := lib
BINDIR := bin

LIBNAME := lib$(PROJ_NAME)
LIBSTATIC := $(LIBDIR)/$(LIBNAME).a
LIBSHARED := $(LIBDIR)/$(LIBNAME).so

CFLAGS += $(addprefix -I,$(INCDIR))

# > mostly project-specific.
SRCS := coro.c
SRCDST := $(addprefix $(SRCDIR)/,$(SRCS))
SRCOBJ := $(patsubst %.c,%.o,$(SRCS))
SRCOBJ := $(addprefix $(OBJDIR)/,$(SRCOBJ))

ASMS := coro_s.S
ASMDST := $(addprefix $(SRCDIR)/,$(ASMS))
ASMOBJ := $(patsubst %.S,%.o,$(ASMS))
ASMOBJ := $(addprefix $(OBJDIR)/,$(ASMOBJ))

LIBOBJS := $(SRCOBJ) $(ASMOBJ)

TESTDIR := test
TESTS := test-basic.c
TESTDST := $(addprefix $(TESTDIR)/,$(TESTS))
TESTOBJ := $(patsubst %.c,%.o,$(TESTS))
TESTOBJ := $(addprefix $(OBJDIR)/,$(TESTOBJ))

TESTBIN := test-coro


.PHONY: static shared clean depends testbin runtest

$(OBJDIR):
	mkdir -p $(OBJDIR) $(BINDIR) $(LIBDIR)

$(SRCOBJ): $(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -c -o $@ $(CFLAGS) $^

$(ASMOBJ): $(OBJDIR)/%.o: $(SRCDIR)/%.S | $(OBJDIR)
	$(CC) -c -o $@ $(CFLAGS) $^

$(LIBSTATIC): $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $^

static: $(LIBSTATIC)

$(LIBSHARED): CFLAGS += -fPIC
$(LIBSHARED): $(LIBOBJS)
	$(CC) -o $@ $(CFLAGS) -shared $(LIBOBJS)
shared: $(LIBSHARED)

# --- test ---
$(TESTOBJ): $(OBJDIR)/%.o: $(TESTDIR)/%.c | $(OBJDIR)
	$(CC) -c -o $@ $(CFLAGS) $^
$(BINDIR)/$(TESTBIN): $(TESTOBJ) $(LIBSTATIC)
	$(CC) -o $@ $(CFLAGS) $^
testbin: $(BINDIR)/$(TESTBIN)

runtest: $(BINDIR)/$(TESTBIN)
	./$(BINDIR)/$(TESTBIN)

# TODO: Study how to build multiple tests.
_TESTBIN := test-basic test-arr test-sched
_TESTBIN := $(addprefix ./bin/,$(_TESTBIN))
$(_TESTBIN): $(BINDIR)/%: test/%.c $(LIBSTATIC)
	$(CC) -o $@ $(CFLAGS) $^
buildtests: $(_TESTBIN)

#TESTSRC := test-basic.c test-arr.c test-sched.c
#$(BINDIR)/test-basic: $(LIBSTATIC)
#	$(CC) -o $@ $(CFLAGS) test/test-basic.c $(LIBSTATIC)
#$(BINDIR)/test-arr: $(LIBSTATIC)
#	$(CC) -o $@ $(CFLAGS) test/test-arr.c $(LIBSTATIC)
#$(BINDIR)/test-sched: $(LIBSTATIC)
#	$(CC) -o $@ $(CFLAGS) test/test-sched.c $(LIBSTATIC)

# --- generic tools ---
clean:
	rm -f $(LIBOBJS) $(TESTOBJ) $(TESTBIN) $(_TESTBIN)

# --- DO NOT MODIFY MANUALLY! ---
# > To update, use make command below:
depends:
	$(CC) -MM -I$(INCDIR) $(SRCDIR)/* $(TESTDIR)/*

# --- machine generated ---
coro.o: src/coro.c include/coro.h
coro_s.o: src/coro_s.S
test-arr.o: test/test-arr.c include/coro.h
test-basic.o: test/test-basic.c include/coro.h
test-sched.o: test/test-sched.c include/coro.h include/list.h