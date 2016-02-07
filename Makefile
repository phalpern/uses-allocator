
CXX = clang++
OPT = -g -fno-inline
CXXFLAGS = $(OPT) -std=c++14 -I.

TARGETS=copy_swap_helper uses_allocator

.PRECIOUS: %.t

all: $(TARGETS)

% : %.t
	./$<

%.t : %.t.cpp %.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(TARGETS:=.t) $(TARGETS:=.o) $(TARGETS:=.t.dSYM) clean
