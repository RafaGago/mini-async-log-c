#ifndef __MALC_CUSTOM_TYPE_HPP__
#define __MALC_CUSTOM_TYPE_HPP__

#include <string>
#include <vector>
#include <type_traits>

#include <bl/base/platform.h>

#include <malcpp/malcpp.hpp>

// base resources to define custom log types. It won't work if you include
// malc_lean.hpp before.

namespace malcpp {
//------------------------------------------------------------------------------
template <uint8_t type_value>
struct builtin_map {
  static constexpr uint8_t value = type_value;
};
//------------------------------------------------------------------------------
template <class T>
struct builtin_type_map;

template<>
struct builtin_type_map<uint8_t>  : public builtin_map<malc_obj_u8> {};
template<>
struct builtin_type_map<int8_t>  : public builtin_map<malc_obj_i8> {};
template<>
struct builtin_type_map<uint16_t> : public builtin_map<malc_obj_u16> {};
template<>
struct builtin_type_map<int16_t> : public builtin_map<malc_obj_i16> {};
template<>
struct builtin_type_map<uint32_t> : public builtin_map<malc_obj_u32> {};
template<>
struct builtin_type_map<int32_t> : public builtin_map<malc_obj_i32> {};
template<>
struct builtin_type_map<uint64_t> : public builtin_map<malc_obj_u64> {};
template<>
struct builtin_type_map<int64_t> : public builtin_map<malc_obj_i64> {};
template<>
struct builtin_type_map<float> : public builtin_map<malc_obj_float> {};
template<>
struct builtin_type_map<double> : public builtin_map<malc_obj_double> {};
//------------------------------------------------------------------------------
struct MALC_EXPORT obj_log_data {
  //----------------------------------------------------------------------------
  static malc_obj_log_data get_as_str (char const* str, std::size_t len);
  //----------------------------------------------------------------------------
  template <int N>
  static inline malc_obj_log_data get (const char (&str)[N])
  {
    return obj_log_data::get_as_str (str, N - 1);
  }
  //----------------------------------------------------------------------------
  static inline malc_obj_log_data get (std::string const& s)
  {
    return obj_log_data::get_as_str (s.c_str(), s.size());
  }
  //----------------------------------------------------------------------------
  static inline malc_obj_log_data get_null_smartptr()
  {
    return obj_log_data::get (MALC_CPP_NULL_SMART_PTR_STR);
  }
  //----------------------------------------------------------------------------
  static malc_obj_log_data get(
    void const* addr, std::size_t count, uint8_t builtin_type
    );
  //----------------------------------------------------------------------------
  template <class T>
  static inline
  typename std::enable_if<
    detail::serialization::is_valid_builtin<T>::value,
    malc_obj_log_data
    >::type
  get (T const* first, std::size_t count)
  {
    return obj_log_data::get(
      (void*) &first[0], count, builtin_type_map<T>::value
      );
  }
  //----------------------------------------------------------------------------
  template <class T>
  static inline
  typename std::enable_if<
    detail::serialization::is_valid_builtin<T>::value,
    malc_obj_log_data
    >::type
  get (T& v)
  {
    return obj_log_data::get(
      (void*) &v, 1, builtin_type_map<T>::value
      );
  }
  //------------------------------------------------------------------------------
  template <class T>
  static inline malc_obj_log_data get (std::vector<T> const &v)
  {
    return obj_log_data::get (&v[0], v.size());
  }
  //------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
class log_entry_adder {
public:
  //----------------------------------------------------------------------------
  log_entry_adder (malc_obj_push_context const& pushcontext) : pc (pushcontext)
  {};
  //----------------------------------------------------------------------------
  int push (malc_obj_log_data const& ld) const noexcept
  {
    return malc_obj_push (&pc, &ld);
  }
  //----------------------------------------------------------------------------
  int push_as_str (char const* str, std::size_t len) const noexcept
  {
    return push (obj_log_data::get_as_str (str, len));
  }
  //----------------------------------------------------------------------------
  template <int N>
  int push (const char (&str)[N]) const noexcept
  {
    return push (obj_log_data::get_as_str (str, N - 1));
  }
  //----------------------------------------------------------------------------
  int push (std::string const& str) const noexcept
  {
    return push (obj_log_data::get (str));
  }
  //----------------------------------------------------------------------------
  int push_null_smartptr() const noexcept
  {
    return push (obj_log_data::get_null_smartptr ());
  }
  //----------------------------------------------------------------------------
  template <class T>
  int push (T const* first, std::size_t count) const noexcept
  {
    return push (obj_log_data::get (first, count));
  }
  //----------------------------------------------------------------------------
  template <class T>
  typename std::enable_if<
    detail::serialization::is_valid_builtin<T>::value,
    int
    >::type
  push (T v) const noexcept
  {
    return push (obj_log_data::get (v));
  }
  //----------------------------------------------------------------------------
  template <class T>
  int push (std::vector<T> const &v) const noexcept
  {
    return push (obj_log_data::get (v));
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  malc_obj_push_context const& pc;
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
template <class inherited>
class custom_log_type {
public:
  //----------------------------------------------------------------------------
  using type = inherited;
  //----------------------------------------------------------------------------
  static malc_obj_table const* get_table()
  {
    return &objtable;
  }
  //----------------------------------------------------------------------------
private:
  static const malc_obj_table objtable;
  //----------------------------------------------------------------------------
  static void destroy (malc_obj_ref* obj, void const*)
  {
    static_cast<inherited*> (obj->obj)->~inherited();
  }
  //----------------------------------------------------------------------------
  static int get_data(
    malc_obj_ref*                obj,
    void const*                  table,
    malc_obj_push_context const* pc,
    bl_alloc_tbl const*          alloc
    )
  {
    BL_TRY {
      return static_cast<inherited*> (obj->obj)->dump_value(
        log_entry_adder (*pc), obj->extra.context, obj->extra.flag, *alloc
        );
    }
    BL_CATCH (...) {
      return 1;
    }
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
template <class T>
const malc_obj_table custom_log_type<T>::objtable{
  get_data,
  destroy,
  sizeof (T)
};
//------------------------------------------------------------------------------
} // namespace malcpp

#endif // _MALCPP_CUSTOM_TYPE_
