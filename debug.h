#ifndef INCLUDED_DEBUG_DOT_H
#define INCLUDED_DEBUG_DOT_H

// Force a warning when instantiated, if C is false (i.e., like an assertion
// failure.  Optionally, give a unique ID to distinguish the instantiation
// from others on the same type.
template <class T, bool C = false, int id = 0> struct PrintType
{
    static char getChar() { return 'a'; }
    static void ignore(void*) { }
    PrintType(const char* = 0) { ignore((void*) getChar()); }
};

template <class T, int id>
struct PrintType<T, true, id> {
    PrintType(const char* = 0) { }
};

#endif // ! defined(INCLUDED_DEBUG_DOT_H)
