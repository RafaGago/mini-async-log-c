#ifndef __MALC_ENTRY_PARSER_H__
#define __MALC_ENTRY_PARSER_H__

#include <bl/base/platform.h>
#include <bl/base/dynamic_string.h>
#include <malc/malc.h>
#include <malc/log_entry.h>

/*----------------------------------------------------------------------------*/
/*  on-header strings only to be able to unit test*/

#define MALC_EP_UNCLOSED_FMT \
  "{<MALC: missing \"}\">"

#define MALC_EP_ESC_BRACES_IN_FMT \
  "<MALC: escaped braces inside format sequence. ignored>"

#define MALC_EP_MISPLACED_OPEN_BRACES \
  "<MALC: multiple open braces inside format sequence. ignored>"

#define MALC_EP_EXCESS_ARGS \
  "<MALC: excess args/missing placeholders>"

#define MALC_EP_MISSING_ARG \
  "<MALC: missing arg>"

#define MALC_EP_ALLOC "entry parser couldn't allocate memory"
/*----------------------------------------------------------------------------*/
#define MALC_EP_DEBUG "[debug]"
#define MALC_EP_TRACE "[trace]"
#define MALC_EP_NOTE  "[note_]"
#define MALC_EP_WARN  "[warn_]"
#define MALC_EP_ERROR "[error]"
#define MALC_EP_CRIT  "[crit_]"
/*----------------------------------------------------------------------------*/
#define TSTAMP_INTEGER  11
#define TSTAMP_DECIMAL 9
/*----------------------------------------------------------------------------*/
typedef struct entry_parser {
  char tstamp[TSTAMP_INTEGER + TSTAMP_DECIMAL + 1 + 1]; /* dot + teminating 0 */
  dstr str;
  dstr fmt;
  bool sanitize_log_entries;
}
entry_parser;
/*----------------------------------------------------------------------------*/
BL_EXPORT bl_err entry_parser_init (entry_parser* ep, alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
BL_EXPORT void entry_parser_destroy (entry_parser* ep);
/*----------------------------------------------------------------------------*/
BL_EXPORT bl_err entry_parser_get_log_strings(
  entry_parser* ep, log_entry const* e, malc_log_strings* strs
  );
/*----------------------------------------------------------------------------*/
#endif
