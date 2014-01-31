/* ------------------------------------------------------------------------- */
/*   "inform" :  The top level of Inform: some central variables, main,      */
/*               switches, the top level parser and the compiler level       */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include "header.h"

int version_number=5, actual_version=5, override_version=0;
int32 scale_factor=4;

int ignoring_mode=0;

int process_filename_flag=0;

int no_symbols=0,     no_routines,    no_locals,
    no_errors=0,      no_warnings=0,  no_abbrevs;

zip  *zcode,      *zcode_p,    *utf_zcode_p,
     *symbols_p,  *symbols_top,
     *strings,    *strings_p,
     *low_strings, *low_strings_p,
     *dictionary, *dict_p,
     *output_p,   *abbreviations_at;

char *all_text, *all_text_p;

int statistics_mode=0,     offsets_mode=0,     tracing_mode=0,
    ignoreswitches_mode=0, bothpasses_mode=0,  hash_mode=0,
    percentages_mode=0,    trace_mode,         ltrace_mode,
    etrace_mode,           listing_mode=0,     concise_mode=0,
    nowarnings_mode=0,     frequencies_mode=0, memory_map_mode=0,
    double_spaced=0,       economy_mode=0,     memout_mode=0,
    transcript_mode=0,     optimise_mode=0,    store_the_text=0,
    abbrev_mode=1,         withdebug_mode=0,   listobjects_mode=0,
    printprops_mode=0,     debugging_file=0,   obsolete_mode=0,
    extend_memory_map=0;

int error_format=DEFAULT_ERROR_FORMAT;

int pass_number, endofpass_flag=0;

int32 code_offset       = 0x800,
      actions_offset    = 0x800,
      preactions_offset = 0x800,
      dictionary_offset = 0x800,
      adjectives_offset = 0x800,
      variables_offset  = 0,
      strings_offset    = 0xc00;

int32 Out_Size, Write_Code_At, Write_Strings_At;
int brace_sp;

int no_dummy_labels;

static int no_blocks_made,
           brace_stack[MAX_BLOCK_NESTING],
           brace_type_stack[MAX_BLOCK_NESTING],
           next_block_type, forloop_flag;

#define MAX_FOR_DEPTH 6

static char *forcloses[MAX_FOR_DEPTH];

#ifdef UNIX
char Temp1_Name[50], Temp2_Name[50];
#endif

#ifdef AMIGA
char Temp1_Name[50], Temp2_Name[50];
#endif

#ifdef MAC_FACE
char *Transcript_Name;
char *Debugging_Name;
#endif

/* ------------------------------------------------------------------------- */
/*   Allocation of large arrays (others are done by "tables" and "symbols")  */
/* ------------------------------------------------------------------------- */

#ifndef ALLOCATE_BIG_ARRAYS
  int     abbrev_values[MAX_ABBREVS];
  int     abbrev_quality[MAX_ABBREVS];
  int     abbrev_freqs[MAX_ABBREVS];
  char    buffer[BUFFER_LENGTH];
  char    sub_buffer[BUFFER_LENGTH];
  char    parse_buffer[BUFFER_LENGTH];
  char    rewrite[BUFFER_LENGTH];
  char    rewrite2[BUFFER_LENGTH];
#else
  int     *abbrev_values;
  int     *abbrev_quality;
  int     *abbrev_freqs;
  char    *buffer;
  char    *sub_buffer;
  char    *parse_buffer;
  char    *rewrite;
  char    *rewrite2;
#endif

static void allocate_the_arrays(void)
{   int i;
#ifdef ALLOCATE_BIG_ARRAYS
    symbols_allocate_arrays();
    tables_allocate_arrays();
    express_allocate_arrays();
    asm_allocate_arrays();
    inputs_allocate_arrays();
    abbrev_values  = my_calloc(sizeof(int), MAX_ABBREVS, "abbrev values");
    abbrev_quality = my_calloc(sizeof(int), MAX_ABBREVS, "abbrev quality");
    abbrev_freqs = my_calloc(sizeof(int),   MAX_ABBREVS, "abbrev freqs");
    buffer     = my_calloc(sizeof(char),    BUFFER_LENGTH, "buffer");
    sub_buffer   = my_calloc(sizeof(char),  BUFFER_LENGTH, "sub_buffer");
    parse_buffer = my_calloc(sizeof(char),  BUFFER_LENGTH, "parse_buffer");
    rewrite    = my_calloc(sizeof(char),    BUFFER_LENGTH, "rewrite buffer 1");
    rewrite2   = my_calloc(sizeof(char),    BUFFER_LENGTH, "rewrite buffer 2");
#endif

    for (i=0; i<MAX_FOR_DEPTH; i++)
        forcloses[i]=my_malloc(BUFFER_LENGTH/2,
                         "complex loop constructs");
}

extern void free_remaining_arrays(void)
{   int i;
#ifdef ALLOCATE_BIG_ARRAYS
    tables_free_arrays();
    asm_free_arrays();
    inputs_free_arrays();

    my_free(&sub_buffer, "sub_buffer");
    my_free(&abbrev_values, "abbrev values");
    my_free(&abbrev_quality,"abbrev quality");
    my_free(&abbrev_freqs,  "abbrev freqs");
    my_free(&buffer, "buffer");

    my_free(&parse_buffer,"parse_buffer");
    my_free(&rewrite, "rewrite buffer 1");
    my_free(&rewrite2, "rewrite buffer 2");
#endif
    for (i=0; i<MAX_FOR_DEPTH; i++)
        my_free(&(forcloses[i]),"for closes");
    my_free(&abbreviations_at,"abbreviations");
    my_free(&zcode,"zcode");
    my_free(&dictionary,"dictionary");
    my_free(&strings,"static strings");
    my_free(&low_strings,"low (synonym) strings");
    my_free(&tokens,"tokens");
    my_free(&properties_table,"properties table");
    my_free(&output_p,"output buffer");
}

/* ------------------------------------------------------------------------- */
/*   Begin pass                                                              */
/* ------------------------------------------------------------------------- */

static void begin_pass(void)
{   
    almade_flag=0;
    trace_mode=tracing_mode; no_routines=0; no_stubbed=0;
    no_abbrevs=0; in_routine_flag=1;

    zcode_p=zcode; strings_p=strings; low_strings_p=low_strings;

    if (store_the_text==1) all_text_p=all_text;

    no_dummy_labels=0;

    total_chars_trans=0; total_bytes_trans=0;

    no_blocks_made=1; brace_sp=0; ltrace_mode=0; forloop_flag=0;
    next_block_type=0;
    if (pass_number==2) ltrace_mode=listing_mode;

    input_begin_pass();
    tables_begin_pass();
    args_begin_pass();
}

/* ------------------------------------------------------------------------- */
/*   Case translation                                                        */
/* ------------------------------------------------------------------------- */

extern void make_lower_case(char *str)
{   int i;
    for (i=0; str[i]!=0; i++)
        if (isupper(str[i])) str[i]=tolower(str[i]);
}

extern void make_upper_case(char *str)
{   int i;
    for (i=0; str[i]!=0; i++)
        if (islower(str[i])) str[i]=toupper(str[i]);
}

/* ------------------------------------------------------------------------- */
/*   Compiler top level: block structures, loops, if statements and commands */
/*   (all using the expression evaluator)                                    */
/* ------------------------------------------------------------------------- */

static void trace_line(int origin)
{   int i, k;
    word(sub_buffer,1);
    if ((ltrace_mode==1)||((listing_mode==1)&&(bothpasses_mode==1))
        ||(sub_buffer[0]=='@')||(sub_buffer[0]=='.')
        ||(sub_buffer[0]=='[')||(sub_buffer[0]==']'))
    {   printf("%4d%s   ",forerrors_line,(origin==0)?" ":"*  ");
        if (sub_buffer[0]=='@') printf("  ");
        i=1, k=1; do { word(sub_buffer,i++); k++;
                  printf("%s ",sub_buffer); } while (sub_buffer[0]!=0);
        printf("\n");
    }
}

static origin_of_line;

static void process_next_line(void)
{   origin_of_line=get_next_line();
    if (origin_of_line==0) strcpy(forerrors_buff,buffer);
    tokenise_line();
    if ((ltrace_mode>=1)||((listing_mode==1)&&(bothpasses_mode==1)))
        trace_line(origin_of_line);
}            

static char forvariable[MAX_IDENTIFIER_LENGTH];
static int forbstack[MAX_IDENTIFIER_LENGTH];
static int newfordepth=0;
static int until_expected;

static int switch_stack[MAX_BLOCK_NESTING];
static int switch_stage[MAX_BLOCK_NESTING];
static int switch_sp=0;

static void cword(char *b, int n)
{   if (n>=0) word(b,n);
    else if (n==-1) strcpy(b,"sp");
    else if (n==-2) error("Attempt to use an assignment as a value");
    else if (n==-3) error("Attempt to use void as a value");
}

#define IF_TYPE     0
#define ELSE_TYPE   1
#define WHILE_TYPE  2
#define SWITCH_TYPE 3
#define DO_TYPE     4
#define NULL_TYPE 100

static void compile_openbrace(void)
{
    brace_stack[brace_sp]        = no_blocks_made++;
    brace_type_stack[brace_sp++] = next_block_type;

    if (forloop_flag==1)
    {   sprintf(rewrite,"@inc %s",forvariable);
        stack_line(rewrite);
        forloop_flag=0;
    }
}

static int last_block_type;

static void compile_closebrace(char *b)
{   int j, block_type; char *p;

    if (brace_sp <= 0) { error("Unmatched '}' found"); return; }

    j=brace_stack[--brace_sp];
    block_type=brace_type_stack[brace_sp];

    if (block_type==WHILE_TYPE)
    {   if ((newfordepth>0)&&(j==forbstack[newfordepth-1]))
        {   p=forcloses[--newfordepth];
            if (p[0]!=0) stack_line(p);
        }
        sprintf(rewrite,"@jump _w%d",j);
        stack_line(rewrite);
    }

    if (block_type==SWITCH_TYPE) switch_sp--;

    if (block_type==DO_TYPE) until_expected=1;

    last_block_type = block_type;

    sprintf(rewrite,"@._f%d",j);
    stack_line(rewrite);
}

static void rearrange_stack(int a, int b, int c)
{   
     if ((a==-1)&&(b==-1)&&(c==-1))
     {   stack_line("@pull temp_global3");
         stack_line("@pull temp_global2");
         stack_line("@pull temp_global");
         stack_line("@push temp_global3");
         stack_line("@push temp_global2");
         stack_line("@push temp_global"); return;
     }
     if ( ((a==-1)&&(b==-1))
          || ((a==-1)&&(c==-1))
          || ((b==-1)&&(c==-1)))
     {   stack_line("@pull temp_global2");
         stack_line("@pull temp_global");
         stack_line("@push temp_global2");
         stack_line("@push temp_global");
     }
}

int call_for_br_flag = 0;

static int brcf[3] = { 0, 0, 0 };
static int btcf[3] = { 0, 0, 0 };

extern void compile_box_routine()
{   if (brcf[pass_number]==1) return;
    brcf[pass_number]=1;
    stack_line("[ Box__Routine n maxw table w w2 line lc t");
    stack_line("@add n 6 sp");
    stack_line("@split_window sp");
    stack_line("@set_window 1");
    stack_line("w = 0 -> 33");
    stack_line("if ( w == 0 )");
    stack_line("{");
    stack_line("w = 80");
    stack_line("}");
    stack_line("w2 = ( w - maxw ) / 2");
    stack_line("style reverse");
    stack_line("@sub w2 2 w");
    stack_line("line = 5");
    stack_line("lc = 0");
    stack_line("@set_cursor 4 w");
    stack_line("spaces maxw + 4");
    stack_line("do");
    stack_line("{");
    stack_line("@set_cursor line w");
    stack_line("spaces maxw + 4");
    stack_line("@set_cursor line w2");
    stack_line("t = table --> lc");
    stack_line("if ( t ~= 0 )");
    stack_line("{");
    stack_line("print_paddr t");
    stack_line("}");
    stack_line("@inc line");
    stack_line("@inc lc");
    stack_line("}");
    stack_line("until ( lc == n ) ");
    stack_line("@set_cursor line w");
    stack_line("spaces maxw + 4");
    stack_line("@buffer_mode 1");
    stack_line("style roman"); /* stack_line("@new_line"); */
    stack_line("@set_window 0");
    stack_line("@split_window 1");

    stack_line("@output_stream $ffff");
    stack_line("@print \"[ \"");
    
    stack_line("lc = 0");
    stack_line(".br_1");
    stack_line("w = table --> lc");
    stack_line("if ( w ~= 0 )");
    stack_line("{");
    stack_line("@print_paddr w");
    stack_line("}");
    stack_line("@inc lc");
    stack_line("if ( lc == n)");
    stack_line("{");
    stack_line("@print \"]^^\"");
    stack_line("@jump br_2");
    stack_line("}");
    stack_line("@print \"^  \"");
    stack_line("@jump br_1");
    stack_line(".br_2");
    stack_line("@output_stream 1");
    stack_line("]");
}

static int routines_needed[8];
static int routines_unspecified[8];

static int compile_unspecifieds(void)
{   int i, f=0; char *p;

    if (pass_number==1)
    {   for (i=1;i<=7;i++)
        {   routines_unspecified[i]=0;
            if (routines_needed[i]==1)
            {   switch(i)
                {   case 1: p="R_Process"; break;
                    case 3: p="DefArt"; break;
                    case 4: p="InDefArt"; break;
                    case 5: p="CDefArt"; break;
                    case 6: p="PrintShortName"; break;
                    case 7: p="EnglishNumber"; break;
                }
                if (find_symbol(p)==-1) routines_unspecified[i]=1;
            }
        }
    }

 for (i=1;i<=7;i++)
  if (routines_unspecified[i]==1)
  { f=1;
    switch(i)
    {   case 1:
            stack_line("[ R_Process");
            stack_line("]");
            break;
        case 3:
            stack_line("[ DefArt x");
            stack_line("@print \"the \"");
            stack_line("@print_obj x");
            stack_line("]");
            break;
        case 4:
            stack_line("[ InDefArt x");
            stack_line("@print \"a \"");
            stack_line("@print_obj x");
            stack_line("]");
            break;
        case 5:
            stack_line("[ CDefArt x");
            stack_line("@print \"The \"");
            stack_line("@print_obj x");
            stack_line("]");
            break;
        case 6:
            stack_line("[ PrintShortName x");
            stack_line("@print_obj x");
            stack_line("]");
            break;
        case 7:
            stack_line("[ EnglishNumber x");
            stack_line("@print_num x");
            stack_line("]");
            break;
    }
  }
  return f;
}

static int pr_implied_flag=0;
static int expected_until;

static void compiler(char *b, int32 code)
{   int i, j, k, trans, dir, labnum, brace_spc;
    char *cond="";
    void_context=0;

    if (code!=ELSE_CODE) last_block_type=NULL_TYPE;

  switch(code)
  { case ASSIGNMENT_CODE:
        assignment(1,1);
        if (next_token!=-1) error("Spurious terms after assignment");
        return;

    case FUNCTION_CODE:
        void_context=1;
        i=expression(1);
        if (i!=-3)
            error("Malformed statement 'Function(...);'");
        if (next_token!=-1) error("Spurious terms after function call");
        return;

    case DO_CODE:
        sprintf(rewrite,"@._s%d",no_blocks_made); stack_line(rewrite);
        next_block_type=DO_TYPE; return;
     
    case FOR_CODE:
        word(b,2);
        if (strcmp(b,"(")==0) goto NewStyleFor;

     obsolete_warning("modern 'for' syntax is 'for (start:condition:update)'");
        forloop_flag=1;
        strcpy(forvariable,b);
        i=expression(3);
        cword(b,i);
        sprintf(rewrite,"@store %s %s",forvariable,b);
        stack_line(rewrite);
        sprintf(rewrite,"@dec %s",forvariable);
        stack_line(rewrite);
        if (next_token==-1)
        {   error("'to' missing in old-style 'for' loop"); return; }
        word(b,next_token++);
        if (strcmp(b,"to")!=0)
        {   error("'to' expected in old-style 'for' loop"); return; }
        word(b,next_token);
        if (b[0]==0)
        { error("Final value missing in old-style 'for' loop"); return; }
        i=expression(next_token);
        if (next_token!=-1)
        { error("'{' required after an old-style 'for' loop"); return; }
        if (i==-1)
        {   error("Old-style 'for' loops must have simple final values");
            return;
        }
        cword(b,i);
        sprintf(rewrite,"while %s < %s",forvariable,b);
        stack_line(rewrite);
        return;

    NewStyleFor:
        if (newfordepth==MAX_FOR_DEPTH)
        {   error("'for' loops too deeply nested"); return; }
        word(b,3);
        if (strcmp(b,":")==0) { next_token=4; }
        else
        {   assignment(3,0);
            if (next_token==-1) { error("':' expected in 'for' loop"); return; }
            word(b,next_token++);
            if (strcmp(b,":")!=0)
            { error("':' expected in 'for' loop"); return; }
        }
        word(b,next_token);
        if (strcmp(b,":")==0)
        {   next_token++; sprintf(rewrite, "while 1==1");
        }
        else
        {   sprintf(rewrite,"while ");
            do
            {   word(b,next_token++);
                if (strcmp(b,":")==0) goto FoundColon;
                sprintf(rewrite+strlen(rewrite),"%s ",b);
            } while (b[0]!=0);
            error("Second ':' expected in 'for' loop"); return;
            FoundColon:;
        }
        stack_line(rewrite);
        word(b,next_token); rewrite[0]=0;
        if (strcmp(b,")")==0)
        {   next_token++; goto FoundEnd;   }

        i=1;
        do
        {   word(b,next_token++);
            if (strcmp(b,"(")==0) i++;
            if (strcmp(b,")")==0)
                if (--i==0) goto FoundEnd;
            sprintf(rewrite+strlen(rewrite),"%s ",b);
        } while (b[0]!=0);
        error("Concluding ')' expected in 'for' loop"); return;

        FoundEnd:
        forbstack[newfordepth]=no_blocks_made;
        strcpy(forcloses[newfordepth++],rewrite);
        return;

    case IF_CODE:    trans=1; goto Translation;
    case UNTIL_CODE:
        if (expected_until==0)
            error("'until' without matching 'do'");
        trans=4; goto Translation;
    case WHILE_CODE: trans=2; goto Translation;
    case SWITCH_CODE:
        next_block_type = SWITCH_TYPE;
        word(b,2);
        if (strcmp(b,"(")!=0)
            error_named("Expected bracketed expression but found", b);
        i=expression(2); switch_stage[switch_sp]=0;
        switch_stack[switch_sp++]=0;
        cword(b,i);
        sprintf(rewrite,"temp_global = %s",b);
        stack_line(rewrite);
        return;

    case BREAK_CODE:
        brace_spc=brace_sp;
        do
        {   i=brace_type_stack[--brace_spc];
            j=brace_stack[brace_spc];
        } while (((i==IF_TYPE)||(i==ELSE_TYPE))&&(brace_spc>=0));
        sprintf(rewrite,"@jump _f%d",j);
        stack_line(rewrite);
        return;

    case ELSE_CODE:
        if (last_block_type==NULL_TYPE)
        {   error("'else' applied to something other than 'if'");
            return;
        }
        if (last_block_type==ELSE_TYPE)
        {   error("'if' statement with more than one 'else'");
            return;
        }
        if (last_block_type==WHILE_TYPE)
        {   error("'else' attached to a loop block");
            return;
        }
        if (last_block_type==SWITCH_TYPE)
        {   error("'else' attached to a 'switch' block");
            return;
        }
        if (pass_number==1)
        {   svals[no_symbols-1]+=3;
            stypes[no_symbols-1]=20;
        }
        sprintf(rewrite,"@jump _f%d",no_blocks_made);
        stack_line(rewrite);
        next_block_type=ELSE_TYPE; return;

    case FONT_CODE:
        word(b,2);
        On_("on") { stack_line("0-->8 = $fffd&(0-->8)"); return; }
        On_("off") { stack_line("0-->8 = 2|(0-->8)"); return; }
        error_named("Expected 'on' or 'off' after 'font' but found",b); return;

    case GIVE_CODE:
        i=expression(2);
        if (next_token==-1)
        {   error("Expected some attributes to 'give'"); return; }
        if (i==-1) stack_line("@pull temp_global");
        do
        {   char *bb;
            word(b,next_token);
            if (b[0]!=0)
            {   if (b[0]=='~') { sprintf(rewrite, "@clear_attr "); bb=b+1; }
                else { bb=b; sprintf(rewrite, "@set_attr "); }
                if (i!=-1) cword(b,i); else strcpy(b,"temp_global");
                sprintf(rewrite+strlen(rewrite), "%s ", b);
                word(b,next_token); next_token++;
                sprintf(rewrite+strlen(rewrite), "%s", bb);
                stack_line(rewrite);
            }
        } while (b[0]!=0);
        return;

    case INVERSION_CODE:
        sprintf(rewrite,"@print \"%d\"",VNUMBER);
        stack_line(rewrite);
        return;

    case MOVE_CODE:
        i=expression(2);
        if (next_token==-1)
        {   error("Expected 'to <object>' in 'move'"); return; }
        word(b,next_token++);
        if (strcmp(b,"to")!=0)
        {   error_named("Expected 'to' in 'move' but found",b); return; }
        j=expression(next_token);
        cword(b,i);
        sprintf(rewrite, "@insert_obj %s",b);
        cword(b,j);
        sprintf(rewrite+strlen(rewrite), " %s",b);
        rearrange_stack(i,j,0);
        stack_line(rewrite);
        return;

    case OBJECTLOOP_CODE:
        word(b,2);
        if (strcmp(b,"(")!=0)
        {   error("Open bracket '(' expected in 'objectloop'"); return; }
        word(b,3);
        sprintf(rewrite,"for ( %s = ",b);
        word(b,4);
             On_("from") { word(b,5);
                           sprintf(rewrite+strlen(rewrite),"%s ",b); }
        else On_("in")   { word(b,5);
                           sprintf(rewrite+strlen(rewrite),
                               "child ( %s ) ",b);
                         }
        else On_("near") { word(b,5);
                           sprintf(rewrite+strlen(rewrite),
                               "child ( parent ( %s ) ) ",b);
                         }
        else { error("'objectloop' must be 'from', 'near' or 'in' something");
               return; }
        word(b,3);
        sprintf(rewrite+strlen(rewrite),
            ": %s ~= 0 : %s = sibling ( %s ) )",b,b,b);
        stack_line(rewrite);
        word(b,6);
        if (strcmp(b,")")!=0)
        {   error("Close bracket ')' expected in 'objectloop'"); return; }
        return;

    case JUMP_CODE:
        word(b,3);
        if (b[0]!=0)
            error_named("Expected label after 'jump' but found", b);
        cond="jump"; goto DoExpression;

    case PRINT_ADDR_CODE:  cond="print_addr";  goto DoExpression;

    case PRINT_CHAR_CODE:  cond="print_char";  goto DoExpression;

    case PRINT_PADDR_CODE: cond="print_paddr"; goto DoExpression;

    case PRINT_OBJ_CODE:   cond="print_obj";   goto DoExpression;

    case PRINT_NUM_CODE:   cond="print_num";   goto DoExpression;

    case PRINT_CODE:
    case PRINT_RET_CODE:
        next_token=2;
        if (pr_implied_flag==1) next_token=1;
        do
        {   sprintf(rewrite,"@print      ");
            word(rewrite+10, next_token);
            if (rewrite[10]=='\"')
            {   stack_line(rewrite); next_token++;
                goto PrintBlockDone;
            }
            if (rewrite[10]=='(')
            {   char *p1, *p2; char buff[128];
                p1=rewrite+10; p2=NULL;
                word(p1, next_token+2);
                if (p1[0]==')')
                {   word(p1, next_token+1); i=0;
                    if (strcmp(p1,"string")==0)
                    {   next_token+=3;
                        i=expression(next_token);
                        sprintf(rewrite,"@print_paddr      ");
                        cword(rewrite+14,i);
                        stack_line(rewrite); goto PrintBlockDone;
                    }
                    if (strcmp(p1,"address")==0)
                    {   next_token+=3;
                        i=expression(next_token);
                        sprintf(rewrite,"@print_addr      ");
                        cword(rewrite+14,i);
                        stack_line(rewrite); goto PrintBlockDone;
                    }
                    if (strcmp(p1,"char")==0)
                    {   next_token+=3;
                        i=expression(next_token);
                        sprintf(rewrite,"@print_char      ");
                        cword(rewrite+14,i);
                        stack_line(rewrite); goto PrintBlockDone;
                    }
                    if (strcmp(p1,"the")==0)    { p2="DefArt";         i=3; }
                    if (strcmp(p1,"a")==0)      { p2="IndefArt";       i=4; }
                    if (strcmp(p1,"The")==0)    { p2="CDefArt";        i=5; }
                    if (strcmp(p1,"name")==0)   { p2="PrintShortName"; i=6; }
                    if (strcmp(p1,"number")==0) { p2="EnglishNumber";  i=7; }
                    routines_needed[i]=1;
                    if (p2 == NULL)
                    {   word(buff, next_token+1); p2=buff;
                    }
                    next_token += 3;
                    i=expression(next_token);
                    sprintf(rewrite, "%s ( ", p2);
                    cword(rewrite+strlen(rewrite),i);
                    sprintf(rewrite+strlen(rewrite), " )");
                    stack_line(rewrite);
                    goto PrintBlockDone;
                }
            }
            if (strcmp(rewrite+10,"object")==0)
            {   i=expression(++next_token);
                sprintf(rewrite,"@print_obj      ");
                cword(rewrite+12,i);
                stack_line(rewrite); goto PrintBlockDone;
            }
            if (strcmp(rewrite+10,"char")==0)
            {   i=expression(++next_token);
                sprintf(rewrite,"@print_char      ");
                cword(rewrite+13,i);
                stack_line(rewrite); goto PrintBlockDone;
            }
            i=expression(next_token);
            sprintf(rewrite,"@print_num      ");
            cword(rewrite+12,i);
            stack_line(rewrite);
            PrintBlockDone: ;
            if (next_token!=-1)
            {   word(rewrite,next_token++);
                if (rewrite[0]==0) break;
                if (strcmp(rewrite,",")!=0)
                {   error_named("Expected ',' in 'print' list but found",
                        rewrite);
                    break;
                }
            }
        } while (next_token!=-1);

        if (code==PRINT_RET_CODE)
        {   stack_line("@new_line");
            stack_line("@rtrue");
        }     
        break;

    case SPACES_CODE:
        i=expression(2);
        cword(b,i);
        sprintf(rewrite,"@store temp_global %s",b);
        stack_line(rewrite);
        sprintf(rewrite,"@jl temp_global 1 ?_x%d",no_dummy_labels+1);
        stack_line(rewrite);
        sprintf(rewrite,"@._x%d",no_dummy_labels);
        stack_line(rewrite);
        stack_line("@print_char ' '");
        stack_line("@sub temp_global 1 temp_global");
        sprintf(rewrite,"@je temp_global 0 ?~_x%d",no_dummy_labels++);
        stack_line(rewrite);
        sprintf(rewrite,"@._x%d",no_dummy_labels++);
        stack_line(rewrite);
        break;

    case BOX_CODE:
        InV3 goto Advanced_Feature;

        if (btcf[pass_number]==0)
        {   btcf[pass_number]=1;
            stack_line("#Global Box__table --> 16");
        }
        i=0; j=-1;
        next_token=2;
        do { word(b,next_token++); j++;
             if (i<strlen(b)) i=strlen(b);
             if (b[0]!=0)
             {   if (strcmp(b,"\"\"")==0)
                     sprintf(rewrite,"Box__table --> %d = 0",j);
                 else
                     sprintf(rewrite,"Box__table --> %d = %s",j,b);
                 stack_line(rewrite);
             }
        } while (b[0]!=0); i-=2;

        sprintf(rewrite,"@call Box__Routine %d %d Box__table temp_global",j,i);
        stack_line(rewrite);

        call_for_br_flag=1;
        return;

    case PUT_CODE:
        obsolete_warning("use 'array->byte=value' or 'array-->word=value'");
        i=expression(2);
        if (next_token==-1)
        { error("Expected 'byte' or 'word' in 'put'"); return; }
        word(b,next_token++);
        On_("byte") cond="B";
        else On_("word") cond="W";
        else
        { error_named("Expected 'byte' or 'word' in 'put' but found",b);
          return; }
        j=expression(next_token);
        k=expression(next_token);
        cword(b,i);
        sprintf(rewrite, "@store%s %s",cond,b);
        cword(b,j);
        sprintf(rewrite+strlen(rewrite), " %s",b);
        cword(b,k);
        sprintf(rewrite+strlen(rewrite), " %s",b);
        rearrange_stack(i,j,k);
        stack_line(rewrite);
        return;

    case READ_CODE:
        i=expression(2);
        if (next_token==-1)
        { error("Expected a parse buffer for 'read'"); return; }
        j=expression(next_token);
        cword(b,i);
        InV5 { sprintf(rewrite,"@storeb %s 1 0",b); stack_line(rewrite); }

        InV3 { strcpy(rewrite,"@sread"); }
        InV5 { strcpy(rewrite,"@aread "); }
        sprintf(rewrite+strlen(rewrite), " %s",b);
        cword(b,j);
        sprintf(rewrite+strlen(rewrite), " %s",b);
        InV5 { sprintf(rewrite+strlen(rewrite), " temp_global");
             }

        rearrange_stack(i,j,0);

        InV3 { if (next_token!=-1)
                   warning("Ignoring Advanced-game status routine");
             }
        InV5
        {    if (next_token!=-1)
             {   word(b,next_token); sprintf(sub_buffer,"@call_1n %s",b);
                 stack_line(sub_buffer);
             }
             else
             {
               stack_line("@split_window 1");
               stack_line("@set_window 1");
               stack_line("@set_cursor 1 1");
               stack_line("@set_text_style 1");
               stack_line("@loadb 0 33 temp_global2");
               stack_line("spaces temp_global2-1");

               /* This should in an ideal world read "temp_global2", not
                  "...-1".  But because of a bug in the InfoTaskForce
                  interpreter (it scrolls the status line, wrongly) we can't
                  risk this, and accept instead the penalty of a status
                  line which is slightly too short.  The Version 5 edition
                  of Zork I does the same, interestingly. */

               stack_line("@set_cursor 1 2");
               stack_line("@print_obj sys_glob0");

               if (statusline_flag==0)
               {   stack_line("@set_cursor 1 51");
                   stack_line("@print \"Score: \"");
                   stack_line("@print_num sys_glob1");
                   stack_line("@set_cursor 1 64");
                   stack_line("@print \"Moves: \"");
                   stack_line("@print_num sys_glob2");
               }
               else
               {   stack_line("@set_cursor 1 51");
                   stack_line("@print \"Time: \"");
                   stack_line("temp_global = sys_glob1 % 12");
                   stack_line("if ( temp_global < 10 )");
                   stack_line("{");
                   stack_line("@print_char ' '");
                   stack_line("}");
                   stack_line("if ( temp_global == 0 )");
                   stack_line("{");
                   stack_line("temp_global = 12");
                   stack_line("}");
                   stack_line("@print_num temp_global");
                   stack_line("@print_char ':'");
                   stack_line("if ( sys_glob2 < 10 )");
                   stack_line("{");
                   stack_line("@print_char '0'");
                   stack_line("}");
                   stack_line("@print_num sys_glob2");
                   stack_line("if ( (sys_glob1/12) > 0 )");
                   stack_line("{");
                   stack_line("@print \" pm\"");
                   stack_line("}");
                   stack_line("else");
                   stack_line("{");
                   stack_line("@print \" am\"");
                   stack_line("}");
               }
               stack_line("@set_cursor 1 1");
               stack_line("style roman");
               /* stack_line("@new_line"); */
               stack_line("@set_window 0");
             }
        }
        stack_line(rewrite);     
        return;        

    case REMOVE_CODE:      cond="remove_obj"; goto DoExpression;

    case RETURN_CODE:
        word(b,2); if (b[0]==0) { stack_line("@ret#true"); return; }
        cond="ret"; goto DoExpression;

    case RESTORE_CODE:
        word(b,2);
        InV3 { sprintf(rewrite,"@restore %s",b); stack_line(rewrite); }
        InV5
        {   stack_line("@restore temp_global");
            sprintf(rewrite,"@je temp_global 2 %s",b); stack_line(rewrite);
        }    
        return;

    case SAVE_CODE:
        word(b,2);
        InV3 { sprintf(rewrite,"@save %s",b); stack_line(rewrite); }
        InV5
        {   stack_line("@save temp_global");
            sprintf(rewrite,"@je temp_global 0 ~%s",b); stack_line(rewrite);
        }    
        return;

    case STRING_CODE:
        i=expression(2);
        if (next_token==-1) { error("Expected a 'string' value"); return; }
        cword(b,i);
        sprintf(rewrite, "(0-->12)-->%s = ",b);
        word(b,next_token);
        sprintf(rewrite+strlen(rewrite), "%s",b);
        stack_line(rewrite);
        return; 

    case STYLE_CODE:
        if (version_number==3) goto Advanced_Feature;
        word(b,2);
        On_("roman")     { stack_line("@set_text_style 0"); return; }
        On_("reverse")   { stack_line("@set_text_style 1"); return; }
        On_("bold")      { stack_line("@set_text_style 2"); return; }
        On_("underline") { stack_line("@set_text_style 4"); return; }
        error_named("The style can be \"roman\", \"bold\", \"underline\" \
or \"reverse\" but not",b);
        return;

    case WRITE_CODE:
        obsolete_warning("write properties using the '.' operator");
        i=expression(2);
        if (next_token==-1)
        { error("Expected some properties to 'write'"); return; }
        if (i==-1)
        { error("The object to 'write' must be a variable or constant");
          return; }
        do
        {   if (next_token!=-1)
            {   j=expression(next_token);
                if (next_token==-1)
                { error("Expected property value to 'write'"); return; }
                k=expression(next_token);
                cword(b,i);
                sprintf(rewrite, "@put_prop %s",b);
                cword(b,j);
                sprintf(rewrite+strlen(rewrite), " %s",b);
                cword(b,k);
                sprintf(rewrite+strlen(rewrite), " %s",b);
                rearrange_stack(j,k,0);
                stack_line(rewrite);
            }
        } while (next_token!=-1);
        return;

    default: error("Internal error - unknown compiler code");
  }
    return;

    Advanced_Feature:
        warning("Ignoring this Advanced-game command"); return;

    DoExpression:
        i=expression(2);
        if (i>=1) word(b,i); else strcpy(b,"sp");
        sprintf(rewrite,"@%s %s",cond, b);
        stack_line(rewrite);
        if (next_token!=-1) error("Spurious terms after expression");
        return;

    Translation:
        next_block_type=IF_TYPE;
        if (trans==2)
        {   sprintf(rewrite,"@._w%d",no_blocks_made);
            next_block_type=WHILE_TYPE;
            stack_line(rewrite);
        }

        if (trans!=4)
        {   dir=1; labnum=no_blocks_made; }
        else
        {   dir=0; labnum=brace_stack[brace_sp]; }
        sprintf(condition_label,"_%s%d", (dir==1)?"f":"s",labnum);

        word(b,2);
        if ((b[0]!='(') && (code==IF_CODE))
            obsolete_warning("'if' conditions should be bracketed");
        condition_context=1; expression(2); condition_context=0;
        if (next_token==-1) return;
        error("Braces '{' are compulsory unless the condition is bracketed");
        return;
}

/* ------------------------------------------------------------------------- */
/*   Line parser: decides whether to send line to compiler or assembler      */
/*                                                                           */
/*   (The eternal lament: This used to be such a _simple_ routine...)        */
/* ------------------------------------------------------------------------- */

static int ignoring_cbrule = 0;
static int outer_mode;

static void parse_line(void)
{   int i, j, k, f, to_f, angle_depth, one_word_flag=0; char *p;
    int action_one, action_two, action_three;
    int32 offset, expect=0;

#ifdef USE_TEMPORARY_FILES
    offset=(subtract_pointers(utf_zcode_p,zcode));
#else
    offset=(subtract_pointers(zcode_p,zcode));
#endif

    if ((ignoring_mode!=0) || (ignoring_routine!=0)) goto ignoringcode;
    if (doing_table_mode==1)
    {   assemble_table(parse_buffer,1); return; }

    if ((resobj_flag==2)||(resobj_flag==3))
    {   word(parse_buffer,1);
        if ((parse_buffer[0]!='@')&&(parse_buffer[0]!=']')
            &&(parse_buffer[0]!='}'))
        {   if (resobj_flag==2)
            {   resobj_flag=0; resume_object(parse_buffer,1); return; }
            else
            {   resobj_flag=0; finish_object(); }
        }
    }

    word(parse_buffer,1); if (parse_buffer[0]==0) return;

    expected_until=0;
    if ((until_expected==1)&&(parse_buffer[0]!='@')&&(parse_buffer[0]!='.'))
    {   until_expected=0; expected_until=1;
        i=prim_find_symbol(parse_buffer,6);
        if ((i<0) || (svals[i]!=UNTIL_CODE))
            error("'do' without matching 'until'");
    }

    if ((outer_mode==1)&&(parse_buffer[0]!='@'))
    {   if (parse_buffer[0]=='[') outer_mode=0;
        if (parse_buffer[0]=='#') i=prim_find_symbol(parse_buffer+1,6);
        else i=prim_find_symbol(parse_buffer,6);
        if ((i<0) || (stypes[i]!=14))
        {   if (i<0)
                error_named("Unknown directive:",parse_buffer);
            else
            switch(stypes[i])
            {   case 15: case 16:
                    error_named("Expected directive or '[' but found statement",
                        parse_buffer); break;
                case 17:
                    error_named("Expected directive or '[' but found opcode",
                        parse_buffer); break;
                default:
                error_named("Expected directive or '[' but found",parse_buffer);
            }
        }
        else
            assemble_directive(parse_buffer,offset,svals[i]);
        return;
    }

    word(sub_buffer,2);
    switch(sub_buffer[0])
    {   case 0:   one_word_flag=1; break;
        case '=': compiler(parse_buffer,ASSIGNMENT_CODE); return;
        case ':':
        case ',': goto switchitem;
        case 't': if ((sub_buffer[1]=='o')&&(sub_buffer[2]==0))
                      goto switchitem;
    }

    if (parse_buffer[0]=='\"') goto impliedprintret;

    if (parse_buffer[1]==0)
    {   switch(parse_buffer[0])
        {   case '<': goto causeaction;
            case ']':
                if ((switch_stack[0]==-1) && (switch_sp==1))
                {   if (switch_stage[--switch_sp]==1)
                    {   if (ignoring_cbrule==1) ignoring_cbrule=0;
                        else { stack_line("}"); stack_line("]"); return; }
                    }
                }
                outer_mode=1;
                break;
            case '.':
                word(parse_buffer,2); assemble_label(offset,parse_buffer);
                return;
            case '@':
                word(parse_buffer,2);
                if ((parse_buffer[0]=='.')&&(parse_buffer[1]==0))
                {   word(parse_buffer,3); assemble_label(offset,parse_buffer);
                    return;
                }
                break;
            case '{': compile_openbrace(); return;
            case '}':
                if (ignoring_cbrule==1) ignoring_cbrule=0;
                else
                if ((brace_sp>1)&&(brace_type_stack[brace_sp-2]==SWITCH_TYPE)
                                &&(brace_type_stack[brace_sp-1]==IF_TYPE))
                {   if (switch_stage[switch_sp-1]==1)
                    {   stack_line("}"); switch_stage[switch_sp-1]=0;
                    }
                }
                compile_closebrace(parse_buffer); return;
        }
    }

    if (parse_buffer[0]=='#') expect=1;
    if (parse_buffer[0]=='@') expect=2;
    if (expect==0) i=prim_find_symbol(parse_buffer,6);
    else i=prim_find_symbol(parse_buffer+1,6);

    if ((expect==1) && ((i==-1)||(stypes[i]!=14)))
    {   error_named("Unknown # directive",parse_buffer); return;
    }
    if ((expect==2) && ((i==-1)||( (stypes[i]!=16) && (stypes[i]!=17) )))
    {   error_named("Unknown assembly opcode",parse_buffer); return;
    }
    if (i==-1)
    {   if (parse_buffer[0]=='(') goto ItsAnExp;
        word(parse_buffer,2);
        if (strcmp(parse_buffer,"(")==0)
        { compiler(parse_buffer,FUNCTION_CODE); return; }
        word(parse_buffer,1);

        ItsAnExp:
        compiler(parse_buffer,ASSIGNMENT_CODE); return;
    }

    j=stypes[i];

    switch(j)
    {   case 17:
            k=svals[i];
            if ((expect!=2)&&(k!=57)&&(k!=58)&&(k!=66)&&(k!=67))
            obsolete_warning("assembly-language lines should begin with '@'");
            assemble_opcode(parse_buffer,offset,k);
            return;
        case 14:
            k=svals[i];
            if ((expect==0)&&(k!=OPENBLOCK_CODE)&&(k!=CLOSEBLOCK_CODE))
          obsolete_warning("directives inside routines should begin with '#'");
            assemble_directive(parse_buffer,offset,k); return;
        case 16:                /* For compiler-modified opcodes */
            if (expect==2)
            {   assemble_opcode(parse_buffer,offset,(svals[i])%100); return;
            }
            if ((svals[i]/100==RESTORE_CODE) || (svals[i]/100==SAVE_CODE))
            {   compiler(parse_buffer,(svals[i])/100); return; }
            if (((svals[i]/100)==PRINT_CODE)||((svals[i]/100)==PRINT_RET_CODE))
            {   word(sub_buffer,2);
                if (sub_buffer[0]!='\"')
                {   compiler(parse_buffer,(svals[i])/100); return; }
            }
            word(sub_buffer,3);
            if (sub_buffer[0]==0)
            {   assemble_opcode(parse_buffer,offset,(svals[i])%100); return;
            }
            compiler(parse_buffer,(svals[i])/100); return;
        default:
            compiler(parse_buffer,svals[i]); return;
    }

  impliedprintret:
    if (one_word_flag==1)
    {   a_from_one=1;
        assemble_opcode(parse_buffer,offset,60);
        a_from_one=0;
        return;
    }
    pr_implied_flag=1;
    compiler(parse_buffer,PRINT_RET_CODE);
    pr_implied_flag=0;
    return;


  causeaction:
    word(parse_buffer,2); routines_needed[1]=1;
    action_one=0;
    action_two=0;
    action_three=0;
    angle_depth=1; i=3;
    if ((parse_buffer[0]=='<') && (parse_buffer[1]==0))
    {   word(parse_buffer,i++); angle_depth=2; }

    if (parse_buffer[0]=='(')
    {   k=expression(i-1); i=next_token; action_one=k;
        if (next_token==-1) goto missingangle;
        cword(parse_buffer,k);
        sprintf(rewrite2,"R_Process ( %s ",parse_buffer);
    }
    else
        sprintf(rewrite2,"R_Process ( ##%s ",parse_buffer);

    j=1;
    do
    {   word(parse_buffer,i++);
        if (parse_buffer[0]==0) goto missingangle;
        if ((parse_buffer[0]=='>') && (parse_buffer[1]==0)) break;
        k=expression(i-1); i=next_token;
        if (j==2) action_two=k; else action_three=k;
        if (next_token==-1) goto missingangle;
        cword(parse_buffer,k);
        sprintf(rewrite2+strlen(rewrite2),
            ", %s ",parse_buffer); j++;
    } while (j<=3);

    if (j>3)
    {   error("The longest form of action command is '<Action noun second>'");
        return;
    }
    sprintf(rewrite2+strlen(rewrite2)," )");
/* printf("Stacking '%s'\n", rewrite2); */
    rearrange_stack(action_one, action_two, action_three);
    stack_line(rewrite2);
    if (angle_depth==2)
    {   word(parse_buffer,i++);
        if (!((parse_buffer[0]=='>') && (parse_buffer[1]==0)))
            error("Angle brackets '>' do not match");
        stack_line("@rtrue");
    }
    return;
  missingangle:
    error("Missing '>' or '>>'");
    return;


  switchitem:
    f=0;
    if (switch_sp>0)
    {   f=1; if (switch_stack[switch_sp-1]==-1) f=0;
    }

    i=1; j=1;
    if (switch_sp==0)
    {   switch_sp++; switch_stack[0]=-1; switch_stage[0]=0; }
    if (switch_stage[switch_sp-1]==1)
    {   switch_stage[switch_sp-1]=0;
        if (return_flag==0) stack_line((f==1)?"break":"@rfalse");
        stack_line("}"); ignoring_cbrule=1;
    }

    if ((parse_buffer[0]==']') && (parse_buffer[1]==0))
    {   if (f==1) { error("Misplaced ']'"); return; }
        word(parse_buffer,2);
        if ((parse_buffer[0]==',') && (parse_buffer[1]==0))
        {   if (resobj_flag==0)
                error("Comma after ']' illegal for global routines");
            else
            {   stack_line("]");
                resobj_flag=2;
                buffer[0]=0; i=3;
                do
                {   word(sub_buffer,i++);
                    sprintf(buffer+strlen(buffer), "%s ",sub_buffer);
                } while (sub_buffer[0]!=0);
                stack_line(buffer); return;
            }
        }
        if (resobj_flag>0)
        error_named("Expected nothing or a comma after ']' but found",
            parse_buffer);
        else
        error_named("Expected nothing after ']' but found",
            parse_buffer);
        resobj_flag=0;
        return;
    }

    if (strcmp(parse_buffer,"default")==0)
    {   if (switch_stage[switch_sp-1]==2)
        {   if (f==1)
                error("Two 'default' clauses in 'switch'");
            else
                error("Two 'default' rules given");
        }
        switch_stage[switch_sp-1]=2;
        return;
    }
    if (switch_stage[switch_sp-1]==2)
    {   if (f==1)
            error("A 'default' clause must come last");
        else
            error("A 'default' rule must come last");
        switch_stage[switch_sp-1]=0;
    }    

    sprintf(sub_buffer, "if ( "); j=0;
    do
    {   word(parse_buffer,i+1);
        if (strcmp(parse_buffer,"to")==0) to_f=1; else to_f=0;
        word(parse_buffer,i);
        if ((parse_buffer[0]==']')&&(i!=1))
        {   error("Misplaced ']'"); return; }
        if ((parse_buffer[0]=='\"') && (strlen(parse_buffer)>60))
        {   TooLongError: 
            if (f==0)
            error("Action name too long or a string: perhaps a statement \
accidentally ended with a comma?");
            else
            error("Switch value too long or a string: perhaps a statement \
accidentally ended with a comma?");
            return;
        }

        if (j>=1)
        {   if ((j==3) || (to_f==1))
            {   sprintf(sub_buffer+strlen(sub_buffer), "|| ");
                j=0;
            }
            else
                sprintf(sub_buffer+strlen(sub_buffer), "or ");
        }

        if ((j==0) && (to_f==0))
            sprintf(sub_buffer+strlen(sub_buffer),
                (f==0)?"sw__var == ":"temp_global == ", parse_buffer);

        if (to_f==1)
            sprintf(sub_buffer+strlen(sub_buffer),
                (f==0)?" ( sw__var >= ":" ( temp_global >= ");

        sprintf(sub_buffer+strlen(sub_buffer),
            (f==0)?"##%s ":"%s ",parse_buffer);
        if (to_f==1)
        {   i=i+2;
            word(parse_buffer,i);
            if ((parse_buffer[0]==']')&&(i!=1)) goto TooLongError;
            sprintf(sub_buffer+strlen(sub_buffer),
                (f==0)?" && sw__var <= ##%s ) ":" && temp_global <= %s ) ",
                parse_buffer);
            j=3;
        }
        else j=j+1;

        word(parse_buffer,i+1); i+=2;
    } while (parse_buffer[0]==',');
    sprintf(sub_buffer+strlen(sub_buffer),")");
    stack_line(sub_buffer); stack_line("{");
    switch_stage[switch_sp-1]=1;
    if (parse_buffer[0]!=':')
        error_named("Expected ':' but found",parse_buffer);
    sub_buffer[0]=0;
    do
    {   word(parse_buffer,i++);
        sprintf(sub_buffer+strlen(sub_buffer),"%s ",parse_buffer);
    } while (parse_buffer[0]!=0);
    stack_line(sub_buffer);
    return;


  ignoringcode:

    if (ignoring_routine==1)
    {   word(parse_buffer,1);
        if (strcmp(parse_buffer,"]")==0) ignoring_routine=0;
        return;
    }

    word(parse_buffer,1); if (parse_buffer[0]==0) return;
    make_upper_case(parse_buffer);
    p=parse_buffer; if (p[0]=='#') p++;

    if (    (strcmp(p,"END")==0)
         || (strcmp(p,"IFV3")==0)
         || (strcmp(p,"IFV5")==0)
         || (strcmp(p,"IFDEF")==0)
         || (strcmp(p,"IFNDEF")==0)
         || (strcmp(p,"ENDIF")==0)
         || (strcmp(p,"IFNOT")==0))
    {   i=prim_find_symbol(p,6);
        assemble_directive(p,offset,svals[i]);
    }
    return;
}

/* ------------------------------------------------------------------------- */
/*   Initialisation and main                                                 */
/* ------------------------------------------------------------------------- */

#define MAX_TOKENS_SPACE 5*BUFFER_LENGTH

static void initialise(void)
{   abbreviations_at=my_malloc(MAX_ABBREVS*MAX_ABBREV_LENGTH,"abbreviations");

#ifdef PC_QUICKC
    if (memout_mode==1)
        printf("Allocation %ld bytes for zcode\n", (long) MAX_ZCODE_SIZE);
    zcode           = halloc((long) MAX_ZCODE_SIZE,1);
    malloced_bytes += MAX_ZCODE_SIZE;
    if (zcode==NULL) fatalerror("Can't hallocate memory for zcode.  Bother.");
#else
    zcode           =my_malloc(MAX_ZCODE_SIZE,"zcode");
#endif

    dictionary      =my_malloc(9*MAX_DICT_ENTRIES+7,"dictionary");
    strings         =my_malloc(MAX_STATIC_STRINGS,"static strings");
    low_strings     =my_malloc(MAX_LOW_STRINGS,"low (synonym) strings");
    tokens          =my_malloc(MAX_TOKENS_SPACE,"tokens");
    properties_table=my_malloc(MAX_PROP_TABLE_SIZE,"properties table");

    zcode_p   = zcode;
    dict_p    = dictionary;
    strings_p = strings;
    low_strings_p = low_strings;

    symbols_p=NULL; symbols_top=symbols_p;

    stack_create();

    dict_p=dictionary+7; dict_entries=0;
    init_symbol_banks(); no_symbols=0;

    stockup_symbols();
    make_s_grid();
    make_lookup();

}

static void print_help(void)
{
    printf(RELEASE_STRING); printf("\n");
#ifdef ALLOCATE_BIG_ARRAYS
    printf("(allocating memory for arrays) ");
#endif
#ifdef PROMPT_INPUT
    printf("(prompting input) ");
#endif
#ifdef USE_TEMPORARY_FILES
    printf("(temporary files) ");
#endif
  printf(
"\n\nThis program is a compiler to Infocom format adventure games.\n\
It is copyright (C) Graham Nelson, 1993, 1994, 1995.\n\n");
#ifndef PROMPT_INPUT
  printf(
"Syntax: \"inform [-list] [+directory] [$memcom ...] <file1> [<file2>]\"\n\n\
<file1> is the name of the Inform source file; Inform translates this into\n\
   \"");
printf(Source_Prefix); printf("<file1>"); printf(Source_Extension);
printf("\"\n\
(unless <file1> contains a '.' or '/', in which case it is left alone).\n\
<file2> may optionally be given as the name of the story file to make.\n\
If it isn't given, Inform writes to\n\
   \"");
printf(Code_Prefix); printf("<file1>"); printf(Code_Extension);
printf("\"\n");

if ((strcmp(V5Code_Prefix,Code_Prefix)==0)
    && (strcmp(V5Code_Extension,Code_Extension)==0))
    printf("(for both version-3 and version-5 files)\n");
else
{   printf("or, for version-5 files,\n\
   \"");
    printf(V5Code_Prefix); printf("<file1>"); printf(V5Code_Extension);
    printf("\"\n");
}

printf("but if it is, then Inform takes <file2> as the full filename.\n\n");
#endif
   printf("\
-list is an optional list of switch letters following the initial hyphen:\n\
  a   list assembly-level instructions compiled\n\
  b   give statistics and/or line/object list in both passes\n\
  c   more concise error messages\n\
  d   contract double spaces after full stops in text\n\
  e   economy mode (slower): make use of declared abbreviations\n");

  printf("  E0  Archimedes-style error messages%s\n",
      (error_format==0)?" (current setting)":"");
  printf("  E1  Microsoft-style error messages%s\n",
      (error_format==1)?" (current setting)":"");

   printf("\
  f   frequencies mode: show how useful abbreviations are\n\
  g   with debugging code: traces all function calls\n\
  h   print this information\n");

   printf("\
  i   ignore default switches set within the file\n\
  j   list objects as constructed\n\
  k   output Infix debugging information to \"%s\"\n\
  l   list all assembly lines\n\
  m   say how much memory has been allocated\n\
  n   print numbers of properties, attributes and actions\n\
  o   print offset addresses\n\
  p   give percentage breakdown of story file\n\
  q   keep quiet about obsolete usages\n\
  r   record all the text to \"%s\"\n\
  s   give statistics\n\
  t   trace Z-code assembly\n", Debugging_Name, Transcript_Name);

   printf("\
  u   work out most useful abbreviations\n\
  v3  compile to version-3 (Standard) story file\n\
  v4  compile to version-4 (Plus) story file\n\
  v5  compile to version-5 (Advanced) story file\n\
  v6  compile to version-6 (graphical) story file\n\
  v7  compile to version-7 (*) story file\n\
  v8  compile to version-8 (*) story file\n\
      (*) formats for very large games, requiring\n\
          slightly modified game interpreters to play\n\
  w   disable warning messages\n\
  x   print # for every 100 lines compiled (in both passes)\n\
  z   print memory map of the Z-machine\n");
#ifdef ARC_THROWBACK
printf("  T   enable throwback of errors in the DDE\n");
#endif

printf("\n\
+directory, if given, is the source of Include files.\n");

printf("\n\
$memcom can be one or more memory allocation commands:\n\
  $list             list current memory allocation settings\n\
  $huge             make the standard \"huge game\" settings %s\n\
  $large            make the standard \"large game\" settings %s\n\
  $small            make the standard \"small game\" settings %s\n\
  $?SETTING         explain briefly what SETTING is for\n\
  $SETTING=number   change SETTING to given number\n\n",
    (DEFAULT_MEMORY_SIZE==HUGE_SIZE)?"(default)":"",
    (DEFAULT_MEMORY_SIZE==LARGE_SIZE)?"(default)":"",
    (DEFAULT_MEMORY_SIZE==SMALL_SIZE)?"(default)":"");

#ifdef ARCHIMEDES
    printf("\
  $temp_files=<str> make Inform put its temporary storage files\n\
                    at <str> (default for <str> is ram: (RAM disc))\n\
                    (e.g. $temp_files=$. to use root of current disc)\n\n");
#endif

#ifndef PROMPT_INPUT
    printf("For example: \"inform -dex $large curses curses_5\".\n");
#endif
}

extern void switches(char *p, int cmode)
{   int i, s=1;
    if (cmode==1)
    {   if (p[0]!='-')
        {   printf(
                "Ignoring second word which should be a -list of switches.\n");
            return;
        }
    }
    for (i=cmode; p[i]!=0; i+=s, s=1)
    {   switch(p[i])
        {
        case 'a': listing_mode=2; break;
        case 'b': statistics_mode=1; bothpasses_mode=1; break;
        case 'c': concise_mode=1; break;
        case 'd': double_spaced=1; break;
        case 'e': economy_mode=1; break;
        case 'f': frequencies_mode=1; break;
        case 'g': withdebug_mode=1; break;
        case 'h': print_help(); break;
        case 'i': ignoreswitches_mode=1; break;
        case 'j': listobjects_mode=1; break;
        case 'k': debugging_file=1; break;
        case 'l': listing_mode=1; break;
        case 'm': memout_mode=1; break;
        case 'n': printprops_mode=1; break;
        case 'o': offsets_mode=1; break;
        case 'p': percentages_mode=1; break;
        case 'q': obsolete_mode=1; break;
        case 'r': transcript_mode=1; break;
        case 's': statistics_mode=1; break;
        case 't': tracing_mode=1; break;
        case 'u': optimise_mode=1; break;
        case 'v': switch(p[i+1])
                  {   case '3': version_number=3;
                                actual_version=3; scale_factor=2;
                                override_version=1; s=2; break;
                      case '4': actual_version=4;
                                version_number=5; scale_factor=4;
                                override_version=1; s=2;
                                break;
                      case '5': actual_version=5;
                                version_number=5; scale_factor=4;
                                override_version=1; s=2;
                                break;
                      case '6': actual_version=6;
                                version_number=5; scale_factor=8;
                                override_version=1; s=2;
                                break;
                      case '7': actual_version=7;
                                version_number=5; scale_factor=4;
                                override_version=1; s=2; extend_memory_map=1;
                                break;
                      case '8': actual_version=8;
                                version_number=5; scale_factor=8;
                                override_version=1; s=2;
                                break;
                      default:  printf("-v must be followed by 3 to 8\n");
                                break;
                  }
                  break;
        case 'w': nowarnings_mode=1; break;
        case 'x': hash_mode=1; break;
        case 'z': memory_map_mode=1; break;

        case 'E': switch(p[i+1])
                  {   case '0': s=2; error_format=0; break;
                      case '1': s=2; error_format=1; break;
                      default:  error_format=1; break;
                  }
                  break;
#ifdef ARC_THROWBACK
        case 'T': throwbackflag = 1; break;
#endif
        default:
          printf("Switch \"-%c\" unknown (try \"inform -h\" for help)\n",p[i]);
          break;
        }
    }

    if (((optimise_mode==1)||(transcript_mode==1))&&(store_the_text==0))
    {   store_the_text=1;
#ifdef PC_QUICKC
        if (memout_mode==1)
            printf("Allocation %ld bytes for transcription text\n",
                (long) MAX_TRANSCRIPT_SIZE);
        all_text = halloc(MAX_TRANSCRIPT_SIZE,1);
        malloced_bytes += MAX_TRANSCRIPT_SIZE;
        if (all_text==NULL)
         fatalerror("Can't hallocate memory for transcription text.  Darn.");
#else
        all_text=my_malloc(MAX_TRANSCRIPT_SIZE,"transcription text");
#endif
    }
}

static void set_include_path(char *p)
{   include_path_set=1;
    if (strlen(p)>=128) fatalerror("Include path too long");
    strcpy(include_path,p+1);
}

static void banner(void)
{
#ifdef MACHINE_STRING
    printf(MACHINE_STRING); printf(" ");
#endif
    printf("Inform 5.5 (v%d/",VNUMBER);
#ifdef ALLOCATE_BIG_ARRAYS
    printf("a");
#endif
#ifdef PROMPT_INPUT
    printf("p");
#endif
#ifdef USE_TEMPORARY_FILES
    printf("t");
#endif
#ifdef TIME_UNAVAILABLE
    printf("u");
#endif
    printf(")\n");
}

static int32 last_mapped_line = -1;

#ifndef MAC_FACE
#ifdef MAC_MPW
int main(int argc, char **argv, char *envp[])
#else
int main(int argc, char **argv)
#endif
{   char *story_name="source", *code_name="output";
    int t1, t2, i, cu_flag;
#ifdef PROMPT_INPUT
    char buffer1[100], buffer2[100], buffer3[100];
#endif
#endif
#ifdef MAC_FACE
int inform_main(char *story_name, char *code_name, char *trans_name,
                char *infix_name)
{   int32 t1, t2, i, cu_flag;

    if ((trans_name == NULL) || (*trans_name == '\0'))
    {   Transcript_Name = malloc(10);
        strcpy(Transcript_Name, "game.txt");
    }
    else Transcript_Name = trans_name;

    if ((infix_name == NULL) || (*infix_name == '\0'))
    {   Debugging_Name = malloc(12);
        strcpy(Debugging_Name,"game.debug");
    }
    else Debugging_Name = infix_name;
#endif

    t1=time(0);
    process_filename_flag=0;
    version_number = 5; actual_version = 5; scale_factor=4;
    banner();

#ifndef MAC_FACE
    set_memory_sizes(DEFAULT_MEMORY_SIZE);

#ifdef ARCHIMEDES
    strcpy(tname_pre, "ram:");
#endif

#ifdef PROMPT_INPUT
    i=0;
    printf("Source filename?\n> ");
    while (gets(buffer1)==NULL); story_name=buffer1;
    printf("Output filename (RETURN for the same)?\n> ");
    while (gets(buffer2)==NULL); code_name=buffer2;
    if (buffer2[0]!=0) process_filename_flag=1;
    do
    {   printf("List of switches (RETURN to finish; \"h\" for help)?\n> ");
        while (gets(buffer3)==NULL); switches(buffer3,0);
    } while (buffer3[0]!=0);
#else
    if (argc==1) { switches("-h",1); return(0); }
    include_path_set=0;
    for (i=1; i<argc; i++)
    {   if ((*(argv[i]))=='+')
            set_include_path(argv[i]);
        if ((*(argv[i]))=='-')
            switches(argv[i],1);
        if ((*(argv[i]))=='$')
            memory_command(argv[i]+1);
    }
    for (i=1; ((i<argc) && (((*(argv[i]))=='-') || ((*(argv[i]))=='+')
                         || ((*(argv[i]))=='$'))); i++) ;

    if (i!=argc)
    {   story_name=argv[i++];

        while (i<argc && (((*(argv[i]))=='-') || ((*(argv[i]))=='+')
                                              || ((*(argv[i]))=='$'))) i++;

        if (argc==i) process_filename_flag=0;
        else { process_filename_flag=1; code_name=argv[i++]; }

        while (i<argc && (((*(argv[i]))=='-') || ((*(argv[i]))=='+')
                                              || ((*(argv[i]))=='$'))) i++;

        if (argc>i+1) printf("Unknown command line parameter: %s\n", argv[i]);
    }
    else
    {   printf("[No input file named.]\n"); return(0);
    }

#endif
#endif

    init_inform_vars();

#ifdef MAC_FACE
    if (((optimise_mode==1)||(transcript_mode==1))&&(store_the_text==0))
    {   store_the_text=1;
        all_text=my_malloc(MAX_TRANSCRIPT_SIZE,"transcription text");
    }
    process_filename_flag = 1;
#endif

#ifdef MAC_MPW
    InitCursorCtl((acurHandle)NULL);
    Show_Cursor(WATCH_CURSOR);
#endif

    allocate_the_arrays();

    if (process_filename_flag==0) strcpy(Code_Name,story_name);
    else strcpy(Code_Name,code_name);

    initialise();

    if (debugging_file==1)
    {   open_debug_file(); pass_number=1; debug_pass=1;
        write_debug_byte(5); write_debug_byte(4);
        write_debug_byte(VNUMBER/256); write_debug_byte(VNUMBER%256);
        last_mapped_line=0;
    }

    for (pass_number=1; pass_number<=2; pass_number++)
    {   input_file = 0; ignoring_mode=0; ignoring_routine=0;
        outer_mode = 1; until_expected = 0;
        total_files_read = 0; total_sl_count = 0;
        load_sourcefile(story_name,0);
        begin_pass(); keep_debug_linenum();
        cu_flag=0;
        compilemode:
        do
        {   process_next_line();
            if ((debugging_file==1) && (origin_of_line==0))
            {   keep_chars_read();
                make_debug_linenum();
                if (pass_number==2)
                {   debug_pass=2;
                    if (total_sl_count/16!=last_mapped_line)
                    {   last_mapped_line=total_sl_count/16;
                        write_debug_byte(15);
                        write_chars_read();
                    }
                    debug_pass=1;
                }
            }
            parse_line();
        } while (endofpass_flag==0);
        if (call_for_br_flag==1)
        {   compile_box_routine();
            call_for_br_flag=0; endofpass_flag=0; goto compilemode;
        }
        if (cu_flag==0)
        {   cu_flag=1;
            if (compile_unspecifieds()==1)
            {   endofpass_flag=0; goto compilemode; }
        }
        if (pass_number==2) check_globals();
        close_all_source();
        if (hash_mode==1) printf("\n");
#ifdef USE_TEMPORARY_FILES
        zcode_p=utf_zcode_p;
        if (pass_number==2) check_temp_files();
#endif
        construct_storyfile();
    }

    symbols_free_arrays();
    express_free_arrays();

    if (no_errors==0) output_file();

    if (debugging_file==1) close_debug_file();

#ifdef USE_TEMPORARY_FILES
    if (no_errors>0) remove_temp_files();
#endif

    if (memout_mode==1)
    {
#ifdef ARCHIMEDES
        printf("Static strings table used %d\n",
            subtract_pointers(strings_p,strings));
        printf("Output buffer used %d\n", Write_Code_At);
        printf("Code area table used %d\n",
            subtract_pointers(zcode_p,zcode));
#else
        printf("Static strings table used %ld\n",
            (long int) subtract_pointers(strings_p,strings));
        printf("Output buffer used %ld\n", (long int) Write_Code_At);
        printf("Code area table used %ld\n",
            (long int) subtract_pointers(zcode_p,zcode));
#endif
        printf("Properties table used %d\n",     properties_size);
#ifdef USE_TEMPORARY_FILES
  printf("(NB: strings and code area can safely be larger than allocation)\n");
#endif
        printf("Allocated a total of %ld bytes of memory\n",
            (long int) malloced_bytes); }
    if ((no_errors+no_warnings)!=0)
        printf("Compiled with %d error%s and %d warning%s%s\n",
            no_errors,(no_errors==1)?"":"s",
            no_warnings,(no_warnings==1)?"":"s",
            (no_errors>0)?" (no output)":"");

    free_remaining_arrays();

    if (optimise_mode==1)
    {   optimise_abbreviations();
    }
    if (store_the_text == 1)
        my_free(&all_text,"transcription text");

    t2=time(0)-t1;
    if (statistics_mode==1)
        printf("Completed in %ld seconds.\n", (long int) t2);
    if (no_errors!=0) return(1);

#ifdef ARC_PROFILING
    _fmapstore("ram:profile");
#endif

#ifdef ARC_THROWBACK
    throwback_end();
#endif

    return(0);
}

extern void init_inform_vars(void)
{   int i;

    init_asm_vars();
    init_express_vars();
    init_files_vars();
    init_inputs_vars();
    init_symbols_vars();
    init_tables_vars();
    init_zcode_vars();

    switch_sp = 0;
    ignoring_cbrule = 0;
    rewrite2 = NULL;

    ignoring_mode = 0;
    no_symbols = 0;
    no_errors = 0;
    no_warnings = 0;
    pass_number = 0;
    endofpass_flag = 0;
    abbrev_mode = 1;
    store_the_text = 0;
    dict_entries=0;

    code_offset = 0x800;
    actions_offset = 0x800;
    preactions_offset = 0x800;
    dictionary_offset = 0x800;
    adjectives_offset = 0x800;
    variables_offset = 0;
    strings_offset = 0xc00;

    newfordepth = 0;
    call_for_br_flag = 0;
    pr_implied_flag = 0;
    last_mapped_line = -1;

    for (i=0; i<3; i++) { brcf[i] = 0; btcf[i] = 0; }

    abbreviations_at = NULL;
    dictionary = NULL;
    strings = NULL;
    low_strings = NULL;
    tokens = NULL;
    properties_table = NULL;
    zcode = NULL;
#ifdef ALLOCATE_BIG_ARRAYS
    abbrev_values = NULL;
    abbrev_quality = NULL;
    abbrev_freqs = NULL;
    buffer = NULL;
    sub_buffer = NULL;
    parse_buffer = NULL;
    rewrite = NULL;
#endif

    for (i=0; i<MAX_FOR_DEPTH; i++)
        forcloses[i] = NULL;

    for (i=0;i<8;i++) routines_needed[i]=0;

}

/* ------------------------------------------------------------------------- */
/*   End of code                                                             */
/* ------------------------------------------------------------------------- */
