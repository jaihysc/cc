# NOTE: Do not call make to build, use build.sh to setup the correct directories
SRCDIR=src
TESTDIR=testu
OBJDIR=out

CFLAGS=-g -Wall -Wextra -I $(SRCDIR) -I $(TESTDIR)

SRCDEPS=$(SRCDIR)/*.h
TESTDEPS=$(TESTDIR)/*.h

SRCOBJ=$(addprefix $(OBJDIR)/$(SRCDIR)/, \
	globals.o lexer.o parser.o symbol.o symtab.o tree.o type.o)
TESTOBJ=$(addprefix $(OBJDIR)/$(TESTDIR)/, \
	testu.o CuTest.o lexer_test.o parser_test.o symbol_test.o symtab_test.o tree_test.o type_test.o)

$(OBJDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c $(SRCDEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.c $(SRCDEPS) $(TESTDEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

unittest: $(SRCOBJ) $(TESTOBJ)
	$(CC) $(CFLAGS) -o $@ $^
