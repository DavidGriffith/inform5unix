/* ------------------------------------------------------------------------- */
/*   "files" : File handling, memory, command-line memory settings,          */
/*             fatal errors (and error throwback on Acorn Archimedes)        */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include "header.h"

int override_error_line=0;
int32 malloced_bytes=0;
Sourcefile InputFiles[MAX_INCLUSION_DEPTH];
int input_file;
int total_files_read;
int total_sl_count;
int include_path_set;

char Source_Name[100], Code_Name[100], include_path[130];
static char home_directory[128];

/* ------------------------------------------------------------------------- */
/*   NB: Arguably temporary files should be made using "tmpfile" in          */
/*   the ANSI C libraries, but we do it by hand since tmpfile is unusual.    */
/* ------------------------------------------------------------------------- */

#ifdef USE_TEMPORARY_FILES
    FILE *Temp1_fp=NULL, *Temp2_fp=NULL;

#ifdef ARCHIMEDES
static char tname_b[128];
char tname_pre[128];
static char *temporary_name(int i)
{   sprintf(tname_b, "%sInfTemp%d", tname_pre, i);
    return(tname_b);
}
#endif

#endif

/* ------------------------------------------------------------------------- */
/*  Line numbering, fatal errors                                             */
/* ------------------------------------------------------------------------- */

extern int current_source_line(void)
{   if (input_file==0) return -1;
    return(InputFiles[input_file-1].source_line);
}

extern void advance_line(void)
{   InputFiles[input_file-1].source_line++;
    InputFiles[input_file-1].line_start
        = InputFiles[input_file-1].chars_read;
    total_sl_count++;
}

extern void declare_systemfile(void)
{   InputFiles[input_file-1].sys_flag=1;
}

extern int is_systemfile(void)
{   return InputFiles[input_file-1].sys_flag;
}

extern void print_error_line(void)
{   int j, flag=0; char *p;
    int i=override_error_line;
    p=InputFiles[input_file-1].filename;
    if (i==0) i=forerrors_line;
    else override_error_line=0;

    if (error_format==0)
    {   if (input_file>1) printf("\"%s\", ", p);
        printf("line %d: ", i);
    }
    else
    {   for (j=0; p[j]!=0; j++)
            if ((p[j]=='/') || (p[j]=='.')) flag=1;
        printf("%s", p);
        if (flag==0) printf("%s",Source_Extension);
        printf("(%d): ", i);
    }
}

extern void fatalerror(char *s)
{   print_error_line();
    printf("Fatal error: %s\n",s);
#ifdef ARC_THROWBACK
    throwback(0, s);
    throwback_end();
#endif
#ifdef MAC_FACE
    asm_free_arrays();
    express_free_arrays();
    inputs_free_arrays();
    symbols_free_arrays();
    tables_free_arrays();
    zcode_free_arrays();
    free_remaining_arrays();
    my_free(&all_text,"transcription text");
    longjmp(mac_env,1);
#endif
    exit(1);
}

static void couldntopen(char *m, char *fn)
{   char err_buffer[128];
    sprintf(err_buffer, "%s \"%s\"", m, fn);
    fatalerror(err_buffer);
}

/* ------------------------------------------------------------------------- */
/*  The memory manager                                                       */
/* ------------------------------------------------------------------------- */

extern void memoryerror(char *s, int32 size)
{   char fe_buff[128];
    sprintf(fe_buff, "The memory setting %s (which is %ld at present) has been \
exceeded.  Try running Inform again with $%s=<some-larger-number> on the \
command line.",s,(long int) size,s);
    fatalerror(fe_buff);
}

#ifdef PC_QUICKC
extern void *my_malloc(int32 size, char *whatfor)
{   char _huge *c;
    if (memout_mode==1)
        printf("Allocating %ld bytes for %s\n",size,whatfor);
    c=(char _huge *)halloc(size,1); malloced_bytes+=size;
    if (c==0) fatalerror("Couldn't hallocate memory");
    return(c);
}
extern void *my_calloc(int32 size, int32 howmany, char *whatfor)
{   void _huge *c;
    if (memout_mode==1)
        printf("Allocating %d bytes: array (%ld entries size %ld) for %s\n",
            size*howmany,howmany,size,whatfor);
    c=(void _huge *)halloc(howmany*size,1); malloced_bytes+=size*howmany;
    if (c==0) fatalerror("Couldn't hallocate memory for an array");
    return(c);
}
#else
extern void *my_malloc(int32 size, char *whatfor)
{   char *c;
    c=malloc((size_t) size); malloced_bytes+=size;
    if (c==0) fatalerror("Couldn't allocate memory");
    if (memout_mode==1)
        printf("Allocating %ld bytes for %s at (%08lx)\n",
            (long int) size,whatfor,(long int) c);
    return(c);
}
extern void *my_calloc(int32 size, int32 howmany, char *whatfor)
{   void *c;
    c=calloc(howmany,(size_t) size); malloced_bytes+=size*howmany;
    if (c==0) fatalerror("Couldn't allocate memory for an array");
    if (memout_mode==1)
        printf("Allocating %ld bytes: array (%ld entries size %ld) \
for %s at (%08lx)\n",
            ((long int)size) * ((long int)howmany),
            (long int)howmany,(long int)size,whatfor,
            (long int) c);
    return(c);
}
#endif

extern void my_free(void *pointer, char *whatitwas)
{   if (memout_mode==1)
        printf("Freeing memory for %s\n",whatitwas);

    if (*(int **)pointer != NULL)
    {   if (memout_mode==1)
            printf("Freeing memory for %s\n",whatitwas);
#ifdef PC_QUICKC
        hfree(*(int **)pointer);
#else
        free(*(int **)pointer);
#endif
        *(int **)pointer = NULL;
    }
}

/* ------------------------------------------------------------------------- */
/*   Dealing with source code files                                          */
/* ------------------------------------------------------------------------- */

#ifdef ARC_THROWBACK
char throwback_name[128*MAX_INCLUSION_DEPTH];  
#endif

#ifdef ARCHIMEDES
#define FN_SEP '.'
#else
#define FN_SEP '/'
#endif

extern void load_sourcefile(char *story_name, int style_flag)
{   char name[128]; int i, flag=0;

    if (input_file==MAX_INCLUSION_DEPTH)
    {   fatalerror("Too many files have included each other: \
increase #define MAX_INCLUSION_DEPTH");
    }
    strcpy(InputFiles[input_file].filename,story_name);
    InputFiles[input_file].sys_flag=0;
    InputFiles[input_file].chars_read=0;
    InputFiles[input_file].file_no=total_files_read+1;

    if (debugging_file==1)
    {   write_debug_byte(1); write_debug_byte(total_files_read+1);
        write_debug_string(story_name);
    }

    for (i=0; story_name[i]!=0; i++)
        if ((story_name[i]=='/') || (story_name[i]=='.')) flag=1;
    if (style_flag==1) flag=0;
    if (flag==0)
    {   if (input_file>0)
        {   if (style_flag==0)
            {   if (include_path_set==1)
                    sprintf(name,"%s%c%s%s", include_path, FN_SEP,
                        story_name, Include_Extension);
                else sprintf(name,"%s%s%s", Include_Prefix, story_name,
                        Include_Extension);
            }
            else sprintf(name,"%s%s%s",
                    home_directory,story_name,Source_Extension);
        }
        else sprintf(name,"%s%s%s",
                Source_Prefix,story_name,Source_Extension);
    }
    else strcpy(name,story_name);

    if (input_file==0)
    {   strcpy(home_directory, name);
        for (i=strlen(home_directory)-1;
             ((i>0)&&(home_directory[i]!=FN_SEP));i--) ;
        if (i!=0) i++; home_directory[i]=0;
    }

    if (debugging_file==1) write_debug_string(name);

#ifdef ARC_THROWBACK
    strcpy(throwback_name+128*input_file, name);
#endif

    InputFiles[input_file].handle = fopen(name,"r");
    if (InputFiles[input_file].handle==NULL)
    {   sprintf(sub_buffer, "Couldn't open input file \"%s\"",name);
        fatalerror(sub_buffer);
    }
    InputFiles[input_file++].source_line = 1;
    total_files_read++;

    if ((ltrace_mode!=0)||(trace_mode!=0))
    {   printf("\nOpening file \"%s\"\n",name);
    }
}

static int32 chars_kept;
static void write_cr(void)
{   write_debug_address(chars_kept);
}

static void close_sourcefile(void)
{   int i;
    if (ferror(InputFiles[input_file-1].handle))
        fatalerror("I/O failure: couldn't read from source file");
    if (debugging_file==1)
    {   write_debug_byte(16);
        write_debug_byte(InputFiles[input_file-1].file_no);
        write_cr();
        i=InputFiles[input_file-1].source_line;
        write_debug_byte(i/256); write_debug_byte(i%256);
    }
    fclose(InputFiles[--input_file].handle);

    if ((ltrace_mode!=0)||(trace_mode!=0)) printf("\nClosing file\n");
    if (input_file>=1)
        InputFiles[input_file-1].source_line--;
}

extern void close_all_source(void)
{   while (input_file>0) close_sourcefile();
}

static int32 last_char_marker= -1;
static int last_char;
extern int file_char(int32 marker)
{   if (marker==last_char_marker) return(last_char);
    last_char_marker=marker;
    if (input_file==0) return(0);
    last_char=fgetc(InputFiles[input_file-1].handle);
    InputFiles[input_file-1].chars_read++;
    if (last_char==EOF)
    {   close_sourcefile();
        if (input_file==0) last_char=0; else last_char='\n';
    }
    return(last_char);
}

extern int file_end(int32 marker)
{   int i;
    i=file_char(marker);
    if (i==0) return(1);
    return(0);
}

/* ------------------------------------------------------------------------- */
/*  Outputting the final story file, with checksums worked out, from storage */
/*  (and closing of the text transcript file, if present)                    */
/* ------------------------------------------------------------------------- */

static int c_low=0, c_high=0;
extern void add_to_checksum(void *address)
{   unsigned char *p;
    p=(unsigned char *) address;
    c_low+=((int) *p);
    if (c_low>=256)
    {   c_low-=256;
        if (++c_high==256) c_high=0;
    }
}

extern void output_file(void)
{   FILE *fout; char *actual_name; int i;
#ifdef ARC_THROWBACK
    char *newname;
#endif
#ifdef US_POINTERS
    unsigned char *t;
#else
    char *t;
#endif
    char *t2;
    int32 length, blanks=0, size=0;

    if (process_filename_flag==0)
    {   
#ifdef ARC_THROWBACK
        newname=Code_Name;
        for (i=0; Code_Name[i]!=0; i++)
            if ((Code_Name[i]=='.'))
                newname=Code_Name+i+1;
#define VFNAME newname
#else
#define VFNAME Code_Name
#endif
        switch(actual_version)
        {   case 3:
                sprintf(sub_buffer,"%s%s%s",
                      Code_Prefix,VFNAME,Code_Extension); break;
            case 4:
                sprintf(sub_buffer,"%s%s%s",
                      V4Code_Prefix,VFNAME,V4Code_Extension); break;
            case 5:
                sprintf(sub_buffer,"%s%s%s",
                      V5Code_Prefix,VFNAME,V5Code_Extension); break;
            case 6:
                sprintf(sub_buffer,"%s%s%s",
                      V6Code_Prefix,VFNAME,V6Code_Extension); break;
            case 7:
                sprintf(sub_buffer,"%s%s%s",
                      V7Code_Prefix,VFNAME,V7Code_Extension); break;
            case 8:
                sprintf(sub_buffer,"%s%s%s",
                      V8Code_Prefix,VFNAME,V8Code_Extension); break;
        }
        actual_name=sub_buffer;
    }
    else actual_name=Code_Name;

    fout=fopen(actual_name,"wb");
    if (fout==NULL) couldntopen("Couldn't open output file",actual_name);

#ifndef USE_TEMPORARY_FILES
    for (t=zcode; t<zcode_p; t++)
        add_to_checksum((void *) t);
    for (t=strings; t<strings_p; t++)
        add_to_checksum((void *) t);
#endif

    for (t=output_p+0x0040; t<output_p+Write_Code_At; t++)
        add_to_checksum((void *) t);

    length=((int32) Write_Strings_At)+ subtract_pointers(strings_p,strings);
    while ((length%scale_factor)!=0) { length++; blanks++; }
    length=length/scale_factor;
    output_p[26]=(length & 0xff00)/0x100;
    output_p[27]=(length & 0xff);

    while (((scale_factor*length)+blanks-1)%512 != 511) blanks++;

    output_p[28]=c_high;
    output_p[29]=c_low;

    if (debugging_file==1)
    {   debug_pass=2; write_debug_byte(9);
        for (i=0; i<64; i++) write_debug_byte((int) (output_p[i]));
        debug_pass=1;
    }

    for (t=output_p; t<output_p+Write_Code_At; t++) { fputc(*t,fout); size++; }

#ifdef USE_TEMPORARY_FILES
    {   FILE *fin;
        fclose(Temp2_fp);
        fin=fopen(Temp2_Name,"rb");
        if (fin==NULL)
            fatalerror("I/O failure: couldn't reopen temporary file 2");
        for (t=zcode; t<zcode_p; t++) { fputc(fgetc(fin),fout); size++; }
        if (ferror(fin))
            fatalerror("I/O failure: couldn't read from temporary file 2");
        fclose(fin);
    }
#else
    for (t=zcode; t<zcode_p; t++) { fputc(*t,fout); size++; }
#endif  
    while (size<Write_Strings_At) { fputc(0,fout); size++; }

#ifdef USE_TEMPORARY_FILES
    {   FILE *fin;
        fclose(Temp1_fp);
        fin=fopen(Temp1_Name,"rb");
        if (fin==NULL)
            fatalerror("I/O failure: couldn't reopen temporary file 1");
        for (t=strings; t<strings_p; t++) { fputc(fgetc(fin),fout); }
        if (ferror(fin))
            fatalerror("I/O failure: couldn't read from temporary file 1");
        fclose(fin);
        remove(Temp1_Name); remove(Temp2_Name);
    }
#else
    for (t=strings; t<strings_p; t++) { fputc(*t,fout); }
#endif  
    while (blanks>0) { fputc(0,fout); blanks--; }

    if (ferror(fout))
        fatalerror("I/O failure: couldn't write to story file");
    fclose(fout);
    if (statistics_mode==2) 
        printf("%d bytes written to '%s'\n",length,actual_name);
#ifdef ARCHIMEDES
    if (actual_version == 3)
        sprintf(buffer,"settype %s 063",actual_name);
    if (actual_version == 4)
        sprintf(buffer,"settype %s 064",actual_name);
    if (actual_version == 5)
        sprintf(buffer,"settype %s 065",actual_name);
    if (actual_version == 6)
        sprintf(buffer,"settype %s 066",actual_name);
    if (actual_version == 7)
        sprintf(buffer,"settype %s 067",actual_name);
    if (actual_version == 8)
        sprintf(buffer,"settype %s 068",actual_name);
    system(buffer);
#endif

    if (transcript_mode==1)
    {
        fout=fopen(Transcript_Name,"wb");
        if (fout==NULL) couldntopen("Couldn't open transcript file",
            Transcript_Name);
#ifdef MACINTOSH
        for (t2=all_text; t2<all_text_p; t2++)
        {   if ((int)*t2==10) fputc('\r',fout);
            else fputc(*t2,fout);
        }
#else
        for (t2=all_text; t2<all_text_p; t2++) { fputc(*t2,fout); }
#endif
        if (ferror(fout))
            fatalerror("I/O failure: couldn't write to transcript file");
        fclose(fout);
#ifdef ARCHIMEDES
        sprintf(buffer,"settype %s text",Transcript_Name);
        system(buffer);
#endif
    }
}

/* ------------------------------------------------------------------------- */
/*  Access to the debugging information file                                 */
/* ------------------------------------------------------------------------- */

static FILE *Debug_fp;
int debug_pass = 1;

extern void open_debug_file(void)
{
    Debug_fp=fopen(Debugging_Name,"wb");
    if (Debug_fp==NULL)
        couldntopen("Couldn't open debugging information file",
        Debugging_Name);
}

extern void write_debug_byte(int i)
{   if (pass_number!=debug_pass) return;
    fputc(i,Debug_fp);
    if (ferror(Debug_fp))
        fatalerror("I/O failure: can't write to debugging info file");
}

extern void write_debug_string(char *s)
{   int i;
    for (i=0; s[i]!=0; i++)
        write_debug_byte((int) s[i]);
    write_debug_byte(0);
}

static dbgl b_l, c_l, d_l, e_l;

extern void make_debug_linenum(void)
{   int i;
    b_l.b1 = InputFiles[input_file-1].file_no;
    i = forerrors_line;
    if ((b_l.b2!=i/256) || (b_l.b3!=i%256)) line_done_flag=0;
    b_l.b2 = i/256; b_l.b3 = i%256;
}

extern void keep_debug_linenum(void)
{   int i;
    c_l.b1 = InputFiles[input_file-1].file_no;
    i = forerrors_line;
    c_l.b2 = i/256; c_l.b3 = i%256;
}

extern void keep_routine_linenum(void)
{   int i;
    d_l.b1 = InputFiles[input_file-1].file_no;
    i = InputFiles[input_file-1].source_line;
    d_l.b2 = i/256; d_l.b3 = i%256;
}

extern void keep_re_linenum(void)
{   int i;
    e_l.b1 = InputFiles[input_file-1].file_no;
    i = InputFiles[input_file-1].source_line;
    e_l.b2 = i/256; e_l.b3 = i%256;
}

extern void write_dbgl(dbgl x)
{   write_debug_byte(x.b1); write_debug_byte(x.b2); write_debug_byte(x.b3);
}

extern void write_debug_linenum(void)   { write_dbgl(b_l); }
extern void write_kept_linenum(void)    { write_dbgl(c_l); }
extern void write_routine_linenum(void) { write_dbgl(d_l); }
extern void write_re_linenum(void)      { write_dbgl(e_l); }

extern void write_debug_address(int32 i)
{   write_debug_byte((int)((i/256)/256));
    write_debug_byte((int)((i/256)%256));
    write_debug_byte((int)(i%256));
}

extern void keep_chars_read(void)
{   chars_kept = InputFiles[input_file-1].line_start;
}

extern void write_present_linenum(void)
{   int i;
    write_debug_byte(InputFiles[input_file-1].file_no);
    i = InputFiles[input_file-1].source_line;
    write_debug_byte(i/256); write_debug_byte(i%256);
}

extern void write_chars_read(void)
{   write_present_linenum();
    write_debug_address(InputFiles[input_file-1].line_start);
}

extern void close_debug_file(void)
{   fputc(0,Debug_fp);
    if (ferror(Debug_fp))
        fatalerror("I/O failure: can't write to debugging info file");
    fclose(Debug_fp);
}

/* ------------------------------------------------------------------------- */
/*  Temporary storage files                                                  */
/* ------------------------------------------------------------------------- */

#ifdef USE_TEMPORARY_FILES
extern void open_temporary_files(void)
{
#ifdef UNIX
    sprintf(Temp1_Name, "%s.proc%d",Temp1_Hdr,(int)getpid());
    sprintf(Temp2_Name, "%s.proc%d",Temp2_Hdr,(int)getpid());
#endif
#ifdef ATARIST
#ifdef TOSFS
    sprintf(Temp1_Name, "%s.proc%d",Temp1_Hdr,(int)getpid());
    sprintf(Temp2_Name, "%s.proc%d",Temp2_Hdr,(int)getpid());
#endif
#endif
#ifdef AMIGA
    sprintf(Temp1_Name, "%s.proc%08x",Temp1_Hdr,(int)FindTask(NULL));
    sprintf(Temp2_Name, "%s.proc%08x",Temp2_Hdr,(int)FindTask(NULL));
#endif
    Temp1_fp=fopen(Temp1_Name,"wb");
    if (Temp1_fp==NULL) couldntopen("Couldn't open temporary file 1",
        Temp1_Name);
    Temp2_fp=fopen(Temp2_Name,"wb");
    if (Temp2_fp==NULL) couldntopen("Couldn't open temporary file 2",
        Temp2_Name);
}

extern void check_temp_files(void)
{
    if (ferror(Temp1_fp))
        fatalerror("I/O failure: couldn't write to temporary file 1");
    if (ferror(Temp2_fp))
        fatalerror("I/O failure: couldn't write to temporary file 2");
}

extern void remove_temp_files(void)
{   fclose(Temp1_fp); fclose(Temp2_fp);
    remove(Temp1_Name); remove(Temp2_Name);
}
#endif

/* ------------------------------------------------------------------------- */
/*   Code for the Acorn Archimedes (only) contributed by Robin Watts, to     */
/*   provide error throwback under the DDE environment                       */
/* ------------------------------------------------------------------------- */

#ifdef ARC_THROWBACK

#define DDEUtils_ThrowbackStart 0x42587
#define DDEUtils_ThrowbackSend  0x42588
#define DDEUtils_ThrowbackEnd   0x42589

#include "kernel.h"

int throwbackflag;

void throwback_start(void)
{    _kernel_swi_regs regs;
     if (throwbackflag==1)
         _kernel_swi(DDEUtils_ThrowbackStart, &regs, &regs);
}

void throwback_end(void)
{   _kernel_swi_regs regs;
    if (throwbackflag==1)
        _kernel_swi(DDEUtils_ThrowbackEnd, &regs, &regs);
}

int throwback_started=0;

void throwback(int severity, char * error)
{   _kernel_swi_regs regs;
    if (throwback_started==0)
    {   throwback_started=1;
        throwback_start();
    }
    if (throwbackflag==1)
    {   regs.r[0] = 1;
        regs.r[2] = (int) throwback_name+(input_file-1)*128;
        regs.r[3] = forerrors_line;
        regs.r[4] = (2-severity);
        regs.r[5] = (int) error;
       _kernel_swi(DDEUtils_ThrowbackSend, &regs, &regs);
    }
}

#endif

/* ------------------------------------------------------------------------- */
/*   Where the memory settings are declared as variables                     */
/* ------------------------------------------------------------------------- */

int BUFFER_LENGTH;
int MAX_QTEXT_SIZE;
int MAX_SYMBOLS;
int MAX_BANK_SIZE;
int SYMBOLS_CHUNK_SIZE;
int BANK_CHUNK_SIZE;
int HASH_TAB_SIZE;
int MAX_OBJECTS;
int MAX_ACTIONS;
int MAX_ADJECTIVES;
int MAX_DICT_ENTRIES;
int MAX_STATIC_DATA;
int MAX_TOKENS;
int MAX_OLDEPTH;
int MAX_ROUTINES;
int MAX_GCONSTANTS;
int MAX_PROP_TABLE_SIZE;
int MAX_FORWARD_REFS;
int STACK_SIZE;
int STACK_LONG_SLOTS;
int STACK_SHORT_LENGTH;
int MAX_ABBREVS;
int MAX_EXPRESSION_NODES;
int MAX_VERBS;
int MAX_VERBSPACE;
int32 MAX_STATIC_STRINGS;
int32 MAX_ZCODE_SIZE;
int MAX_LOW_STRINGS;
int32 MAX_TRANSCRIPT_SIZE;
int MAX_CLASSES;
int MAX_CLASS_TABLE_SIZE;

/* ------------------------------------------------------------------------- */
/*   Memory control from the command line                                    */
/* ------------------------------------------------------------------------- */

static void list_memory_sizes(void)
{   printf("\n  Current memory settings:\n");
    printf("  ========================\n");
    printf("  %20s = %d\n","MAX_ABBREVS",MAX_ABBREVS);
    printf("  %20s = %d\n","MAX_ACTIONS",MAX_ACTIONS);
    printf("  %20s = %d\n","MAX_ADJECTIVES",MAX_ADJECTIVES);
    printf("  %20s = %d\n","MAX_BANK_SIZE",MAX_BANK_SIZE);
    printf("  %20s = %d\n","BANK_CHUNK_SIZE",BANK_CHUNK_SIZE);
    printf("  %20s = %d\n","BUFFER_LENGTH",BUFFER_LENGTH);
    printf("  %20s = %d\n","MAX_CLASSES",MAX_CLASSES);
    printf("  %20s = %d\n","MAX_CLASS_TABLE_SIZE",MAX_CLASS_TABLE_SIZE);
    printf("  %20s = %d\n","MAX_DICT_ENTRIES",MAX_DICT_ENTRIES);
    printf("  %20s = %d\n","MAX_EXPRESSION_NODES",MAX_EXPRESSION_NODES);
    printf("  %20s = %d\n","MAX_FORWARD_REFS",MAX_FORWARD_REFS);
    printf("  %20s = %d\n","MAX_GCONSTANTS",MAX_GCONSTANTS);
    printf("  %20s = %d\n","HASH_TAB_SIZE",HASH_TAB_SIZE);
    printf("  %20s = %d\n","MAX_LOW_STRINGS",MAX_LOW_STRINGS);
    printf("  %20s = %d\n","MAX_OBJECTS",MAX_OBJECTS);
    printf("  %20s = %d\n","MAX_OLDEPTH",MAX_OLDEPTH);
    printf("  %20s = %d\n","MAX_PROP_TABLE_SIZE",MAX_PROP_TABLE_SIZE);
    printf("  %20s = %d\n","MAX_QTEXT_SIZE",MAX_QTEXT_SIZE);
    printf("  %20s = %d\n","MAX_ROUTINES",MAX_ROUTINES);
    printf("  %20s = %d\n","MAX_SYMBOLS",MAX_SYMBOLS);
    printf("  %20s = %d\n","STACK_LONG_SLOTS",STACK_LONG_SLOTS);
    printf("  %20s = %d\n","STACK_SHORT_LENGTH",STACK_SHORT_LENGTH);
    printf("  %20s = %d\n","STACK_SIZE",STACK_SIZE);
    printf("  %20s = %d\n","MAX_STATIC_DATA",MAX_STATIC_DATA);
    printf("  %20s = %ld\n","MAX_STATIC_STRINGS",
           (long int) MAX_STATIC_STRINGS);
    printf("  %20s = %d\n","SYMBOLS_CHUNK_SIZE",SYMBOLS_CHUNK_SIZE);
    printf("  %20s = %d\n","MAX_TOKENS",MAX_TOKENS);
    printf("  %20s = %ld\n","MAX_TRANSCRIPT_SIZE",
           (long int) MAX_TRANSCRIPT_SIZE);
    printf("  %20s = %d\n","MAX_VERBS",MAX_VERBS);
    printf("  %20s = %d\n","MAX_VERBSPACE",MAX_VERBSPACE);
    printf("  %20s = %ld\n","MAX_ZCODE_SIZE",
           (long int) MAX_ZCODE_SIZE);
    printf("  ========================\n");
}

extern void set_memory_sizes(int size_flag)
{
    if (size_flag == HUGE_SIZE)
    {
        BUFFER_LENGTH   = 4000;
        MAX_QTEXT_SIZE  = 3995;
        MAX_SYMBOLS     = 10000;

        MAX_BANK_SIZE      = 4000;
        SYMBOLS_CHUNK_SIZE = 5000;
        BANK_CHUNK_SIZE    = 512;
        HASH_TAB_SIZE      = 512;

        MAX_OBJECTS = 640;

        MAX_ACTIONS      = 200;
        MAX_ADJECTIVES   = 50;
        MAX_DICT_ENTRIES = 2000;
        MAX_STATIC_DATA  = 4000;

        MAX_TOKENS = 128;
        MAX_OLDEPTH = 8;
        MAX_ROUTINES = 1000;
        MAX_GCONSTANTS = 50;

        MAX_PROP_TABLE_SIZE = 30000;

        MAX_FORWARD_REFS = 2048;

        STACK_SIZE = 64;
        STACK_LONG_SLOTS = 5;
        STACK_SHORT_LENGTH = 80;

        MAX_ABBREVS = 64;

        MAX_EXPRESSION_NODES = 200;
        MAX_VERBS = 200;
        MAX_VERBSPACE = 4096;

#ifdef USE_TEMPORARY_FILES
        MAX_STATIC_STRINGS = 2000;
        MAX_ZCODE_SIZE = 2000;
#else
        MAX_STATIC_STRINGS = 150000;
        MAX_ZCODE_SIZE = 150000;
#endif

        MAX_LOW_STRINGS = 2048;

        MAX_TRANSCRIPT_SIZE = 200000;

        MAX_CLASSES = 32;
        MAX_CLASS_TABLE_SIZE = 1000;
    }
    if (size_flag == LARGE_SIZE)
    {
        BUFFER_LENGTH   = 2000;
        MAX_QTEXT_SIZE  = 1995;
        MAX_SYMBOLS     = 6400;

        MAX_BANK_SIZE      = 3200;
        SYMBOLS_CHUNK_SIZE = 5000;
        BANK_CHUNK_SIZE    = 512;
        HASH_TAB_SIZE      = 512;

        MAX_OBJECTS = 512;

        MAX_ACTIONS      = 150;
        MAX_ADJECTIVES   = 50;
        MAX_DICT_ENTRIES = 1300;
        MAX_STATIC_DATA  = 4000;

        MAX_TOKENS = 128;
        MAX_OLDEPTH = 8;
        MAX_ROUTINES = 500;
        MAX_GCONSTANTS = 50;

        MAX_PROP_TABLE_SIZE = 15000;

        MAX_FORWARD_REFS = 2048;

        STACK_SIZE = 64;
        STACK_LONG_SLOTS = 5;
        STACK_SHORT_LENGTH = 80;

        MAX_ABBREVS = 64;

        MAX_EXPRESSION_NODES = 200;
        MAX_VERBS = 140;
        MAX_VERBSPACE = 4096;

#ifdef USE_TEMPORARY_FILES
        MAX_STATIC_STRINGS = 2000;
        MAX_ZCODE_SIZE = 2000;
#else
        MAX_STATIC_STRINGS = 150000;
        MAX_ZCODE_SIZE = 150000;
#endif

        MAX_LOW_STRINGS = 2048;

        MAX_TRANSCRIPT_SIZE = 200000;

        MAX_CLASSES = 32;
        MAX_CLASS_TABLE_SIZE = 1000;
    }
    if (size_flag == SMALL_SIZE)
    {
        BUFFER_LENGTH   = 2000;
        MAX_QTEXT_SIZE  = 1995;
        MAX_SYMBOLS     = 3000;

        MAX_BANK_SIZE      = 1000;
        SYMBOLS_CHUNK_SIZE = 2500;
        BANK_CHUNK_SIZE    = 512;
        HASH_TAB_SIZE      = 512;

        MAX_OBJECTS = 300;

        MAX_ACTIONS      = 150;
        MAX_ADJECTIVES   = 50;
        MAX_DICT_ENTRIES = 700;
        MAX_STATIC_DATA  = 2000;

        MAX_TOKENS = 100;
        MAX_OLDEPTH = 8;
        MAX_ROUTINES = 400;
        MAX_GCONSTANTS = 50;

        MAX_PROP_TABLE_SIZE = 8000;

        MAX_FORWARD_REFS = 2048;

        STACK_SIZE = 64;
        STACK_LONG_SLOTS = 5;
        STACK_SHORT_LENGTH = 80;

        MAX_ABBREVS = 64;

        MAX_EXPRESSION_NODES = 40;
        MAX_VERBS = 110;
        MAX_VERBSPACE = 2048;

#ifdef USE_TEMPORARY_FILES
        MAX_STATIC_STRINGS = 1000;
        MAX_ZCODE_SIZE = 1000;
#else
        MAX_STATIC_STRINGS = 50000;
        MAX_ZCODE_SIZE = 100000;
#endif

        MAX_LOW_STRINGS = 1024;

        MAX_TRANSCRIPT_SIZE = 100000;

        MAX_CLASSES = 32;
        MAX_CLASS_TABLE_SIZE = 800;
    }
}

static void explain_parameter(char *command)
{   printf("\n");
    if (strcmp(command,"BUFFER_LENGTH")==0)
    {   printf(
"  BUFFER_LENGTH is the maximum length of a line of source code (when white\n\
  space has been removed).  It costs %d bytes to increase it by one.\n",
        5+STACK_LONG_SLOTS);
        return;
    }
    if (strcmp(command,"MAX_QTEXT_SIZE")==0)
    {   printf(
"  MAX_QTEXT_SIZE is the maximum length of a quoted string.  It must not \n\
  exceed BUFFER_LENGTH minus 5.\n");
        return;
    }
    if (strcmp(command,"MAX_SYMBOLS")==0)
    {   printf(
"  MAX_SYMBOLS is the maximum number of symbols - names of variables, \n\
  objects, routines, the many internal Inform-generated names and so on.\n");
        return;
    }
    if (strcmp(command,"MAX_BANK_SIZE")==0)
    {   printf(
"  MAX_BANK_SIZE is the maximum number of symbols in each of the seven \n\
  \"banks\".\n");
        return;
    }
    if (strcmp(command,"SYMBOLS_CHUNK_SIZE")==0)
    {   printf(
"  The symbols names are stored in memory which is allocated in chunks \n\
  of size SYMBOLS_CHUNK_SIZE.\n");
        return;
    }
    if (strcmp(command,"BANK_CHUNK_SIZE")==0)
    {   printf(
"  The symbol banks are stored in memory which is allocated in chunks of \n\
  size BANK_CHUNK_SIZE.\n");
        return;
    }
    if (strcmp(command,"HASH_TAB_SIZE")==0)
    {   printf(
"  HASH_TAB_SIZE is the size of the hash tables used for the heaviest \n\
  symbols banks.\n");
        return;
    }
    if (strcmp(command,"MAX_OBJECTS")==0)
    {   printf(
"  MAX_OBJECTS is the maximum number of objects.  (If compiling a version-3 \n\
  game, 255 is an absolute maximum in any event.)\n");
        return;
    }
    if (strcmp(command,"MAX_ACTIONS")==0)
    {   printf(
"  MAX_ACTIONS is the maximum number of actions - that is, routines such as \n\
  TakeSub which are referenced in the grammar table.\n");
        return;
    }
    if (strcmp(command,"MAX_ADJECTIVES")==0)
    {   printf(
"  MAX_ADJECTIVES is the maximum number of different \"adjectives\" in the \n\
  grammar table.  Adjectives are misleadingly named: they are words such as \n\
  \"in\", \"under\" and the like.\n");
        return;
    }
    if (strcmp(command,"MAX_DICT_ENTRIES")==0)
    {   printf(
"  MAX_DICT_ENTRIES is the maximum number of words which can be entered \n\
  into the game's dictionary.  It costs 29 bytes to increase this by one.\n");
        return;
    }
    if (strcmp(command,"MAX_STATIC_DATA")==0)
    {   printf(
"  MAX_STATIC_DATA is the size of an array of integers holding initial \n\
  values for arrays and strings stored as ASCII inside the Z-machine.  It \n\
  should be at least 1024 but seldom needs much more.\n");
        return;
    }
    if (strcmp(command,"MAX_TOKENS")==0)
    {   printf(
"  The maximum number of tokens (words, strings, separators like an equals \n\
  sign) per line of source code is MAX_TOKENS: it is not expensive to \n\
  increase.\n");
        return;
    }
    if (strcmp(command,"MAX_OLDEPTH")==0)
    {   printf(
"  MAX_OLDEPTH is the maximum depth of objectloop nesting: it only costs \n\
  about 40 bytes to increase it by one.\n");
        return;
    }
    if (strcmp(command,"MAX_ROUTINES")==0)
    {   printf(
"  MAX_ROUTINES is the maximum number of routines of code, including \n\
  routines embedded in object definitions.  Cheap to increase.\n");
        return;
    }
    if (strcmp(command,"MAX_GCONSTANTS")==0)
    {   printf(
"  MAX_GCONSTANTS is too complicated to explain here, but cheap and rare.\n");
        return;
    }
    if (strcmp(command,"MAX_PROP_TABLE_SIZE")==0)
    {   printf(
"  MAX_PROP_TABLE_SIZE is the number of bytes allocated to hold the \n\
  properties table.\n");
        return;
    }
    if (strcmp(command,"MAX_FORWARD_REFS")==0)
    {   printf(
"  MAX_FORWARD_REFS is the maximum number of forward references to constants \n\
  not yet defined in source code.  It costs 4 bytes to increase by one.\n");
        return;
    }
    if ((strcmp(command,"STACK_SIZE")==0)
        || (strcmp(command,"STACK_LONG_SLOTS")==0)
        || (strcmp(command,"STACK_SHORT_LENGTH")==0))
    {   printf(
"  The Inform preprocessor maintains a stack of modified lines and assembly \n\
  code to be processed in due course.  The maximum size is STACK_SIZE \n\
  awaiting processing and needs to be at least 32 or so, but ideally more \n\
  like 64.  The stack can contain at most STACK_LONG_SLOTS entries which \n\
  are longer than STACK_SHORT_LENGTH characters.\n\n");
        printf(
"  Total memory consumption for the preprocessor stack in bytes is\n\n\
    STACK_SIZE * STACK_SHORT_LENGTH + STACK_LONG_SLOTS * BUFFER_LENGTH\n\n\
  and currently amounts to %d bytes.\n",
  STACK_SIZE * STACK_SHORT_LENGTH + STACK_LONG_SLOTS * BUFFER_LENGTH); 
        return;
    }
    if (strcmp(command,"MAX_ABBREVS")==0)
    {   printf(
"  MAX_ABBREVS is the maximum number of declared abbreviations.  It is not \n\
  allowed to exceed 64.\n");
        return;
    }
    if (strcmp(command,"MAX_EXPRESSION_NODES")==0)
    {   printf(
"  MAX_EXPRESSION_NODES is the maximum number of nodes in the expression \n\
  evaluator's tree.  In effect, it measures how complicated algebraic \n\
  expressions are allowed to be.  Increasing it by one costs about 48 \n\
  bytes.\n");
        return;
    }
    if (strcmp(command,"MAX_VERBS")==0)
    {   printf(
"  MAX_VERBS is the maximum number of verbs (such as \"take\") which can be \n\
  defined, each with its own grammar.  To increase it by one costs about\n\
  128 bytes.  A full game will contain at least 100.\n");
        return;
    }
    if (strcmp(command,"MAX_VERBSPACE")==0)
    {   printf(
"  MAX_VERBSPACE is the size of workspace used to store verb words, so may\n\
  need increasing in games with many synonyms: unlikely to exceed 4K.\n");
        return;
    }
    if (strcmp(command,"MAX_STATIC_STRINGS")==0)
    {
#ifdef USE_TEMPORARY_FILES
        printf(
"  MAX_STATIC_STRINGS is the size in bytes of a buffer to hold compiled\n\
  strings before they're written into a temporary file.  2000 bytes is \n\
  plenty.");
#else
        printf(
"  MAX_STATIC_STRINGS is the size in bytes of a buffer to hold all the \n\
  strings so far compiled.  It needs to be fairly large, typically half to \n\
  three-quarters the size of the final output game.  Recompiling Inform \n\
  with #define USE_TEMPORARY_FILES set will reduce this to only 2000 or \n\
  so by using the filing system to hold caches of strings and code.");
#endif
        return;

    }
    if (strcmp(command,"MAX_ZCODE_SIZE")==0)
    {   
#ifdef USE_TEMPORARY_FILES
        printf(
"  MAX_ZCODE_SIZE is the size in bytes of a buffer to hold compiled \n\
  Z-machine code before it's written into a temporary file.  2000 bytes \n\
  is plenty.");
#else
        printf(
"  MAX_ZCODE_SIZE is the size in bytes of a buffer to hold all the \n\
  Z-machine code so far compiled.  It needs to be fairly large, typically \n\
  a quarter to a half of the size of the final output game.  Recompiling \n\
  Inform with #define USE_TEMPORARY_FILES set will reduce this to only 2000 \n\
  or so by using the filing system to hold caches of strings and code.");
#endif
        return;
    }
    if (strcmp(command,"MAX_LOW_STRINGS")==0)
    {   printf(
"  MAX_LOW_STRINGS is the size in bytes of a buffer to hold all the \n\
  compiled \"low strings\" which are to be written above the synonyms table \n\
  in the Z-machine.  1024 is plenty.\n");
        return;
    }
    if (strcmp(command,"MAX_TRANSCRIPT_SIZE")==0)
    {   printf(
"  MAX_TRANSCRIPT_SIZE is only allocated if expressly requested, and would\n\
  the size in bytes of a buffer to hold the entire text of the game being\n\
  compiled: it therefore has to be enormous, say 100000 to 200000.\n");
        return;
    }
    if (strcmp(command,"MAX_CLASSES")==0)
    {   printf(
"  MAX_CLASSES maximum number of object classes which can be defined.  This\n\
  is cheap to increase.\n");
        return;
    }
    if (strcmp(command,"MAX_CLASS_TABLE_SIZE")==0)
    {   printf(
"  MAX_CLASS_TABLE_SIZE is the number of bytes allocated to hold the table \n\
  of properties to inherit from each class.\n");
        return;
    }

    printf("No such memory setting as \"%s\"\n",command);

    return;
}

extern void memory_command(char *command)
{   int i, k, flag=0; int32 j;

    for (k=0; command[k]!=0; k++)
        if (islower(command[k])) command[k]=toupper(command[k]);

    if (command[0]=='?') { explain_parameter(command+1); return; }

    if (strcmp(command, "HUGE")==0) { set_memory_sizes(HUGE_SIZE); return; }
    if (strcmp(command, "LARGE")==0) { set_memory_sizes(LARGE_SIZE); return; }
    if (strcmp(command, "SMALL")==0) { set_memory_sizes(SMALL_SIZE); return; }
    if (strcmp(command, "LIST")==0)  { list_memory_sizes(); return; }
    for (i=0; command[i]!=0; i++)
    {   if (command[i]=='=')
        {   command[i]=0;
#ifdef ARCHIMEDES
            if (strcmp(command, "TEMP_FILES")==0)
            {   strcpy(tname_pre, command+i+1);
                return;
            }
#endif
            j=(int32) atoi(command+i+1);
            if ((j==0) && (command[i+1]!='0'))
            {   printf("Bad numerical setting in $ command \"%s=%s\"\n",
                    command,command+i+1);
                return;
            }
            if (strcmp(command,"BUFFER_LENGTH")==0)
                BUFFER_LENGTH=j, flag=1;
            if (strcmp(command,"MAX_QTEXT_SIZE")==0)
                MAX_QTEXT_SIZE=j, flag=1;
            if (strcmp(command,"MAX_SYMBOLS")==0)
                MAX_SYMBOLS=j, flag=1;
            if (strcmp(command,"MAX_BANK_SIZE")==0)
                MAX_BANK_SIZE=j, flag=1;
            if (strcmp(command,"SYMBOLS_CHUNK_SIZE")==0)
                SYMBOLS_CHUNK_SIZE=j, flag=1;
            if (strcmp(command,"BANK_CHUNK_SIZE")==0)
                BANK_CHUNK_SIZE=j, flag=1;
            if (strcmp(command,"HASH_TAB_SIZE")==0)
                HASH_TAB_SIZE=j, flag=1;
            if (strcmp(command,"MAX_OBJECTS")==0)
                MAX_OBJECTS=j, flag=1;
            if (strcmp(command,"MAX_ACTIONS")==0)
                MAX_ACTIONS=j, flag=1;
            if (strcmp(command,"MAX_ADJECTIVES")==0)
                MAX_ADJECTIVES=j, flag=1;
            if (strcmp(command,"MAX_DICT_ENTRIES")==0)
                MAX_DICT_ENTRIES=j, flag=1;
            if (strcmp(command,"MAX_STATIC_DATA")==0)
                MAX_STATIC_DATA=j, flag=1;
            if (strcmp(command,"MAX_TOKENS")==0)
                MAX_TOKENS=j, flag=1;
            if (strcmp(command,"MAX_OLDEPTH")==0)
                MAX_OLDEPTH=j, flag=1;
            if (strcmp(command,"MAX_ROUTINES")==0)
                MAX_ROUTINES=j, flag=1;
            if (strcmp(command,"MAX_GCONSTANTS")==0)
                MAX_GCONSTANTS=j, flag=1;
            if (strcmp(command,"MAX_PROP_TABLE_SIZE")==0)
                MAX_PROP_TABLE_SIZE=j, flag=1;
            if (strcmp(command,"MAX_FORWARD_REFS")==0)
                MAX_FORWARD_REFS=j, flag=1;
            if (strcmp(command,"STACK_SIZE")==0)
                STACK_SIZE=j, flag=1;
            if (strcmp(command,"STACK_LONG_SLOTS")==0)
                STACK_LONG_SLOTS=j, flag=1;
            if (strcmp(command,"STACK_SHORT_LENGTH")==0)
                STACK_SHORT_LENGTH=j, flag=1;
            if (strcmp(command,"MAX_ABBREVS")==0)
                MAX_ABBREVS=j, flag=1;
            if (strcmp(command,"MAX_EXPRESSION_NODES")==0)
                MAX_EXPRESSION_NODES=j, flag=1;
            if (strcmp(command,"MAX_VERBS")==0)
                MAX_VERBS=j, flag=1;
            if (strcmp(command,"MAX_VERBSPACE")==0)
                MAX_VERBSPACE=j, flag=1;
            if (strcmp(command,"MAX_STATIC_STRINGS")==0)
                MAX_STATIC_STRINGS=j, flag=1;
            if (strcmp(command,"MAX_ZCODE_SIZE")==0)
                MAX_ZCODE_SIZE=j, flag=1;
            if (strcmp(command,"MAX_LOW_STRINGS")==0)
                MAX_LOW_STRINGS=j, flag=1;
            if (strcmp(command,"MAX_TRANSCRIPT_SIZE")==0)
                MAX_TRANSCRIPT_SIZE=j, flag=1;
            if (strcmp(command,"MAX_CLASSES")==0)
                MAX_CLASSES=j, flag=1;
            if (strcmp(command,"MAX_CLASS_TABLE_SIZE")==0)
                MAX_CLASS_TABLE_SIZE=j, flag=1;

            if (flag==0)
                printf("No such memory setting as \"%s\"\n",command);

            return;
        }
    }
    printf("No such memory $ command as \"%s\"\n",command);
}
 
extern void init_files_vars(void)
{   override_error_line = 0;
    malloced_bytes = 0;
    last_char_marker = -1;
    c_low = 0;
    c_high = 0;
    debug_pass = 1;
}
