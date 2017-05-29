/* copy_swap_helper.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <copy_swap_helper.h>

#include <vector>
#include <cstdlib>
#include <cassert>
#include <test_assert.h>

// STL-style test allocator (doesn't actually allocate memory, but that's not
// important for this test).
template <class T>
class MySTLAlloc
{
    int m_id;
public:
    typedef T value_type;

    explicit MySTLAlloc(int id = -1) : m_id(id) { }

    T* allocate(std::size_t) { throw std::bad_alloc(); }
    void deallocate(T*, std::size_t) { }

    // Don't propagate on copy construction
    MySTLAlloc select_on_container_copy_construction() const {
        return MySTLAlloc();
    }

    int id() const { return m_id; }
};

template <class T>
inline bool operator==(const MySTLAlloc<T>& a, const MySTLAlloc<T>& b)
{
    return a.id() == b.id();
}

template <class T>
inline bool operator!=(const MySTLAlloc<T>& a, const MySTLAlloc<T>& b)
{
    return a.id() != b.id();
}

// STL-style test allocator that propagates on container move, copy, and swap
// (doesn't actually allocate memory, but that's not important for this test).
template <class T>
class MyPocAlloc
{
    int m_id;
public:
    typedef T value_type;

    explicit MyPocAlloc(int id = -1) : m_id(id) { }

    T* allocate(std::size_t) { throw std::bad_alloc(); }
    void deallocate(T*, std::size_t) { }

    // Propagate on copy construction
    const MyPocAlloc& select_on_container_copy_construction() const {
        return *this;
    }

    // Propagate on move assignment, copy assignment, and swap.
    // For any reasonable allocator, these three traits will be the same:
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type propagate_on_container_copy_assignment;
    typedef std::true_type propagate_on_container_swap;

    int id() const { return m_id; }
};

template <class T>
inline bool operator==(const MyPocAlloc<T>& a, const MyPocAlloc<T>& b)
{
    return a.id() == b.id();
}

template <class T>
inline bool operator!=(const MyPocAlloc<T>& a, const MyPocAlloc<T>& b)
{
    return a.id() != b.id();
}

class NoAlloc
{
};

template <typename Alloc>
class TestTypeBase {
protected:
    typedef Alloc AllocType; // Used in derived class
    typedef std::allocator_traits<Alloc> AT;
    Alloc m_alloc;
    TestTypeBase() : m_alloc() { }
    TestTypeBase(const Alloc& a) : m_alloc(a) { }
    TestTypeBase(const TestTypeBase& other)
        : m_alloc(AT::select_on_container_copy_construction(other.m_alloc)) { }
    TestTypeBase(TestTypeBase&& other) : m_alloc(other.m_alloc) { }
    void operator=(const TestTypeBase& other) {
        if (AT::propagate_on_container_copy_assignment::value)
            m_alloc = other.m_alloc;
    }
    void operator=(TestTypeBase&& other) {
        if (AT::propagate_on_container_move_assignment::value)
            m_alloc = other.m_alloc;
    }
    void swap(TestTypeBase& other) noexcept {
        using std::swap;
        if (AT::propagate_on_container_swap::value)
            swap(m_alloc, other.m_alloc);
        else
            assert(m_alloc == other.m_alloc);
    }
public:
    typedef Alloc allocator_type;
    Alloc get_allocator() const { return m_alloc; }
};

template <>
class TestTypeBase<NoAlloc>
{
protected:
    typedef NoAlloc AllocType;
    TestTypeBase() { }
    TestTypeBase(const AllocType&) { }
    void swap(TestTypeBase& other) noexcept { }
};

// Template to generate test types.
// Alloc should be one of:
//    NoAlloc          (no allocator)
//    MySTLAlloc<int>  (STL-style allocator)
// Prefix is either
//    false (allocator is supplied at end of ctor args)
//    true  (allocator is supplied after `allocator_arg` at start of ctor args
template <typename Alloc, bool Prefix = false>
class TestType : public TestTypeBase<Alloc>
{
    typedef TestTypeBase<Alloc> Base;
    using typename Base::AllocType;

    typedef typename std::conditional<Prefix,
                                      AllocType, NoAlloc>::type PrefixAlloc;
    typedef typename std::conditional<Prefix,
                                      NoAlloc, AllocType>::type SuffixAlloc;

    int                  m_value;

public:
    TestType() : Base(), m_value(0) { }
    TestType(std::allocator_arg_t, const PrefixAlloc& a)
        : Base(a), m_value(0) { }
    TestType(const SuffixAlloc& a)
        : Base(a), m_value(0) { }

    TestType(int v) : Base(), m_value(v) { }
    TestType(std::allocator_arg_t, const PrefixAlloc& a, int v)
        : Base(a), m_value(v) { }
    TestType(int v, const SuffixAlloc& a)
        : Base(a), m_value(v) { }

    TestType(const TestType& other) : Base(other), m_value(other.m_value) { }
    TestType(std::allocator_arg_t, const PrefixAlloc& a, const TestType& other)
        : Base(a), m_value(other.m_value) { }
    TestType(const TestType& other, const SuffixAlloc& a)
        : Base(a), m_value(other.m_value) { }

    TestType(TestType&& other)
        : Base(std::move(other)), m_value(other.m_value) { other.m_value = -1; }
    TestType(std::allocator_arg_t, const PrefixAlloc& a, TestType&& other)
        : Base(a), m_value(other.m_value) { other.m_value = -1; }
    TestType(TestType&& other, const SuffixAlloc& a)
        : Base(a), m_value(other.m_value) { other.m_value = -1; }

    ~TestType() { }

    void swap(TestType& other) noexcept {
        this->Base::swap(other);
        std::swap(m_value, other.m_value);
    }

    int value() const { return m_value; }
};

template <typename Alloc, bool Prefix = false>
bool operator==(const TestType<Alloc, Prefix>& a,
                const TestType<Alloc, Prefix>& b)
{
    return a.value() == b.value();
}

template <typename Alloc, bool Prefix = false>
bool operator!=(const TestType<Alloc, Prefix>& a,
                const TestType<Alloc, Prefix>& b)
{
    return a.value() != b.value();
}

template <typename Alloc, bool Prefix = false>
void swap(TestType<Alloc, Prefix>& a, TestType<Alloc, Prefix>& b) noexcept
{
    a.swap(b);
}

namespace internal = std::experimental::fundamentals_v3::internal;

int main()
{
    using std::experimental::copy_swap_helper;
    using std::experimental::swap_assign;

    typedef MySTLAlloc<int> IntAlloc;
    IntAlloc A0; // Default
    IntAlloc A1(1);
    IntAlloc A2(2);

    {
        typedef TestType<NoAlloc> Obj;

        // Internals testing
        TEST_ASSERT(! internal::has_get_allocator_v<Obj>);
        TEST_ASSERT(! (internal::uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(! (internal::uses_prefix_allocator_v<Obj, IntAlloc>));

        Obj x(3);
        TEST_ASSERT(3 == x.value());
        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        Obj z(copy_swap_helper(x, TestType<IntAlloc>(A1)));
        TEST_ASSERT(z == x);
        const Obj q(9);
        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        Obj& zr = swap_assign(z, Obj(8));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z == Obj(8));
    }

    {
        typedef TestType<IntAlloc> Obj;

        // Internals testing
        TEST_ASSERT(  internal::has_get_allocator_v<Obj>);
        TEST_ASSERT(  (internal::uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(! (internal::uses_prefix_allocator_v<Obj, IntAlloc>));

        Obj x(3, A1);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(A1 == x.get_allocator());

        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        TEST_ASSERT(A0 == cc.get_allocator()); // No allocator propagation

        Obj ca(x, A2); // Testing TestType extended copy ctor
        TEST_ASSERT(3 == ca.value());
        TEST_ASSERT(A2 == ca.get_allocator());

        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        TEST_ASSERT(A1 == y.get_allocator());

        const Obj q(9, A2);
        Obj z(copy_swap_helper(x, q));
        TEST_ASSERT(z == x);
        TEST_ASSERT(A2 == z.get_allocator());

        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        TEST_ASSERT(A1 == y.get_allocator());

        Obj& zr = swap_assign(z, Obj(8, A1));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z.value() == 8);
        TEST_ASSERT(A2 == z.get_allocator());
    }

    {
        typedef TestType<IntAlloc, true> Obj;

        // Internals testing
        TEST_ASSERT(  internal::has_get_allocator_v<Obj>);
        TEST_ASSERT(! (internal::uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(  (internal::uses_prefix_allocator_v<Obj, IntAlloc>));

        Obj x(std::allocator_arg, A1, 3);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(A1 == x.get_allocator());

        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        TEST_ASSERT(A0 == cc.get_allocator()); // No allocator propagation

        Obj ca(std::allocator_arg,A2,x); // Testing TestType extended copy ctor
        TEST_ASSERT(3 == ca.value());
        TEST_ASSERT(A2 == ca.get_allocator());

        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        TEST_ASSERT(A1 == y.get_allocator());

        const Obj q(std::allocator_arg, A2, 9);
        Obj z(copy_swap_helper(x, q));
        TEST_ASSERT(z == x);
        TEST_ASSERT(A2 == z.get_allocator());

        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        TEST_ASSERT(A1 == y.get_allocator());

        Obj& zr = swap_assign(z, Obj(std::allocator_arg, A1, 8));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z.value() == 8);
        TEST_ASSERT(A2 == z.get_allocator());
    }

    // Test propagating allocator with `copy_swap` and `move_swap`
    {
        typedef MyPocAlloc<int> IntPocAlloc;
        typedef TestType<IntPocAlloc> Obj;

        // Internals testing
        TEST_ASSERT(  internal::has_get_allocator_v<Obj>);
        TEST_ASSERT(  (internal::uses_suffix_allocator_v<Obj, IntPocAlloc>));
        TEST_ASSERT(! (internal::uses_prefix_allocator_v<Obj, IntPocAlloc>));

        IntPocAlloc PA0;
        IntPocAlloc PA1(1);
        IntPocAlloc PA2(2);

        Obj x(3, PA1);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(PA1 == x.get_allocator());

        Obj y(4, PA2);
        TEST_ASSERT(4 == y.value());
        TEST_ASSERT(PA2 == y.get_allocator());

        Obj q(9, PA2);
        Obj &rx = swap_assign(x, q);
        TEST_ASSERT(&rx == &x);
        TEST_ASSERT(x == q);
        TEST_ASSERT(PA2 == x.get_allocator());

        Obj &ry = swap_assign(y, Obj(8, PA1));
        TEST_ASSERT(&ry == &y);
        TEST_ASSERT(y.value() == 8);
        TEST_ASSERT(PA1 == y.get_allocator());
    }

    return errorCount();
}
