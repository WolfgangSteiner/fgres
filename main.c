/*{{{}}}*/
/*{{{  copyright*/
/************************************************************************
 *	fgres main routine, including argument parsing and signal	*
 *	handlers							*
 *									*
 *	Copyright (c) 1994, W. Stumvoll, Germany			*
 *	#include "README"						*
 ************************************************************************/
/*}}}  */

#include "global.h"

/*{{{  variables*/
public int verbose=0;
private const char std_file[]=STD_FILE;
/*}}}  */

/*{{{  mallocing*/
/*{{{  do_malloc*/
public void *do_malloc(size_t l)
 { void *x;

   x=malloc(l);
   if (!x)
      exit_ma_prog(error_memory,(long int)l,EX_OSERR);

   return(x);
 }
/*}}}  */
/*{{{  do_realloc*/
public void *do_realloc(void *x,size_t l)
 {
   x=realloc(x,l);
   if (!x)
      exit_ma_prog(error_memory,(long int)l,EX_OSERR);

   return(x);
 }
/*}}}  */
/*}}}  */
/*{{{  signals*/
int got_sig_stop= -1;

/*{{{  init_signals*/
private void init_signals(void)
 {
#   ifdef SIGTERM
      if (signal(SIGTERM,got_sterm)==SIG_IGN)
          signal(SIGTERM,SIG_IGN);
#   endif
#   ifdef SIGHUP
      if (signal(SIGHUP,got_scrash)==SIG_IGN)
          signal(SIGHUP,SIG_IGN);
#   endif
#   ifdef SIGQUIT
      if (signal(SIGQUIT,got_sterm)==SIG_IGN)
          signal(SIGQUIT,SIG_IGN);
#   endif
#   ifdef SIGINT
      if (signal(SIGINT,got_sterm)==SIG_IGN)
          signal(SIGINT,SIG_IGN);
#   endif
 }
/*}}}  */
/*{{{  got_scrash*/
private void got_scrash(int sig)
{
  fprintf(stderr,"killed on signal %d\n",sig);
# ifdef DELAYED_CLOSE
     if (dat_dummy && *dat_dummy && unlink(dat_dummy))
        exit_ma_prog(error_unlink,error_msg(msg_unknown),EX_OSERR);
# endif
  exit(EX_OSERR);
}
/*}}}  */
/*{{{  got_sterm*/
private void got_sterm(int sig)
 {
   init_signals();
   if (got_sig_stop)
      got_scrash(sig);
   got_sig_stop=sig;
 }
/*}}}  */
/*}}}  */
/*{{{  filter functions*/
/*{{{  filter*/
void filter(int fin,int fout,int init)
 { off_t buff_len;

   debug_m("start filtering\n");
   dat_init(fin,fout);
   if (init)
      cmd_init();
   while ((buff_len=dat_get()))
    { off_t find;
      const struct cmd_list *pat;

      for (;;)
       { switch (cmd_exec(dat_start,buff_len,&find,&pat))
          { case nop:
               break;
            default:
               assert(0,"CRASH");
            case adress:
            case pattern:
             { int wrt_off;

               debug_ma("match replace %p\n",pat);
               dat_put(dat_start,find);
               dat_skip(find+pat->len_search);
               wrt_off=cmd_group_handle(pat->act_list);
               if (pat->pos_mode==pre_pos)
                /*{{{  handle pre position*/
                { char buff[sizeof(off_t)*3];

                  sprintf(buff,"%lx",(long int)(skip_data-pat->len_search+START_COUNT));
                  dat_put((unsigned char*)buff,(off_t)strlen(buff));
                }
                /*}}}  */
               dat_put(pat->r,pat->len_replace);
               dat_null(pat->null_pad);
               if (pat->pos_mode==post_pos)
                /*{{{  handle post position*/
                { char buff[sizeof(off_t)*3];

                  sprintf(buff,"%lx",(long int)(skip_data-pat->len_search+START_COUNT));
                  dat_put((unsigned char*)buff,(off_t)strlen(buff));
                }
                /*}}}  */
               if (wrt_off)
                  dat_stop_write();
               buff_len=dat_get();
               continue;
             }
          }
         break;
       }
      debug_ma("no match in %ld bytes\n",(long int)buff_len);
      if (buff_len>=program.max_length)
         buff_len-=program.max_length-1;
      dat_put(dat_start,buff_len);
      dat_skip(buff_len);
    }
   dat_flush();   
 }
/*}}}  */
/*{{{  filter_file*/
int filter_file(const char * const fname,int old_ret)
 { int f; 
   struct stat buf;

   if ((f=open(fname,O_RDWR))<0)
    /*{{{  complain*/
    { fprintf(stderr,error_open_file,"filter",fname,error_msg(msg_unknown));
      old_ret=EX_NOINPUT;
    }
    /*}}}  */
   else if (fstat(f,&buf) || S_ISDIR(buf.st_mode))
    /*{{{  complain on directory*/
    { fprintf(stderr,error_open_file,"filter",fname,"directory");
      old_ret=EX_NOINPUT;
    }
    /*}}}  */
   else
    /*{{{  filter*/
    { filter(f,f,1);
      if (close_data(f))
       { fprintf(stderr,error_close_f,fname,error_msg(msg_unknown));
         old_ret=EX_CANTCREAT;
       }
      if (verbose)
         fprintf(stderr,msg_statistic,fname,(long int)dat_read,(long int)dat_write);
    }
    /*}}}  */

   return(old_ret);
 }
/*}}}  */
/*{{{  filter_pipe*/
int filter_pipe(const char **in_list,int in_count,const char *out_name)
 { int fin,fout;
   int first;
   int ret;

   ret=EX_OK;
   assert(out_name,"output missing");
   if (!strcmp(out_name,std_file))
    /*{{{  use stdout*/
    { fout=fileno(stdout);
      out_name="stdout";
    }
    /*}}}  */
   else if ((fout=open(out_name,O_WRONLY))<0)
    /*{{{  complain*/
    { fprintf(stderr,error_open_file,"filter-output",out_name,error_msg(msg_unknown));
      ret=EX_CANTCREAT;
    }
    /*}}}  */
   if (fout>=0)
    { first=1;
      assert(in_count && in_list[0],"input missing");
      do
       { assert(in_count && in_list[0],"input missing");
         if (!strcmp(*in_list,std_file))
          /*{{{  use stdin*/
          { fin=fileno(stdin);
            *in_list="stdin";
          }
          /*}}}  */
         else if ((fin=open(*in_list,O_RDONLY))<0)
          /*{{{  complain*/
          { fprintf(stderr,error_open_file,"filter-input",*in_list,error_msg(msg_unknown));
            ret=EX_NOINPUT;
          }
          /*}}}  */
         if (fin>=0)
          { filter(fin,fout,first);
            first=0;
            if (close_data(fin))
             { fprintf(stderr,error_close_f,*in_list,error_msg(msg_unknown));
               ret=EX_IOERR;
             }
            if (verbose)
               fprintf(stderr,msg_statistic,*in_list,(long int)dat_read,(long int)dat_write);
          }
         in_list++;
       }
      while (--in_count);
      if (close_data(fout))
       { fprintf(stderr,error_close_f,out_name,error_msg(msg_unknown));
         ret=EX_CANTCREAT;
       }
      if (verbose)
         fprintf(stderr,msg_out,out_name);
    }

   return(ret);
 }
/*}}}  */
/*}}}  */
/*{{{  main*/
public int main(argc,argv) int argc;char **argv;
 { const char *output_file=0;
   int ef_used;
   cmd_list_t *ef_list;
   int r_used;
   const char **r_list;
   extern int optind;
   int ret;

   /*{{{  argument decoding*/
   { extern int getopt();
     extern char *optarg;

     /*{{{  init buffer for -e/f -r*/
     ef_list=do_malloc(argc*sizeof(cmd_list_t));
     ef_used=0;
     r_list=do_malloc(argc*sizeof(const char**));
     r_used=0;
     /*}}}  */
     /*{{{  handle arguments*/
     for (;;)
      { int c;

        /*{{{  c=getopt(options)*/
        c=getopt
           ( argc,argv,
             OPT_BIN
             OPT_BLOCKS":"
             OPT_CASE
             OPT_CMD":"
             OPT_CMDF":"
             OPT_SEP":"
             OPT_GROW
             OPT_HELP
             OPT_IN":"
             OPT_OUT":"
             OPT_PAD
             OPT_SHRINK
             OPT_VERB
           );
        /*}}}  */
        /*{{{  OPT_IN*/
        if (c==OPT_IN[0])
         { r_list[r_used++]=optarg;
           continue;
         }
        /*}}}  */
        /*{{{  OPT_OUT*/
        if (c==OPT_OUT[0])
         { output_file=optarg;
           continue;
         }
        /*}}}  */
        /*{{{  OPT_CMD[F]*/
        if (c==OPT_CMD[0] || c==OPT_CMDF[0])
         { ef_list[ef_used].file=(c==OPT_CMDF[0]);
           ef_list[ef_used].cmd=optarg;
           ef_used++;
           continue;
         }
        /*}}}  */
        /*{{{  OPT_SEP*/
        if (c==OPT_SEP[0])
         { if
            (    !(separator_list=(unsigned char*)optarg)
              || test_separator(CMD_SEPARATOR)
              || test_separator(QUOTE_CHAR)
            )
            {
              fprintf(stderr,error_separator);
              separator_list=dflt_sep_list;
            }
           continue;
         }
        /*}}}  */
        /*{{{  OPT_BIN/GROW/CASE*/
        if (c==OPT_BIN[0])
         { rp_mode=same;
           continue;
         }
        if (c==OPT_CASE[0])
         { f_memcmp=case_memcmp;
           continue;
         }
        if (c==OPT_GROW[0])
         { rp_mode|=higher;
           continue;
         }
        if (c==OPT_SHRINK[0])
         { rp_mode|=lower;
           continue;
         }
        if (c==OPT_PAD[0])
         { pad=1;
           continue;
         }
        /*}}}  */
        /*{{{  OPT_BLOCKS*/
        if (c==OPT_BLOCKS[0])
         { no_blocks=atoi(optarg);
           if (no_blocks<=1)
            { fprintf(stderr,error_noblocks);
              goto error_opt;
            }
           continue;
         }
        /*}}}  */
        /*{{{  OPT_HELP/VERB/error*/
        if (c==OPT_VERB[0])
         { verbose=1;
           continue;
         }
        if (c==OPT_HELP[0])
           exit_ma_prog(msg_long_usage,msg_short_usage,EX_USAGE);
        if (c=='?')
         { error_opt:

           exit_m_prog(msg_short_usage,EX_USAGE);
         }
        /*}}}  */
        break;
      }
     /*}}}  */
   }
   /*}}}  */
   init_signals();
   /*{{{  compile program*/
   if (!ef_used)
    /*{{{  compile command line replace*/
    {
      /*{{{  no serach given -> error*/
      if (argc-optind==0)
       { fprintf(stderr,error_no_prog);
         goto error_opt;
       }
      /*}}}  */
      debug_maa("need to compile args '%s' '%s'\n",argv[optind],(argc-optind>=2)?argv[optind+1]:"");
      if (argc-optind>=2)
       /*{{{  search and replace given*/
       { simple_cmd_cmp
          ( (unsigned char*)argv[optind],
            (unsigned char*)argv[optind+1]
          );
         optind+=2;
       }
       /*}}}  */
      else
       /*{{{  only search given*/
       { simple_cmd_cmp
          ( (unsigned char*)argv[optind],
            (unsigned char*)0
          );
         optind+=1;
       }
       /*}}}  */
    }
    /*}}}  */
   else
      cmd_compiler(ef_list,ef_used);
   free(ef_list);
   /*}}}  */
   /*{{{  filter files/pipe*/
   /*{{{  command line files and pipe arguments -> error*/
   if (argc-optind>0 && (r_used || output_file))
    { fprintf(stderr,error_redirect);
      goto error_opt;
    }
   /*}}}  */
   ret=EX_OK;
   if (optind==argc)
    /*{{{  use pipe arguments*/
    { debug_m("need to filter pipe\n");
      /*{{{  no -r, so fake one: -r -*/
      if (!r_used)
       { r_used=1;
         r_list[0]=std_file;
       }
      /*}}}  */
      /*{{{  no -w, so fake one: -w -*/
      if (!output_file)
         output_file=std_file;
      /*}}}  */
      ret=filter_pipe(r_list,r_used,output_file);
    }
    /*}}}  */
   else
    /*{{{  use command line files*/
      do
       { debug_ma("need to filter %s\n",argv[optind]);
         ret=filter_file(argv[optind],ret);
         if (got_sig_stop>=0)
            got_sterm(got_sig_stop);
       }
      while (++optind<argc);
    /*}}}  */
   /*}}}  */

   return(ret);
 }
/*}}}  */
