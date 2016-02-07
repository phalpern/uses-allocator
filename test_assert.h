/* test_assert.h                  -*-C++-*-
 *
 * Copyright (C) 2016  Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_TEST_ASSERT_DOT_H
#define INCLUDED_TEST_ASSERT_DOT_H

#include <iostream>

class TestContext
{
    const TestContext* m_prevContext;
    const char*        m_file;
    int                m_line;
    const char*        m_str;

    static const TestContext* &currContextRef() {
        // Header-only static variable
        static const TestContext* currContext = nullptr;
        return currContext;
    }

public:
    static const TestContext* currContext() { return currContextRef(); }
    
    TestContext(const char* file, int line, const char* str)
        : m_prevContext(currContext())
        , m_file(file), m_line(line), m_str(str) 
    {
        currContextRef() = this;
    }
        
    ~TestContext() { currContextRef() = m_prevContext; }

    const TestContext* prevContext() const { return m_prevContext; }
    const char* file() const { return m_file; }
    unsigned line() const { return m_line; }
    const char* str() const { return m_str; }
};

inline
int& errorCount()
{
    // Header-only static 
    static int errorCount = 0;
    return errorCount;
}

#define TEST_ASSERT(c) do {                                             \
        if (! (c)) {                                                    \
            std::cout << __FILE__ << ':' << __LINE__                    \
                      << ": Assertion failed: " #c << std::endl;        \
            for (const TestContext* ctx = TestContext::currContext();   \
                 ctx; ctx = ctx->prevContext())                         \
                std::cout << ctx->file() << ':' << ctx->line()          \
                          << ":  Context: " << ctx->str() << std::endl; \
            ++errorCount();                                             \
        }                                                               \
    } while (false)

#endif // ! defined(INCLUDED_TEST_ASSERT_DOT_H)
