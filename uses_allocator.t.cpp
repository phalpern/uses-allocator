/* uses_allocator.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include "uses_allocator.h"

#include <vector>
#include <cstdlib>
#include <test_assert.h>

namespace std {

inline namespace Cpp17 {

enum class byte : unsigned char { };

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
} // close namespace Cpp17

inline namespace Cpp20 {
namespace pmr {
using namespace Cpp17::pmr;

template <class T = std::byte>
class polymorphic_allocator {
    memory_resource *p_rsrc;
public:
    polymorphic_allocator(memory_resource *r) : p_rsrc(r) { }

    memory_resource *resource() const { return p_rsrc; }
};

template <class T1, class T2>
bool operator==(const polymorphic_allocator<T1>& a1,
                const polymorphic_allocator<T2>& a2) {
    return *a1.resource() == *a2.resource();
}

template <class T1, class T2>
bool operator!=(const polymorphic_allocator<T1>& a1,
                const polymorphic_allocator<T2>& a2) {
    return ! (a1 == a2);
}

} // close namespace pmr
} // close namespace Cpp20
} // close namespace std

namespace exp = std::Cpp20;
namespace pmr = exp::pmr;
namespace internal = exp::internal;

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

class EraseAlloc
{
};

using PolyAlloc = pmr::polymorphic_allocator<>;

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
class TestTypeBase<PolyAlloc>
{
protected:
    typedef pmr::polymorphic_allocator<> AllocType;
    AllocType m_alloc;
    TestTypeBase() : m_alloc(&DefaultResource) { }
    TestTypeBase(const AllocType& a) : m_alloc(a) { }
    TestTypeBase(const TestTypeBase& other) : m_alloc(&DefaultResource) { }
public:
    typedef AllocType allocator_type;
    allocator_type get_allocator() const { return m_alloc; }

    template <typename A2>
    bool match_allocator(const A2&) const { return false; }
    template <typename T2>
    bool match_allocator(const pmr::polymorphic_allocator<T2>& a2) const
        { return a2 == m_alloc; }
    bool match_resource(pmr::memory_resource* r) const
        { return *r == *m_alloc.resource(); }
};

template <>
class TestTypeBase<EraseAlloc>
{
protected:
    // Erase the allocator type
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
    typedef pmr::polymorphic_allocator<> allocator_type;

    allocator_type get_allocator() const
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
//    PolyAlloc        (polymorphic allocator)
//    EraseAlloc       (type-erased allocator)
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

namespace std {
    template <class Alloc, bool Prefix>
    struct uses_allocator<TestType<EraseAlloc, Prefix>, Alloc> : true_type {};
}

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

// is_inbounds derives from true_type if `T` is a tuple and
// `I < tuple_size<Tuple>::value`; otherwise false_type.
template <std::size_t I, typename T>
struct is_inbounds : std::false_type { };

template <std::size_t I, typename... E>
struct is_inbounds<I, std::tuple<E...>>
    : std::integral_constant<bool, I < sizeof...(E)> { };

// Implementation of `match_tuple_element` (below).
// The primary template is used when `I` is in bounds.
template <std::size_t I, typename T, typename Tuple,
          bool InBounds = is_inbounds<I, Tuple>::value>
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

// Implementation of `match_tuple_element2` (below).
// The primary template is used when `I` is in bounds.
template <std::size_t I, std::size_t J, typename T, typename Tuple,
          bool InBounds = (I < std::tuple_size<Tuple>::value)>
struct match_tuple_element2_imp
{
private:
    typedef std::remove_reference_t<typename std::tuple_element<I,Tuple>::type>
        ElemI;

public:
    static constexpr bool match_type =
        match_tuple_element_imp<J, T, ElemI>::match_type;

    template <typename U>
    static bool match_value(const Tuple& tpl, const U& val) {
        return match_tuple_element<J, T>(std::get<I>(tpl), val);
    }
};

// Implementation of `match_tuple_element2` (below).
// This specialization is used when `I` is out of bounds.
template <std::size_t I, std::size_t J, typename T, typename Tuple>
struct match_tuple_element2_imp<I, J, T, Tuple, false>
{
    static constexpr bool match_type = false;

    template <typename U>
    static bool match_value(const Tuple&, const U&) {
        return false;
    }
};

// Returns true if the `J`th element of the `I`th subtuple of a tuple `tpl` is
// of type exactly matching the specified template parameter type `T` and has
// a value that compares equal to the specified `value`; otherwise returns
// false.  If `I` or `J` is out of bounds or if the `I`th element of `tpl` is
// not a (sub)tuple, compiles correctly and always returns false.
template <std::size_t I, std::size_t J, typename T, typename Tuple, typename U>
bool match_tuple_element2(const Tuple& tpl, const U& value) {
    return match_tuple_element2_imp<I, J, T, Tuple>::match_value(tpl, value);
}

// Returns true if the `J`th element of the `I`th subtuple of a tuple `tpl` is
// of type exactly matching the specified template parameter type `T`;
// otherwise returns false.  If `I` or `J` is out of bounds or if the `I`th
// element of `tpl` is not a (sub)tuple, compiles correctly and always returns
// false.
template <std::size_t I, std::size_t J, typename T, typename Tuple>
bool match_tuple_element2(const Tuple& tpl) {
    return match_tuple_element2_imp<I, J, T, Tuple>::match_type;
}

template <class Alloc, bool Prefix, bool usesAlloc, bool usesMemRsrc>
void runTest()
{
    typedef TestType<Alloc, Prefix> Obj;
    typedef MySTLAlloc<char>        CharAlloc;
    typedef MySTLAlloc<int>         IntAlloc;
    typedef pmr::memory_resource*   pmr_ptr;
    typedef MyMemResource*          MyPmrPtr;

    char objBuffer alignas(Obj) [sizeof(Obj)];
    Obj *pUninitObj = reinterpret_cast<Obj*>(objBuffer);

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

    TEST_ASSERT(usesAlloc   == (std::uses_allocator<Obj, CharAlloc>::value));
    TEST_ASSERT(usesMemRsrc == (std::uses_allocator<Obj, pmr_ptr>::value));

    // Test with allocator only
    {
        auto args = exp::uses_allocator_construction_args<Obj>(A1);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = usesAlloc ? (Prefix ? 2 : 1) : 0;
        const std::size_t allocArg = PrefixAlloc ? 1 : numArgs - 1;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT(usesAlloc ==
                    (match_tuple_element<allocArg,const IntAlloc&>(args, A1)));
        TEST_ASSERT(PrefixAlloc ==
                    (match_tuple_element<0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.value());
        TEST_ASSERT(usesAlloc == V.match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == V.match_resource(pR0));

        Obj X = exp::make_using_allocator<Obj>(A1);
        TEST_ASSERT(0 == X.value());
        TEST_ASSERT(usesAlloc == X.match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == X.match_resource(pR0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               A1);
        TEST_ASSERT(0 == pY->value());
        TEST_ASSERT(usesAlloc == pY->match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == pY->match_resource(pR0));
        pY->~Obj();
    }

    // Test with allocator and value.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(A1,
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

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(val == V.value());
        TEST_ASSERT(usesAlloc == V.match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == V.match_resource(pR0));

        Obj X = exp::make_using_allocator<Obj>(A1, 7);
        TEST_ASSERT(7 == X.value());
        TEST_ASSERT(usesAlloc == X.match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == X.match_resource(pR0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               A1, 7);
        TEST_ASSERT(7 == pY->value());
        TEST_ASSERT(usesAlloc == pY->match_allocator(A1));
        TEST_ASSERT((!usesAlloc && usesMemRsrc) == pY->match_resource(pR0));
        pY->~Obj();
    }

    // Test with memory resource
    {
        auto args = exp::uses_allocator_construction_args<Obj>(pR1);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs = usesMemRsrc ? (Prefix ? 2 : 1) : 0;
        const std::size_t rsrcArg = PrefixRsrc ? 1 : numArgs - 1;
        TEST_ASSERT(expNumArgs == numArgs);
        TEST_ASSERT(usesMemRsrc ==
                    (match_tuple_element<rsrcArg,const MyPmrPtr&>(args, pR1)));
        TEST_ASSERT(PrefixRsrc ==
                    (match_tuple_element<0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.value());
        TEST_ASSERT(usesMemRsrc == V.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == V.match_allocator(A0));

        Obj X = exp::make_using_allocator<Obj>(pR1);
        TEST_ASSERT(0 == X.value());
        TEST_ASSERT(usesMemRsrc == X.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == X.match_allocator(A0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               pR1);
        TEST_ASSERT(0 == pY->value());
        TEST_ASSERT(usesMemRsrc == pY->match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == pY->match_allocator(A0));
        pY->~Obj();
    }

    // Test with memory resource and value.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(pR1,
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

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(val == V.value());
        TEST_ASSERT(usesMemRsrc == V.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == V.match_allocator(A0));

        Obj X = exp::make_using_allocator<Obj>(pR1, 7);
        TEST_ASSERT(7 == X.value());
        TEST_ASSERT(usesMemRsrc == X.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == X.match_allocator(A0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               pR1, 7);
        TEST_ASSERT(7 == pY->value());
        TEST_ASSERT(usesMemRsrc == pY->match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc && usesAlloc) == pY->match_allocator(A0));
        pY->~Obj();
    }
}

template <class T, bool = false> struct PrintType;
template <class T> struct PrintType<T, true> { };

template <class Alloc1, bool Prefix1, bool usesAlloc1, bool usesMemRsrc1,
          class Alloc2, bool Prefix2, bool usesAlloc2, bool usesMemRsrc2>
void runPairTest()
{
    using std::allocator_arg;
    using std::allocator_arg_t;
    using std::pair;

    typedef TestType<Alloc1, Prefix1> Elem1;
    typedef TestType<Alloc2, Prefix2> Elem2;
    typedef pair<Elem1, Elem2>        Obj;
    typedef MySTLAlloc<char>          CharAlloc;
    typedef MySTLAlloc<int>           IntAlloc;
    typedef pmr::memory_resource*     pmr_ptr;
    typedef MyMemResource*            MyPmrPtr;

    char objBuffer alignas(Obj) [sizeof(Obj)];
    Obj *pUninitObj = reinterpret_cast<Obj*>(objBuffer);

    constexpr bool PrefixAlloc1     = usesAlloc1 && Prefix1;
    constexpr bool PrefixAlloc2     = usesAlloc2 && Prefix2;
    constexpr bool PrefixRsrc1      = usesMemRsrc1 && Prefix1;
    constexpr bool PrefixRsrc2      = usesMemRsrc2 && Prefix2;
    constexpr bool usesTypeErasure1 = usesAlloc1 && usesMemRsrc1;
    constexpr bool usesTypeErasure2 = usesAlloc2 && usesMemRsrc2;
    constexpr bool usesAlloc        = usesAlloc1 || usesAlloc2;
    constexpr bool usesMemRsrc      = usesMemRsrc1 || usesMemRsrc2;

    int   val1 = 3;  // Value stored into first element of constructed pairs
    short val2 = 7;  // Value stored into second element of constructed pairs
    pair<int, short> val{ val1, val2 };

    IntAlloc A0; // Default
    IntAlloc A1(1);
    MyPmrPtr pR0 = &DefaultResource;
    MyMemResource R1(1); MyPmrPtr pR1 = &R1;

    TEST_ASSERT(usesAlloc1   == (std::uses_allocator<Elem1, CharAlloc>::value));
    TEST_ASSERT(usesAlloc2   == (std::uses_allocator<Elem2, CharAlloc>::value));
    TEST_ASSERT(usesMemRsrc1 == (std::uses_allocator<Elem1, pmr_ptr>::value));
    TEST_ASSERT(usesMemRsrc2 == (std::uses_allocator<Elem2, pmr_ptr>::value));

    // Test with no constructor arguments
    {
        auto args = std::tuple<>{};

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.first.value());
        TEST_ASSERT(0 == V.second.value());
        TEST_ASSERT(usesAlloc1 == V.first.match_allocator(A0));
        TEST_ASSERT(usesAlloc2 == V.second.match_allocator(A0));
        TEST_ASSERT(usesMemRsrc1 == V.first.match_resource(pR0));
        TEST_ASSERT(usesMemRsrc2 == V.second.match_resource(pR0));
    }

    // Test with values.
    {
        auto args = std::tuple<int&&, short>(std::move(val1), val2);
        TEST_ASSERT((match_tuple_element<0, int&&>(args, val1)));
        TEST_ASSERT((match_tuple_element<1, short>(args, val2)));

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(val1 == V.first.value());
        TEST_ASSERT(val2 == V.second.value());
        TEST_ASSERT(usesAlloc1 == V.first.match_allocator(A0));
        TEST_ASSERT(usesAlloc2 == V.second.match_allocator(A0));
        TEST_ASSERT(usesMemRsrc1 == V.first.match_resource(pR0));
        TEST_ASSERT(usesMemRsrc2 == V.second.match_resource(pR0));
    }

    // Test with allocator only
    {
        auto args = exp::uses_allocator_construction_args<Obj>(A1);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesAlloc ? 3 : 0;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesAlloc1 ? (Prefix1 ? 2 : 1) : 0;
        const std::size_t expNumArgs2 = usesAlloc2 ? (Prefix2 ? 2 : 1) : 0;
        const std::size_t allocArg1 = PrefixAlloc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixAlloc2 ? 1 : expNumArgs2 - 1;
        TEST_ASSERT(usesAlloc1 ==
                    (match_tuple_element2<1, allocArg1,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(usesAlloc2 ==
                    (match_tuple_element2<2, allocArg2,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(PrefixAlloc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixAlloc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.first.value());
        TEST_ASSERT(0 == V.second.value());
        TEST_ASSERT(usesAlloc1 == V.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == V.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    V.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    V.second.match_resource(pR0));

        Obj X = exp::make_using_allocator<Obj>(A1);
        TEST_ASSERT(0 == X.first.value());
        TEST_ASSERT(0 == X.second.value());
        TEST_ASSERT(usesAlloc1 == X.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.second.match_resource(pR0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               A1);
        TEST_ASSERT(0 == pY->first.value());
        TEST_ASSERT(0 == pY->second.value());
        TEST_ASSERT(usesAlloc1 == pY->first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == pY->second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    pY->first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    pY->second.match_resource(pR0));
        pY->~Obj();
    }

    // Test with allocator and two values.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(A1,
                                                               std::move(val1),
                                                               val2);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesAlloc ? 3 : 2;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesAlloc1 ? (Prefix1 ? 3 : 2) : 1;
        const std::size_t expNumArgs2 = usesAlloc2 ? (Prefix2 ? 3 : 2) : 1;
        const std::size_t valArg1 = PrefixAlloc1 ? expNumArgs1 - 1 : 0;
        const std::size_t valArg2 = PrefixAlloc2 ? expNumArgs2 - 1 : 0;
        const std::size_t allocArg1 = PrefixAlloc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixAlloc2 ? 1 : expNumArgs2 - 1;

        PrintType<decltype(args), !usesAlloc>();
        if (usesAlloc)
        {
            TEST_ASSERT((match_tuple_element2<1, valArg1, int&&>(args, val1)));
            TEST_ASSERT((match_tuple_element2<2, valArg2, short&>(args, val2)));
        }
        else
        {
            TEST_ASSERT((match_tuple_element<0, int&&>(args, val1)));
            TEST_ASSERT((match_tuple_element<1, short&>(args, val2)));
        }

        TEST_ASSERT(usesAlloc1 ==
                    (match_tuple_element2<1, allocArg1,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(usesAlloc2 ==
                    (match_tuple_element2<2, allocArg2,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(PrefixAlloc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixAlloc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(std::move(args));
        TEST_ASSERT(val1 == V.first.value());
        TEST_ASSERT(val2 == V.second.value());
        TEST_ASSERT(usesAlloc1 == V.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == V.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    V.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    V.second.match_resource(pR0));

        Obj X = exp::make_using_allocator<Obj>(A1, std::move(val1), val2);
        TEST_ASSERT(val1 == X.first.value());
        TEST_ASSERT(val2 == X.second.value());
        TEST_ASSERT(usesAlloc1 == X.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.second.match_resource(pR0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               A1,
                                                               std::move(val1),
                                                               val2);
        TEST_ASSERT(val1 == pY->first.value());
        TEST_ASSERT(val2 == pY->second.value());
        TEST_ASSERT(usesAlloc1 == pY->first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == pY->second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    pY->first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    pY->second.match_resource(pR0));
        pY->~Obj();
    }

    // Test with allocator and lvalue pair value.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(A1, val);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesAlloc ? 3 : 1;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesAlloc1 ? (Prefix1 ? 3 : 2) : 1;
        const std::size_t expNumArgs2 = usesAlloc2 ? (Prefix2 ? 3 : 2) : 1;
        const std::size_t valArg1 = PrefixAlloc1 ? expNumArgs1 - 1 : 0;
        const std::size_t valArg2 = PrefixAlloc2 ? expNumArgs2 - 1 : 0;
        const std::size_t allocArg1 = PrefixAlloc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixAlloc2 ? 1 : expNumArgs2 - 1;

        if (usesAlloc)
        {
            TEST_ASSERT((match_tuple_element2<1, valArg1, const int&>(args,
                                                                      val1)));
            TEST_ASSERT((match_tuple_element2<2, valArg2, const short&>(args,
                                                                        val2)));
        }
        else
        {
            TEST_ASSERT((match_tuple_element<0, pair<int, short>&>(args, val)));
        }

        TEST_ASSERT(usesAlloc1 ==
                    (match_tuple_element2<1, allocArg1,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(usesAlloc2 ==
                    (match_tuple_element2<2, allocArg2,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(PrefixAlloc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixAlloc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(std::move(args));
        TEST_ASSERT(val1 == V.first.value());
        TEST_ASSERT(val2 == V.second.value());
        TEST_ASSERT(usesAlloc1 == V.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == V.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    V.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    V.second.match_resource(pR0));

        Obj X = exp::make_using_allocator<Obj>(A1, val);
        TEST_ASSERT(val1 == X.first.value());
        TEST_ASSERT(val2 == X.second.value());
        TEST_ASSERT(usesAlloc1 == X.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.second.match_resource(pR0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               A1, val);
        TEST_ASSERT(val1 == pY->first.value());
        TEST_ASSERT(val2 == pY->second.value());
        TEST_ASSERT(usesAlloc1 == pY->first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == pY->second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    pY->first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    pY->second.match_resource(pR0));
        pY->~Obj();
    }

    // Test with allocator and rvalue pair value.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(A1,
                                                               std::move(val));
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesAlloc ? 3 : 1;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesAlloc1 ? (Prefix1 ? 3 : 2) : 1;
        const std::size_t expNumArgs2 = usesAlloc2 ? (Prefix2 ? 3 : 2) : 1;
        const std::size_t valArg1 = PrefixAlloc1 ? expNumArgs1 - 1 : 0;
        const std::size_t valArg2 = PrefixAlloc2 ? expNumArgs2 - 1 : 0;
        const std::size_t allocArg1 = PrefixAlloc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixAlloc2 ? 1 : expNumArgs2 - 1;

        if (usesAlloc)
        {
            TEST_ASSERT((match_tuple_element2<1, valArg1, const int&>(args,
                                                                      val1)));
            TEST_ASSERT((match_tuple_element2<2, valArg2, const short&>(args,
                                                                      val2)));
        }
        else
        {
            TEST_ASSERT((match_tuple_element<0, pair<int, short>&&>(args,
                                                                    val)));
        }

        TEST_ASSERT(usesAlloc1 ==
                    (match_tuple_element2<1, allocArg1,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(usesAlloc2 ==
                    (match_tuple_element2<2, allocArg2,const IntAlloc&>(args,
                                                                        A1)));
        TEST_ASSERT(PrefixAlloc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixAlloc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(std::move(args));
        TEST_ASSERT(val1 == V.first.value());
        TEST_ASSERT(val2 == V.second.value());
        TEST_ASSERT(usesAlloc1 == V.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == V.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    V.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    V.second.match_resource(pR0));

        Obj X = exp::make_using_allocator<Obj>(A1, std::make_pair(val1, val2));
        TEST_ASSERT(val1 == X.first.value());
        TEST_ASSERT(val2 == X.second.value());
        TEST_ASSERT(usesAlloc1 == X.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.second.match_resource(pR0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               A1,
                                                               std::move(val));
        TEST_ASSERT(val1 == pY->first.value());
        TEST_ASSERT(val2 == pY->second.value());
        TEST_ASSERT(usesAlloc1 == pY->first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == pY->second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    pY->first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    pY->second.match_resource(pR0));
        pY->~Obj();
    }

    // Test with memory_resource
    {
        auto args = exp::uses_allocator_construction_args<Obj>(pR1);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesMemRsrc ? 3 : 0;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesMemRsrc1 ? (Prefix1 ? 2 : 1) : 0;
        const std::size_t expNumArgs2 = usesMemRsrc2 ? (Prefix2 ? 2 : 1) : 0;
        const std::size_t allocArg1 = PrefixRsrc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixRsrc2 ? 1 : expNumArgs2 - 1;
        TEST_ASSERT(usesMemRsrc1 ==
                    (match_tuple_element2<1, allocArg1,const MyPmrPtr&>(args,
                                                                        pR1)));
        TEST_ASSERT(usesMemRsrc2 ==
                    (match_tuple_element2<2, allocArg2,const MyPmrPtr&>(args,
                                                                        pR1)));
        TEST_ASSERT(PrefixRsrc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixRsrc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(args);
        TEST_ASSERT(0 == V.first.value());
        TEST_ASSERT(0 == V.second.value());
        TEST_ASSERT(usesMemRsrc1 == V.first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == V.second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    V.first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    V.second.match_allocator(A0));

        Obj X = exp::make_using_allocator<Obj>(pR1);
        TEST_ASSERT(0 == X.first.value());
        TEST_ASSERT(0 == X.second.value());
        TEST_ASSERT(usesMemRsrc1 == X.first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == X.second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    X.first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    X.second.match_allocator(A0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               pR1);
        TEST_ASSERT(0 == pY->first.value());
        TEST_ASSERT(0 == pY->second.value());
        TEST_ASSERT(usesMemRsrc1 == pY->first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == pY->second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    pY->first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    pY->second.match_allocator(A0));
        pY->~Obj();
    }

    // Test with memory_resource and two values.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(pR1,
                                                               std::move(val1),
                                                               val2);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesMemRsrc ? 3 : 2;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesMemRsrc1 ? (Prefix1 ? 3 : 2) : 1;
        const std::size_t expNumArgs2 = usesMemRsrc2 ? (Prefix2 ? 3 : 2) : 1;
        const std::size_t valArg1 = PrefixRsrc1 ? expNumArgs1 - 1 : 0;
        const std::size_t valArg2 = PrefixRsrc2 ? expNumArgs2 - 1 : 0;
        const std::size_t allocArg1 = PrefixRsrc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixRsrc2 ? 1 : expNumArgs2 - 1;

        if (usesMemRsrc)
        {
            TEST_ASSERT((match_tuple_element2<1, valArg1, int&&>(args, val1)));
            TEST_ASSERT((match_tuple_element2<2, valArg2, short&>(args, val2)));
        }
        else
        {
            TEST_ASSERT((match_tuple_element<0, int&&>(args, val1)));
            TEST_ASSERT((match_tuple_element<1, short&>(args, val2)));
        }

        TEST_ASSERT(usesMemRsrc1 ==
                    (match_tuple_element2<1, allocArg1,const MyPmrPtr&>(args,
                                                                        pR1)));
        TEST_ASSERT(usesMemRsrc2 ==
                    (match_tuple_element2<2, allocArg2,const MyPmrPtr&>(args,
                                                                        pR1)));
        TEST_ASSERT(PrefixRsrc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixRsrc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(std::move(args));
        TEST_ASSERT(val1 == V.first.value());
        TEST_ASSERT(val2 == V.second.value());
        TEST_ASSERT(usesMemRsrc1 == V.first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == V.second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    V.first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    V.second.match_allocator(A0));

        Obj X = exp::make_using_allocator<Obj>(pR1, std::move(val1), val2);
        TEST_ASSERT(val1 == X.first.value());
        TEST_ASSERT(val2 == X.second.value());
        TEST_ASSERT(usesMemRsrc1 == X.first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == X.second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    X.first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    X.second.match_allocator(A0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               pR1,
                                                               std::move(val1),
                                                               val2);
        TEST_ASSERT(val1 == pY->first.value());
        TEST_ASSERT(val2 == pY->second.value());
        TEST_ASSERT(usesMemRsrc1 == pY->first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == pY->second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    pY->first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    pY->second.match_allocator(A0));
        pY->~Obj();
    }

    // Test with memory_resource and lvalue pair value.
    {
        auto args = exp::uses_allocator_construction_args<Obj>(pR1, val);
        const std::size_t numArgs = std::tuple_size<decltype(args)>::value;
        const std::size_t expNumArgs  = usesMemRsrc ? 3 : 1;
        TEST_ASSERT(expNumArgs == numArgs);

        const std::size_t expNumArgs1 = usesMemRsrc1 ? (Prefix1 ? 3 : 2) : 1;
        const std::size_t expNumArgs2 = usesMemRsrc2 ? (Prefix2 ? 3 : 2) : 1;
        const std::size_t valArg1 = PrefixRsrc1 ? expNumArgs1 - 1 : 0;
        const std::size_t valArg2 = PrefixRsrc2 ? expNumArgs2 - 1 : 0;
        const std::size_t allocArg1 = PrefixRsrc1 ? 1 : expNumArgs1 - 1;
        const std::size_t allocArg2 = PrefixRsrc2 ? 1 : expNumArgs2 - 1;

        if (usesMemRsrc)
        {
            TEST_ASSERT((match_tuple_element2<1, valArg1, const int&>(args,
                                                                      val1)));
            TEST_ASSERT((match_tuple_element2<2, valArg2, const short&>(args,
                                                                        val2)));
        }
        else
        {
            TEST_ASSERT((match_tuple_element<0, pair<int, short>&>(args, val)));
        }

        TEST_ASSERT(usesMemRsrc1 ==
                    (match_tuple_element2<1, allocArg1,const MyPmrPtr&>(args,
                                                                        pR1)));
        TEST_ASSERT(usesMemRsrc2 ==
                    (match_tuple_element2<2, allocArg2,const MyPmrPtr&>(args,
                                                                        pR1)));
        TEST_ASSERT(PrefixRsrc1 ==
                    (match_tuple_element2<1, 0,const allocator_arg_t&>(args)));
        TEST_ASSERT(PrefixRsrc2 ==
                    (match_tuple_element2<2, 0,const allocator_arg_t&>(args)));

        Obj V = std::make_from_tuple<Obj>(std::move(args));
        TEST_ASSERT(val1 == V.first.value());
        TEST_ASSERT(val2 == V.second.value());
        TEST_ASSERT(usesMemRsrc1 == V.first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == V.second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    V.first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    V.second.match_allocator(A0));

        Obj X = exp::make_using_allocator<Obj>(pR1, std::move(val1), val2);
        TEST_ASSERT(val1 == X.first.value());
        TEST_ASSERT(val2 == X.second.value());
        TEST_ASSERT(usesMemRsrc1 == X.first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == X.second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    X.first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    X.second.match_allocator(A0));

        Obj *pY = exp::uninitialized_construct_using_allocator(pUninitObj,
                                                               pR1,
                                                               std::move(val1),
                                                               val2);
        TEST_ASSERT(val1 == pY->first.value());
        TEST_ASSERT(val2 == pY->second.value());
        TEST_ASSERT(usesMemRsrc1 == pY->first.match_resource(pR1));
        TEST_ASSERT(usesMemRsrc2 == pY->second.match_resource(pR1));
        TEST_ASSERT((!usesMemRsrc1 && usesAlloc1) ==
                    pY->first.match_allocator(A0));
        TEST_ASSERT((!usesMemRsrc2 && usesAlloc2) ==
                    pY->second.match_allocator(A0));
        pY->~Obj();
    }

    // Nested pair, allocator only
    {
        typedef pair<Elem1, pair<Elem1, Elem2>>        Obj;

        Obj X = exp::make_using_allocator<Obj>(A1);
        TEST_ASSERT(0 == X.first.value());
        TEST_ASSERT(0 == X.second.first.value());
        TEST_ASSERT(0 == X.second.second.value());
        TEST_ASSERT(usesAlloc1 == X.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc1 == X.second.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.second.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.second.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.second.second.match_resource(pR0));
    }

    // Nested pair, with allocator and two values.
    {
        typedef pair<pair<Elem1, Elem2>, Elem2>        Obj;

        Obj X = exp::make_using_allocator<Obj>(A1, std::make_pair(val1,val2),
                                               val2);
        TEST_ASSERT(val1 == X.first.first.value());
        TEST_ASSERT(val2 == X.first.second.value());
        TEST_ASSERT(val2 == X.second.value());
        TEST_ASSERT(usesAlloc1 == X.first.first.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.first.second.match_allocator(A1));
        TEST_ASSERT(usesAlloc2 == X.second.match_allocator(A1));
        TEST_ASSERT((!usesAlloc1 && usesMemRsrc1) ==
                    X.first.first.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.first.second.match_resource(pR0));
        TEST_ASSERT((!usesAlloc2 && usesMemRsrc2) ==
                    X.second.match_resource(pR0));
    }
}


int main()
{
    typedef MySTLAlloc<int>       IntAlloc;
    typedef pmr::memory_resource* pmr_ptr;

#define TEST(Alloc, Prefix, expUsesAlloc, expUsesMemRsrc) do {  \
        TestContext tc(__FILE__, __LINE__,                      \
                       "TestType<" #Alloc "," #Prefix ">");     \
        runTest<Alloc, Prefix, expUsesAlloc, expUsesMemRsrc>(); \
    } while (false)

    TEST(NoAlloc,    0, 0, 0);
    TEST(IntAlloc,   0, 1, 0);
    TEST(IntAlloc,   1, 1, 0);
    TEST(PolyAlloc,  0, 0, 1);
    TEST(PolyAlloc,  1, 0, 1);
    TEST(EraseAlloc, 0, 1, 1);
    TEST(EraseAlloc, 1, 1, 1);

#define PAIR_TEST(Alloc1, Prefix1, expUsesAlloc1, expUsesMemRsrc1,      \
                  Alloc2, Prefix2, expUsesAlloc2, expUsesMemRsrc2) do { \
        TestContext tc(__FILE__, __LINE__,                              \
                       "pair<TestType<" #Alloc1 "," #Prefix1 ">, "      \
                       "TestType<" #Alloc2 "," #Prefix2 ">>");          \
        runPairTest<Alloc1, Prefix1, expUsesAlloc1, expUsesMemRsrc1,    \
                    Alloc2, Prefix2, expUsesAlloc2, expUsesMemRsrc2>(); \
    } while (false)

    PAIR_TEST(NoAlloc,    0, 0, 0, NoAlloc,    0, 0, 0);
    PAIR_TEST(NoAlloc,    0, 0, 0, IntAlloc,   0, 1, 0);
    PAIR_TEST(NoAlloc,    0, 0, 0, IntAlloc,   1, 1, 0);
    PAIR_TEST(NoAlloc,    0, 0, 0, PolyAlloc,  0, 0, 1);
    PAIR_TEST(NoAlloc,    0, 0, 0, PolyAlloc,  1, 0, 1);
    PAIR_TEST(NoAlloc,    0, 0, 0, EraseAlloc, 0, 1, 1);
    PAIR_TEST(NoAlloc,    0, 0, 0, EraseAlloc, 1, 1, 1);

    PAIR_TEST(IntAlloc,   0, 1, 0, NoAlloc,    0, 0, 0);
    PAIR_TEST(IntAlloc,   1, 1, 0, NoAlloc,    0, 0, 0);
    PAIR_TEST(IntAlloc,   1, 1, 0, IntAlloc,   0, 1, 0);
    PAIR_TEST(IntAlloc,   0, 1, 0, PolyAlloc,  0, 0, 1);
    PAIR_TEST(IntAlloc,   1, 1, 0, EraseAlloc, 1, 1, 1);

    PAIR_TEST(PolyAlloc,  0, 0, 1, NoAlloc,    0, 0, 0);
    PAIR_TEST(PolyAlloc,  1, 0, 1, NoAlloc,    0, 0, 0);
    PAIR_TEST(PolyAlloc,  1, 0, 1, IntAlloc,   1, 1, 0);
    PAIR_TEST(PolyAlloc,  0, 0, 1, PolyAlloc,  0, 0, 1);
    PAIR_TEST(PolyAlloc,  0, 0, 1, EraseAlloc, 1, 1, 1);

    PAIR_TEST(EraseAlloc, 0, 1, 1, NoAlloc,    0, 0, 0);
    PAIR_TEST(EraseAlloc, 1, 1, 1, NoAlloc,    0, 0, 0);
    PAIR_TEST(EraseAlloc, 1, 1, 1, IntAlloc,   0, 1, 0);
    PAIR_TEST(EraseAlloc, 1, 1, 1, PolyAlloc,  1, 0, 1);
    PAIR_TEST(EraseAlloc, 0, 1, 1, EraseAlloc, 1, 1, 1);

    return errorCount();
}
