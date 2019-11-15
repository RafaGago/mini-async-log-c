#ifndef __MALC_CPP11__
#define __MALC_CPP11__

#include <type_traits>
#define MALC_COMMON_NAMESPACED
#include <malc/impl/common.h>

namespace malcpp { namespace detail { namespace fmt {

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
};
//------------------------------------------------------------------------------
#if 0
// this will be needed for tuples
//------------------------------------------------------------------------------
template<int ...>
struct fwd_sequence { };

template<int N, int ...S>
struct fwd_sequence_gen : fwd_sequence_gen<N-1, N-1, S...> { };

template<int ...S>
struct fwd_sequence_gen<0, S...> {
  typedef fwd_sequence<S...> type;
};
#endif

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
template <class...>
struct typelist {};
//------------------------------------------------------------------------------
template <class... types>
typelist<types...> make_typelist (types... args)
{
  return typelist<types...>();
}
//------------------------------------------------------------------------------
template <class... >
struct typelist_next;

template <>
struct typelist_next<typelist<> > {
  using first     = typelist<>;
  using remainder = typelist<>;
};

template <class T, class... types>
struct typelist_next <typelist<T, types...> > {
  using first     = T;
  using remainder = typelist<types...>;
};
//------------------------------------------------------------------------------
struct normal_iteration_tag {};
struct check_remainder_tag {};
//------------------------------------------------------------------------------
template <class T>
struct keep_iterating_args {
  using type = normal_iteration_tag;
};
template <>
struct keep_iterating_args<typelist<> > {
  using type = check_remainder_tag;
};
//------------------------------------------------------------------------------
class format_string {
public:
  //----------------------------------------------------------------------------
  template <class tlist>
  static constexpr int validate (literal l)
  {
    return iterate<1, tlist> (l, 0);
  }
private:
  //----------------------------------------------------------------------------
  struct error {
    template <int N, class type>
    struct when_parsing_arg {
      static int excess_placeholders_in_format_string()
      {
        return fmterr_excess_placeholders;
      }
      static int excess_arguments_on_log_call()
      {
        return fmterr_excess_arguments;
      }
      static int invalid_modifiers_in_placeholder()
      {
        return fmterr_invalid_modifiers;
      }
      static int unclosed_bracket_in_format_string()
      {
        return fmterr_unclosed_lbracket;
      }
    };
  };
  //----------------------------------------------------------------------------
  template <int N, class tlist>
  static constexpr int iterate (const literal& l, int litpos)
  {
    using iter_tag = typename keep_iterating_args<tlist>::type;
    using next     = typelist_next<tlist>;
    return consume<N, next> (l ,litpos, iter_tag());
  }
  //----------------------------------------------------------------------------
  template <int N, class tlistnext>
  static constexpr int consume(
    const literal& l, int litpos, normal_iteration_tag
    )
  {
    using first     = typename tlistnext::first;
    using remainder = typename tlistnext::remainder;
    using T = typename std::remove_reference<
      typename std::remove_cv<first>::type
      >::type;
    /* function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times*/
    return verify_next<N, tlistnext, first, remainder>(
      l, placeholder::validate_next<T> (l, litpos)
      );
  }
  //----------------------------------------------------------------------------
  template <int N, class tlistnext, class T, class remainder>
  static constexpr int verify_next(
    const literal& l, int validate_result
    )
  {
    // the called error functions here aren't constexpr, so they will generate a
    // compile error on static constexts while still allowing unit testing on
    // a non static context.
    return
      (validate_result == fmterr_notfound)
      ? error::when_parsing_arg<N, T>::excess_arguments_on_log_call()
      : (validate_result == fmterr_invalid_modifiers)
        ? error::when_parsing_arg<N, T>::invalid_modifiers_in_placeholder()
        : (validate_result == fmterr_unclosed_lbracket)
          ? error::when_parsing_arg<N, T>::unclosed_bracket_in_format_string()
          : iterate<N + 1, remainder> (l ,validate_result);
  }
  //----------------------------------------------------------------------------
  template <int N, class tlistnext>
  static constexpr int consume(
    const literal& l, int litpos, check_remainder_tag
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
  static constexpr int process_last_error (int err)
  {
    // the called error functions here aren't constexpr, so they will generate a
    // compile error on static constexts while still allowing unit testing on
    // a non static context.

    // deliberately ommitting the 80 char limit to make the message passed
    // through the data type and function name readable in one line on the
    // compiler output.
    typedef remainder_type_tag notype;
    return
      (err == fmterr_notfound)
      ? fmterr_success
      : (err == fmterr_unclosed_lbracket)
        ? error::when_parsing_arg<N, notype>::unclosed_bracket_in_format_string()
        : error::when_parsing_arg<N, notype>::excess_placeholders_in_format_string();
  }
};
//------------------------------------------------------------------------------
#if 0
template <int N, class... types>
bl_err log (void* handle, int severity, const char(&arr)[N], types... args)
{
  return 0;
}
#endif

}}} // namespace malcpp { namespace detail { namespace fmtstr {

#endif /* __MALC_CPP11__ */