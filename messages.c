/*{{{}}}*/
/*{{{  copyright*/
/************************************************************************
 *	fgres messages							*
 *									*
 *	Copyright (c) 1994, W. Stumvoll, Germany			*
 *	#include "README"						*
 ************************************************************************/
/*}}}  */

#include "global.h"

/*{{{  format constants for messages*/
#define PROG_POS_F	"(%s %d)"
#define FILE_TYPE_F	"%s"
#define FILE_NAME_F	"%s"
#define SYS_ERROR_F	"(%s)"
#define BYTE_COUNT_F	"%ld"
/*}}}  */

/*{{{  error_msg*/
public const char *error_msg(const char *def)
 {
   return((errno<sys_nerr) ? sys_errlist[errno] : def);
 }
/*}}}  */
/*{{{  global messages:error messages*/
public const char error_close_f[]="cannot close "FILE_NAME_F" "SYS_ERROR_F"\n";
public const char error_empty_search[]="empty search "PROG_POS_F"\n";
public const char error_group[]="group active "PROG_POS_F"\n";
public const char error_grow_command[]="growing command "PROG_POS_F"\n";
public const char error_invalid_group[]="invalid group id "PROG_POS_F"\n";
public const char error_memory[]="no more memory, malloc "BYTE_COUNT_F" bytes failed\n";
public const char error_noblocks[]="-"OPT_BLOCKS": invalid value\n";
public const char error_no_group[]="no group active "PROG_POS_F"\n";
public const char error_no_prog[]="no program given\n";
public const char error_open_file[]="cannot open "FILE_TYPE_F" file "FILE_NAME_F" "SYS_ERROR_F"\n";
public const char error_open_group[]="group open at end of program\n";
public const char error_pos[]="-"OPT_BIN" disables position output "PROG_POS_F"\n";
public const char error_redirect[]="option "OPT_IN"/"OPT_OUT" and file argument conflict\n";
public const char error_seek[]="seek failed "SYS_ERROR_F"\n";
public const char error_separator[]="-"OPT_SEP": invalid string, using default\n";
public const char error_shrink_command[]="shrinking command "PROG_POS_F"\n";
public const char error_stop_write[]="-"OPT_BIN" disables stop writing "PROG_POS_F"\n";
public const char error_truncate[]="truncate file failes, padded with "BYTE_COUNT_F" 0\n";
public const char error_unlink[]="remove temporary file failed "SYS_ERROR_F"\n";
public const char error_write[]="write failed "SYS_ERROR_F"\n";
/*}}}  */
/*{{{  usage*/
public const char msg_short_usage[]=
  "fgres - version "VERSION", usage is:\n"
  "   fgres <options> [-"OPT_IN" in] [-"OPT_OUT" out] search [replace]\n"
  "   fgres <options> search [replace [file .. ]]\n"
  "   fgres <options> <program> file .. \n"
  "   fgres <options> <program> [-"OPT_IN" in] [-"OPT_OUT" out]\n"
  "where:  <options>:	[-"OPT_BIN OPT_CASE OPT_GROW OPT_HELP OPT_PAD OPT_SHRINK OPT_VERB"] [-"OPT_BLOCKS" number] [-"OPT_SEP" limiter]\n"
  "        <program>:	(-"OPT_CMD" cmd | -"OPT_CMDF" file) ..\n";
public const char msg_long_usage[]=
  "%swith:\n"
  "   -"OPT_BIN		  "\t"	"binary, filesize may not be changed\n"
  "   -"OPT_CASE	  "\t"	"case insensitive pattern match\n"
  "   -"OPT_PAD		  "\t"	"pad short and cut long replaces\n"
  "   -"OPT_GROW	  "\t"	"allow file grow\n"
  "   -"OPT_SHRINK	  "\t"	"allow file shrink\n"
  "   -"OPT_CMD		  "\t"	"command\n"
  "   -"OPT_CMDF	  "\t"	"command file\n"
  "   -"OPT_IN		  "\t"	"input file\n"
  "   -"OPT_OUT		  "\t"	"output file\n"
  "   -"OPT_BLOCKS	  "\t"	"set number of stored I/O blocks for file replacement k\n"
  "   -"OPT_HELP	  "\t"	"this message\n"
  "   -"OPT_VERB	  "\t"	"verbose\n";
/*}}}  */
/*{{{  some msg's*/
public const char msg_unknown[]="unkown";
public const char e_command[]="-e argument";
public const char msg_no_file[]="no file";
public const char msg_out[]="written to file "FILE_NAME_F"\n";
public const char msg_statistic[]="filtered "FILE_NAME_F" ("BYTE_COUNT_F" -> "BYTE_COUNT_F")\n";
public const char simple_command[]="command line argument";
/*}}}  */
/*{{{  global messages:warnings*/
public const char warn_long_replace[]="cut long replace "PROG_POS_F"\n";
public const char warn_short_replace[]="padding short replace "PROG_POS_F"\n";
public const char warn_temp_file[]="(using temporary file "FILE_NAME_F")\n";
/*}}}  */
