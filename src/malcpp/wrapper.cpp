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

/*----------------------------------------------------------------------------*/
wrapper::wrapper (malc* ptr) noexcept
{
  m_ptr = ptr;
}
/*----------------------------------------------------------------------------*/
wrapper::~wrapper()
{
  m_ptr = nullptr;
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::get_cfg (cfg& c) const noexcept
{
  assert (m_ptr);
  return malc_get_cfg (handle(), (::malc_cfg*) &c);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::init (cfg const& c) noexcept
{
  assert (m_ptr);
  return malc_init (handle(), (::malc_cfg*) &c);
}
  /*----------------------------------------------------------------------------*/
bl_err wrapper::init() noexcept
{
  assert (m_ptr);
  return malc_init (handle(), nullptr);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::flush() noexcept
{
  assert (m_ptr);
  return malc_flush (handle());
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::terminate (bool dontblock) noexcept
{
  assert (m_ptr);
  return malc_terminate (handle(), dontblock);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::producer_thread_local_init (size_t bytes) noexcept
{
  assert (m_ptr);
  return malc_producer_thread_local_init (handle(), bytes);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::run_consume_task (unsigned timeout_us) noexcept
{
  assert (m_ptr);
  return malc_run_consume_task (handle(), timeout_us);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::add_destination (size_t& dest_id, malc_dst const& dst) noexcept
{
  assert (m_ptr);
  return malc_add_destination (handle(), &dest_id, (::malc_dst*) &dst);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::get_destination_instance(
  void** instance, size_t dest_id
  ) const noexcept
{
  assert (m_ptr);
  return malc_get_destination_instance (handle(), instance, dest_id);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::get_destination_cfg (dst_cfg& c, size_t dest_id) const noexcept
{
  assert (m_ptr);
  return malc_get_destination_cfg (handle(), (::malc_dst_cfg*) &c, dest_id);
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::set_destination_cfg (dst_cfg const& c, size_t dest_id) noexcept
{
  assert (m_ptr);
  return malc_set_destination_cfg (handle(), (::malc_dst_cfg*) &c, dest_id);
}
/*----------------------------------------------------------------------------*/
void wrapper::set_handle (malc* h)
{
  m_ptr = h;
}
/*----------------------------------------------------------------------------*/
bl_err wrapper::add_destination_impl(size_t& id, malc_dst const& tbl)
{
  return malc_add_destination (handle(), &id, (::malc_dst*) &tbl);
}
/*----------------------------------------------------------------------------*/


} // namespace malcpp

#endif /* #ifdef  __cplusplus */
