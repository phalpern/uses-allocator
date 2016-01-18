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
    template <typename A2>
    bool match_allocator(const A2& a2) const { return a2.id() == m_alloc.id(); }
    bool match_resource(pmr::memory_resource*) const { return false; }
};

template <> 
class TestTypeBase<NoAlloc>
{
protected:
    typedef NoAlloc AllocType;
    TestTypeBase() { }
    TestTypeBase(const AllocType&) { }
public:
    template <typename A2>
    bool match_allocator(const A2&) const { return false; }
    bool match_resource(pmr::memory_resource*) const { return false; }
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
    template <typename A2>
    bool match_allocator(const A2&) const { return false; }
    bool match_resource(pmr::memory_resource* r) const 
        { return *r == *m_alloc; }
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
        template <typename R> AllocEraser(R *const &r)
            : m_resource_p(r) { }
        AllocEraser(const AllocEraser& other)
            : m_resource(other.m_resource) 
        {
            if (other.m_resource_p == &other.m_resource)
                m_resource_p = &m_resource;
            else
                m_resource_p = other.m_resource_p;
        }
        int id() const {
            MyMemResource* mmr = dynamic_cast<MyMemResource*>(m_resource_p);
            return mmr ? mmr->id() : -1;
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
    template <typename A2>
    bool match_allocator(const A2& a2) const
        { return a2.id() == m_alloc.id(); }
    bool match_resource(pmr::memory_resource* r) const 
        { return *r == *m_alloc.m_resource_p; }
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
                std::cout << "  Context: " << ctx->str() << std::endl;  \
            ++errorCount;                                               \
        }                                                               \
    } while (false)

// Implementation of `match_tuple_element` (below).
// The primary template is used when `I` is in bounds.
template <std::size_t I, typename T, typename Tuple,
          bool InBounds = (I < std::tuple_size<Tuple>::value)>
struct match_tuple_element_imp
{
private:
    template <typename U>
    static bool do_match(const Tuple& tpl, const U& val, std::true_type)
        { return std::get<I>(tpl) == val; }

    template <typename U>
    static bool do_match(const Tuple&, const U&, std::false_type)
        { return false; }

public:
    static constexpr bool match_type = 
          std::is_same<typename std::tuple_element<I,Tuple>::type, T>::value;

    template <typename U>
    static bool match_value(const Tuple& tpl, const U& val) {
        return do_match(tpl, val, std::integral_constant<bool, match_type>());
    }
};

// Implementation of `match_tuple_element` (below).
// This specialization is used when `I` is out of bounds.
template <std::size_t I, typename T, typename Tuple>
struct match_tuple_element_imp<I, T, Tuple, false>
{
    static constexpr bool match_type = false;

    template <typename U>
    static bool match_value(const Tuple&, const U&) {
        return false;
    }
};

// Returns true if the `I`th element of a tuple `tpl` is of type exactly
// matching the specified template parameter type `T` and has a value that
// compares equal to the specified `value`; otherwise returns false.
// If `I` is out of bounds, compiles correctly and always returns false.
template <std::size_t I, typename T, typename Tuple, typename U>
bool match_tuple_element(const Tuple& tpl, const U& value) {
    return match_tuple_element_imp<I, T, Tuple>::match_value(tpl, value);
}

// Returns true if the `I`th element of a tuple `tpl` is of type exactly
// matching the specified template parameter type `T` otherwise returns false.
// If `I` is out of bounds, compiles correctly and always returns false.
template <std::size_t I, typename T, typename Tuple>
bool match_tuple_element(const Tuple& tpl) {
    return match_tuple_element_imp<I, T, Tuple>::match_type;
}

template <class Alloc, bool Prefix, bool usesAlloc, bool usesMemRsrc>
void runTest()
{
    typedef TestType<Alloc, Prefix> Obj;
    typedef MySTLAlloc<char>        CharAlloc;
    typedef MySTLAlloc<int>         IntAlloc;
    typedef pmr::memory_resource*   pmr_ptr;
    typedef MyMemResource*          MyPmrPtr;

    using std::allocator_arg;
    using std::allocator_arg_t;

    constexpr bool PrefixAlloc = (usesAlloc && Prefix);
    constexpr bool PrefixRsrc  = (usesMemRsrc && Prefix);
    constexpr bool usesTypeErasure = usesAlloc && usesMemRsrc;

    int val = 3;  // Value stored into constructed objects

    IntAlloc A0; // Default
    IntAlloc A1(1);
    MyPmrPtr pR0 = &DefaultResource;
    MyMemResource R1(1); MyPmrPtr pR1 = &R1;

    TEST_ASSERT(usesAlloc   == (exp::uses_allocator<Obj, CharAlloc>::value));
    TEST_ASSERT(usesMemRsrc == (exp::uses_allocator<Obj, pmr_ptr>::value));

    // Test with no constructor arguments
    {
        auto args = std::tuple<>();
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = 0;
        TEST_ASSERT(expNumArgs == numArgs);

        Obj V = exp::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.value());
        TEST_ASSERT(usesAlloc == V.match_allocator(A0));
        TEST_ASSERT(usesMemRsrc == V.match_resource(pR0));
    }

    // Test with value.
    {
        auto args = std::tuple<int&&>(std::move(val));
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = 1;
        const std::size_t valArg = 0;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT((match_tuple_element<valArg, int&&>(args, val)));

        Obj V = exp::make_from_tuple<Obj>(args);
        TEST_ASSERT(val == V.value());
        TEST_ASSERT(usesAlloc == V.match_allocator(A0));
        TEST_ASSERT(usesMemRsrc == V.match_resource(pR0));
    }

    // Test with allocator
    {
        auto args = exp::forward_uses_allocator_args<Obj>(allocator_arg, A1);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = usesAlloc ? (Prefix ? 2 : 1) : 0;
        const std::size_t allocArg = PrefixAlloc ? 1 : numArgs - 1;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT(usesAlloc ==
                    (match_tuple_element<allocArg,const IntAlloc&>(args, A1)));
        TEST_ASSERT(PrefixAlloc ==
                    (match_tuple_element<0,const allocator_arg_t&>(args)));

        Obj V = exp::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.value());
        TEST_ASSERT(usesAlloc == V.match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == V.match_resource(pR0));
    }

    // Test with allocator and value.
    {
        auto args = exp::forward_uses_allocator_args<Obj>(allocator_arg, A1,
                                                          std::move(val));
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = usesAlloc ? (Prefix ? 3 : 2) : 1;
        const std::size_t valArg = PrefixAlloc ? numArgs - 1 : 0;
        const std::size_t allocArg = PrefixAlloc ? 1 : numArgs - 1;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT((match_tuple_element<valArg, int&&>(args, val)));
        TEST_ASSERT(usesAlloc ==
                    (match_tuple_element<allocArg,const IntAlloc&>(args, A1)));
        TEST_ASSERT(PrefixAlloc ==
                    (match_tuple_element<0,const allocator_arg_t&>(args)));

        Obj V = exp::make_from_tuple<Obj>(args);
        TEST_ASSERT(val == V.value());
        TEST_ASSERT(usesAlloc == V.match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == V.match_resource(pR0));
    }

    // Test memory resource
    {
        auto args = exp::forward_uses_allocator_args<Obj>(allocator_arg, pR1);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = usesMemRsrc ? (Prefix ? 2 : 1) : 0;
        const std::size_t rsrcArg = PrefixRsrc ? 1 : numArgs - 1;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT(usesMemRsrc ==
                    (match_tuple_element<rsrcArg,const MyPmrPtr&>(args, pR1)));
        TEST_ASSERT(PrefixRsrc ==
                    (match_tuple_element<0,const allocator_arg_t&>(args)));

        Obj V = exp::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.value());
        TEST_ASSERT(usesMemRsrc == V.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == V.match_allocator(A0));
    }

    // Test with memory resource and value.
    {
        auto args = exp::forward_uses_allocator_args<Obj>(allocator_arg,pR1,
                                                          std::move(val));
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = usesMemRsrc ? (Prefix ? 3 : 2) : 1;
        const std::size_t valArg = PrefixRsrc ? numArgs - 1 : 0;
        const std::size_t rsrcArg = PrefixRsrc ? 1 : numArgs - 1;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT((match_tuple_element<valArg, int&&>(args, val)));
        TEST_ASSERT(usesMemRsrc ==
                    (match_tuple_element<rsrcArg,const MyPmrPtr&>(args, pR1)));
        TEST_ASSERT(PrefixRsrc ==
                    (match_tuple_element<0,const allocator_arg_t&>(args)));

        Obj V = exp::make_from_tuple<Obj>(args);
        TEST_ASSERT(val == V.value());
        TEST_ASSERT(usesMemRsrc == V.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == V.match_allocator(A0));
    }
}
             

int main()
{
    typedef MySTLAlloc<int>       IntAlloc;
    typedef pmr::memory_resource* pmr_ptr;
    typedef exp::erased_type      ET;

#define TEST(Alloc, Prefix, expUsesAlloc, expUsesMemRsrc) do {  \
        TestContext tc("TestType<" #Alloc "," #Prefix ">");     \
        runTest<Alloc, Prefix, expUsesAlloc, expUsesMemRsrc>(); \
    } while (false)

    TEST(NoAlloc,  0, 0, 0);
    TEST(IntAlloc, 0, 1, 0);
    TEST(pmr_ptr,  0, 0, 1);
    TEST(ET,       0, 1, 1);
    TEST(NoAlloc,  1, 0, 0);
    TEST(IntAlloc, 1, 1, 0);
    TEST(pmr_ptr,  1, 0, 1);
    TEST(ET,       1, 1, 1);
}
