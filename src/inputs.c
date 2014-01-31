/* ------------------------------------------------------------------------- */
/*   "inputs" : Input routines, preprocessor, non-fatal errors               */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/*   On non-ASCII machines, translate_to_ascii must be altered               */
/* ------------------------------------------------------------------------- */

#include "header.h"

int32 marker_in_file;
int  internal_line, total_source_line;

char *tokens;

char *forerrors_buff;
int forerrors_line;

static char *tokens_p; static int32 **token_adds;
static int *token_numbers;

#ifdef MAC_68K
static char **stack, **stack_longs;
#else
static int32 **stack, **stack_longs;
#endif
static int *stack_nos,
           *stack_slot_free;

extern void inputs_allocate_arrays(void)
{   forerrors_buff = my_malloc(BUFFER_LENGTH, "errors buffer");
    token_adds = my_calloc(sizeof(char *), MAX_TOKENS, "token addresses");
    token_numbers = my_calloc(sizeof(int), MAX_TOKENS, "token numbers");
    stack = my_calloc(sizeof(char *), STACK_SIZE, "stack pointers");
    stack_longs = my_calloc(sizeof(char *), STACK_LONG_SLOTS,
        "stack long slot pointers");
    stack_nos = my_calloc(sizeof(int), STACK_SIZE,
        "stack numbers");
    stack_slot_free = my_calloc(sizeof(int), STACK_SIZE,
        "stack free space flags");
}

static char *stackp1;
static char *stackp2;

extern void inputs_free_arrays(void)
{   my_free(&forerrors_buff, "errors buffer");
    my_free(&token_adds, "token addresses");
    my_free(&token_numbers, "token numbers");
    my_free(&stack,"stack_pointers");
    my_free(&stack_longs,"stack long slot pointers");
    my_free(&stack_nos,"stack numbers");
    my_free(&stack_slot_free,"stack free space flags");
    my_free(&stackp1,"preprocessor stack");
    my_free(&stackp2,"pp stack long slots");
}

/* ------------------------------------------------------------------------- */
/*   Preprocessor stack routines                                             */
/*                                                                           */
/*   This stack simulates inserting lines into the source, used when (eg) a  */
/*   source line like                                                        */
/*                     if 5*fish+2*loaves > multitude                        */
/*                                                                           */
/*   is replaced by assembly lines; the assembly lines are stacked up and    */
/*   will be read in before the next source line.  Most of the complication  */
/*   is to allow for these lines in turn to be rewritten back without them   */
/*   falling out of order, as they would in a simple FIFO stack.             */
/*                                                                           */
/*   The stack needs a reasonable size (10 to 20 at the very minimum)        */
/*   but almost all its lines will be used for short assembler instructions, */
/*   so it needs little provision for full-blown lines (probably only one    */
/*   "long slot" will ever be used, unless "write" is used).                 */
/* ------------------------------------------------------------------------- */

static int entries=0, stack_next=0;
static int ppstack_openb, ppstack_closeb;

extern void stack_create(void)
{   int i; char *stackp;
    stackp=my_malloc(STACK_SIZE*STACK_SHORT_LENGTH,"preprocessor stack");
    stackp1 = stackp;
#ifdef MAC_68K
    for (i=0; i<STACK_SIZE; i++)
        stack[i]=(stackp+i*STACK_SHORT_LENGTH);
#else
    for (i=0; i<STACK_SIZE; i++)
        stack[i]=(int32 *) (stackp+i*STACK_SHORT_LENGTH);
#endif
    stackp=my_malloc(STACK_LONG_SLOTS*BUFFER_LENGTH,"pp stack long slots");
    stackp2 = stackp;
    for (i=0; i<STACK_LONG_SLOTS; i++)
    {   
#ifdef MAC_68K
        stack_longs[i]=(stackp+i*BUFFER_LENGTH);
#else
        stack_longs[i]=(int32 *) (stackp+i*BUFFER_LENGTH);
#endif
        *(stack_longs[i])=0;
    }
    entries=0;
    for (i=0; i<STACK_SIZE; i++) stack_slot_free[i]=1;
}

extern void stack_line(char *p)
{   int i, f, slot;

    /* printf("<%s>\n",p); */

    for (i=0; (i<STACK_SIZE)&&(stack_slot_free[i]==0); i++);
    if (i==STACK_SIZE)
        memoryerror("STACK_SIZE", STACK_SIZE);

    slot=i; stack_slot_free[slot]=0;
    for (i=entries; i>stack_next; i--)
        stack_nos[i]=stack_nos[i-1];
    stack_nos[stack_next++]=slot;
    entries++;

    if (strlen(p)<STACK_SHORT_LENGTH)
        strcpy((char *) stack[slot],p);
    else
    {   *(stack[slot])=0; f=0;
        for (i=0; i<STACK_LONG_SLOTS; i++)
            if ((*(stack_longs[i])==0)&&(f==0))
            {   strcpy((char *)stack_longs[i],p);
                *(stack[slot]+1)=i;
                f=1;
            }
        if (f==0) {
            memoryerror("STACK_LONG_SLOTS",STACK_LONG_SLOTS);
        }
    }
}

static void destack_line(char *p)
{   int i, slot;
    stack_next=0;
    slot=stack_nos[0];
    stack_slot_free[slot]=1;
    i= *(stack[slot]);
    if (i!=0)
        strcpy(p,(char *)stack[slot]);
    else
    {   i= *(stack[slot]+1);
        strcpy(p,(char *)stack_longs[i]);
        *(stack_longs[i])=0;
    }
    entries--;
    for (i=0; i<entries; i++) stack_nos[i]=stack_nos[i+1];
}

/* ------------------------------------------------------------------------- */
/*   Character-level parsing and error reporting routines                    */
/* ------------------------------------------------------------------------- */

static int firsthash_flag=1, recenthash_flag=0;

extern void input_begin_pass(void)
{   total_source_line=0; total_files_read=1;
    internal_line=0; endofpass_flag=0; marker_in_file=0;
#ifdef USE_TEMPORARY_FILES
    utf_zcode_p=zcode;
    if (pass_number==2) open_temporary_files();
#endif
    ppstack_openb=0; ppstack_closeb=0;
    firsthash_flag=1; recenthash_flag=0;
}

static void print_hash(void)
{   if (firsthash_flag==1) { printf("%d:",pass_number); firsthash_flag=0; }
    printf("#"); recenthash_flag=1; fflush(stdout);
}

static int errors[MAX_ERRORS];
static char named_errors_buff[256];

static void message(int style, char *s)
{   if (recenthash_flag==1) printf("\n");
    recenthash_flag=0;
    print_error_line();
    printf("%s: %s\n",(style==1)?"Error":"Warning",s);
#ifdef ARC_THROWBACK
    throwback(style, s);
#endif
    if ((style==1)&&(concise_mode==0))
    {   sprintf(forerrors_buff+68,"  ...etc");
        printf("> %s\n",forerrors_buff);
    }
}

extern void error(char *s)
{   int i;
    if (no_errors==MAX_ERRORS) { fatalerror("Too many errors: giving up"); }
    for (i=0; i<no_errors; i++)
        if (errors[i]==internal_line) return;
    errors[no_errors++]=internal_line;
    message(1,s);
}

extern void warning_named(char *s1, char *s2)
{   int i;
    i=strlen(s1)+strlen(s2)+3;
    if (i>250) s2[247-strlen(s1)]=0;
    sprintf(named_errors_buff,"%s \"%s\"",s1,s2);
    no_warnings++;
    message(2,named_errors_buff);
}

extern void warning(char *s1)
{   if (pass_number==1) return;
    no_warnings++;
    message(2,s1);
}

extern void obsolete_warning(char *s1)
{   if ((pass_number==1) || (obsolete_mode==1)
        || (is_systemfile()==1)) return;
    sprintf(named_errors_buff, "Obsolete usage: %s",s1);
    no_warnings++;
    message(2,named_errors_buff);
}

extern void error_named(char *s1, char *s2)
{   int i;
    i=strlen(s1)+strlen(s2)+3;
    if (i>250) s2[247-strlen(s1)]=0;
    sprintf(named_errors_buff,"%s \"%s\"",s1,s2);
    error(named_errors_buff);
}

extern void no_such_label(char *lname)
{   error_named("No such label as",lname);
}

static void reached_new_line(void)
{   total_source_line++;
    advance_line();
    if (total_source_line%100==0)
    {   if (hash_mode==1) print_hash();
#ifdef MAC_MPW
        SpinCursor(32); /* I.e., allow other tasks to run */
#endif
    }
}

static int not_line_end(int c)
{   if (c=='\n') reached_new_line();
    if ((c==0)||(c=='\n')) return(0);
    return(1);
}

static int quoted_mode;
static int not_case_mode;
static int one_quoted_mode;
static int non_terminator(char c)
{   if (c=='\n') reached_new_line();
    if (quoted_mode!=0)
    {   if ((c==0)||(c=='\\')) return(0);
        return(1);
    }
    if ((c==0)||(c==';')||(c=='!')||(c=='\\')||(c=='{')||(c=='}')) return(0);
    if ((c==':')&&(not_case_mode==0)&&(one_quoted_mode==0)) return(0);
    return(1);
}

/* ------------------------------------------------------------------------- */
/*   The Tokeniser (18)... coming to cinemas near you                        */
/*     incorporating the martial arts classic                                */
/*   Tokeniser II - This Time It's Optimal,                                  */
/*     with Dolph Lundgren as Dilip Sequeira and Gan as the two short planks */
/*   (and now a somewhat sub-standard sequel,                                */
/*     Tokeniser III: The Next Specification is Ugly)                        */
/* ------------------------------------------------------------------------- */

static int no_tokens;

#define NUMBER_SEPARATORS 27

#define QUOTE_CODE    1000
#define DQUOTE_CODE   1001
#define NEWLINE_CODE  1002
#define NULL_CODE     1003
#define SPACE_CODE    1004

/*  This list can't safely be changed without also changing the header
    separator #defines... */

static const char separators[NUMBER_SEPARATORS][4] = { 
    "->", "-->", "--", "-", "++", "+", "*", "/", "%",
    "||", "|", "&&","&",
    "==", "=", "~=", ">=", ">", 
    "<=", "<", "(", ")", ",", ".&", ".#", ".", ":" };

static int char_grid[256];

extern void make_s_grid(void)
{   int i, j;
    for (i=0; i<256; i++) char_grid[i]=0;
    for (i=0; i<NUMBER_SEPARATORS; i++)
    { j=separators[i][0];
      if(char_grid[j]==0) char_grid[j]=i*16+1; else char_grid[j]++;
    }
    char_grid['\''] = QUOTE_CODE;
    char_grid['\"'] = DQUOTE_CODE;
    char_grid['\n'] = NEWLINE_CODE;
    char_grid[0]    = NULL_CODE;
    char_grid[' ']  = SPACE_CODE;
    char_grid[TAB_CHARACTER]  = SPACE_CODE;
}

extern void tokenise_line(void)
{   char *p,*q;
    int i, j, k, bite, tok_l, tn=-1, next_tn=-1, quoted_size;
    const char *r;
    no_tokens=0; tokens_p=tokens; token_adds[0]=(int32 *) tokens_p;
    p=buffer;
    for (i=0, tok_l=0; i<MAX_TOKENS; i++)
    { if(tok_l)
      {   for(j=0;j<tok_l;j++) *tokens_p++= *p++; tok_l=0;
          tn=next_tn; goto got_tok;
      }
      while(char_grid[*p]==SPACE_CODE) p++;
      for(bite=0;1;)
      { 
        if ((*p=='-') && (isdigit(p[1])))
        {
            if (bite) { *tokens_p++=0;
              token_numbers[no_tokens]=tn; tn=-1;
              token_adds[++no_tokens]=(int32 *)tokens_p; }
            *tokens_p++=*p++;
            *tokens_p++=*p++;
            while (isdigit(*p)) *tokens_p++=*p++;
            bite=0; goto got_tok;
        }
        switch(char_grid[*p]) 
        {  case 0:            *tokens_p++= *p++; bite=1; break;
           case SPACE_CODE:   goto got_tok;
           case DQUOTE_CODE:  quoted_size=0;
                              do { *tokens_p++= *p++;
                                   if (quoted_size++==MAX_QTEXT_SIZE)
                                   {   
                            error("Too much text for one pair of \"s to hold");
                                       *p='\"'; break;
                                   }
                                 } while (*p && *p!='\n' && *p!='\"');
                              if (*p=='\"') *tokens_p++= *p++; goto got_tok;
           case NEWLINE_CODE: reached_new_line();
                              if (bite) goto got_tok; return;
           case QUOTE_CODE:   quoted_size=0;
                              do { *tokens_p++= *p++;
                                   if (quoted_size++==MAX_QTEXT_SIZE)
                                   {   
                            error("Too much text for one pair of 's to hold");
                                       *p='\''; break;
                                   }
                                 } while (*p && *p!='\n' && *p!='\'');
                              if (*p=='\'') *tokens_p++= *p++; goto got_tok;
           case NULL_CODE:    if (bite) goto got_tok; return;
           default:  for (j=char_grid[*p]>>4,k=j+(char_grid[*p]&15);j<k;j++) 
                     {   for (q=p,r=separators[j];*q== *r && *r;q++,r++);
                         if (!*r) 
                         {   if(bite)
                             {   tok_l=q-p; next_tn=j; }
                             else
                             {   while(p<q) *tokens_p++= *p++;
                                 tn=j;
                             }
                             goto got_tok;
                         }
                     }
                     *tokens_p++= *p++;bite=1;
        }
      }
    got_tok:
      *tokens_p++=0; token_numbers[no_tokens]=tn; tn=-1;
      token_adds[++no_tokens]=(int32 *)tokens_p;
    }
    error("Too many tokens on line (note: to increase the maximum, \
set $MAX_TOKENS=some-bigger-number on the Inform command line)");

    no_tokens=MAX_TOKENS-1;
    return;
}

extern void word(char *b1, int32 w)
{   if (w>no_tokens) { b1[0]=0; return; }
    strcpy(b1, (char *)token_adds[w-1]);
}

extern int word_token(int w)
{   if (w>no_tokens) return(-1);
    return(token_numbers[w-1]);
}

extern void dequote_text(char *b1)
{   int i;
    if (*b1!='\"') error_named("Open quotes \" expected for text but found",b1);
    for (i=0; b1[i]!=0; i++) b1[i]=b1[i+1];
    i=i-2;
    if (b1[i]!='\"')
        error_named("Close quotes \" expected for text but found",b1);
    b1[i]=0;
}

extern void textword(char *b1, int w)
{   word(b1,w); dequote_text(b1);
}

/* ------------------------------------------------------------------------- */
/*   Get the next line of input, and return 1 if it came from the            */
/*   preprocessor stack and 0 if it really came from the source files.       */
/*   So:                                                                     */
/*     If something's waiting on the stack, send that.                       */
/*     If there are braces to be opened or closed, send those.               */
/*     If at the end of the source, send an "end" directive.                 */
/*     Otherwise, keep going until a ; is reached which is not in 's or "s;  */
/*       throw away everything on any text line after a comment ! character; */
/*       fold out characters between a \ and the first non-space on the next */
/*       line.                                                               */
/* ------------------------------------------------------------------------- */

static int braces_level=0;
static int inferred_braces[MAX_BLOCK_NESTING];
static int ib_sp=0; 

static char first_tok[17];

extern int get_next_line(void)
{   int i, j, k, ftp, first_token_mode,
        block_expected=0, b_l, b_f; char d, d2;
    internal_line++;

    quoted_mode=0; one_quoted_mode=0;
    do
    {   if (entries!=0) { destack_line(buffer); return(1); }
        if (ppstack_openb>0)
        {   strcpy(buffer,"{"); ppstack_openb--; braces_level++; return(1); }
        if (ppstack_closeb>0)
        {   strcpy(buffer,"}"); ppstack_closeb--; braces_level--;
            if ((ib_sp>0)&&(braces_level-1==inferred_braces[ib_sp-1]))
              {   ib_sp--; ppstack_closeb++; }
            return(1);
        }

        if (file_end(marker_in_file)==1) { strcpy(buffer,"#end"); return(1); }
        i=0; j=0; first_token_mode=1; not_case_mode=0; ftp=0; block_expected=0;
      GNLL:
        for (; non_terminator(d=file_char(marker_in_file+i)); i++, j++)
        {
            if (j>=BUFFER_LENGTH-2)
            {   error("Line too long (note: to increase the maximum length, \
set $BUFFER_LENGTH=some-bigger-number on the Inform command line)");
                buffer[BUFFER_LENGTH-2]=0;
                if (pass_number==1)
                {   buffer[76]=0;
                    printf("The overlong line began:\n<%s>\n",buffer);
                }
                return(0);
            }
            buffer[j]=d;
            if (d=='\"' && one_quoted_mode==0) quoted_mode=1-quoted_mode;
            if (d=='\'' && quoted_mode==0) one_quoted_mode=1-one_quoted_mode;
            if ((d=='[') && (pass_number==2) && (quoted_mode==0)
                && (one_quoted_mode==0)) keep_routine_linenum();
            if ((d==']') && (pass_number==2) && (quoted_mode==0)
                && (one_quoted_mode==0)) keep_re_linenum();
            if (d=='(') not_case_mode=1;
            if ((first_token_mode==1) && (d!=' ') && (d!='\n') && (d!='!')
                                      && (d!=TAB_CHARACTER))
            {   forerrors_line = current_source_line();
                first_token_mode=2;
            }
            if (first_token_mode==2)
            {   if (!isalpha(d))
                {   first_token_mode=0;
                    if (ftp>0)
                    {   first_tok[ftp]=0;
                        d2=first_tok[0];
                        if (   ((d2=='o')&&(strcmp(first_tok, "objectloop")==0))
                             ||((d2=='f')&&(strcmp(first_tok, "for")==0))
                             ||((d2=='i')&&(strcmp(first_tok, "if")==0))
                             ||((d2=='s')&&(strcmp(first_tok, "switch")==0))
                             ||((d2=='w')&&(strcmp(first_tok, "while")==0)) )
                        {   block_expected=1; b_l=0; b_f=0; }
                        else
                        if (   ((d2=='d')&&(strcmp(first_tok, "do")==0))
                             ||((d2=='e')&&(strcmp(first_tok, "else")==0))  )
                            goto MakeBlock;
                    }
                }
                else
                {   if (ftp==16) ftp=0;
                    else if (isupper(d)) first_tok[ftp++]=tolower(d);
                    else first_tok[ftp++]=d;
                }
            }
            if ((block_expected==1)&&(quoted_mode==0))
            {   if (d=='!')
                {   j--;
                    while (not_line_end(file_char(marker_in_file+i))) i++;
                    i++; goto GNLL;
                }
                if (d=='(') { b_l++; b_f=1; }
                else
                    if ((b_f==0)&&(d!=' ')&&(d!='\n'))
                    {   block_expected=0;
                    }    
                if (d==')')
                {   b_l--;
                    if (b_l==0)
                    {   
                        MakeBlock: j++; i++;
                        buffer[j]=0;

                        ppstack_openb++; marker_in_file+=i; i=0;
                        do
                        {   d=file_char(marker_in_file+i++);
                            if (d=='!')
                                do
                                {   d=file_char(marker_in_file+i++);
                                } while (d!='\n');
                            if (d=='\n') reached_new_line();
                        } while ((d==' ')||(d=='\n'));
                        marker_in_file+=(i-1);
/* ??? */               if (d==';') ppstack_closeb++;
    /* else? */                    else if (d=='{') marker_in_file++;
                        else inferred_braces[ib_sp++]=braces_level;

                        for (i=0; buffer[i]!=0; i++)
                            if (buffer[i]=='\n') buffer[i]=' ';
                        return(0);
                    }
                }
            }
        }

        switch(d)
        {   case '!': while (not_line_end(file_char(marker_in_file+i))) i++;
                      i++; goto GNLL;
            case '{': ppstack_openb++; break;
            case '}': ppstack_closeb++; break;
            case ':': buffer[j++]=':'; break;
            case '\\':
                while (not_line_end(file_char(marker_in_file+i))) i++; i++;
                while ( ((k=file_char(marker_in_file+i))==' ')
                        || (k==TAB_CHARACTER) ) i++; goto GNLL;
        }

        buffer[j]=0;
        marker_in_file+=i+1;

        for (i=0; buffer[i]!=0; i++)
          if (buffer[i]=='\n') buffer[i]=' ';

        for (i=0; buffer[i]!=0; i++)
          if (buffer[i]!=' ')
          {   if ((ib_sp>0)&&(braces_level-1==inferred_braces[ib_sp-1]))
              {   ib_sp--; ppstack_closeb++; }
              return(0);
          }
    } while (1==1);
    return(0);
}

extern void init_inputs_vars(void)
{   entries = 0;
    stack_next = 0;
    firsthash_flag = 0;
    recenthash_flag = 0;
    braces_level = 0;
    ib_sp = 0;

    forerrors_buff = NULL;
    token_adds = NULL;
    token_numbers = NULL;
    stack = NULL;
    stack_longs = NULL;
    stack_nos = NULL;
    stack_slot_free = NULL;
}
