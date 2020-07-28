/*{{{}}}*/
/*{{{  copyright*/
/************************************************************************
 *	Compiler for command language and program executer		*
 *									*
 *	Copyright (c) 1994, W. Stumvoll, Germany			*
 *	#include "README"						*
 ************************************************************************/
/*}}}  */

#include "global.h"

/*{{{  variables*/
public const unsigned char dflt_sep_list[]={ DFLT_SEPARATOR,'\0' };
public const unsigned char *separator_list=dflt_sep_list;
public prg_t program;
public rp_mode_t rp_mode=DFLT_MODE;
public int pad=0;
public int (*f_memcmp)(const void*,const void*,size_t)=memcmp;
/*}}}  */

#ifdef DEBUG_OUT
   /*{{{  show all program buffers!*/
   public void debug_prog(char *s)
    { int i;

      fprintf(stderr,"Program at:%s (min/max=%ld/%ld)\n",s,(long)program.min_length,(long)program.max_length);
      for (i=0;i<=(1<<CHAR_BIT);i++)
       { struct cmd_list *l;

         for (l=program.list[i];l;l=l->next)
          { struct act_list *a;

            fprintf(stderr," %p %c ",l,(l->mode==active)?'+':'-');
            if (l->type==adress)
               fprintf(stderr," add %ld ",l->guard.add);
            else
             { int k;

               fprintf(stderr," >");
               for (k=0;k<l->len_search;k++)
                  fprintf(stderr,"%c",l->guard.s[k]?l->guard.s[k]:'.');
               fprintf(stderr,"< ");
             }
            fprintf(stderr,"-> %ld/%ld >",(long)l->len_replace,(long)l->null_pad);
            { int k;

              for (k=0;k<l->len_replace;k++)
                 fprintf(stderr,"%c",l->r[k]?l->r[k]:'.');
            }
            if ((a=l->act_list))
               do
                  if (a->group_and_action>0)
                     fprintf(stderr,"(START %d)",a->group_and_action);
                  else
                     fprintf(stderr,"(STOP %d)",-a->group_and_action);
               while ((a=a->next));
            fprintf(stderr,"\n");
          }
       }
    }
   /*}}}  */
#endif
/*{{{  test_separator*/
public int test_separator(unsigned int c)
 {
   return(c && strchr((char*)separator_list,c));
 }
/*}}}  */
/*{{{  case_memcmp*/
public int case_memcmp(const void *s1,const void *s2,size_t l)
 { char c1,c2;
   const char *p1,*p2;

   for (p1=(char*)s1,p2=(char*)s2;l;l--);
    { c1= *p1++;
      c2= *p2++;
      if (c1!=c2)
       { c1=toupper(c1);
         if (c1!=c2)
            return(1);
       }
    }
   return(0);
 }
/*}}}  */
/*{{{  cmd_init*/
public void cmd_init(void)
 { int i;

   dat_start_write();
   program.list[1<<CHAR_BIT]=program.pos_pattern;
   for (i=1<<CHAR_BIT;i>=0;i--)
    { struct cmd_list *c;

      for (c=program.list[i];c;c=c->next)
         c->mode=(c->group==DFLT_GROUP_ID);
    }
 }
/*}}}  */
/*{{{  cmd_exec*/
public enum guard_t cmd_exec
 ( const unsigned char *s,
   off_t length,
   off_t * const pos,
   const struct cmd_list * * const pat
 )
 { const unsigned char *start;
   enum guard_t ret;
   off_t len;

   debug_prog("pre exec");
   ret=nop;
   len=length;
   /*{{{  search for pattern*/
   if (len>=program.min_length)
      for (start=s;;)
       { size_t diff;
         unsigned int c;

         assert(len>0,"check empty string?");
         diff=program.skip_width[c= *s];
         if (diff)
          /*{{{  skip over text*/
          { s+=diff;
            if ((len-=diff)<=0)
               break;
          }
          /*}}}  */
         else
          /*{{{  test matches*/
          { struct cmd_list *list;

            for (list=program.list[c];list;list=list->next)
               if (list->mode==active)
                { const unsigned char *word;
                  size_t lg;

                  lg=list->len_search-1;
                  word=s-lg;
                  if (word>=start && !f_memcmp(word,list->guard.s,(size_t)lg))
                   { *pos=word-start;
                     *pat=list;
                     ret=list->type;
                     goto ad_handle;
                   }
                }
            if (--len)
               s++;
            else
               break;
          }
          /*}}}  */
       }
   /*}}}  */
 ad_handle:
   /*{{{  check, if a adress pattern is front of the found string pattern*/
   { off_t ad;
     struct cmd_list *x;

     debug_maa("skip %ld len %ld=>",(long)skip_data,(long)length);
     /*{{{  get compare adress*/
     if (ret==nop)
        ad=skip_data+(length-program.min_length);
     else
        ad= *pos;
     /*}}}  */
     debug_maa("check adress %s %ld\n",((ret==nop)?"nomatch":"match"),(long)ad);
     /*{{{  compare against adress patterns*/
     for (x=program.list[1<<CHAR_BIT];x;x=x->next)
      { debug_ma(" -> %ld\n",(long)x->guard.add);
        if (x->mode==active && x->guard.add<ad)
         { ret=adress;
           *pos=x->guard.add-skip_data-1;
           *pat=x;
           break;
         }
      }
     /*}}}  */
     /*{{{  cut remove unneded adress patterns*/
     while
      (    program.list[1<<CHAR_BIT]
        && program.list[1<<CHAR_BIT]->guard.add<=ad
      )
        program.list[1<<CHAR_BIT]=program.list[1<<CHAR_BIT]->next;
     /*}}}  */
   }
   /*}}}  */
   return(ret);
 }
/*}}}  */
/*{{{  cmd_group_handle*/
public int cmd_group_handle(struct act_list *l)
 { int ret;

   debug_ma("group_handle %p\n",l);
   for (ret=0;l;l=l->next)
    { int id;
      enum run_t new;


      debug_ma("group_handle %d\n",l->group_and_action);
      /*{{{  get id and action*/
      id=l->group_and_action;
      if (id>0)
         new=active;
      else
       { new=inactive;
         id=0-id;
       }
      /*}}}  */
      if (id==WRT_GROUP_ID)
       /*{{{  start/stop writing*/
       { if (new==active)
            dat_start_write();
         else
            ret=1;
         debug_ma("write-switch %d\n",new==active);
       }
       /*}}}  */
      else
       /*{{{  [in]activate command group*/
       { int i;

         debug_maa("switch group %d %d\n",id,new==active);
         for (i=1<<CHAR_BIT;i>=0;i--)
          { struct cmd_list *c;

            for (c=program.list[i];c;c=c->next)
               if (c->group==id)
                  c->mode=new;
          }
       }
       /*}}}  */
    }
   return(ret);
 }
/*}}}  */
/*{{{  cmd_cmp*/
private const char *cmd_file_name=0;
private int cmd_lineno=0;
/*{{{  add_prg*/
private void add_prg(struct cmd_list * const pat)
 { off_t len,pad_count;

   len=pat->len_search;
   pad_count=0;
   /*{{{  handle -bgsp*/
   if (!(rp_mode&lower) && len>pat->len_replace)
    /*{{{  shrinking command, but not allowed*/
    { if (pad)
       { unsigned char *new_rep;

         pad_count=len-pat->len_replace;
         if (verbose)
            fprintf(stderr,warn_short_replace,cmd_file_name,cmd_lineno);
       }
      else
         exit_maa_prog(error_shrink_command,cmd_file_name,cmd_lineno,EX_DATAERR);
    }
    /*}}}  */
   if (!(rp_mode&higher) && len<pat->len_replace)
    /*{{{  growing command, but not allowed*/
    { if (pad)
       { pat->len_replace=len;
         if (verbose)
            fprintf(stderr,warn_long_replace,cmd_file_name,cmd_lineno);
       }
      else
         exit_maa_prog(error_grow_command,cmd_file_name,cmd_lineno,EX_DATAERR);
    }
    /*}}}  */
   pat->null_pad=pad_count;
   /*}}}  */
   if (pat->type==pattern)
    /*{{{  insert pattern match*/
    { unsigned int key;

      if (f_memcmp==case_memcmp)
       /*{{{  search pattern to uppercase*/
       { off_t l;

         for (l=0;l<len;l++)
           ((char*)(pat->guard.s))[l]=toupper(pat->guard.s[l]);
       }
       /*}}}  */
      key=pat->guard.s[len-1];
      /*{{{  add to list, sorted by len*/
      pat->next=program.list[key];
      program.list[key]=pat;
      if (f_memcmp==case_memcmp)
         program.list[tolower(key)]=program.list[key];
      /*}}}  */
      /*{{{  add to jump table*/
      { int i;

        /*{{{  shrink or set maximum skip width*/
        for (i=0;i<1<<CHAR_BIT;i++)
           if (program.skip_width[i]>len || program.mode==no_prg)
              program.skip_width[i]=len;
        /*}}}  */
        /*{{{  reduce for chars in current string*/
        for (i=0;i<len;i++)
         { unsigned int c;

           c=pat->guard.s[i];
           if (program.skip_width[c]>len-i-1)
              program.skip_width[c]=len-i-1;
           if (f_memcmp==case_memcmp)
            { unsigned int c1;

              c1=tolower(c);
              if (c1!=c)
                 if (program.skip_width[c1]>len-i-1)
                    program.skip_width[c1]=len-i-1;
            }
         }
        /*}}}  */
      }
      /*}}}  */
    }
    /*}}}  */
   else
    /*{{{  insert adress match*/
    { off_t pos;
      struct cmd_list **x;

      assert(pat->type==adress,"unknown op type");
      pos=pat->guard.add;
      for (x=program.list+(1<<CHAR_BIT);;x= &((*x)->next))
           if (!*x || (*x)->guard.add>pos)
            { pat->next= *x;
              *x=pat;
              break;
            }
      program.pos_pattern=program.list[1<<CHAR_BIT];
    }
    /*}}}  */
   /*{{{  set min/max len*/
   if (program.mode==no_prg || len+pad_count>program.max_length)
      program.max_length=len+pad_count;
   if (program.mode==no_prg || len+pad_count<program.min_length)
      program.min_length=len+pad_count;
   /*}}}  */
 }
/*}}}  */

/*{{{  simple_cmd_cmp*/
public void simple_cmd_cmp
 ( const unsigned char * const search,
   const unsigned char * replace
 )
 { struct cmd_list *p;

   cmd_file_name=simple_command;
   cmd_lineno=1;
   if (!replace)
      replace=(unsigned char*)"";
   assert(search && replace,"null command pointer given");
   p=do_malloc(sizeof(struct cmd_list));
   p->group=DFLT_GROUP_ID;
   p->type=pattern;
   p->len_search=strlen((char*)search);
   p->guard.s=search;
   p->len_replace=strlen((char*)replace);
   p->r=replace;
   p->pos_mode=no_pos;
   p->act_list=0;
   add_prg(p);
   program.mode=ok_prg;
   debug_prog("post-simple command!");
 }
/*}}}  */
/*{{{  cmd_compiler*/
/*{{{  white_skip*/
private const unsigned char *white_skip(const unsigned char *s)
 {
   while (test_separator(*s))
      s++;
   return(s);
 }
/*}}}  */
/*{{{  grp_skip*/
private const unsigned char *grp_skip(const unsigned char *s)
 {
   while (isalnum(*s) || *s=='_')
      s++;
   return(s);
 }
/*}}}  */
/*{{{  get_group*/
private int get_group(const unsigned char * const name)
 {
   static int next_id=WRT_GROUP_ID+DFLT_GROUP_ID+1;
   static const struct grp_list
    { const unsigned char *name;
      int id;
      struct grp_list *next;
    } pos_grp={ (unsigned char*)POS_GROUP,POS_GROUP_ID,0 },
      wrt_grp={ (unsigned char*)WRT_GROUP,WRT_GROUP_ID,(struct grp_list*)&pos_grp },
      dflt_grp={ (unsigned char*)DFLT_GROUP,DFLT_GROUP_ID,(struct grp_list*)&wrt_grp };
   static struct grp_list *groups=(struct grp_list*)&dflt_grp;
   int res;

   res=DFLT_GROUP_ID;
   if (name && name[0])
    { struct grp_list *x;

      for (x=groups;;x=x->next)
         if (!x)
          /*{{{  add new group, break*/
          { struct grp_list *new;

            new=do_malloc(sizeof(struct grp_list));
            res=new->id=next_id++;
            new->name=name;
            new->next=groups;
            groups=new;
            break;
          }
          /*}}}  */
         else if (!strcmp((char*)name,(char*)x->name))
          /*{{{  found, break*/
          { res=x->id;
            break;
          }
          /*}}}  */

    }

   return(res);
 }
/*}}}  */
/*{{{  test_comment*/
private int test_comment(const unsigned char * const s)
 {
   return
    (    s[0]==CMD_SEPARATOR
      || (s[0]==QUOTE_CHAR && s[1]==CMD_ADRESS && s[2]==CMD_ADRESS)
    );
 }
/*}}}  */
/*{{{  cmd_reading*/
private cmd_list_t *cmd_data=0;
private int cmd_open=0;
private unsigned char *cmd_buffer=0;
private size_t cmd_size=0;

/*{{{  init_cmd_read*/
private void init_cmd_read(cmd_list_t *cmds,int used)
 {
   assert(used,"no program given\n");
   cmd_data=cmds;
   cmd_open=used;
   debug_m("init cmd_read\n");
 }
/*}}}  */
/*{{{  cmd_read*/
private const unsigned char *cmd_read(void)
 { 

   debug_maa("cmd_read open=%d cur=>%s<\n",cmd_open,cmd_data->cmd?cmd_data->cmd:"empty");
   for (;cmd_open;)
    { if (cmd_data->file)
       /*{{{  get data from file*/
       { size_t readin;
         int c;
         static FILE *f=0;

         if (!cmd_buffer)
          /*{{{  malloc new space for command*/
          { cmd_size=BLOCKSIZE;
            debug_ma("growing cmd buffer to %ld\n",(long int)cmd_size);
            cmd_buffer=do_malloc(cmd_size);
          }
          /*}}}  */
         /*{{{  maybe open command file*/
         if (!f)
          { cmd_file_name=cmd_data->cmd;
            cmd_lineno=0;
            if (!(f=fopen(cmd_file_name,"r")))
               exit_maaa_prog(error_open_file,"command",cmd_file_name,error_msg(msg_unknown),EX_DATAERR);
          }
         /*}}}  */
         for (readin=0;;)
          { switch ((c=fgetc(f)))
             { default:
                  if (readin==cmd_size)
                   /*{{{  extend read buffer*/
                   { cmd_size=2*cmd_size;
                     debug_ma("growing cmd buffer to %ld\n",(long int)cmd_size);
                     cmd_buffer=do_realloc(cmd_buffer,cmd_size);
                   }
                   /*}}}  */
                  cmd_buffer[readin++]=c;
                  continue;
               case EOF:
                  fclose(f);
                  f=0;
                  cmd_data++;
                  cmd_open--;
               case CMD_SEPARATOR:
                  cmd_lineno++;
                  cmd_buffer[readin]=CMD_SEPARATOR;
                  cmd_buffer[readin+1]='\0';
                  break;
             }
            break;
          }
         if (cmd_buffer[0]==CMD_SEPARATOR && cmd_buffer[1]=='\0' && !f)
            continue;
       }
       /*}}}  */
      else
       /*{{{  get cmd-line arg*/
       { static const char lim[]= { CMD_SEPARATOR,'\0' };
         static int e_count=0;

         cmd_file_name=e_command;
         cmd_lineno=++e_count;
         if (cmd_size<strlen(cmd_data->cmd)+2)
          /*{{{  malloc new space for command*/
          { if (cmd_buffer)
               free(cmd_buffer);
            cmd_size=strlen(cmd_data->cmd);
            debug_ma("growing cmd buffer to %ld\n",(long int)cmd_size);
            cmd_buffer=do_malloc(cmd_size);
          }
          /*}}}  */
         strcpy((char*)cmd_buffer,cmd_data->cmd);
         strcat((char*)cmd_buffer,lim);
         cmd_data++;
         cmd_open--;
       }
       /*}}}  */
      /*{{{  overread leading gap and maybe get loop on comment or empty line*/
      { const unsigned char *ret;

        switch ((ret=white_skip(cmd_buffer))[0])
         { case QUOTE_CHAR:
              if (ret[1]==CMD_ADRESS && ret[2]==CMD_ADRESS)
           case CMD_SEPARATOR:
                 continue;
           default:
              return(ret);
         }
      }
      /*}}}  */
    }
   return(0);
 }
/*}}}  */
/*{{{  end_cmd_read*/
private void end_cmd_read(void)
 {
   assert(!cmd_open,"crashed ending compilation");
   if (cmd_buffer)
      free(cmd_buffer);
   debug_m("end cmd_read\n");
 }
/*}}}  */
/*}}}  */

public void cmd_compiler(cmd_list_t *cmds,int used)
 { const unsigned char *buff;
   const unsigned char *err_msg=error_no_prog;

   init_cmd_read(cmds,used);
   while ((buff=cmd_read()))
    { static int active_group=DFLT_GROUP_ID;

      debug_prog("pre compiler");
      debug_ma("Compile: >%s<\n",buff);
      if
       /*{{{  group command*/
       (    buff[0]==QUOTE_CHAR
         && buff[1]==CMD_ADRESS
         && (test_separator(buff[2]) || buff[2]==CMD_SEPARATOR)
       )
       /*}}}  */
       /*{{{  handle group braces*/
       { buff=white_skip(buff+2);
         if (test_comment(buff))
          /*{{{  stop group*/
            if (active_group!=DFLT_GROUP_ID)
               active_group=DFLT_GROUP_ID;
            else
             { err_msg=error_no_group;
               goto error_exit;
             }
          /*}}}  */
         else
          /*{{{  start group*/
          { unsigned char *s;
            unsigned char c;

            if (active_group!=DFLT_GROUP_ID)
             { err_msg=error_group;
               goto error_exit;
             }
            s=(unsigned char*)grp_skip(buff);
            c=s[0];
            *s='\0';
            switch ((active_group=get_group(buff)))
             { default:
                  s[0]=c;
                  buff=white_skip(s);
                  if (test_comment(buff))
                     break;
               case POS_GROUP_ID:
               case WRT_GROUP_ID:
               case DFLT_GROUP_ID:
                  err_msg=error_invalid_group;
                  goto error_exit;
             }
          }
          /*}}}  */
       }
       /*}}}  */
      else
       /*{{{  command*/
       {
         /*{{{  variables*/
         struct cmd_list *p;
         unsigned char *dest;
         enum guard_t mode;
         enum pos_mode_t pos_mode;
         struct { off_t l;off_t o; } f_add;
         struct { off_t l;unsigned char *t; } p1,p2;
         struct act_list *actions;
         /*}}}  */

         if (buff[0]==QUOTE_CHAR && buff[1]==CMD_ADRESS)
          /*{{{  get adress*/
          { p1.l=0;
            buff+=2;
            mode=adress;
            f_add.o=(off_t)strtol((char*)buff,(char**)&buff,0);
            f_add.o=f_add.o+1-START_COUNT;
            f_add.l=0;
            if (buff[0]==CMD_REGION_LEN)
             { buff++;
               f_add.l=(off_t)strtol((char*)buff,(char**)&buff,0);
             }
            buff=white_skip(buff);
            dest=(unsigned char*)buff;
          }
          /*}}}  */
         else
          /*{{{  scan search*/
          { mode=pattern;
            for (p1.l=0,dest=p1.t=(unsigned char*)buff;;)
             { unsigned int c;

               switch ((c=buff[0]))
                { default:
                     if (test_separator(c))
                      { buff=white_skip(buff);
                  case CMD_SEPARATOR:
                        break;
                  case QUOTE_CHAR:
                        switch ((c= *++buff))
                         { case QUOTE_FOR_NL:
                              c='\n';
                              break;
                           case QUOTE_FOR_TAB:
                              c='\t';
                              break;
                           default:
                              if (!test_separator(c))
                               { const unsigned char *s;

                                 s=buff;
                                 c=(unsigned int)strtol
                                                  ( (char*)buff,
                                                    (char**)&buff,
                                                    0
                                                  );
                                 if (s==buff)
                                    c=s[0];
                               }
                           case QUOTE_CHAR:
                           case CMD_ADRESS:
                              break;
                         }
                      }
                     *dest++=c;
                     buff++;
                     p1.l++;
                     continue;
                }
               break;
             }
            if (!p1.l)
             { err_msg=error_empty_search;
               goto error_exit;
             }
          }
          /*}}}  */
         /*{{{  scan replace*/
         p2.l=0;
         p2.t=dest;
         if (buff[0]==QUOTE_CHAR && buff[1]==CMD_ADRESS)
          /*{{{  empty replace*/
            buff+=2;
          /*}}}  */
         else
          /*{{{  scan word*/
            for (;;)
             { unsigned char c;

               switch ((c=buff[0]))
                { default:
                     if (test_separator(c))
                      {
                  case CMD_SEPARATOR:
                        break;
                  case QUOTE_CHAR:
                        switch ((c= *++buff))
                         { case QUOTE_FOR_NL:
                              c='\n';
                              break;
                           case QUOTE_FOR_TAB:
                              c='\t';
                              break;
                           default:
                              if (!test_separator(c))
                               { const unsigned char *s;

                                 s=buff;
                                 c=(unsigned int)strtol
                                                  ( (char*)buff,
                                                    (char**)&buff,
                                                    0
                                                  );
                                 if (s==buff)
                                    c=s[0];
                               }
                           case QUOTE_CHAR:
                           case CMD_ADRESS:
                              break;
                         }
                      }
                     *dest++=c;
                     p2.l++;
                     buff++;
                     continue;
                }
               break;
             }
          /*}}}  */
         /*}}}  */
         /*{{{  handle groups commands*/
         pos_mode=no_pos;
         for (actions=0;;)
          { int grp_mode=1;

            buff=white_skip(buff);
            switch (*buff)
             { case QUOTE_CHAR:
                  if (test_comment(buff))
               case CMD_SEPARATOR:
                     break;
               default:
                  err_msg=error_invalid_group;
                  goto error_exit;
               /*{{{  START/STOP handle the group*/
               case CMD_STOP:
                  grp_mode= -1;
               case CMD_START:
                { off_t grp_len;
                  struct act_list *new;
                  unsigned char *name;

                  buff++;
                  grp_len=grp_skip(buff)-buff;
                  /*{{{  malloc and store action name*/
                  new=do_malloc((size_t)(sizeof(struct act_list)+grp_len+1));
                  name=(unsigned char*)((struct act_list*)new+1);
                  memcpy(name,buff,(size_t)grp_len);
                  name[grp_len]='\0';
                  /*}}}  */
                  /*{{{  check for action permission*/
                  switch ((new->group_and_action=get_group(name)*grp_mode))
                   { case -WRT_GROUP_ID:
                        if (!(rp_mode&lower))
                         { err_msg=error_stop_write;
                           goto error_exit;
                         }
                        break;
                     case POS_GROUP_ID:
                     case -POS_GROUP_ID:
                        if (!(rp_mode&higher))
                         { err_msg=error_pos;
                           goto error_exit;
                         }
                        pos_mode=(enum pos_mode_t)-new->group_and_action;
                        break;
                     default:
                        break;
                   }
                  /*}}}  */
                  /*{{{  link command to list*/
                  new->next=actions;
                  actions=new;
                  /*}}}  */
                  buff+=grp_len;
                  continue;
                }
               /*}}}  */
             }
            break;
          }
         /*}}}  */
         /*{{{  build repl command*/
         p=do_malloc((size_t)(sizeof(struct cmd_list)+p1.l+p2.l));
         p->group=active_group;
         if ((p->type=mode)==pattern)
          { p->len_search=p1.l;
            p->guard.s=(const unsigned char*)((struct cmd_list*)p+1);
            memcpy((char*)p->guard.s,p1.t,(size_t)p1.l);
            p->r=p->guard.s+p1.l;
          }
         else
          { p->len_search=f_add.l;
            if (f_add.l>program.max_length)
               program.max_length=f_add.l;
            p->guard.add=f_add.o;
            p->r=(const unsigned char*)((struct cmd_list*)p+1)+p1.l;
          }
         p->len_replace=p2.l;
         memcpy((char*)p->r,p2.t,(size_t)p2.l);
         p->pos_mode=pos_mode;
         p->act_list=actions;
         /*}}}  */
         add_prg(p);
       }
       /*}}}  */
      program.mode=(active_group==DFLT_GROUP_ID)?ok_prg:open_grp_prg;
    }
   end_cmd_read();
   debug_prog("pre-checks");
   /*{{{  check compiled program*/
   switch (program.mode)
    { case open_grp_prg:
         err_msg=error_open_group;
      case no_prg:
         exit_m_prog(err_msg,EX_DATAERR);
      case ok_prg:
         if (!program.max_length)
            program.max_length=1;
         break;
    }
   /*}}}  */
   debug_prog("post-checks!");
   return;

 error_exit:
   exit_maa_prog(err_msg,cmd_file_name,cmd_lineno,EX_DATAERR);
 }
/*}}}  */
/*}}}  */
