
CXX ?= clang++
OPT = -g -fno-inline
CXXFLAGS = $(OPT) -std=c++14 -I.

TARGETS=copy_swap_helper make_from_tuple uses_allocator

.PRECIOUS: %.t

all: $(TARGETS)

% : %.t
	./$<

%.t : %.t.cpp %.h test_assert.h
	$(CXX) $(CXXFLAGS) $< -o $@

%.html : %.md
	pandoc --number-sections -s -S $< -o $@

%.pdf : %.md ../header.tex
	$(eval DOCNUM=$(shell sed -n  -e '/^% [NDP][0-9x][0-9x][0-9x][0-9x]/s/^% \([NDP][0-9x][0-9x][0-9x][0-9x]\([Rr][0-9]\+\)\?\).*/\1/p' -e '/^[NDP][0-9x][0-9x][0-9x][0-9x]\([Rr][0-9]\+\)\?$$/q' $<))
	sed -e s/DOCNUM/$(DOCNUM)/g ../header.tex > $*.hdr.tex
	pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H $*.hdr.tex $< -o $@
	rm -f $*.hdr.tex

%.docx : %.md
	pandoc --number-sections -s -S $< -o $@

%.clean: %.md
	mkdir -p old
	mv $*.[hpd][tdo][mfc]* old  # Move .html, .pdf, and .docx files to old

uses_allocator.t :: make_from_tuple.h

clean:
	rm -rf $(TARGETS:=.t) $(TARGETS:=.o) $(TARGETS:=.t.dSYM) clean
