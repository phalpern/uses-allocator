// uses_allocator.t.cpp                  -*-C++-*-

#include "uses_allocator.h"

#include <iostream>
#include <vector>
#include <cstdlib>

namespace exp = std::experimental;
namespace pmr = exp::pmr;
namespace internal = exp::fundamentals_v2::internal;

// STL-style test allocator (doesn't meet the Allocator requirements, but
// that's not important for this test).
template <class T>
class MySTLAlloc
{
    int m_id;
public:
    explicit MySTLAlloc(int id = -1) : m_id(id) { }

    template <class U>
    MySTLAlloc(const MySTLAlloc<U>& other) : m_id(other.id()) { }
    
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
    typedef Alloc AllocType;
    AllocType m_alloc;
    TestTypeBase() : m_alloc() { }
    TestTypeBase(const AllocType& a) : m_alloc(a) { }
public:
    typedef AllocType allocator_type;
    Alloc get_allocator() const { return m_alloc; }
};

template <> 
class TestTypeBase<NoAlloc>
{
protected:
    typedef NoAlloc AllocType;
    TestTypeBase() { }
    TestTypeBase(const AllocType&) { }
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

    TestType(const TestType& other) : Base(), m_value(other.m_value) { }
    TestType(std::allocator_arg_t, const PrefixAlloc& a, const TestType& other)
        : Base(a), m_value(other.m_value) { }
    TestType(const TestType& other, const SuffixAlloc& a) 
        : Base(a), m_value(other.m_value) { }

    ~TestType() { }

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

static int errorCount = 0;
class TestContext
{
    const TestContext* m_prevContext;
    const char*        m_str;
    static const TestContext* s_currContext;

public:
    static const TestContext* currContext() { return s_currContext; }
    
    TestContext(const char* str) : m_prevContext(s_currContext), m_str(str) {
        s_currContext = this;
    }
        
    ~TestContext() { s_currContext = m_prevContext; }

    const TestContext* prevContext() const { return m_prevContext; }
    const char* str() const { return m_str; }
};
const TestContext *TestContext::s_currContext = nullptr;

#define TEST_ASSERT(c) do {                                             \
        if (! (c)) {                                                    \
            std::cout << __FILE__ << ':' << __LINE__                    \
                      << ": Assertion failed: " #c << std::endl;        \
            for (const TestContext* ctx = TestContext::currContext();   \
                 ctx; ctx = ctx->prevContext())                         \
                std::cout << "Context: " << ctx->str() << std::endl;    \
            ++errorCount;                                               \
        }                                                               \
    } while (false)

template <class Alloc, bool Prefix, bool usesAlloc, bool usesMemRsrc>
void runTest()
{
    typedef TestType<Alloc, Prefix> Obj;
    typedef MySTLAlloc<char>        CharAlloc;
    typedef MySTLAlloc<int>         IntAlloc;
    typedef pmr::memory_resource*   pmr_ptr;

    constexpr bool usesTypeErasure = usesAlloc && usesMemRsrc;
    constexpr int val = 3;  // Value stored in constructed objects

    // IntAlloc A0; // Default
    // IntAlloc A1(1);
    // IntAlloc A2(2);
    // MyMemResource& R0 = DefaultResource;
    // MyMemResource R1(1), *pR1 = &R1;
    // MyMemResource R2(2), *pR2 = &R2;

    TEST_ASSERT(usesAlloc   == (exp::uses_allocator<Obj, CharAlloc>::value));
    TEST_ASSERT(usesMemRsrc == (exp::uses_allocator<Obj, pmr_ptr>::value));
}
             

int main()
{
    typedef MySTLAlloc<int>       IntAlloc;
    typedef pmr::memory_resource* pmr_ptr;
    typedef exp::erased_type      ET;

#define TEST(Alloc, Prefix, usesAlloc, usesMemRsrc) do {        \
        TestContext tc("TestType<" #Alloc "," #Prefix ">");     \
        runTest<Alloc, Prefix, usesAlloc, usesMemRsrc>();       \
    } while (false)

    TEST(NoAlloc,  0, 0, 0);
    TEST(IntAlloc, 0, 1, 0);
    TEST(pmr_ptr,  0, 0, 1);
    TEST(ET,       0, 1, 1);

//    using internal::make_args_uses_allocator;
}