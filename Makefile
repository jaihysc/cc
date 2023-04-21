# NOTE: Do not call make to build, use build.sh to setup the correct directories
SRCDIR=src
TESTDIR=testu
OBJDIR=out
OUTDIR=out

SRC_CFLAGS:=${cc_flags} -I $(SRCDIR)
TEST_CFLAGS=-g -Wall -Wextra -I $(SRCDIR) -I $(TESTDIR)

SRCDEPS=$(SRCDIR)/*.h
TESTDEPS=$(TESTDIR)/*.h

SRCOBJ=$(addprefix $(OBJDIR)/$(SRCDIR)/, \
	cfg.o errorcode.o globals.o il2gen.o il2statement.o lexer.o parser.o symbol.o symtab.o tree.o type.o vec.o)
TESTOBJ=$(addprefix $(OBJDIR)/$(TESTDIR)/, \
	testu.o CuTest.o cfg_test.o lexer_test.o parser_test.o symbol_test.o symtab_test.o tree_test.o type_test.o)

$(OBJDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c $(SRCDEPS)
	$(CC) $(SRC_CFLAGS) -c -o $@ $<

$(OBJDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.c $(SRCDEPS) $(TESTDEPS)
	$(CC) $(TEST_CFLAGS) -c -o $@ $<

all: $(OUTDIR)/parse $(OUTDIR)/asmgen $(OUTDIR)/unittest

$(OUTDIR)/parse: $(SRCOBJ) $(OBJDIR)/$(SRCDIR)/main.o
	$(CC) $(SRC_CFLAGS) -o $@ $^

$(OUTDIR)/asmgen: $(SRCDIR)/asmgen/asm_gen.c $(OBJDIR)/$(SRCDIR)/type.o $(OBJDIR)/$(SRCDIR)/vec.o
	$(CC) $(SRC_CFLAGS) -o $@ $^

$(OUTDIR)/unittest: $(SRCOBJ) $(TESTOBJ)
	$(CC) $(TEST_CFLAGS) -o $@ $^
