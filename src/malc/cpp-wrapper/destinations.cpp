#ifdef __cplusplus

#include <cstring>
#include <malc/libexport.h>
#include <malc/malc.h>
#include <malc/destinations/array.h>
#include <malc/destinations/file.h>
#include <malc/destinations/stdouterr.h>
#include <malc/malc.hpp>
#include <malc/destinations.hpp>

namespace malcpp {

/* Implementation note: there are unorthodox castings because I wanted to remove
the malc C types from the global namespace when a user includes "malc.hpp".

The casts are done between the same datatypes. Creating explicit conversions
would require converting POD types to classes, which is ugly and tedious too,
and I would be telling the compiler the same but in a different way. I find that
this solution keeps the castings localized and under control.
*/
/*----------------------------------------------------------------------------*/
/* definitions on destinations.hpp */
/*----------------------------------------------------------------------------*/
bl_err file_dst::set_cfg (file_dst_cfg const& cfg) noexcept
{
  return malc_file_set_cfg ((malc_file_dst*) this, (::malc_file_cfg*) &cfg);
}
/*----------------------------------------------------------------------------*/
bl_err file_dst::get_cfg (file_dst_cfg& cfg) const noexcept
{
  return malc_file_get_cfg ((malc_file_dst*) this, (::malc_file_cfg*) &cfg);
}
/*----------------------------------------------------------------------------*/
malc_dst file_dst::get_dst_tbl()
{
  // these types are actually the same and come from the same header file
  // included twice, one of the inclusion is made inside the "malcpp" namespace.
  // This is done to C++ify the C library as much as possible, hence all the
  // non-conventional things are doing here under the hood.
  // memcpy is used to avoid type punning warnings.
  ::malcpp::malc_dst dst;
  static_assert (sizeof malc_file_dst_tbl == sizeof dst, "");
  memcpy (&dst, &malc_file_dst_tbl, sizeof dst);
  return dst;
}
/*----------------------------------------------------------------------------*/
void array_dst::set_array(
  char* mem, bl_uword mem_entries, bl_uword entry_chars
  ) noexcept
{
  malc_array_dst_set_array(
    (malc_array_dst*) this, mem, mem_entries, entry_chars
    );
}
/*----------------------------------------------------------------------------*/
bl_uword array_dst::size() const noexcept
{
  return malc_array_dst_size ((malc_array_dst const*) this);
}
/*----------------------------------------------------------------------------*/
bl_uword array_dst::capacity() const noexcept
{
  return malc_array_dst_capacity ((malc_array_dst const*) this);
}
/*----------------------------------------------------------------------------*/
char const* array_dst::get_entry (bl_uword idx) const noexcept
{
  return malc_array_dst_get_entry ((malc_array_dst const*) this, idx);
}
/*----------------------------------------------------------------------------*/
malc_dst array_dst::get_dst_tbl()
{
  // these types are actually the same and come from the same header file
  // included twice, one of the inclusion is made inside the "malcpp" namespace.
  // This is done to C++ify the C library as much as possible, hence all the
  // non-conventional things are doing here under the hood.
  // memcpy is used to avoid type punning warnings.
  ::malcpp::malc_dst dst;
  static_assert (sizeof malc_array_dst_tbl == sizeof dst, "");
  memcpy (&dst, &malc_array_dst_tbl, sizeof dst);
  return dst;
}
/*----------------------------------------------------------------------------*/
bl_err stdouterr_dst::set_stderr_severity (bl_uword sev) noexcept
{
  return malc_stdouterr_set_stderr_severity ((malc_stdouterr_dst*) this, sev);
}
/*----------------------------------------------------------------------------*/
malc_dst stdouterr_dst::get_dst_tbl()
{
    // these types are actually the same and come from the same header file
  // included twice, one of the inclusion is made inside the "malcpp" namespace.
  // This is done to C++ify the C library as much as possible, hence all the
  // non-conventional things are doing here under the hood.
  // memcpy is used to avoid type punning warnings.
  ::malcpp::malc_dst dst;
  static_assert (sizeof malc_stdouterr_dst_tbl == sizeof dst, "");
  memcpy (&dst, &malc_stdouterr_dst_tbl, sizeof dst);
  return dst;
}
/*----------------------------------------------------------------------------*/
/* definitions on malc.hpp */
/*----------------------------------------------------------------------------*/
namespace detail {
/*----------------------------------------------------------------------------*/
dst_access_untyped::dst_access_untyped() noexcept
{
  m_owner = nullptr;
  m_id    = (bl_u32) -1ll;
}
/*----------------------------------------------------------------------------*/
bl_err dst_access_untyped::get_cfg (dst_cfg& c) const noexcept
{
  if (!is_valid()) {
    return bl_mkerr (bl_invalid);
  }
  return malc_get_destination_cfg (owner(), (::malc_dst_cfg*) &c, id());
}
/*----------------------------------------------------------------------------*/
bl_err dst_access_untyped::set_cfg (dst_cfg const& c) const noexcept
{
  if (!is_valid()) {
    return bl_mkerr (bl_invalid);
  }
  return malc_set_destination_cfg (owner(), (::malc_dst_cfg*) &c, id());
}
/*----------------------------------------------------------------------------*/
bl_err dst_access_untyped::untyped_try_get (void*& instance) const noexcept
{
  if (!is_valid()) {
    return bl_mkerr (bl_invalid);
  }
  bl_err e = malc_get_destination_instance (owner(), &instance, id());
  return e;
}
/*----------------------------------------------------------------------------*/
bool dst_access_untyped::is_valid() const noexcept
{
  return m_owner != nullptr;
}
/*----------------------------------------------------------------------------*/
malc* dst_access_untyped::owner() const noexcept
{
  return m_owner;
}
/*----------------------------------------------------------------------------*/
bl_u32 dst_access_untyped::id() const noexcept
{
  return m_id;
}
/*----------------------------------------------------------------------------*/
} // namespace detail

} // namespace malcpp {

#endif /* __cplusplus */