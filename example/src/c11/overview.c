#include <stdio.h>

#include <bl/base/default_allocator.h>
#include <bl/base/thread.h>

#include <malc/malc.h>
#include <malc/destinations/file.h>
#include <malc/destinations/stdouterr.h>

malc* ilog = nullptr;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return ilog;
}
/*----------------------------------------------------------------------------*/
void dtor_using_free (void* context, malc_ref const* refs, bl_uword refs_count)
{
  for (bl_uword i = 0; i < refs_count; ++i) {
    free ((void*) refs[i].ref);
  }
}
/*----------------------------------------------------------------------------*/
int log_thread (void* ctx)
{
  bl_err err;
  /* integers */
  err = log_error ("10: {}", 10);
  err = log_error ("10 using \"03\" specifier: {03}", 10);
  err = log_error ("10 using \"+03\" specifier: {+03}", 10);
  err = log_error ("10 using \"03x\" specifier: {03x}", 10);
  err = log_error ("10 using \"03x\" specifier: {03X}", 10);
  err = log_error(
    "10 using \"0Nx\" specifier. N = number of nibbles: {0Nx}",
    (bl_u32) 10
    );
  err = log_error(
    "10 using \"0Wx\" specifier. W = max number of int digits: {0W}",
    (bl_u32) 10
    );

  /* floating point */
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
  err = log_error ("a string by value: {}", logstrcpy (str, sizeof str - 1));

  /* logging a string by reference.

  the logger will call the deallocation function when the reference is no
  longer needed. Be aware that:

 -If the entry is filtered out because of the severity or if an error happens
  the destructor might be called in-place from the current thread.

 -If the log call is stripped at compile time the log call dissapears, so there
  is no memory deallocation, as the ownership would have never been passed to
  the logger. That's why all this code is wrapped is on the
  MALC_STRIP_LOG_ERROR, as we are logging with "error" severity. There is a
  MALC_STRIP_LOG_[SEVERITY] macro defined for each of stripped severities.

  The error code returned on stripped severities will be "bl_nothing_to_do".
  */
#ifndef MALC_STRIP_LOG_ERROR
  char* dstr = malloc (sizeof str);
  assert (dstr);
  memcpy (dstr, str, sizeof str);
#endif
  err = log_error(
    "a string by ref: {}",
    logstrref (dstr, sizeof str - 1),
    logrefdtor (dtor_using_free, nullptr)
    );

  err = log_error ("a literal: {}", loglit (1 ? "literal one" : "literal two"));

  /* bytes */
  bl_u8 const mem[] = { 10, 11, 12, 13 };
  err = log_error ("[10,11,12,13] by value: {}", logmemcpy (mem, sizeof mem));

#ifndef MALC_STRIP_LOG_ERROR
  bl_u8* dmem = malloc (sizeof mem);
  assert (dmem);
  memcpy (dmem, mem, sizeof mem);
#endif
  err = log_error(
    "[10,11,12,13] by ref: {}",
    logmemref (dmem, sizeof mem),
    logrefdtor (dtor_using_free, nullptr)
    );

  /* severities */
  err = log_debug ("this is not seen on stdout (sev > debug)");

  /* log entry with a conditional.*/
  err = log_error_if (1, "conditional debug line");

  /* if the conditional is false the parameter list is not evaluated. Beware
  of parameter lists with side effects*/
  int side_effect = 0;
  err = log_error_if (0, "ommited conditional debug line: {}", ++side_effect );
  assert (side_effect == 0);

  /* brace escape */
  err = log_error ("brace escape only requires to skip the open brace: {{}");

/* passing the instance explicitly instead of through
   "get_malc_log_instance()" */
  err = log_error_i (ilog, "passing the log instance explicitly.");

  (void) malc_terminate (ilog, false); /* terminating the logger. Will force the
                                          event loop to exit */
  return err.own;
}
/*----------------------------------------------------------------------------*/
int add_configure_destinations (void)
{
  bl_u32    stdouterr_id;
  bl_u32    file_id;
  bl_err err;

  /* destination register */
  err = malc_add_destination (ilog, &stdouterr_id, &malc_stdouterr_dst_tbl);
  if (err.own) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    return err.own;
  }
  err = malc_add_destination (ilog, &file_id, &malc_file_dst_tbl);
  if (err.own) {
    fprintf (stderr, "Error creating the file destination\n");
    return err.own;
  }
  /* from here and below it's just sink/destination configuration when the
  defaults are not suitable. Here is just done for demo purposes. */

  /* destination generic cfg, setting log severities */
  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = true;
  dcfg.show_severity      = true;
  dcfg.severity_file_path = nullptr;

  dcfg.severity = malc_sev_warning;
  err = malc_set_destination_cfg (ilog, &dcfg, stdouterr_id);
  if (err.own) {
    fprintf (stderr, "Unable to set generic stdout/stderr destination config\n");
    return err.own;
  }
  dcfg.severity = malc_sev_debug;
  err = malc_set_destination_cfg (ilog, &dcfg, file_id);
  if (err.own) {
    fprintf (stderr, "Unable to set generic file destination config\n");
    return err.own;
  }
  /* destination specific cfg. */
  malc_stdouterr_dst* stdouterr;
  err = malc_get_destination_instance (ilog, (void**) &stdouterr, stdouterr_id);
  if (err.own) {
    fprintf (stderr, "bug when retrieving the stdout/stderr destination\n");
    return err.own;
  }
  malc_file_dst* file;
  err = malc_get_destination_instance (ilog, (void**) &file, file_id);
  if (err.own) {
    fprintf (stderr, "bug when retrieving the stdout/stderr destination\n");
    return err.own;
  }
  /* severities equal and above error will be output on stderr (just for demo
  purposes, as it's already the default) */
  err = malc_stdouterr_set_stderr_severity (stdouterr, malc_sev_error);
  if (err.own) {
    fprintf(
      stderr, "Unable to set specific stdout/stderr destination config\n"
      );
    return err.own;
  }
  malc_file_cfg fcfg;
  err = malc_file_get_cfg (file, &fcfg);
  if (err.own) {
    fprintf (stderr, "Unable to get specific file destination config\n");
    return err.own;
  }
  fcfg.prefix = "malc-overview";
  fcfg.suffix = ".log";
  err = malc_file_set_cfg (file, &fcfg);
  if (err.own) {
    fprintf (stderr, "Unable to set specific file destination config\n");
  }
  return err.own;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  bl_err              err;
  bl_alloc_tbl           alloc = bl_get_default_alloc(); /* Using malloc and free */
  /* logger allocation/initialization */
  ilog = bl_alloc (&alloc,  malc_get_size());
  if (!ilog) {
    fprintf (stderr, "Unable to allocate memory for the malc instance\n");
    return bl_alloc;
  }
  err = malc_create (ilog, &alloc);
  if (err.own) {
    fprintf (stderr, "Error creating the malc instance\n");
    goto dealloc;
  }
  err.own = add_configure_destinations();
  if (err.own) {
    goto destroy;
  }
  /* logger startup */
  malc_cfg cfg;
  err = malc_get_cfg (ilog, &cfg);
  if (err.own) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    goto destroy;
  }
  cfg.consumer.start_own_thread = false;
  err = malc_init (ilog, &cfg);
  if (err.own) {
    fprintf (stderr, "unable to start logger\n");
    goto destroy;
  }
  /* threads can start logging */
  bl_thread t;
  err = bl_thread_init (&t, log_thread, nullptr);
  if (err.own) {
    fprintf (stderr, "unable to start a log thread\n");
    goto destroy;
  }

  do {
    err = malc_run_consume_task (ilog, 10000);
  }
  while (!err.own || err.own == bl_nothing_to_do);
  err = bl_mkok();

destroy:
  (void) malc_destroy (ilog);
dealloc:
  bl_dealloc (&alloc, ilog);
  return err.own;
}
/*----------------------------------------------------------------------------*/
