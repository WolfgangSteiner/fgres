/*{{{}}}*/
/*{{{  copyright*/
/************************************************************************
 *	fgres global constants and prototypes				*
 *									*
 *	Copyright (c) 1994, W. Stumvoll, Germany			*
 *	#include "README"						*
 ************************************************************************/
/*}}}  */

/*{{{  includes*/
#define _POSIX_SOURCE
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "patchlevel.h"
/*}}}  */

/*{{{  constants*/
#ifndef CHAR_BIT
#  define CHAR_BIT 8
#endif
#define WRT_GROUP_ID 1
#define POS_GROUP_ID 2
#define DFLT_GROUP_ID 3
#define CMD_SEPARATOR '\n'
#ifndef BLOCKS
/*{{{  global constants*/
#define BLOCKS		(64)
/*}}}  */
#endif
#ifndef BLOCKSIZE
/*{{{  global constants*/
#define BLOCKSIZE	(16384)
/*}}}  */
#endif
/*{{{  global constants*/
#define START_COUNT	(0)
#define STD_FILE	"-"
#define WRT_GROUP	"write"
#define POS_GROUP	"position"
#define DFLT_GROUP	"default"
#define QUOTE_CHAR	'\\'
#define QUOTE_FOR_NL	'n'
#define QUOTE_FOR_TAB	't'
#define DFLT_SEPARATOR	' ','\t'
#define CMD_ADRESS	'#'
#define CMD_REGION_LEN	'-'
#define CMD_START	'+'
#define CMD_STOP	'-'
#define OPT_BIN		"b"
#define OPT_CMD		"e"
#define OPT_CMDF	"f"
#define OPT_GROW	"g"
#define OPT_HELP	"h"
#define OPT_CASE	"i"
#define OPT_BLOCKS	"n"
#define OPT_PAD		"p"
#define OPT_IN		"r"
#define OPT_SHRINK	"s"
#define OPT_VERB	"v"
#define OPT_OUT		"w"
#define OPT_SEP		"F"
/*}}}  */
/*}}}  */

/*{{{  debug/assert switching*/
/*#define DEBUG   					 /* enables asserts */
#ifdef DEBUG
#  define DEBUG_OUT				/* enable debugging outputs */
#endif
/*}}}  */
/*{{{  public/private*/
#define public
#define private static
/*}}}  */

/*{{{  system externals*/
extern int sys_nerr;
extern char *sys_errlist[];
extern int memcmp(const void*,const void*,size_t);
/*}}}  */
/*{{{  exit*/
/*{{{  get exit values*/
#ifndef NOSYSEXIT
#  include <sysexits.h>
#endif
#ifndef EX_OK
#  define EX_OK		0	/* successful termination */
#endif
#ifndef EX_USAGE
#  define EX_USAGE	64	/* command line usage error */
#endif
#ifndef EX_DATAERR
#  define EX_DATAERR	65	/* data format error */
#endif
#ifndef EX_NOINPUT
#  define EX_NOINPUT	66	/* cannot open input */
#endif
#ifndef EX_SOFTWARE
#  define EX_SOFTWARE	70	/* internal software error */
#endif
#ifndef EX_OSERR
#  define EX_OSERR	71	/* system error (e.g., can't fork) */
#endif
#ifndef EX_CANTCREAT
#  define EX_CANTCREAT	73	/* can't create (user) output file */
#endif
#ifndef EX_IOERR
#  define EX_IOERR	74	/* input/output error */
#endif
/*}}}  */
#define exit_m_prog(msg,ret) (fprintf(stderr,msg),exit(ret))
#define exit_ma_prog(msg,arg1,ret) (fprintf(stderr,msg,arg1),exit(ret))
#define exit_maa_prog(msg,arg1,arg2,ret) (fprintf(stderr,msg,arg1,arg2),exit(ret))
#define exit_maaa_prog(msg,arg1,arg2,arg3,ret) (fprintf(stderr,msg,arg1,arg2,arg3),exit(ret))
/*}}}  */
/*{{{  file_io*/
extern char *dat_dummy;
extern off_t dat_read;
extern off_t dat_write;
extern unsigned char *dat_start;
extern unsigned char *dat_end;
extern off_t skip_data;
extern int no_blocks;
extern int close_data(const int);
extern void dat_init(int,int);
extern off_t dat_get(void);
extern void dat_skip(off_t);
extern void dat_put(const unsigned char*,off_t);
extern void dat_flush(void);
extern void dat_start_write(void);
extern void dat_stop_write(void);
/*}}}  */
/*{{{  main*/
extern int verbose;
extern void *do_malloc(size_t);
extern void *do_realloc(void*,size_t);
/*}}}  */
/*{{{  messages*/
/*{{{  error messages*/
extern const char error_close_f[];
extern const char error_empty_search[];
extern const char error_group[];
extern const char error_grow_command[];
extern const char error_invalid_group[];
extern const char error_memory[];
extern const char *error_msg(const char *);
extern const char error_noblocks[];
extern const char error_no_group[];
extern const char error_no_prog[];
extern const char error_open_file[];
extern const char error_open_group[];
extern const char error_pos[];
extern const char error_redirect[];
extern const char error_seek[];
extern const char error_separator[];
extern const char error_shrink_command[];
extern const char error_stop_write[];
extern const char error_truncate[];
extern const char error_unlink[];
extern const char error_write[];
/*}}}  */
/*{{{  usage*/
extern const char msg_short_usage[];
extern const char msg_long_usage[];
/*}}}  */
/*{{{  some msg's*/
extern const char msg_unknown[];
extern const char e_command[];
extern const char msg_no_file[];
extern const char msg_out[];
extern const char msg_statistic[];
extern const char simple_command[];
/*}}}  */
/*{{{  warnings*/
extern const char warn_long_replace[];
extern const char warn_short_replace[];
extern const char warn_temp_file[];
/*}}}  */
/*}}}  */
/*{{{  program*/
/*{{{  types*/
/*{{{  cmd_list_t*/
typedef struct
 { int file;
   const char *cmd;
 } cmd_list_t;
/*}}}  */
/*{{{  prg_t*/
typedef struct
 { enum { no_prg=0,open_grp_prg,ok_prg } mode;
   off_t min_length;
   off_t max_length;
   off_t skip_width[1<<CHAR_BIT];
   struct cmd_list
    { int group;
      enum run_t { inactive=0,active } mode;
      enum guard_t { nop,pattern,adress } type;
      off_t len_search;
      union { const unsigned char *s; off_t add; } guard;
      off_t len_replace;
      const unsigned char *r;
      off_t null_pad;
      struct act_list
       { int group_and_action;
         struct act_list *next;
       } *act_list;
      enum pos_mode_t
       { no_pos=0,
         pre_pos= -POS_GROUP_ID,
         post_pos= POS_GROUP_ID
       } pos_mode;
      struct cmd_list *next;
    } *list[(1<<CHAR_BIT)+1],*pos_pattern;
 } prg_t;
/*}}}  */
/*{{{  rp_mode_t*/
typedef enum
 { same=0,
   lower=1,
   higher=2,
   same_or_lower=same|lower,
   same_or_higher=same|higher,
   all=same|lower|higher,
   DFLT_MODE=all
 } rp_mode_t;
/*}}}  */
/*}}}  */
extern const unsigned char dflt_sep_list[];
extern const unsigned char *separator_list;
extern prg_t program;
extern rp_mode_t rp_mode;
extern int pad;
extern int (*f_memcmp)(const void*,const void*,size_t);
#ifdef DEBUG
   extern void debug_prog(char*);
#endif
extern int test_separator(unsigned int);
extern int case_memcmp(const void*,const void*,size_t);
extern void cmd_init(void);
extern enum guard_t cmd_exec
 ( const unsigned char*,off_t,off_t*const,const struct cmd_list**const );
extern int cmd_group_handle(struct act_list*);
extern void simple_cmd_cmp(const unsigned char*const,const unsigned char*);
extern void cmd_compiler(cmd_list_t*,int);
/*}}}  */

/*{{{  assert*/
#ifdef DEBUG
#  define assert(check,com) \
   ((check)\
     ?0\
     :(fprintf(stderr,"assertation failed (%s) at %d\n",com,__LINE__),\
       abort(),\
       0\
      )\
   )
#else
#  define assert(check,str)
#endif
/*}}}  */
/*{{{  debugging functions*/
#ifdef DEBUG_OUT
#  define debug_m(msg) fprintf(stderr,msg)
#  define debug_ma(msg,arg) fprintf(stderr,msg,arg)
#  define debug_maa(msg,arg1,arg2) fprintf(stderr,msg,arg1,arg2)
   extern void debug_prog(char*);
#else
#  define debug_m(msg)
#  define debug_ma(msg,arg)
#  define debug_maa(msg,arg1,arg2)
#  define debug_prog(s)
#endif
/*}}}  */
