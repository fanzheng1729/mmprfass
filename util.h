#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <algorithm>
#include <functional>
#include <utility>

namespace util
{
#if __cplusplus >= 201103L
    using std::none_of;
#else
    template<class II, class Pred>
    bool none_of(II first, II last, Pred pred)
    {
        return std::find_if(first, last, pred) == last;
    }
#endif // __cplusplus >= 201103L

    template<class R, class T>
    struct mem_fn_t
    {
        typedef R & result_type;
        typedef T argument_type;
    private:
        typedef R T::* PM;
        PM pm;
    public:
        mem_fn_t(PM mem) : pm(mem) {}
        R & operator()(T * p) const { return p->*pm; }
        R const & operator()(T const * p) const { return p->*pm; }
        R & operator()(T & t) const { return t.*pm; }
        R const & operator()(T const & t) const { return t.*pm; }
    };
    template<class R, class T> mem_fn_t<R, T> mem_fn(R T::* mem) {return mem;}

#ifdef __cpp_lib_not_fn
    template<class Pred> auto not1(Pred const & pred)
    { return std::not_fn(pred); }
#else
    using std::not1;
    template<class T, class R>
    std::unary_negate<std::pointer_to_unary_function<T, R> >
    not1(R f(T)) { return std::not1(std::ptr_fun(f)); }
#endif // __cpp_lib_not_fn

#ifdef __cpp_lib_robust_nonmodifying_seq_ops
    using std::equal;
    using std::mismatch;
#else
    template<class II1, class II2>
    bool equal(II1 first1, II1 last1, II2 first2, II2 last2)
    {
        return last1 - first1 == last2 - first2 &&
                std::equal(first1, last1, first2);
    }
    template<class II1, class II2>
    std::pair<II1, II2> mismatch(II1 first1, II1 last1, II2 first2, II2 last2)
    {
        last1 = std::min(last1, first1 + (last2 - first2));
        return std::mismatch(first1, last1, first2);
    }
#endif // __cpp_lib_robust_nonmodifying_seq_ops
} // namespace util

#endif // UTIL_H_INCLUDED
