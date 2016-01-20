
CXX = clang++
OPT = -g
CXXFLAGS = $(OPT) -std=c++14 -I.

TARGETS=uses_allocator

all: $(TARGETS)

uses_allocator : uses_allocator.t
	./$<

uses_allocator.t : uses_allocator.t.cpp uses_allocator.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(TARGETS:=.t) $(TARGETS:=.o) $(TARGETS:=.t.dSYM) clean
