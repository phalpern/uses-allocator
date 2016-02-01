
CXX = clang++
OPT = -g -fno-inline
CXXFLAGS = $(OPT) -std=c++14 -I.

TARGETS=copy_swap_helper

.PRECIOUS: %.t

all: $(TARGETS)

% : %.t
	./$<

%.t : %.t.cpp uses_allocator.h copy_swap_helper.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(TARGETS:=.t) $(TARGETS:=.o) $(TARGETS:=.t.dSYM) clean
