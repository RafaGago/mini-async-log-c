#ifndef __MALC_METAPROGRAMMING_COMMON__
#define __MALC_METAPROGRAMMING_COMMON__

#include <type_traits>

namespace malcpp { namespace detail {
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

}} //namespace malcpp { namespace detail {

#endif