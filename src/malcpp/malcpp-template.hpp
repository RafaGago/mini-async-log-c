#ifdef __cplusplus

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

template <bool e, bool c, bool d>
void malcpp<e,c,d>::destroy_impl() noexcept
{
  if (!this->handle()) {
    return;
  }
  (void) malc_destroy (this->handle());
  bl_dealloc (get_alloc_tbl(), this->handle());
  this->set_handle (nullptr);
}
/*----------------------------------------------------------------------------*/
template <bool e, bool c, bool d>
bl_err malcpp<e,c,d>::construct_impl (bl_alloc_tbl alloc) noexcept
{
  if (this->handle()) {
    return bl_mkerr (bl_invalid);
  }
  malc* ptr = (malc*) bl_alloc(
    &alloc,
    malc_get_size() + std::alignment_of<bl_alloc_tbl>::value + sizeof alloc
    );
  if (!ptr) {
    return bl_mkerr (bl_alloc);
  }
  this->set_handle (ptr);
  *get_alloc_tbl() = alloc;
  bl_err err = malc_create (this->handle(), get_alloc_tbl());
  if (err.own) {
    bl_dealloc (get_alloc_tbl(), ptr);
    this->set_handle (nullptr);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
/* packing an alloc table together with malc's memory */
template <bool e, bool c, bool d>
bl_alloc_tbl* malcpp<e,c,d>::get_alloc_tbl() noexcept
{
  bl_uword tbl_addr = ((bl_uword) this->handle()) + malc_get_size();
  tbl_addr         += std::alignment_of<bl_alloc_tbl>::value;
  tbl_addr         &= ~(std::alignment_of<bl_alloc_tbl>::value - 1);
  return (bl_alloc_tbl*) tbl_addr;
}
/*----------------------------------------------------------------------------*/

} // namespace malcpp {

#endif /* __cplusplus */