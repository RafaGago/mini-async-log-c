#include <stdio.h>
#include <string.h>
#include <thread>
#include <chrono>

#include <malc/malc.hpp>
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
class demo_destination {
public:
  /*--------------------------------------------------------------------------*/
   /* required */
  demo_destination (const bl_alloc_tbl& alloc)
  {
    /* We don't use the passed allocator */
  };
  /*--------------------------------------------------------------------------*/
  /* required */
  bool flush()
  {
    printf ("%s: flushing\n", m_name.c_str());
    fflush (stdout);
    return true;
  }
  /*--------------------------------------------------------------------------*/
  /* required */
  bool idle_task()
  {
    printf ("%s: doing short lived maintenance tasks\n", m_name.c_str());
    return true;
  }
  /*--------------------------------------------------------------------------*/
  /* required */
  bool write (bl_u64 nsec, bl_uword severity, malc_log_strings const& strs)
  {
    printf(
      "%s: enrty: %s %s %s\n",
      m_name.c_str(),
      strs.timestamp,
      strs.sev,
      strs.text
      );
    return true;
  }
  /*--------------------------------------------------------------------------*/
   /* own function */
  void set_name (char const* name)
  {
    m_name = name;
  }
  /*--------------------------------------------------------------------------*/
private:
  std::string m_name;
};
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

malc_owner log;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return log.handle();
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  /* destination register */
  auto dst = log.add_destination<demo_destination>();
  if (!dst.is_valid()) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    return 1;
  }
  dst.try_get()->set_name ("demo-destination");

  /* logger startup */
  malc_cfg cfg;
  bl_err err = log.get_cfg (cfg);
  if (err.bl) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    return err.bl;
  }
  cfg.consumer.start_own_thread = true;
  err = log.init (cfg);
  if (err.bl) {
    fprintf (stderr, "unable to start logger\n");
    return err.bl;
  }
  log_error (err, "Hello malc");
  log_warning (err, "testing {}, {}, {.1}", 1, 2, 3.f);
  return 0;
}
/*----------------------------------------------------------------------------*/
