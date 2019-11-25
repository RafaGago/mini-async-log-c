#include <thread>
#include <malc/malc.hpp>

malcpp::malcpp<> log;
/*----------------------------------------------------------------------------*/
/* koenig-lookup on this function is used on the log macros to get the relevant
malc instance. You can override the function name with the macro
"MALC_GET_LOGGER_INSTANCE_FUNCNAME".

If you want to pass the instance explicitly to the macros you can use the "_i"
suffixed macros.
*/
static inline decltype(log)& get_malc_logger_instance()
{
  return log;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  /* destination register: adding logging to stdout/stderr */
  auto stdouterr = log.add_destination<malcpp::stdouterr_dst>();

  /* generic configuration (provided by malc: available to all destinations) */
  malcpp::dst_cfg dcfg = stdouterr.get_cfg();
  dcfg.show_timestamp  = false;
  dcfg.show_severity   = false;
  dcfg.severity        = malcpp::sev_warning;
  stdouterr.set_cfg (dcfg);

  /* destination-specific configuration*/
  (void) stdouterr.get().set_stderr_severity (malcpp::sev_debug);

  /* destination register: logging to files */
  auto file = log.add_destination<malcpp::file_dst>();

  /* generic configuration (provided by malc: available to all destinations) */
  dcfg = file.get_cfg();
  dcfg.show_timestamp = true;
  dcfg.show_severity  = true;
  dcfg.severity       = malcpp::sev_debug;
  file.set_cfg (dcfg);

  /* destination-specific configuration*/
  malcpp::file_dst_cfg fcfg;
  (void) file.get().get_cfg (fcfg);
  fcfg.prefix = "malc-cpp-wrapper-example";
  (void) file.get().set_cfg (fcfg);

  /* logger startup */
  malcpp::cfg cfg = log.get_cfg();
  cfg.consumer.start_own_thread = true;
  log.init (cfg);

  /* threads can start logging */
  std::thread thr;
  thr = std::thread([](){
    bl_err err;
    /* integers + printf modifiers */
    err = log_error ("10: {}", 10);
    err = log_error ("10 using \"03\" specifier: {03}", 10);
    err = log_error ("10 using \"+03\" specifier: {+03}", 10);
    err = log_error ("10 using \"03x\" specifier: {03x}", 10);
    err = log_error ("10 using \"03x\" specifier: {03X}", 10);
    err = log_error(
      "10 using \"0Nx\" custom specifier. N = number of nibbles: {0Nx}",
      (bl_u32) 10
      );
    err = log_error(
      "10 using \"0Wx\" custom specifier. W = max number of int digits: {0W}",
      (bl_u32) 10
      );

    /* floating point + printf modifiers */
    err = log_error ("1.: {}", 1.);
    err = log_error ("1. using \".3\" specifier: {.3}", 1.);
    err = log_error ("1. using \"012.3\" specifier: {012.3}", 1.);
    err = log_error ("1. using \"+012.3\" specifier: {+012.3}", 1.);
    err = log_error ("1. using \"-012.3\" specifier: {-012.3}", 1.);
    err = log_error ("1. using \"e\" specifier: {e}", 1.);
    err = log_error ("1. using \"g\" specifier: {g}", 1.);
    err = log_error ("1. using \"a\" specifier: {a}", 1.);
    err = log_error ("1. using \"E\" specifier: {E}", 1.);
    err = log_error ("1. using \"G\" specifier: {G}", 1.);
    err = log_error ("1. using \"A\" specifier: {A}", 1.);

    /* strings */
    char const str[] = "a demo string";
    err = log_error(
      "a string copied by value: {}", malcpp::strcp (str, sizeof str - 1)
      );

    /* logging a string by reference.

    the logger will call the deallocation function when the reference is no
    longer needed. Be aware that:

   *If the entry is filtered out because of the severity, or if an error happens
    the destructor might be called in-place from the current thread.

   *If the log call is stripped at compile time the log call is ommited, so
    there is no memory deallocation, as the ownership would have never been
    passed to the logger. That's why all this code is wrapped is on the
    MALC_STRIP_LOG_ERROR, as we are logging with "error" severity. There is a
    MALC_STRIP_LOG_[SEVERITY] macro defined for each of stripped severities.

    The error code returned on stripped severities will be "bl_nothing_to_do".
    */
  #ifndef MALC_STRIP_LOG_ERROR
    char* dstr = (char*) malloc (sizeof str - 1);
    assert (dstr);
    memcpy (dstr, str, sizeof str - 1);

    void* context = (void*) 0x1ee7;
    auto destructorfn =
      [] (void* context, malcpp::malc_ref const* refs, bl_uword refs_count) {
        assert (refs_count == 1); // the log entry just has one reference value
        assert (context == (void*) 0x1ee7);
        free (refs[0].ref);
      };
  #endif
    err = log_error(
      "a string by ref: {}",
      malcpp::strref (dstr, sizeof str - 1),
      malcpp::refdtor (destructorfn, context)
      );

    /* literal (or string guaranteed to outlive the logger)*/
    err = log_error(
      "a literal: {}", malcpp::lit (1 ? "literal one" : "literal two")
      );

    /* bytes: a memory area to log as an hex stream. */
    bl_u8 const mem[] = { 10, 11, 12, 13 };
    err = log_error(
      "[10,11,12,13] by value: {}", malcpp::memcp (mem, sizeof mem)
      );

  #ifndef MALC_STRIP_LOG_ERROR
    /* bytes by reference: they work exactly as the strings by reference*/
    bl_u8* dmem = (bl_u8*) malloc (sizeof mem);
    assert (dmem);
    memcpy (dmem, mem, sizeof mem);
  #endif
    err = log_error(
      "[10,11,12,13] by ref: {}",
       malcpp::memref (dmem, sizeof mem),
      malcpp::refdtor (destructorfn, context)
      );

    /* severities */
    err = log_debug ("this is not seen on stdout (sev > debug)");

    /* log entry with a conditional.*/
    err = log_error_if (true, "conditional debug line");

    /* if the conditional is false the parameter list is not evaluated. Beware
    of parameter lists with side effects*/
    int side_effect = 0;
    err = log_error_if(
      false, "ommited conditional debug line: {}", ++side_effect
      );
    assert (side_effect == 0);

    /* brace escape */
    err = log_error ("escaping only requires to skip the open bracket: {{}");

    /* passing the instance explicitly instead of through
     "get_malc_log_instance()" */
    err = log_error_i (log, "passing the log instance explicitly.");
  });
  thr.join();
  return 0;
}
/*----------------------------------------------------------------------------*/