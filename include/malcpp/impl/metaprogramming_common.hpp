#ifndef __MALC_METAPROGRAMMING_COMMON__
#define __MALC_METAPROGRAMMING_COMMON__

#include <type_traits>

namespace malcpp { namespace detail {
//------------------------------------------------------------------------------
class literal {
public:
  //----------------------------------------------------------------------------
  constexpr literal (const char *const lit, const int size) :
    m_lit  (lit),
    m_size (size)
  {}
  //----------------------------------------------------------------------------
  template <int N>
  constexpr literal (const char(&arr)[N]) : literal (arr, N - 1)
  {
      static_assert (N >= 1, "not a string literal");
  }
  //----------------------------------------------------------------------------
  constexpr operator const char*() const  { return m_lit; }
  constexpr int size() const              { return m_size; }
  constexpr char operator[] (int i) const { return m_lit[i]; }
  //----------------------------------------------------------------------------
  constexpr int find (char c, int beg, int end, int notfoundval = -1) const
  {
    return
      (beg < end)
      ? (m_lit[beg] == c) ? beg : find (c, beg + 1, end, notfoundval)
      : notfoundval;
  }
  //----------------------------------------------------------------------------
  constexpr bool has_repeated_chars (int beg, int end) const
  {
    return
      ((end - beg) > 1)
      ? find (m_lit[beg], beg + 1, end, -1) == -1
        ? has_repeated_chars (beg + 1, end)
        : true
      : false;
  }
  //----------------------------------------------------------------------------
  constexpr int count (int beg, int end, char c, int countv = 0) const
  {
    return
      (beg < end)
      ? count (beg + 1, end, c, countv + (int) (c == m_lit[beg]))
      : countv;
  }
  //----------------------------------------------------------------------------
  constexpr literal substr (int beg, int end) const
  {
    return literal (m_lit + beg, end - beg);
  }
  //----------------------------------------------------------------------------
  constexpr literal substr (int beg) const
  {
    return substr (beg, m_size);
  }
  //----------------------------------------------------------------------------
private:
  //--------------------------------------------------------------------------
  const char *const m_lit;
  const int         m_size;
    //--------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
template <int...>
struct intlist {};
//------------------------------------------------------------------------------
template <class... types>
struct intlist_sizeof {};

template <int... values>
struct intlist_sizeof<intlist<values...> >
{
  static constexpr unsigned value = sizeof...(values);
};
//------------------------------------------------------------------------------
template <class...>
struct typelist {};
//------------------------------------------------------------------------------
template <class... types>
constexpr typelist<types...> make_typelist (types... args)
{
  return typelist<types...>();
};
//------------------------------------------------------------------------------
#if 0
template <class... types>
struct typelist_sizeof : public typelist_sizeof<typelist<types...> > {};

template <class... types>
struct typelist_sizeof<typelist<types...> >
{
  static constexpr unsigned value = sizeof...(types);
};
#endif
//------------------------------------------------------------------------------
struct typelist_has_args {};
struct typelist_no_more_args {};
struct typelist_null {};
//------------------------------------------------------------------------------
template <class... types>
struct typelist_fwit;

template <>
struct typelist_fwit<typelist<> > {
  using head               = typelist_no_more_args;
  using tail               = typelist<>;
  using has_more_args_type = typelist_no_more_args;
};

template <class T, class... types>
struct typelist_fwit <typelist<T, types...> > {
  using head               = T;
  using tail               = typelist<types...>;
  using has_more_args_type = typelist_has_args;
};
//------------------------------------------------------------------------------
#if 0
template <int N, int I, class... types>
struct typelist_split_impl;

template <int N, int I, class T, class... fwd, class... bckwd>
struct typelist_split_impl<N, I, typelist<T, fwd...>, typelist<bckwd...> > :
  public typelist_split_impl<
    N, I + 1, typelist<fwd...>, typelist<bckwd..., T>
    >
{};

template <int N, class T, class... fwd, class... bckwd>
struct typelist_split_impl<N, N, typelist<T, fwd...>, typelist<bckwd...> > {
  using type      = T;
  using backwards = typelist<bckwd...>;
  using forward   = typelist<fwd...>;
};

template <int N, int I, class... bckwd>
struct typelist_split_impl<N, I, typelist<>, typelist<bckwd...> > {
  using type      = typelist_no_more_args;
  using backwards = typelist<bckwd...>;
  using forward   = typelist<>;
};

template <int N, class... types>
struct typelist_split : public typelist_split<N, typelist<types...> > {};

template <int N, class... types>
struct typelist_split<N, typelist<types...> > :
  public typelist_split_impl<N, 0, typelist<types...>, typelist<> >
{};
//------------------------------------------------------------------------------
template <int N, template <class> class filter, class... types>
struct typelist_filter_impl;

template <
  int                    N,
  template <class> class filter,
  class                  T,
  class...               types,
  class...               filtered,
  int...                 filteredidxs
  >
struct typelist_filter_impl<
  N,
  filter,
  typelist<T, types...>,
  typelist<filtered...>,
  intlist <filteredidxs...>
  > :
    public typelist_filter_impl<
      N + 1,
      filter,
      typelist<types...>,
      typename std::conditional<
        filter<T>::value, typelist<filtered..., T>, typelist<filtered...>
        >::type,
      typename std::conditional<
        filter<T>::value,
        intlist<filteredidxs..., N>,
        intlist<filteredidxs..., N>
        >::type
      >
{};

template<
  int N, template <class> class filter, class... filtered, int... filteredidxs
  >
struct typelist_filter_impl<
  N, filter, typelist<>, typelist<filtered...>, intlist <filteredidxs...>
  >
{
  using list    = typelist<filtered...>;
  using indexes = intlist <filteredidxs...>;
};

template <template <class> class filter, class... types>
struct typelist_filter : public typelist_filter<filter, typelist<types...> > {};

template <template <class> class filter, class... types>
struct typelist_filter<filter, typelist<types...> > :
  public typelist_filter_impl<
    0, filter, typelist<types...>, typelist<>, intlist<>
    >
{};
#endif
/*----------------------------------------------------------------------------*/
template<class T>
using remove_cvref_t =
  typename std::remove_cv<typename std::remove_reference<T>::type>::type;
/*----------------------------------------------------------------------------*/
}} //namespace malcpp { namespace detail {

#endif