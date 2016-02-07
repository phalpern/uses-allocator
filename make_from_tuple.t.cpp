/* make_from_tuple.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2016  Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <make_from_tuple.h>

#include <utility>
#include <string>
#include <test_assert.h>

namespace exp = std::experimental;

class TestType
{
    std::tuple<int, double, std::string> m_data;

public:
    TestType() : m_data(0, 0.0, "") { }

    template <class T1> explicit TestType(T1&& v1)
        : m_data(std::forward<T1>(v1), 0.0, "") { }
    template <class T1, class T2> TestType(T1&& v1, T2&& v2)
        : m_data(std::forward<T1>(v1), std::forward<T2>(v2), "") { }
    template <class T1, class T2, class T3> TestType(T1&& v1, T2&& v2, T3&& v3)
        : m_data(std::forward<T1>(v1), std::forward<T2>(v2),
                 std::forward<T3>(v3)) { }

    friend bool operator==(const TestType& a, const TestType& b);
};

bool operator==(const TestType& a, const TestType& b)
{
    return a.m_data == b.m_data;
}

bool operator!=(const TestType& a, const TestType& b)
{
    return ! (a == b);
}

template <class TupleType>
void runTest(const TupleType& tpl, const TestType& exp)
{
    typedef TestType TT;
    
    {
        TupleType tplCopy(tpl);

        // Test make_from_tuple with lvalue
        TT Obj1 = exp::make_from_tuple<TT>(tpl);
        TEST_ASSERT(exp == Obj1);

        // Test make_from_tuple with rvalue
        TT Obj2 = exp::make_from_tuple<TT>(std::move(tplCopy));
        TEST_ASSERT(exp == Obj2);
    }

    {
        TupleType tplCopy(tpl);

        char ObjBuf alignas(TT) [sizeof(TT)];

        // Test uninitialized_construct_from_tuple with lvalue
        TT *pObj1 = reinterpret_cast<TT*>(ObjBuf);
        exp::uninitialized_construct_from_tuple(pObj1, tpl);
        TEST_ASSERT(exp == *pObj1);
        pObj1->~TT();

        // Test uninitialized_construct_from_tuple with rvalue
        TT *pObj2 = reinterpret_cast<TT*>(ObjBuf);
        exp::uninitialized_construct_from_tuple(pObj2, std::move(tplCopy));
        TEST_ASSERT(exp == *pObj2);
        pObj2->~TT();
    }
}

int main()
{
    using std::string;

    typedef TestType TT;

#define TEST(tpl, exp) do {                                                   \
        TestContext tc(__FILE__, __LINE__, "tuple = " #tpl ", exp = " #exp ); \
        runTest(tpl, TT exp);                                                 \
    } while (false)

    using std::tuple;

    TEST((tuple<>{}                                 ), ( 0, 0.0, ""       ));
    TEST((tuple<int>{1}                             ), ( 1, 0.0, ""       ));
    TEST((tuple<int,float>{1, 2}                    ), ( 1, 2.0, ""       ));
    TEST((tuple<int,double,string>{1,2,"hello"}     ), ( 1, 2.0, "hello"  ));
    TEST((tuple<int,double,const char*>{1,2,"hello"}), ( 1, 2.0, "hello"  ));

    // Test using `forward_as_tuple`, which cannot be copy-constructed
    {
        std::string hello("hello");

        TT exp{ 1, 2, "hello" };

        TT Obj = exp::make_from_tuple<TT>(
            std::forward_as_tuple(1, 2, std::move(hello)));
        TEST_ASSERT(exp == Obj);
    }

    {
        std::string hello("hello");

        TT exp{ 1, 2, "hello" };

        char ObjBuf alignas(TT) [sizeof(TT)];

        // Test uninitialized_construct_from_tuple with rvalue
        TT *pObj = reinterpret_cast<TT*>(ObjBuf);
        exp::uninitialized_construct_from_tuple(pObj,
                                std::forward_as_tuple(1, 2, std::move(hello)));
        TEST_ASSERT(exp == *pObj);
        pObj->~TT();
    }
    
    return errorCount();
}

