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
_TESTBIN := test-basic test-arr test-sched
_TESTBIN := $(addprefix ./bin/,$(_TESTBIN))
$(_TESTBIN): $(BINDIR)/%: test/%.c $(LIBSTATIC)
	$(CC) -o $@ $(CFLAGS) $^
buildtests: $(_TESTBIN)

# --- generic tools ---
clean:
	rm -f $(LIBOBJS) $(_TESTBIN)

clean_all: clean
	rm -f $(LIBSTATIC) $(LIBSHARED)

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