// copy_swap_helper.t.cpp                  -*-C++-*-

#include <copy_swap_helper.h>

#include <iostream>
#include <vector>
#include <cstdlib>

namespace std {
namespace experimental {
inline namespace fundamentals_v2 {

// Types from existing TS draft
struct erased_type { };
namespace pmr {

struct memory_resource
{
    bool is_equal(const memory_resource& other) const noexcept
        { return do_is_equal(other); }
protected:
    virtual bool do_is_equal(const memory_resource& other) const noexcept = 0;
};

bool operator==(const memory_resource& a, const memory_resource& b)
{
    return a.is_equal(b);
}

bool operator!=(const memory_resource& a, const memory_resource& b)
{
    return ! (a == b);
}

} // close namespace pmr
} // close namespace fundamentals_v2
} // close namespace experimental
} // close namespace std
    
using namespace std::experimental::fundamentals_v3::internal;
namespace pmr = std::experimental::pmr;

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

// Memory resource (doesn't meet requirements in LFTS2, but that's not
// important for this test).
class MyMemResource : public pmr::memory_resource
{
    int m_id;
public:
    explicit MyMemResource(int id = -1) : m_id(id) { }

    int id() const { return m_id; }

protected:
    virtual bool do_is_equal(const pmr::memory_resource& other) const noexcept;
};

static MyMemResource DefaultResource;

bool 
MyMemResource::do_is_equal(const pmr::memory_resource& other) const noexcept
{
    const MyMemResource *pOther = dynamic_cast<const MyMemResource *>(&other);
    return pOther && pOther->id() == this->id();
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
    void swap(TestTypeBase& other) {
        using std::swap;
        if (AT::propagate_on_container_swap::value)
            swap(m_alloc, other.m_alloc);
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
    void swap(TestTypeBase& other) { }
};

template <> 
class TestTypeBase<pmr::memory_resource*>
{
protected:
    typedef pmr::memory_resource* AllocType;
    AllocType m_alloc;
    TestTypeBase() : m_alloc(&DefaultResource) { }
    TestTypeBase(const AllocType& a) : m_alloc(a) { }
    TestTypeBase(const TestTypeBase& other) : m_alloc(&DefaultResource) { }
    TestTypeBase(TestTypeBase&& other) : m_alloc(other.m_alloc) { }
    void operator=(const TestTypeBase& other) { }
    void operator=(TestTypeBase&& other) { }
    void swap(TestTypeBase& other) { }
public:
    typedef AllocType allocator_type;
    AllocType get_memory_resource() const { return m_alloc; }
};

template <> 
class TestTypeBase<std::experimental::erased_type>
{
protected:
    // Erased the allocator type
    struct AllocEraser {
        MyMemResource         m_resource;
        pmr::memory_resource* m_resource_p;

        AllocEraser() : m_resource_p(&DefaultResource) { }
        template <typename A> AllocEraser(const A& a)
            : m_resource(a.id()), m_resource_p(&m_resource) { }
        AllocEraser(pmr::memory_resource* const &r)
            : m_resource_p(r) { }
        AllocEraser(const AllocEraser& other)
            : m_resource(other.m_resource) 
        {
            if (other.m_resource_p == &other.m_resource)
                m_resource_p = &m_resource;
            else
                m_resource_p = other.m_resource_p;
        }
    };

    typedef AllocEraser AllocType;
    AllocType m_alloc;
    TestTypeBase() : m_alloc() { }
    TestTypeBase(const AllocType& a) : m_alloc(a) { }
    TestTypeBase(const TestTypeBase& other) : m_alloc() { }
    TestTypeBase(TestTypeBase&& other) : m_alloc(other.m_alloc) { }
    void operator=(const TestTypeBase& other) { }
    void operator=(TestTypeBase&& other) { }
    void swap(TestTypeBase& other) { }

public:
    typedef AllocType allocator_type;
    pmr::memory_resource* get_memory_resource() const
        { return m_alloc.m_resource_p; }
};

// Template to generate test types.
// Alloc should be one of:
//    NoAlloc          (no allocator)
//    MySTLAlloc<int>  (STL-style allocator)
//    memory_resource* (polymorphic resource)
//    erased_type      (type-erased allocator/resource)
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

    void swap(TestType& other) {
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
void swap(TestType<Alloc, Prefix>& a, TestType<Alloc, Prefix>& b)
{
    a.swap(b);
}

static int errorCount = 0;
#define TEST_ASSERT(c) do {                                             \
        if (! (c)) {                                                    \
            std::cout << __FILE__ << ':' << __LINE__                    \
                      << ": Assertion failed: " #c << std::endl;        \
            ++errorCount;                                               \
        }                                                               \
    } while (false)

int main()
{
    using std::experimental::copy_swap_helper;
    using std::experimental::swap_assign;

    typedef MySTLAlloc<int> IntAlloc;
    typedef pmr::memory_resource* pmr_ptr;
    IntAlloc A0; // Default
    IntAlloc A1(1);
    IntAlloc A2(2);
    MyMemResource& R0 = DefaultResource;
    MyMemResource R1(1), *pR1 = &R1;
    MyMemResource R2(2), *pR2 = &R2;

    {
        typedef TestType<NoAlloc> Obj;
        TEST_ASSERT(! has_get_allocator_v<Obj>);
        TEST_ASSERT(! has_get_memory_resource_v<Obj>);
        TEST_ASSERT(! (uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(! (uses_prefix_allocator_v<Obj, IntAlloc>));

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
        TEST_ASSERT(  has_get_allocator_v<Obj>);
        TEST_ASSERT(! has_get_memory_resource_v<Obj>);
        TEST_ASSERT(  (uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(! (uses_prefix_allocator_v<Obj, IntAlloc>));

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
        typedef TestType<pmr_ptr> Obj;
        TEST_ASSERT(! has_get_allocator_v<Obj>);
        TEST_ASSERT(  has_get_memory_resource_v<Obj>);
        TEST_ASSERT(  (uses_suffix_allocator_v<Obj, pmr_ptr>));
        TEST_ASSERT(! (uses_prefix_allocator_v<Obj, pmr_ptr>));

        Obj x(3, pR1);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(pR1 == x.get_memory_resource());

        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        TEST_ASSERT(R0 == *cc.get_memory_resource()); // No resource propagation
        
        Obj ca(x, pR2); // Testing TestType extended copy ctor
        TEST_ASSERT(3 == ca.value());
        TEST_ASSERT(pR2 == ca.get_memory_resource());
        
        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        TEST_ASSERT(pR1 == y.get_memory_resource());

        const Obj q(9, pR2);
        Obj z(copy_swap_helper(x, q));
        TEST_ASSERT(z == x);
        TEST_ASSERT(pR2 == z.get_memory_resource());

        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        TEST_ASSERT(pR1 == y.get_memory_resource());

        Obj& zr = swap_assign(z, Obj(8, pR1));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z.value() == 8);
        TEST_ASSERT(pR2 == z.get_memory_resource());
    }
    
    {
        typedef TestType<std::experimental::erased_type> Obj;
        TEST_ASSERT(! has_get_allocator_v<Obj>);
        TEST_ASSERT(  has_get_memory_resource_v<Obj>);
        TEST_ASSERT(  (uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(! (uses_prefix_allocator_v<Obj, IntAlloc>));

        Obj x(3, A1);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(R1 == *x.get_memory_resource());

        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        TEST_ASSERT(R0 == *cc.get_memory_resource()); // No resource propagation
        
        Obj ca(x, A2); // Testing TestType extended copy ctor
        TEST_ASSERT(3 == ca.value());
        TEST_ASSERT(R2 == *ca.get_memory_resource());
        
        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        TEST_ASSERT(R1 == *y.get_memory_resource());

        const Obj q(9, A2);
        Obj z(copy_swap_helper(x, q));
        TEST_ASSERT(z == x);
        TEST_ASSERT(R2 == *z.get_memory_resource());

        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        TEST_ASSERT(R1 == *y.get_memory_resource());

        Obj& zr = swap_assign(z, Obj(8, A1));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z.value() == 8);
        TEST_ASSERT(R2 == *z.get_memory_resource());
    }
    
    {
        typedef TestType<IntAlloc, true> Obj;
        TEST_ASSERT(  has_get_allocator_v<Obj>);
        TEST_ASSERT(! has_get_memory_resource_v<Obj>);
        TEST_ASSERT(! (uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(  (uses_prefix_allocator_v<Obj, IntAlloc>));

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
    
    {
        typedef TestType<pmr_ptr, true> Obj;
        TEST_ASSERT(! has_get_allocator_v<Obj>);
        TEST_ASSERT(  has_get_memory_resource_v<Obj>);
        TEST_ASSERT(! (uses_suffix_allocator_v<Obj, pmr_ptr>));
        TEST_ASSERT(  (uses_prefix_allocator_v<Obj, pmr_ptr>));

        Obj x(std::allocator_arg, pR1, 3);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(pR1 == x.get_memory_resource());

        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        TEST_ASSERT(R0 == *cc.get_memory_resource()); // No resource propagation
        
        Obj ca(std::allocator_arg,pR2,x); // Testing TestType extended copy ctor
        TEST_ASSERT(3 == ca.value());
        TEST_ASSERT(pR2 == ca.get_memory_resource());
        
        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        TEST_ASSERT(pR1 == y.get_memory_resource());

        const Obj q(std::allocator_arg, pR2, 9);
        Obj z(copy_swap_helper(x, q));
        TEST_ASSERT(z == x);
        TEST_ASSERT(pR2 == z.get_memory_resource());

        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        TEST_ASSERT(pR1 == y.get_memory_resource());

        Obj& zr = swap_assign(z, Obj(std::allocator_arg, pR1, 8));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z.value() == 8);
        TEST_ASSERT(pR2 == z.get_memory_resource());
    }
    
    {
        typedef TestType<std::experimental::erased_type, true> Obj;
        TEST_ASSERT(! has_get_allocator_v<Obj>);
        TEST_ASSERT(  has_get_memory_resource_v<Obj>);
        TEST_ASSERT(! (uses_suffix_allocator_v<Obj, IntAlloc>));
        TEST_ASSERT(  (uses_prefix_allocator_v<Obj, IntAlloc>));

        Obj x(std::allocator_arg, A1, 3);
        TEST_ASSERT(3 == x.value());
        TEST_ASSERT(R1 == *x.get_memory_resource());

        Obj cc(x);  // Testing TestType copy constructor
        TEST_ASSERT(3 == cc.value());
        TEST_ASSERT(R0 == *cc.get_memory_resource()); // No resource propagation
        
        Obj ca(std::allocator_arg,A2,x); // Testing TestType extended copy ctor
        TEST_ASSERT(3 == ca.value());
        TEST_ASSERT(R2 == *ca.get_memory_resource());
        
        Obj y(copy_swap_helper(x));
        TEST_ASSERT(y == x);
        TEST_ASSERT(R1 == *y.get_memory_resource());

        const Obj q(std::allocator_arg, A2, 9);
        Obj z(copy_swap_helper(x, q));
        TEST_ASSERT(z == x);
        TEST_ASSERT(R2 == *z.get_memory_resource());

        Obj& yr = swap_assign(y, q);
        TEST_ASSERT(&yr == &y);
        TEST_ASSERT(y == q);
        TEST_ASSERT(R1 == *y.get_memory_resource());

        Obj& zr = swap_assign(z, Obj(std::allocator_arg, A1, 8));
        TEST_ASSERT(&zr == &z);
        TEST_ASSERT(z.value() == 8);
        TEST_ASSERT(R2 == *z.get_memory_resource());
    }

    // Test propagating allocator with `copy_swap` and `move_swap`
    {
        typedef MyPocAlloc<int> IntPocAlloc;
        typedef TestType<IntPocAlloc> Obj;

        TEST_ASSERT(  has_get_allocator_v<Obj>);
        TEST_ASSERT(! has_get_memory_resource_v<Obj>);
        TEST_ASSERT(  (uses_suffix_allocator_v<Obj, IntPocAlloc>));
        TEST_ASSERT(! (uses_prefix_allocator_v<Obj, IntPocAlloc>));

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
}
