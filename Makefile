
CXX = clang++
OPT = -g -fno-inline
CXXFLAGS = $(OPT) -std=c++14 -I.

TARGETS=copy_swap_helper make_from_tuple uses_allocator

.PRECIOUS: %.t

all: $(TARGETS)

% : %.t
	./$<

%.t : %.t.cpp %.h test_assert.h
	$(CXX) $(CXXFLAGS) $< -o $@

uses_allocator.t :: make_from_tuple.h

clean:
	rm -rf $(TARGETS:=.t) $(TARGETS:=.o) $(TARGETS:=.t.dSYM) clean
