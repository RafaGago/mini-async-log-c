#ifndef __MALC_CPP11__
#define __MALC_CPP11__

#ifndef MALC_COMMON_NAMESPACED
  #error "Don't try to include this file directly"
#endif

#include <type_traits>
#include <tuple>

#include <bl/base/preprocessor_basic.h>

#include <malc/common.h>
#include <malc/impl/common.h>
#include <malc/impl/logging.h>
#include <malc/impl/serialization.h>

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
//------------------------------------------------------------------------------
template <class T>
struct is_malc_refvalue {
  static constexpr bool value =
    std::is_same<remove_cvref_t<T>, malc_memref>::value |
    std::is_same<remove_cvref_t<T>, malc_strref>::value;
};
//------------------------------------------------------------------------------
namespace fmt { // string format validation
//------------------------------------------------------------------------------
class literal {
public:
  //----------------------------------------------------------------------------
  template <int N>
  constexpr literal (const char(&arr)[N]) :
      m_lit  (arr),
      m_size (N - 1)
  {
      static_assert (N >= 1, "not a string literal");
  }
  //----------------------------------------------------------------------------
  constexpr operator const char*() const  { return m_lit; }
  constexpr int size() const              { return m_size; }
  constexpr char operator[] (int i) const { return m_lit[i]; }
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
private:
  //--------------------------------------------------------------------------
  const char *const m_lit;
  const int         m_size;
    //--------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
enum fmterr : int {
  fmterr_success             = 0,
  fmterr_notfound            = -1,
  fmterr_invalid_modifiers   = -2,
  fmterr_excess_placeholders = -3,
  fmterr_excess_arguments    = -4,
  fmterr_unclosed_lbracket   = -5,
  fmterr_missing_refdtor     = -6,
  fmterr_misplaced_refdtor   = -7,
  fmterr_repeated_refdtor    = -8,
  fmterr_excess_refdtor      = -9,
};
//------------------------------------------------------------------------------
struct fmtret {
  static constexpr unsigned argbits = 16;
  //----------------------------------------------------------------------------
  static constexpr unsigned make (int code, unsigned arg)
  {
    return arg | (((unsigned) (code * -1)) << argbits);
  }
  //----------------------------------------------------------------------------
  static constexpr int get_code (unsigned fmtretval)
  {
    return ((int)(fmtretval >> argbits)) * -1;
  }
  //----------------------------------------------------------------------------
  static constexpr unsigned get_arg (unsigned fmtretval)
  {
    return fmtretval & ((1 << argbits) - 1);
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
struct remainder_type_tag {};
//------------------------------------------------------------------------------
class modifiers {
public:
  //----------------------------------------------------------------------------
  template<class T>
  static constexpr typename std::enable_if<
    std::is_integral<T>::value, int
    >::type
    validate (const literal& l, int beg, int end, T*)
  {
    return (beg == end)
      ? fmterr_success
      : validate_int (l, beg, end, literal (" #+-0"), beg);
  }
  //----------------------------------------------------------------------------
  template<class T>
  static constexpr typename std::enable_if<
    std::is_floating_point<T>::value, int
    >::type
    validate (const literal& l, int beg, int end, T*)
  {
    /*TBI*/
    return (beg == end)
      ? fmterr_success
      : validate_float (l, beg, end, literal (" #+-0"), beg);
  }
  //----------------------------------------------------------------------------
  template<class T>
  static constexpr typename std::enable_if<
    std::is_same<T, remainder_type_tag>::value, int
    >::type
    validate (const literal& l, int beg, int end, T*)
  {
    /* This is called when all function parameters are processed to check if
    there are still placeholders on the string. At this point anything we return
    other than "fmterr_notfound" will be an error.

    If there is a consecutive open and close bracket we consider it as a found
    placeholder, it will be reported  as "fmt_excess_placeholders". If they are
    non-consecutive we consider that the user forgot to escape the left bracket.
    Both return will generate an error on the parser. */
    return (beg == end) ?  fmterr_success : fmterr_unclosed_lbracket;
  }
  //----------------------------------------------------------------------------
  template <class T>
  static constexpr int validate (const literal& l, int beg, int end, ...)
  {
    /* unknown types have no modifiers, type validation happens later.
    "..." the elipsis operator gives this overload the lowest priority when
    searching for resolution candidates. */
    return (beg == end) ? fmterr_success : fmterr_invalid_modifiers;
  }
private:
  //----------------------------------------------------------------------------
  static constexpr int validate_specifiers(
    const literal& l, int beg, int end, const literal& modf
    )
  {
    return
      (beg < end)
      ? (end - beg == 1)
        ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
          //keep validating
          ? validate_specifiers (l, beg + 1, end, modf)
          //unknown modifier
          : fmterr_invalid_modifiers
        //only one specifier allowed
        : fmterr_invalid_modifiers
      : fmterr_success;
  }
  //----------------------------------------------------------------------------
  static constexpr int validate_int_width(
    const literal& l,
    int beg,
    int end,
    const literal& modf,
    int own = 0,
    int num = 0
    )
  {
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? (l[beg] == 'W' || l[beg] == 'N')
          ? (own == 0 && num == 0)
            ? validate_int_width (l, beg + 1, end, modf, own + 1, num)
            : fmterr_invalid_modifiers
          : (own == 0)
            ? validate_int_width (l, beg + 1, end, modf, own, num + 1 )
            : fmterr_invalid_modifiers
        //next stage
        : validate_specifiers (l, beg, end, literal ("xXo"))
      : fmterr_success;
  }
  //----------------------------------------------------------------------------
  static constexpr int validate_int(
    const literal& l, int beg, int end, const literal& modf, int stgbeg
    )
  {
    // this function validates the flags or jumps to the width modifiers
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? validate_int (l, beg + 1, end, modf, stgbeg)
        //next stage
        : l.has_repeated_chars (stgbeg, beg)
          ? fmterr_invalid_modifiers
          : validate_int_width (l, beg, end, literal ("WN0123456789"))
      : l.has_repeated_chars (stgbeg, beg)
        ? fmterr_invalid_modifiers
        : fmterr_success;
  }
  //----------------------------------------------------------------------------
  static constexpr int validate_float_width_precision(
    const literal& l, int beg, int end, const literal& modf, int has_dot = 0
    )
  {
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? (has_dot == 0 || l[beg] != '.')
          ? validate_float_width_precision(
            l, beg + 1, end, modf, has_dot + (l[beg] == '.')
            )
          // duplicated dot
          : fmterr_invalid_modifiers
        //next stage
        : validate_specifiers (l, beg, end, literal ("fFeEgGaA"))
      : fmterr_success;
  }
  //----------------------------------------------------------------------------
  static constexpr int validate_float(
    const literal& l, int beg, int end, const literal& modf, int stgbeg
    )
  {
    // this function validates the flags or jumps to the width/precision
    // modifiers
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? validate_float (l, beg + 1, end, modf, stgbeg)
        //next stage
        : l.has_repeated_chars (stgbeg, beg)
          ? fmterr_invalid_modifiers
          : validate_float_width_precision(
            l, beg, end, literal (".0123456789")
            )
      : l.has_repeated_chars (stgbeg, beg)
        ? fmterr_invalid_modifiers
        : fmterr_success;
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
class placeholder {
public:
  //----------------------------------------------------------------------------
  template <class T>
  static constexpr int validate_next (const literal& l, int pos)
  {
    /* function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times*/
    return validate_next_step1_lbracket<T> (l, find_lbracket (l, pos));
  }
private:
  //----------------------------------------------------------------------------
  static constexpr int find_lbracket(
    const literal& l, int pos, bool prev_is_lbracket = false
    )
  {
    return
      (pos < l.size())
      ? (prev_is_lbracket)
        ? (l[pos] != '{')
          ? pos // found: bracket on previous iteration is unescaped
          : find_lbracket (l, pos + 1, false) // previous was escaped
        : find_lbracket (l, pos + 1, (l[pos] == '{'))
      : (prev_is_lbracket)
        ? pos
        : fmterr_notfound;
  }
  //----------------------------------------------------------------------------
  static constexpr int find_rbracket (const literal& l, int pos)
  {
    return l.find ('}', pos, l.size(), fmterr_notfound);
  }
  //----------------------------------------------------------------------------
  template <class T>
  static constexpr int validate_next_step1_lbracket (const literal& l, int beg)
  {
    /* function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times*/
    return (beg != fmterr_notfound) ?
      validate_next_step2_rbracket<T> (l, beg, find_rbracket (l, beg)) : beg;
  }
  //----------------------------------------------------------------------------
  template <class T>
  static constexpr int validate_next_step2_rbracket(
    const literal& l, int beg, int end
    )
  {
    /* function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times*/
    return (end != fmterr_notfound)
      ? validate_next_step3_validation(
          end, modifiers::validate<T> (l, beg, end, (T*) nullptr)
          )
      : fmterr_unclosed_lbracket;
  }
  //----------------------------------------------------------------------------
  static constexpr int validate_next_step3_validation(
    int end, int validation_ret
    )
  {
    return (validation_ret == fmterr_success) ? end + 1 : validation_ret;
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
/* the validation for the reference values is done separately to avoid
   complicating things by adding too many parameters and hard to follow ternary
   operators on the main validation class.

   This is done at the expense of iterating the parameter list twice. If
   compile-time performance is a problem this can be optimized to  be done in a
   single pass.*/
//------------------------------------------------------------------------------
class refvalues {
public:
  //----------------------------------------------------------------------------
  /* returns the amount of elements to remove from the typelist for the
  validation phase or an error */
  template <class tlist>
  static constexpr int validate()
  {
    using next = typelist_fwit<tlist>;
    return do_validate<next> (typename next::has_more_args_type());
  }
private:
  //----------------------------------------------------------------------------
  template <class listfwit>
  static constexpr int do_validate(
    typelist_has_args,
    int reftypes  = 0,
    int dtors     = 0,
    bool dtorlast = false
    )
  {
    using head = typename listfwit::head;
    using tail = typename listfwit::tail;
    using T =    remove_cvref_t<head>;
    using next = typelist_fwit<tail>;
    return do_validate<next>(
      typename next::has_more_args_type(),
      reftypes + (int) is_malc_refvalue<T>::value,
      dtors + (int) std::is_same<T, malc_refdtor>::value,
      std::is_same<T, malc_refdtor>::value
      );
  }
  //----------------------------------------------------------------------------
  template <class listfwit>
  static constexpr int do_validate(
    typelist_no_more_args,
    int reftypes  = 0,
    int dtors     = 0,
    bool dtorlast = false
    )
  {
    return
      (dtors > 0)
        ? (reftypes == 0)
          ? fmterr_excess_refdtor
          : (dtors == 1) // dtors > 0 && reftypes > 0
            ? (dtorlast)
              ? 1
              : fmterr_misplaced_refdtor
            : fmterr_repeated_refdtor
        : (reftypes == 0) // dtors == 0
          ? 0
          : fmterr_missing_refdtor;
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
class format_string {
public:
  //----------------------------------------------------------------------------
  template <class tlist>
  static constexpr unsigned validate (literal l)
  {
    return process_ref_args<tlist, refvalues::validate<tlist>()> (l);
  }
private:
  //----------------------------------------------------------------------------
  template <class tlist, int valresult>
  static constexpr unsigned process_ref_args (literal l)
  {
    return (valresult >= 0) ?
      iterate<1, tlist> (l, 0) :
      fmtret::make (valresult, 1);
  }
  //----------------------------------------------------------------------------
  template <int N, class tlist>
  static constexpr unsigned iterate (const literal& l, int litpos)
  {
    using next = typelist_fwit<tlist>;
    return consume<N, next>(
      l ,litpos, typename next::has_more_args_type()
      );
  }
  //----------------------------------------------------------------------------
  template <int N, class listfwit>
  static constexpr unsigned consume(
    const literal& l, int litpos, typelist_has_args
    )
  {
    using head = typename listfwit::head;
    using tail = typename listfwit::tail;
    using T    = remove_cvref_t<head>;
    /* function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times.. */
    return std::is_same<T, malc_refdtor>::value == false
      ? verify_next<N, tail> (l, placeholder::validate_next<T> (l, litpos))
      : iterate<N + 1, tail> (l , litpos); //malc_refdtor: next.
  }
  //----------------------------------------------------------------------------
  template <int N, class tail>
  static constexpr unsigned verify_next(
    const literal& l, int validate_result
    )
  {
    return
      (validate_result == fmterr_notfound)
      ? fmtret::make (fmterr_excess_arguments, N)
      : (validate_result < fmterr_success)
        ? fmtret::make (validate_result, N)
        : iterate<N + 1, tail> (l , validate_result);
  }
  //----------------------------------------------------------------------------
  template <int N, class listfwit>
  static constexpr unsigned consume(
    const literal& l, int litpos, typelist_no_more_args
    )
  {
    /* no remaining function arguments, check if the format is correct on the
    tail of the format string.

    function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times */
    return process_last_error<N>(
        placeholder::validate_next<remainder_type_tag> (l ,litpos)
        );
  }
  //----------------------------------------------------------------------------
  template <int N>
  static constexpr unsigned process_last_error (int err)
  {
    return
      (err == fmterr_notfound)
      ? fmtret::make (fmterr_success, N)
      : (err == fmterr_unclosed_lbracket)
        ? fmtret::make (fmterr_unclosed_lbracket, N)
        : fmtret::make (fmterr_excess_placeholders, N);
  }
};
/*----------------------------------------------------------------------------*/
#define MALCPP_INVARG_LIT \
  "malc: invalid printf formating modifiers for the given type on the "
#define MALCPP_INVARG_LIT_SFX " placeholder."
/*----------------------------------------------------------------------------*/
template <int res, unsigned arg>
struct generate_compile_errors {
  static constexpr bool excessargs()
  {
    return (res != fmterr_excess_arguments);
  }
  static constexpr bool excesspchs()
  {
    return (res != fmterr_excess_placeholders);
  }
  static constexpr bool unclosedb()
  {
    return (res != fmterr_unclosed_lbracket);
  }
  static constexpr bool invmodif (unsigned num)
  {
    return (res != fmterr_invalid_modifiers) || arg != num;
  }
  static constexpr bool invmodif()
  {
    return (res != fmterr_invalid_modifiers) || arg < 50;
  }
  static constexpr bool missrefdt()
  {
    return (res != fmterr_missing_refdtor);
  }
  static constexpr bool misprefdt()
  {
    return (res != fmterr_misplaced_refdtor);
  }
  static constexpr bool reprefdt()
  {
    return (res != fmterr_repeated_refdtor);
  }
  static constexpr bool xsrefdt()
  {
    return (res != fmterr_excess_refdtor);
  }
  static_assert(
    excessargs(), "malc: too little placeholders in format string"
    );
  static_assert (excesspchs(), "malc: too many placeholders in format string.");
  static_assert (unclosedb(), "malc: unclosed left bracket in format string.");
  static_assert(
    missrefdt(),
    "malc: the last parameter of a call when using \"memref/strref\" must be a \"refdtor\". Got nothing."
    );
  static_assert(
    misprefdt(),
    "malc: the \"redtor\" must be the last parameter of the log call."
    );
  static_assert(
    reprefdt(),
    "malc: the \"redtor\" must be the last parameter of the log call. Got more than one."
    );
  static_assert(
    xsrefdt(),
    "malc: got a \"redtor\" without any \"memref/strref\" type on the call parameters."
    );
  static_assert (invmodif (1), MALCPP_INVARG_LIT "1st" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (2), MALCPP_INVARG_LIT "2nd" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (3), MALCPP_INVARG_LIT "3rd" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (4), MALCPP_INVARG_LIT "4th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (5), MALCPP_INVARG_LIT "5th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (6), MALCPP_INVARG_LIT "6th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (7), MALCPP_INVARG_LIT "7th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (8), MALCPP_INVARG_LIT "8th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (9), MALCPP_INVARG_LIT "9th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (10), MALCPP_INVARG_LIT "10th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (11), MALCPP_INVARG_LIT "11th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (12), MALCPP_INVARG_LIT "12th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (13), MALCPP_INVARG_LIT "13th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (14), MALCPP_INVARG_LIT "14th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (15), MALCPP_INVARG_LIT "15th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (16), MALCPP_INVARG_LIT "16th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (17), MALCPP_INVARG_LIT "17th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (18), MALCPP_INVARG_LIT "18th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (19), MALCPP_INVARG_LIT "19th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (20), MALCPP_INVARG_LIT "20th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (21), MALCPP_INVARG_LIT "21th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (22), MALCPP_INVARG_LIT "22th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (23), MALCPP_INVARG_LIT "23th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (24), MALCPP_INVARG_LIT "24th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (25), MALCPP_INVARG_LIT "25th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (26), MALCPP_INVARG_LIT "26th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (27), MALCPP_INVARG_LIT "27th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (28), MALCPP_INVARG_LIT "28th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (29), MALCPP_INVARG_LIT "29th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (30), MALCPP_INVARG_LIT "30th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (31), MALCPP_INVARG_LIT "31th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (32), MALCPP_INVARG_LIT "32th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (33), MALCPP_INVARG_LIT "33th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (34), MALCPP_INVARG_LIT "34th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (35), MALCPP_INVARG_LIT "35th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (36), MALCPP_INVARG_LIT "36th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (37), MALCPP_INVARG_LIT "37th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (38), MALCPP_INVARG_LIT "38th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (39), MALCPP_INVARG_LIT "39th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (40), MALCPP_INVARG_LIT "40th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (41), MALCPP_INVARG_LIT "41th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (42), MALCPP_INVARG_LIT "42th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (43), MALCPP_INVARG_LIT "43th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (44), MALCPP_INVARG_LIT "44th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (45), MALCPP_INVARG_LIT "45th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (46), MALCPP_INVARG_LIT "46th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (47), MALCPP_INVARG_LIT "47th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (48), MALCPP_INVARG_LIT "48th" MALCPP_INVARG_LIT_SFX);
  static_assert (invmodif (49), MALCPP_INVARG_LIT "49th" MALCPP_INVARG_LIT_SFX);
  static_assert(
    invmodif(),
    "malc: invalid printf formating modifiers for the given type on a placeholder over the 49th"
    );
};
/*----------------------------------------------------------------------------*/
template <unsigned arg>
struct generate_compile_errors<fmterr_success, arg> {};
/*----------------------------------------------------------------------------*/
template <unsigned fmtretval>
static void static_validation()
{
  generate_compile_errors<
    fmtret::get_code (fmtretval), fmtret::get_arg (fmtretval)
    >();
};
/*----------------------------------------------------------------------------*/
} //namespace fmtstr {
/*----------------------------------------------------------------------------*/
namespace serialization {
/*----------------------------------------------------------------------------*/
template <typename T>
struct type_base {
  static inline bl_uword size (T v)      { return sizeof v; }
  static inline T        transform (T v) { return v; }
};

template<typename T>
struct type {};

template<>
struct type<float> : public type_base<float> {
  static const char id = malc_type_float;
};

template<>
struct type<double> : public type_base<double> {
  static const char id = malc_type_double;
};

template<>
struct type<bl_i8> :public type_base<bl_i8> {
  static const char id = malc_type_i8;
};

template<>
struct type<bl_u8> : public type_base<bl_u8> {
  static const char id = malc_type_u8;
};

template<> struct type<bl_i16> : public type_base<bl_i16> {
  static const char id = malc_type_i16;
};

template<> struct type<bl_u16> : public type_base<bl_u16> {
    static const char id = malc_type_u16;
};
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<> struct type<bl_i32> : public type_base<bl_i32> {
  static const char id = malc_type_i32;
};

template<> struct type<bl_u32> : public type_base<bl_u32> {
  static const char id = malc_type_u32;
};

template<> struct type<bl_i64> : public type_base<bl_i64> {
  static const char id = malc_type_i64;
};

template<> struct type<bl_u64> : public type_base<bl_u64> {
  static const char id = malc_type_u64;
};
/*----------------------------------------------------------------------------*/
#else /* #if MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
// as the compression the only that does is to look for trailing zeros, the
// integers under 4 bytes and floating point values are omitted. This
// compression works best if a lot of 64-bit integers with small values are
// passed.
template<>
struct type<bl_i32> {
  static const char id = malc_type_i32;
  static inline malc_compressed_32 transform (bl_i32 v)
  {
    return malc_get_compressed_i32 (v);
  }
};

template<>
struct type<bl_u32> {
  static const char id = malc_type_u32;
  static inline malc_compressed_32 transform (bl_u32 v)
  {
    return malc_get_compressed_u32 (v);
  }
};

template<>
struct type<bl_i64> {
  static const char id = malc_type_i64;
  static inline malc_compressed_64 transform (bl_i64 v)
  {
    return malc_get_compressed_i64 (v);
  }
};

template<>
struct type<bl_u64> {
  static const char id = malc_type_u64;
  static inline malc_compressed_64 transform (bl_u64 v)
  {
    return malc_get_compressed_u64 (v);
  }
};
/*----------------------------------------------------------------------------*/
#endif /* #else  #if MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<> struct type<void*> : public type_base<void*> {
    static const char id = malc_type_ptr;
};

template<> struct type<malc_lit> : public type_base<malc_lit> {
    static const char id = malc_type_lit;
};

template<> struct type<malc_strref> {
  static const char id = malc_type_strref;
  static inline malc_strref transform (malc_strref v) { return v; }
  static inline bl_uword size (malc_strref v)
  {
    return bl_sizeof_member (malc_strref, str) +
           bl_sizeof_member (malc_strref, len);
  }
};

template<> struct type<malc_memref> {
  static const char id = malc_type_memref;
  static inline malc_memref transform (malc_memref v) { return v; }
  static inline bl_uword size (malc_memref v)
  {
    return bl_sizeof_member (malc_memref, mem) +
           bl_sizeof_member (malc_memref, size);
  }
};

template<>
struct type<malc_refdtor> {
  static const char id = malc_type_refdtor;
  static inline malc_refdtor transform (malc_refdtor v) { return v; }
  static inline bl_uword size (malc_refdtor v)
  {
    return bl_sizeof_member (malc_refdtor, func) +
           bl_sizeof_member (malc_refdtor, context);
  }
};
/*----------------------------------------------------------------------------*/
#else // #if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<>
struct type<void*> {
  static const char id = malc_type_ptr;
  static inline malc_compressed_ptr transform (void* v)
  {
    return malc_get_compressed_ptr (v);
  }
};

template<>
struct type<malc_lit> {
  static const char id = malc_type_lit;
  static inline malc_compressed_ptr transform (malc_lit v)
  {
    return malc_get_compressed_ptr ((void*) v.lit);
  }
};

template<>
struct type<malc_strref> {
  static const char id = malc_type_strref;
  static inline malc_compressed_ref transform (malc_strref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.str);
    r.size = v.len;
    return r;
  }
};

template<>
struct type<malc_memref> {
  static const char id = malc_type_memref;
  static inline malc_compressed_ref transform (malc_memref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.mem);
    r.size = v.size;
    return r;
  }
};

template<>
struct type<malc_refdtor> {
  static const char id = malc_type_refdtor;
  static inline malc_compressed_refdtor transform (malc_refdtor v)
  {
    malc_compressed_refdtor r;
    r.func    = malc_get_compressed_ptr ((void*) v.func);
    r.context = malc_get_compressed_ptr ((void*) v.context);
    return r;
  }
};
/*----------------------------------------------------------------------------*/
#endif // #if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<>
struct type<const void*> : public type<void*> {};

template<>
struct type<volatile void*> : public type<void*> {};

template<>
struct type<const volatile void*> : public type<void*> {};

template<>
struct type<void* const> : public type<void*> {};

template<>
struct type<const void* const> : public type<void*> {};

template<>
struct type<volatile void* const> : public type<void*> {};

template<>
struct type<const volatile void* const> : public type<void*> {};

template<>
struct type<malc_strcp> {
  static const char id  = malc_type_strcp;
  static inline malc_strcp transform (malc_strcp v) { return v; }
  static inline bl_uword size (malc_strcp v)
  {
    return bl_sizeof_member (malc_strcp, len) + v.len;
  }
};

template<>
struct type<malc_memcp> {
  static const char id = malc_type_memcp;
  static inline malc_memcp transform (malc_memcp v) { return v; }
  static inline bl_uword size (malc_memcp v)
  {
    return bl_sizeof_member (malc_memcp, size) + v.size;
  }
};
/*----------------------------------------------------------------------------*/
#if MALC_COMPRESSION == 1
/*----------------------------------------------------------------------------*/
template<>
struct type<malc_compressed_32> {
  static inline bl_uword size (malc_compressed_32 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};

template<>
struct type<malc_compressed_64> {
  static inline bl_uword size (malc_compressed_64 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};

template<>
struct type<malc_compressed_ref> {
  static inline bl_uword size (malc_compressed_ref v)
  {
    return bl_sizeof_member (malc_compressed_ref, size) +
           malc_compressed_get_size (v.ref.format_nibble);
  }
};

template<>
struct type<malc_compressed_refdtor> {
  static inline bl_uword size (malc_compressed_refdtor v)
  {
    return malc_compressed_get_size (v.func.format_nibble) +
           malc_compressed_get_size (v.context.format_nibble);
  }
};
/*----------------------------------------------------------------------------*/
#endif /* #if MALC_COMPRESSION == 1 */
/*----------------------------------------------------------------------------*/
} // namespace serialization {
/*----------------------------------------------------------------------------*/
template <int sev, class T>
struct info {};

template <int sev, template <class...> class T, class... Args>
struct info<sev, T<Args...> >
{
  static const char* generate()
  {
    static const char info[] = {
      (char) sev,
      (::malcpp::detail::serialization::type<remove_cvref_t<Args> >::id) ...,
      0
    };
    return info;
  }
};
/*----------------------------------------------------------------------------*/
template <int END, int N = 0>
class arg_ops {
public:
  //----------------------------------------------------------------------------
  template <class T>
  static inline bl_uword get_payload_size (const T& tup)
  {
    bl_uword v = serialization::type<
      remove_cvref_t<decltype (std::get<N>(tup))>
        >::size (std::get<N>(tup));
    return v + arg_ops<END, N + 1>::get_payload_size (tup);
  }
  //----------------------------------------------------------------------------
  template <class T>
  static inline void serialize (malc_serializer& ser, const T& tup)
  {
    malc_serialize (&ser, std::get<N>(tup));
    arg_ops<END, N + 1>::serialize (ser, tup);
  }
  //----------------------------------------------------------------------------
  template <class T, int... refs>
  static inline void try_deallocate_refvalues(
    const T& tup, malc_refdtor* dtor, intlist<refs...>
    )
  {
    arg_ops<END, N + 1>::try_deallocate_refvalues(
      tup,
      arg_ops<END, N>::get_refdtor_pointer (std::get<N> (tup)),
      typename std::conditional<
        is_malc_refvalue<decltype (std::get<N> (tup))>::value,
        intlist<refs..., N>,
        intlist<refs...>
        >::type()
      );
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  template <class T>
  static inline malc_refdtor* get_refdtor_pointer (T)
  {
    return nullptr;
  }
  //----------------------------------------------------------------------------
  static inline malc_refdtor* get_refdtor_pointer (malc_refdtor& rd)
  {
    return &rd;
  }
  //----------------------------------------------------------------------------
};

template <int END>
class arg_ops<END, END> {
public:
  //----------------------------------------------------------------------------
  template <class T>
  static inline bl_uword get_payload_size (const T& tup)
  {
    return 0;
  }
  //----------------------------------------------------------------------------
  template <class T>
  static inline void serialize (malc_serializer& ser, const T& tup)
  {}
  //----------------------------------------------------------------------------
  template <class T, int... refs>
  static inline void try_deallocate_refvalues(
    const T& tup, malc_refdtor* dtor, intlist<refs...>
    )
  {
    static const int refscount = intlist_sizeof<intlist<refs...> >::value;
    if (refscount > 0 && dtor && dtor->func) {
      malc_ref r[] = { to_malc_ref (std::get<refs> (tup))... };
      dtor->func (dtor->context, r, refscount);
    }
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  static inline malc_ref to_malc_ref (const malc_memref& v)
  {
    malc_ref r = { v.mem, v.size };
    return r;
  }
  //----------------------------------------------------------------------------
  static inline malc_ref to_malc_ref (const malc_strref& v)
  {
    malc_ref r = { v.str, v.len };
    return r;
  }
  //----------------------------------------------------------------------------
};
/*----------------------------------------------------------------------------*/
template <class malctype, class... types>
static bl_err log(
  malc_const_entry const& en, malctype& malc, const char*, types... args
  )
{
  using argops = arg_ops<sizeof...(types)>;
  auto values = std::make_tuple(
    serialization::type<types>::transform (args)...
    );
  malc_serializer s;
  bl_err err = malc_log_entry_prepare(
    malc.handle(), &s, &en, argops::get_payload_size (values)
    );
  if (err.own) {
#if MALC_PTR_COMPRESSION == 0
      auto& values_untransformed = values;
#else
      auto values_untransformed = std::make_tuple (args...);
#endif
      argops::try_deallocate_refvalues(
        values_untransformed, nullptr, intlist<>()
        );
    //}
    return err;
  }
  argops::serialize (s, values);
  malc_log_entry_commit (malc.handle(), &s);
  return err;
}
//------------------------------------------------------------------------------
}} // namespace malcpp { namespace detail {
/*----------------------------------------------------------------------------*/
/* Implementation of  MALC_LOG_IF_PRIVATE/MALC_LOG_PRIVATE */
/*----------------------------------------------------------------------------*/
#define MALC_LOG_IF_PRIVATE(cond, malcref, sev, ...) \
/* Reminder: This can't be done without macros because the arguments must  */ \
/* be only evaluated (lazily) when logging */ \
  [&]() { \
    bl_err err = bl_mkok(); \
    /* Trigger string literal validation */ \
    ::malcpp::detail::fmt::static_validation< \
      ::malcpp::detail::fmt::format_string::validate< \
        decltype (malcpp::detail::make_typelist( \
          bl_pp_vargs_ignore_first (__VA_ARGS__) \
          )) \
        > (bl_pp_vargs_first (__VA_ARGS__) "") \
      >(); \
    if ((cond) && (sev) >= malc_get_min_severity (malcref.handle())) { \
      /* Save known data at compile time, so we don't pass it to the logger */ \
      static const malc_const_entry bl_pp_tokconcat (malcppent, __LINE__) =  {\
        bl_pp_vargs_first (__VA_ARGS__), \
        ::malcpp::detail::info< \
          sev, \
          decltype (malcpp::detail::make_typelist( \
            bl_pp_vargs_ignore_first (__VA_ARGS__) \
          ))>::generate(), \
        0 \
      }; \
      /* Make the log call, the malc ptr will always be the first ARG, */ \
      /* otherwise the __VA_ARGS__ expansion should be more complicated. */ \
      err = ::malcpp::detail::log( \
        bl_pp_tokconcat (malcppent, __LINE__), malcref, __VA_ARGS__ \
        ); \
    } \
    return err; \
  }()
/*----------------------------------------------------------------------------*/
#define MALC_LOG_PRIVATE(malcref, sev, ...) \
    MALC_LOG_IF_PRIVATE (1, (malcref), (sev), __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#endif /* __MALC_CPP11__ */
