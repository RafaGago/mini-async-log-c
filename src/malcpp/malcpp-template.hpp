#ifdef __cplusplus

#include <type_traits>

#include <bl/base/default_allocator.h>
#include <bl/base/static_integer_math.h>

#include <malc/libexport.h>
#include <malc/malc.h>
#include <malcpp/malcpp.hpp>

/* Implementation note: there are unorthodox castings because I wanted to remove
the malc C types from the global namespace when a user includes "malc.hpp".

The casts are done between the same datatypes. Creating explicit conversions
would require converting POD types to classes, which is ugly and tedious too,
and I would be telling the compiler the same but in a different way. I find that
this solution keeps the castings localized and under control.
*/

namespace malcpp {
/*----------------------------------------------------------------------------*/
struct malc_alloc_data {
  bl_alloc_tbl const* alloc;
  bl_alloc_tbl        alloc_table;
};
/*----------------------------------------------------------------------------*/
template <class out_type>
static out_type* get_aligned_footer_data (void* header_ptr, bl_word header_size)
{
  static constexpr bl_uword align = std::alignment_of<out_type>::value;

  static_assert (sizeof (void*) == sizeof (bl_uword), "");
  static_assert (bl_is_pow2 (align), "");

  bl_uword footer_addr = ((bl_uword) header_ptr) + header_size;
  footer_addr += align;
  footer_addr &= ~(align - 1);
  return static_cast<out_type*> ((void*) footer_addr);
}
/*----------------------------------------------------------------------------*/
template <bool e, bool c, bool d>
bl_err malcpp<e,c,d>::destroy_impl() noexcept
{
  if (!this->handle()) {
    return bl_mkok();
  }
  bl_err err = malc_destroy (this->handle());
  if (err.own) {
    return err;
  }
  bl_dealloc (get_alloc_tbl(), this->handle());
  this->set_handle (nullptr);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
template <bool e, bool c, bool d>
bl_err malcpp<e,c,d>::construct_impl (bl_alloc_tbl const* alloc) noexcept
{
  if (this->handle()) {
    return bl_mkerr (bl_invalid);
  }
  bl_alloc_tbl defalloc = bl_get_default_alloc();

  void* ptr = bl_alloc(
    (alloc) ? alloc : &defalloc,
    malc_get_size() +
      std::alignment_of<malc_alloc_data>::value +
      sizeof (malc_alloc_data)
    );
  if (!ptr) {
    return bl_mkerr (bl_alloc);
  }
  this->set_handle (static_cast<malc*> (ptr));

  malc_alloc_data* mad = get_aligned_footer_data<malc_alloc_data>(
    ptr, malc_get_size()
    );

  mad->alloc_table = defalloc;
  mad->alloc       = (alloc) ? alloc : &mad->alloc_table;

  bl_err err = malc_create (this->handle(), mad->alloc);
  if (err.own) {
    bl_dealloc (mad->alloc, ptr);
    this->set_handle (nullptr);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
/* packing an malc_alloc_data together with malc's memory */
template <bool e, bool c, bool d>
bl_alloc_tbl const* malcpp<e,c,d>::get_alloc_tbl() noexcept
{
  malc_alloc_data* mad = get_aligned_footer_data<malc_alloc_data>(
    static_cast<void*> (this->handle()), malc_get_size()
    );
  return mad->alloc;
}
/*----------------------------------------------------------------------------*/

} // namespace malcpp {

#endif /* __cplusplus */