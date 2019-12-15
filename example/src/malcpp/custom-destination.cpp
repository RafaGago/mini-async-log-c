//------------------------------------------------------------------------------
// This file shows how to add a destination/sink.
// It does use the error-code based interface.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <thread>

#include <malcpp/malcpp_lean.hpp>

//------------------------------------------------------------------------------
class demo_destination {
public:
  //----------------------------------------------------------------------------
   /* required */
  demo_destination (const bl_alloc_tbl& alloc)
  {
    /* Ignoring the passed allocator */
  };
  //----------------------------------------------------------------------------
  /* required */
  bool flush()
  {
    printf ("%s: flushing\n", m_name.c_str());
    fflush (stdout);
    return true;
  }
  //----------------------------------------------------------------------------
  /* required */
  bool idle_task()
  {
    printf ("%s: doing short lived maintenance tasks\n", m_name.c_str());
    return true;
  }
  //----------------------------------------------------------------------------
  /* required */
  bool write (bl_u64 nsec, bl_uword severity, malcpp::log_strings const& strs)
  {
    printf(
      "%s: entry: %s %s %s\n",
      m_name.c_str(),
      strs.timestamp,
      strs.sev,
      strs.text
      );
    return true;
  }
  //----------------------------------------------------------------------------
   /* own function */
  void set_name (char const* name)
  {
    m_name = name;
  }
  //----------------------------------------------------------------------------
private:
  std::string m_name;
};
//------------------------------------------------------------------------------
// false, false, true; non-throwing with explicit construction but scope-based
// destruction
//------------------------------------------------------------------------------
static inline malcpp::malcpp<false, false, true>& get_malc_instance()
{
  static malcpp::malcpp<false, false, true> log;
  return log;
}
//------------------------------------------------------------------------------
int main (int argc, char const* argv[])
{
  auto& log = get_malc_instance();
  bl_err err = log.construct();
  if (err.own) {
    fprintf (stderr, "unable to construct malc\n");
    return err.own;
  }
  /* destination register */
  auto dst = log.add_destination<demo_destination>();
  if (!dst.is_valid()) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    return 1;
  }
  dst.try_get()->set_name ("demo-destination");

  /* logger startup */
  malcpp::cfg cfg;
  err = log.get_cfg (cfg);
  if (err.own) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    return err.own;
  }
  cfg.consumer.start_own_thread = true;
  err = log.init (cfg);
  if (err.own) {
    fprintf (stderr, "unable to start logger\n");
    return err.own;
  }
  err = log_error ("Hello malc");
  err = log_warning ("testing {}, {}, {.1}", 1, 2, 3.f);
  return 0;
}
//------------------------------------------------------------------------------
