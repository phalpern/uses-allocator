
CXX ?= clang++
OPT = -g -fno-inline
CXXFLAGS = $(OPT) -std=c++14 -I.
WD := $(shell basename $(PWD))

TARGETS=copy_swap_transaction make_from_tuple uses_allocator

.PRECIOUS: %.t

all: $(TARGETS)

% : %.t
	./$<

%.t : %.t.cpp %.h test_assert.h
	$(CXX) $(CXXFLAGS) $< -o $@

%.pdf : %.md
	cd .. && make $(WD)/$@

%.html : %.md
	cd .. && make $(WD)/$@

%.docx : %.md
	pandoc --number-sections -s -S $< -o $@

%.clean: %.md
	mkdir -p old
	mv $*.[hpd][tdo][mfc]* old  # Move .html, .pdf, and .docx files to old

uses_allocator.t :: make_from_tuple.h

clean:
	rm -rf $(TARGETS:=.t) $(TARGETS:=.o) $(TARGETS:=.t.dSYM) clean
