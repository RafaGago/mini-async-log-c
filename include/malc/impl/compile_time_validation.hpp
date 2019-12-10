#ifndef __MALC_COMPILE_TIME_VALIDATION__
#define __MALC_COMPILE_TIME_VALIDATION__

#include <malc/impl/metaprogramming_common.hpp>
#include <malc/impl/serialization.hpp>

//------------------------------------------------------------------------------
namespace malcpp { namespace detail { namespace fmt {

struct remainder_type_tag {};

}// namespace fmt {
namespace serialization {

// A dummy type to check from the last argument placeholder on the format string
// to the end of the format string.
template <>
struct logged_type<::malcpp::detail::fmt::remainder_type_tag> {
  static constexpr bool validate_format_modifiers (literal const& l)
  {
    return l.size() == 0; //whatever we return will give excess placeholders.
  }
};

} // namespace serialization
namespace fmt { // string format validation
//------------------------------------------------------------------------------
enum fmterr : int {
  fmterr_success_with_refs   = 1,
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
  fmterr_invalid_type        = -10,
};
//------------------------------------------------------------------------------
struct fmtret {
  //----------------------------------------------------------------------------
  static constexpr unsigned valbytes = 2;
  static constexpr unsigned valbits  = 8 * valbytes;
  static constexpr unsigned valmask  = ((1u << valbits) - 1);
  //----------------------------------------------------------------------------
  static_assert (sizeof (unsigned) >= valbytes * 2, "");
  //----------------------------------------------------------------------------
  static constexpr unsigned make (bl_i16 code, unsigned arg)
  {
    return arg | (((unsigned) code) << valbits);
  }
  //----------------------------------------------------------------------------
  static constexpr bl_i16 get_code (unsigned fmtretval)
  {
    return (bl_i16)(fmtretval >> valbits);
  }
  //----------------------------------------------------------------------------
  static constexpr unsigned get_arg (unsigned fmtretval)
  {
    return fmtretval & valmask;
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
class placeholder {
public:
  //----------------------------------------------------------------------------
  template <class T>
  static constexpr int validate_next (const literal& l)
  {
    /* function chaining because variables can't be created inside constexpr,
    so to declare variables on has to call functions with extra args. The
    variables are created to avoid doing the same compile time calculations
    many times*/
    return validate_next_step1_lbracket<T> (l, find_lbracket (l, 0));
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
          end,
          serialization::get_logged_type<T>::validate_format_modifiers(
            l.substr (beg, end)
            )
          )
      : fmterr_unclosed_lbracket;
  }
  //----------------------------------------------------------------------------
  static constexpr int validate_next_step3_validation(
    int end, bool validation_ret
    )
  {
    return (validation_ret) ? end + 1 : fmterr_invalid_modifiers;
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
    return iterate<1, tlist> (l, 0);
  }
private:
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
    return
      (std::is_same<T, serialization::malc_refdtor>::value == false)
      ? (serialization::get_logged_type<T>::type_id !=
          serialization::malc_type_error)
        ? verify_next<N, tail>(
          l, litpos, placeholder::validate_next<T> (l.substr (litpos))
          )
        : fmtret::make (fmterr_invalid_type, N)
      : iterate<N + 1, tail> (l , litpos); //malc_refdtor: skipping.
  }
  //----------------------------------------------------------------------------
  template <int N, class tail>
  static constexpr unsigned verify_next(
    const literal& l, int placeholder_end_offset, int placeholder_end
    )
  {
    return
      (placeholder_end == fmterr_notfound)
      ? fmtret::make (fmterr_excess_arguments, N)
      : (placeholder_end < fmterr_success)
        ? fmtret::make (placeholder_end, N)
        : iterate<N + 1, tail> (l, placeholder_end_offset + placeholder_end);
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
        placeholder::validate_next<remainder_type_tag> (l.substr (litpos))
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
  static constexpr unsigned validate()
  {
    using next = typelist_fwit<tlist>;
    return do_validate<next> (typename next::has_more_args_type());
  }
private:
  //----------------------------------------------------------------------------
  template <class listfwit>
  static constexpr unsigned do_validate(
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
      reftypes + (int) serialization::is_malc_refvalue<T>::value,
      dtors + (int) std::is_same<T, serialization::malc_refdtor>::value,
      std::is_same<T, serialization::malc_refdtor>::value
      );
  }
  //----------------------------------------------------------------------------
  template <class listfwit>
  static constexpr unsigned do_validate(
    typelist_no_more_args,
    int reftypes  = 0,
    int dtors     = 0,
    bool dtorlast = false
    )
  {
    return
      (dtors > 0)
        ? (reftypes == 0)
          ? fmtret::make (fmterr_excess_refdtor, 0)
          : (dtors == 1) // dtors > 0 && reftypes > 0
            ? (dtorlast)
              ? fmtret::make (fmterr_success_with_refs, 0)
              : fmtret::make (fmterr_misplaced_refdtor, 0)
            : fmtret::make (fmterr_repeated_refdtor, 0)
        : (reftypes == 0) // dtors == 0
          ? fmtret::make (fmterr_success, 0)
          : fmtret::make (fmterr_missing_refdtor, 0);
  }
  //----------------------------------------------------------------------------
};
/*----------------------------------------------------------------------------*/
#define MALCPP_INVARG_LIT \
  "malc: invalid printf formatting modifiers for the given type on the "
#define MALCPP_INVARG_LIT_SFX " placeholder."
#define MALCPP_INVTYP_LIT "malc: invalid type on the "
#define MALCPP_INVTYP_LIT_SFX " argument."
/*----------------------------------------------------------------------------*/
template <int res, unsigned arg>
struct generate_compile_errors {
  static constexpr int get_result() { return res; };

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
  static constexpr bool invtype (unsigned num)
  {
    return (res != fmterr_invalid_type) || arg != num;
  }
  static constexpr bool invtype()
  {
    return (res != fmterr_invalid_type) || arg < 50;
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
    "malc: the last parameter of a call using \"memref/strref\" must be a \"refdtor\". Got nothing."
    );
  static_assert(
    misprefdt(),
    "malc: the \"redtor\" must be the last parameter of the log call."
    );
  static_assert(
    reprefdt(),
    "malc: Only one \"redtor\" must be the last parameter of the log call. Got many."
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
  static_assert (invtype (1),  MALCPP_INVTYP_LIT "1st"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (2),  MALCPP_INVTYP_LIT "2nd"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (3),  MALCPP_INVTYP_LIT "3rd"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (4),  MALCPP_INVTYP_LIT "4th"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (5),  MALCPP_INVTYP_LIT "5th"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (6),  MALCPP_INVTYP_LIT "6th"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (7),  MALCPP_INVTYP_LIT "7th"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (8),  MALCPP_INVTYP_LIT "8th"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (9),  MALCPP_INVTYP_LIT "9th"  MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (10), MALCPP_INVTYP_LIT "10th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (11), MALCPP_INVTYP_LIT "11th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (12), MALCPP_INVTYP_LIT "12th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (13), MALCPP_INVTYP_LIT "13th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (14), MALCPP_INVTYP_LIT "14th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (15), MALCPP_INVTYP_LIT "15th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (16), MALCPP_INVTYP_LIT "16th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (17), MALCPP_INVTYP_LIT "17th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (18), MALCPP_INVTYP_LIT "18th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (19), MALCPP_INVTYP_LIT "19th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (20), MALCPP_INVTYP_LIT "20th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (21), MALCPP_INVTYP_LIT "21th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (22), MALCPP_INVTYP_LIT "22th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (23), MALCPP_INVTYP_LIT "23th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (24), MALCPP_INVTYP_LIT "24th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (25), MALCPP_INVTYP_LIT "25th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (26), MALCPP_INVTYP_LIT "26th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (27), MALCPP_INVTYP_LIT "27th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (28), MALCPP_INVTYP_LIT "28th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (29), MALCPP_INVTYP_LIT "29th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (30), MALCPP_INVTYP_LIT "30th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (31), MALCPP_INVTYP_LIT "31th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (32), MALCPP_INVTYP_LIT "32th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (33), MALCPP_INVTYP_LIT "33th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (34), MALCPP_INVTYP_LIT "34th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (35), MALCPP_INVTYP_LIT "35th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (36), MALCPP_INVTYP_LIT "36th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (37), MALCPP_INVTYP_LIT "37th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (38), MALCPP_INVTYP_LIT "38th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (39), MALCPP_INVTYP_LIT "39th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (40), MALCPP_INVTYP_LIT "40th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (41), MALCPP_INVTYP_LIT "41th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (42), MALCPP_INVTYP_LIT "42th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (43), MALCPP_INVTYP_LIT "43th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (44), MALCPP_INVTYP_LIT "44th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (45), MALCPP_INVTYP_LIT "45th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (46), MALCPP_INVTYP_LIT "46th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (47), MALCPP_INVTYP_LIT "47th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (48), MALCPP_INVTYP_LIT "48th" MALCPP_INVTYP_LIT_SFX);
  static_assert (invtype (49), MALCPP_INVTYP_LIT "49th" MALCPP_INVTYP_LIT_SFX);
  static_assert(
    invtype(),"malc: invalid argument type over the 49th argument"
    );
};
/*----------------------------------------------------------------------------*/
template <unsigned arg>
struct generate_compile_errors<fmterr_success, arg>
{
  static constexpr int get_result() { return fmterr_success; };
};
/*----------------------------------------------------------------------------*/
template <unsigned arg>
struct generate_compile_errors<fmterr_success_with_refs, arg>
{
  static constexpr int get_result() { return fmterr_success_with_refs; };
};
/*----------------------------------------------------------------------------*/
template <unsigned refserr, unsigned fmtstrerr>
static constexpr int static_arg_validation()
{
  return generate_compile_errors<
    fmtret::get_code (fmtstrerr) < fmterr_success ?
      fmtret::get_code (fmtstrerr) : fmtret::get_code (refserr),
    fmtret::get_code (fmtstrerr) < fmterr_success ?
      fmtret::get_arg (fmtstrerr) : fmtret::get_arg (refserr)
    >().get_result();
};
/*----------------------------------------------------------------------------*/
}}} //namespace malcpp { namespace detail { namespace fmtstr {

#endif
