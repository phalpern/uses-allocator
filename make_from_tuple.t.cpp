/* make_from_tuple.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2016  Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <make_from_tuple.h>

#include <utility>
#include <tuple>
#include <string>
#include <test_assert.h>

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
        TT Obj1 = std::make_from_tuple<TT>(tpl);
        TEST_ASSERT(exp == Obj1);

        // Test make_from_tuple with rvalue
        TT Obj2 = std::make_from_tuple<TT>(std::move(tplCopy));
        TEST_ASSERT(exp == Obj2);
    }

    {
        TupleType tplCopy(tpl);

        char ObjBuf alignas(TT) [sizeof(TT)];

        // Test uninitialized_construct_from_tuple with lvalue
        TT *pObj1 = reinterpret_cast<TT*>(ObjBuf);
        std::uninitialized_construct_from_tuple(pObj1, tpl);
        TEST_ASSERT(exp == *pObj1);
        pObj1->~TT();

        // Test uninitialized_construct_from_tuple with rvalue
        TT *pObj2 = reinterpret_cast<TT*>(ObjBuf);
        std::uninitialized_construct_from_tuple(pObj2, std::move(tplCopy));
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
    TEST((tuple<int,double,string>{1,2,"three"}     ), ( 1, 2.0, "three"  ));
    TEST((tuple<int,double,const char*>{1,2,"three"}), ( 1, 2.0, "three"  ));
    TEST((tuple<TT>(TT{ 4, 5, "six" }))              , ( 4, 5.0, "six"    ));
    TEST((std::pair<int,int>(4, 5))                  , ( 4, 5.0, ""       ));

    // Test using `forward_as_tuple`, which produces a tuple that cannot be
    // copy-constructed because it contains rvalue references.
    {
        std::string three("three");

        TT exp{ 1, 2, "three" };

        TT Obj = std::make_from_tuple<TT>(
            std::forward_as_tuple(1, 2, std::move(three)));
        TEST_ASSERT(exp == Obj);
        // Move-construction will leave moved-from string empty in
        // any reasonable implementation.
        TEST_ASSERT(three.empty());
    }

    {
        std::string three("three");

        TT exp{ 1, 2, "three" };

        char ObjBuf alignas(TT) [sizeof(TT)];

        // Test uninitialized_construct_from_tuple with rvalue
        TT *pObj = reinterpret_cast<TT*>(ObjBuf);
        std::uninitialized_construct_from_tuple(pObj,
                                std::forward_as_tuple(1, 2, std::move(three)));
        TEST_ASSERT(exp == *pObj);
        // Move-construction will leave moved-from string empty in
        // any reasonable implementation.
        TEST_ASSERT(three.empty());
        pObj->~TT();
    }

    return errorCount();
}
