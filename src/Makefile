#
# hcxselect - A CSS selector engine for htmlcxx
#

LEX = flex
CXXFLAGS += -Wall -Wno-unused-function -g
CXXFLAGS += $(shell pkg-config --cflags htmlcxx)
LFLAGS += --reentrant

all: lib

.cpp.o: lexer.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

lexer.h: lexer.l
	$(LEX) $(LFLAGS) -o $@ $^

libhcxselect.a: hcxselect.o
	$(AR) rcs libhcxselect.a hcxselect.o

lib: lexer.h libhcxselect.a

clean:
	$(RM) *.o *.a lexer.h
