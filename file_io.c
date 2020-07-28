/*{{{}}}*/
/*{{{  copyright*/
/************************************************************************
 *	File I/O routines fgres, including insito handling for files	*
 *									*
 *	Copyright (c) 1994, W. Stumvoll, Germany			*
 *	#include "README"						*
 ************************************************************************/
/*}}}  */

#include "global.h"

/*{{{  variables*/
/*{{{  counters for input/output*/
public off_t dat_read;
public off_t dat_write;
/*}}}  */
/*{{{  buffersize*/
private off_t r_b_size;
/*}}}  */
/*{{{  read buffer*/
private unsigned char *read_buff=0;
private off_t read_pos;
public unsigned char *dat_start;
public unsigned char *dat_end;
public off_t skip_data;
/*}}}  */
/*{{{  write buffer*/
private off_t really_written;
private int stored;
private int overflow;
private int do_write;
private unsigned int written_blocks;
private unsigned char write_buffer[BLOCKSIZE];
private off_t write_pos;
private struct wrt_b_t
 { unsigned char t[BLOCKSIZE];
   struct wrt_b_t *next;
 } *wrt_over_buff;
public int no_blocks=BLOCKS;
/*}}}  */
/*{{{  data files*/
private int dat_fin= -1;
private int dat_fout= -1;
public char *dat_dummy=0;
private int dat_save_out= -1;
/*}}}  */
/*}}}  */

/*{{{  file access*/
/*{{{  write_data*/
private int write_data(int fildes,const unsigned char *b,size_t n)
 { int ret;

   for (ret=0;n;)
    { ret=write(fildes,b,n);
      if (ret==-1)
#ifdef EINTR
         if (errno==EINTR)
            ret=0;
         else
#endif
            exit_ma_prog(error_write,error_msg(msg_no_file),EX_IOERR);
      n-=ret;
      b+=ret;
    }
   return(ret);
 }
/*}}}  */
/*{{{  read_data*/
private int read_data(int fildes,unsigned char *b,size_t n)
 { size_t read_in;

   for (read_in=0;read_in<n;)
    { size_t ret;

      ret=read(fildes,b,n-read_in);
      if
       /*{{{  ret==0 && repeat cannot be succesfull*/
       (    ret==0
         && (    1
#       ifdef EAGAIN
              && errno!=EAGAIN
#       endif
#       ifdef EINTR
              && errno!=EINTR
#       endif
#       ifdef EWOULDBLOCK
              && errno!=EWOULDBLOCK
#       endif
            )
       )
       /*}}}  */
         break;
      read_in+=ret;
      b+=ret;
    }
   return(read_in);
 }
/*}}}  */
/*{{{  close_data*/
public int close_data(const int fildes)
 { int ret;

   for (;;)
    { ret=close(fildes);
      if (!ret || errno!=EINTR)
         break;
    }
   return(ret);
 }
/*}}}  */
/*{{{  block_init*/
private void block_init(void)
 {
   written_blocks=0;
   really_written=0;
   overflow=0;
   stored=0;
 }
/*}}}  */
/*{{{  block_flush*/
private void block_flush(int mode)
 {
   assert(dat_dummy,"flush and not inplace");
   if (mode)
    /*{{{  flush all data*/
    {
      /*{{{  seek to write position*/
      if (lseek(dat_fout,(off_t)0,0)==-1 || lseek(dat_save_out,really_written,0)==-1)
         goto seek_error;
      /*}}}  */
      /*{{{  write local memory data*/
      while (wrt_over_buff)
       { struct wrt_b_t *x;

         stored--;
         write_data(dat_save_out,wrt_over_buff->t,(size_t)BLOCKSIZE);
         x=wrt_over_buff->next;
         free(wrt_over_buff);
         wrt_over_buff=x;
       }
      /*}}}  */
      /*{{{  write data from file*/
      while (written_blocks--)
       { read_data(dat_fout,read_buff,(size_t)BLOCKSIZE);
         write_data(dat_save_out,read_buff,(size_t)BLOCKSIZE);
       }
      /*}}}  */
    }
    /*}}}  */
   else if (!overflow && wrt_over_buff && really_written+BLOCKSIZE<dat_read)
    /*{{{  flush start of overflow list*/
    {
      /*{{{  maybe seek to write position*/
      if (read_pos!=really_written)
         if (lseek(dat_save_out,really_written,0)==-1)
            goto seek_error;
      /*}}}  */
      /*{{{  write*/
      do
       { struct wrt_b_t *x;

         stored--;
         write_data(dat_save_out,wrt_over_buff->t,(size_t)BLOCKSIZE);
         x=wrt_over_buff->next;
         free(wrt_over_buff);
         wrt_over_buff=x;
         really_written+=BLOCKSIZE;
       }
      while (wrt_over_buff && really_written+BLOCKSIZE<dat_read);
      /*}}}  */
      read_pos=really_written;
    }
    /*}}}  */
   return;

 seek_error:
   exit_ma_prog(error_seek,error_msg(msg_unknown),EX_IOERR);
 }
/*}}}  */
/*{{{  block_write*/
private void block_write(const unsigned char * const block)
 {
   if (dat_dummy)
    /*{{{  maybe store file data local*/
    { struct wrt_b_t *new;

      block_flush(0);
      if (!overflow && really_written+BLOCKSIZE<dat_read)
       /*{{{  write direct on file*/
       { if (read_pos!=really_written)
            if (lseek(dat_save_out,really_written,0)==-1)
               exit_ma_prog(error_seek,error_msg(msg_unknown),EX_IOERR);
         write_data(dat_save_out,block,(size_t)BLOCKSIZE);
         really_written+=BLOCKSIZE;
         read_pos=really_written;
       }
       /*}}}  */
      else
       /*{{{  store data*/
       { if (stored<no_blocks && (new=malloc(sizeof(struct wrt_b_t))))
          { stored++;
            /*{{{  add new block to list*/
            if (wrt_over_buff)
             { struct wrt_b_t *x;

               for (x=wrt_over_buff;x->next;x=x->next);
               x->next=new;
             }
            else
               wrt_over_buff=new;
            new->next=0;
            /*}}}  */
            memcpy(new->t,block,(size_t)BLOCKSIZE);
          }
         else
          { overflow=1;
            if (written_blocks==0)
               fprintf(stderr,warn_temp_file,dat_dummy);
            write_data(dat_fout,block,(size_t)BLOCKSIZE);
            written_blocks++;
          }

       }
       /*}}}  */
    }
    /*}}}  */
   else
      write_data(dat_fout,block,(size_t)BLOCKSIZE);
 }
/*}}}  */
/*}}}  */
/*{{{  data read/write functions*/
/*{{{  dat_init*/
public void dat_init(int fin,int fout)
 {
   if (fin==fout)
    /*{{{  use temporary file*/
    { debug_m("data init for in-place\n");
      dat_save_out=fout;
      errno=sys_nerr;
      if
       (    !(dat_dummy=tmpnam((char*)0))
         || (fout=open(dat_dummy,O_RDWR|O_CREAT|O_TRUNC,0600))<0
       )
         exit_maa_prog(error_open_file,"temporary",error_msg(msg_no_file),EX_CANTCREAT);
#     ifndef DELAYED_CLOSE
         if (unlink(dat_dummy))
            exit_ma_prog(error_unlink,error_msg(msg_unknown),EX_OSERR);
#     endif
      dat_init(fin,fout);
    }
    /*}}}  */
   else
    /*{{{  init buffers for read/write*/
    { debug_m("data init for different files\n");
      block_init();
      dat_fin=fin;
      dat_fout=fout;
      /*{{{  maybe malloc store for program*/
      if (!read_buff)
       { for
          ( r_b_size=BLOCKSIZE
          ; r_b_size<2*program.max_length
          ; r_b_size+=BLOCKSIZE
          )
          ;
         read_buff=do_malloc((size_t)(2*r_b_size));
       }
      /*}}}  */
      read_pos=0;
      dat_read=dat_write=skip_data=0;
      dat_end=dat_start=read_buff;
      write_pos=0;
    }
    /*}}}  */
 }
/*}}}  */
/*{{{  dat_get*/
public off_t dat_get(void)
 { off_t used;

   if ((used=dat_end-dat_start)<program.max_length)
    /*{{{  buffer to small, read new data*/
    { off_t diff;

      if (used && dat_start!=read_buff)
       /*{{{  shift data to start of buffer*/
       { off_t i;

         debug_ma("shift %ld bytes back\n",(long int)used);
         for (i=used,dat_end=read_buff;i;*dat_end++= *dat_start++,i--);
       }
       /*}}}  */
      dat_start=read_buff;
      dat_end=read_buff+used;
      /*{{{  maybe seek forward to reading position*/
      if (read_pos!=dat_read)
         if (lseek(dat_fin,dat_read,0)==-1)
            exit_ma_prog(error_seek,error_msg(msg_unknown),EX_IOERR);
      /*}}}  */
      diff=(off_t)read_data(dat_fin,dat_end,(size_t)r_b_size);
      used+=diff;
      dat_read+=diff;
      read_pos=dat_read;
      dat_end=dat_start+used;
      debug_ma("read got %ld bytes => ",(long int)diff);
    }
    /*}}}  */
   debug_ma("can handle %ld bytes\n",(long int)used);
   return(used);
 }
/*}}}  */
/*{{{  dat_skip*/
public void dat_skip(off_t skip_width)
 {
   debug_ma("skip %ld bytes\n",(long int)skip_width);
   skip_data+=skip_width;
   if ((dat_start+=skip_width)>dat_end)
    { debug_maa("start %p end %p\n",dat_start,dat_end);
      exit_m_prog("crashed skip width\n",EX_SOFTWARE);
    }
 }
/*}}}  */
/*{{{  dat_put*/
public void dat_put(const unsigned char * s,off_t l)
 { off_t store_len;

   debug_maa("put %ld bytes (%s)\n",(long int)l,do_write?"done":"ignored");
   if (l && do_write)
    { dat_write+=l;
      store_len=BLOCKSIZE-write_pos;
      if (store_len>=l)
       { memcpy(write_buffer+write_pos,s,(size_t)l);
         write_pos+=l;
       }
      else
       { memcpy(write_buffer+write_pos,s,(size_t)store_len);
         l-=store_len;
         s+=store_len;
         block_write(write_buffer);
         while (l>BLOCKSIZE)
          { block_write(s);
            s+=BLOCKSIZE;
            l-=BLOCKSIZE;
          }
         memcpy(write_buffer,s,(size_t)l);
         write_pos=l;

       }
    }
 }
/*}}}  */
/*{{{  dat_null*/
public void dat_null(off_t l)
 { 
   debug_maa("put %ld 0 bytes (%s)\n",(long int)l,do_write?"done":"ignored");
   while (l)
    { const unsigned char c[]="";

      dat_put(c,(off_t)1);
      l--;
    }
 }
/*}}}  */
/*{{{  dat_flush*/
#ifdef NO_FTRUNCATE
#  define ftruncate(x,y) 1
#endif

public void dat_flush(void)
 {
   if (dat_dummy)
    /*{{{  flush stored write data, append last block and maybe truncate*/
    { block_flush(1);
      if (write_pos)
         write_data(dat_save_out,write_buffer,(size_t)write_pos);
      /*{{{  maybe truncate or pad with 0*/
      if (dat_write<dat_read && ftruncate(dat_save_out,dat_write))
       { fprintf(stderr,error_truncate,(long int)(dat_read-dat_write));
         memset(write_buffer,0,(size_t)BLOCKSIZE);
         while (dat_write<dat_read)
          { off_t use;

            use=dat_read-dat_write;
            if (use>BLOCKSIZE)
               use=BLOCKSIZE;
            write_data(dat_save_out,write_buffer,(size_t)use);
            dat_write+=use;
          }
       }
      /*}}}  */
      /*{{{  close and unlink temporary file*/
      if (close(dat_fout))
         exit_maa_prog(error_close_f,dat_dummy,error_msg(msg_unknown),EX_CANTCREAT);
#      ifdef DELAYED_CLOSE
         if (unlink(dat_dummy))
            exit_ma_prog(error_unlink,error_msg(msg_unknown),EX_OSERR);
#      endif
      /*}}}  */
      dat_fout=dat_save_out;
      dat_save_out=0;
    }
    /*}}}  */
   else
      write_data(dat_fout,write_buffer,(size_t)write_pos);
 }
/*}}}  */
/*{{{  dat_start_write*/
public void dat_start_write(void)
 {
   do_write=1;
 }
/*}}}  */
/*{{{  dat_stop_write*/
public void dat_stop_write(void)
 {
   do_write=0;
 }
/*}}}  */
/*}}}  */
