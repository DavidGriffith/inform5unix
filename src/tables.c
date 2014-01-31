/* ------------------------------------------------------------------------- */
/*   "tables" :  Maintains tables of dictionary, objects, actions,           */
/*               grammar and global variables;                               */
/*               and constructs the tables part of the story file            */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include "header.h"

int no_attributes, no_properties, no_globals, no_fake_actions=0;
int dict_entries;
int release_number=1, statusline_flag=0;

int time_set=0;
char time_given[7];

int globals_size, properties_size;

int32 prop_defaults[64];
int prop_longflag[64];
int prop_additive[64];
char *properties_table;

int max_no_objects=0;

int resobj_flag = 0;

static int no_verbs=0, no_actions=0, no_adjectives=0,
           no_objects=0, no_classes=0, classes_size=0;
static int fp_no_actions=0;
static int embedded_routine = 0;

static fpropt full_object;

/* ------------------------------------------------------------------------- */
/*   Arrays used by this file                                                */
/* ------------------------------------------------------------------------- */

#ifndef ALLOCATE_BIG_ARRAYS
  static verbt   vs[MAX_VERBS];
  static objectt objects[MAX_OBJECTS];
  static int     table_init[MAX_STATIC_DATA];
  static int32   actions[MAX_ACTIONS],
                 preactions[MAX_ACTIONS],
                 adjectives[MAX_ADJECTIVES];
  static dict_word adjcomps[MAX_ADJECTIVES];
  static int32   gvalues[240];
  static int     dict_places_list[MAX_DICT_ENTRIES],
                 dict_places_back[MAX_DICT_ENTRIES],
                 dict_places_inverse[MAX_DICT_ENTRIES];
  static dict_word dict_sorts[MAX_DICT_ENTRIES];
  static zip     class_pointer[MAX_CLASS_TABLE_SIZE];
  static int     classes_here[MAX_CLASSES];
  static int32  *classes_at[MAX_CLASSES];
#else
  static verbt   *vs;
  static objectt *objects;
  static int     *table_init;
  static int32   *actions,
                 *preactions,
                 *adjectives;
  static dict_word *adjcomps;
  static int32   *gvalues;
  static int     *dict_places_list,
                 *dict_places_back,
                 *dict_places_inverse;
  static dict_word *dict_sorts;
  static int     *classes_here;
  static int32   **classes_at;
  static zip     *class_pointer;
#endif

static char *verblist;
static char *verblist_p;

extern void tables_allocate_arrays(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    vs         = my_calloc(sizeof(verbt),   MAX_VERBS, "verbs");
    objects    = my_calloc(sizeof(objectt), MAX_OBJECTS, "objects");
    table_init = my_calloc(sizeof(int),     MAX_STATIC_DATA, "static data");
    actions    = my_calloc(sizeof(int32),   MAX_ACTIONS, "actions");
    preactions = my_calloc(sizeof(int32),   MAX_ACTIONS, "preactions");
    adjectives = my_calloc(sizeof(int32),   MAX_ADJECTIVES, "adjectives");
    adjcomps   = my_calloc(sizeof(dict_word), MAX_ADJECTIVES, "adj comps");
    gvalues    = my_calloc(sizeof(int32),   240, "global values");
    dict_places_list = my_calloc(sizeof(int),  MAX_DICT_ENTRIES,
                                               "dictionary places list");
    dict_places_back = my_calloc(sizeof(int),  MAX_DICT_ENTRIES,
                                               "dictionary places back");
    dict_places_inverse = my_calloc(sizeof(int),  MAX_DICT_ENTRIES,
                                               "inverse of dictionary places");
    dict_sorts = my_calloc(sizeof(dict_word),   MAX_DICT_ENTRIES,
                                               "dictionary sort codes");
    class_pointer = my_malloc(MAX_CLASS_TABLE_SIZE, "class table");
    classes_here = my_calloc(sizeof(int), MAX_CLASSES,
                                               "inherited classes list");
    classes_at = my_calloc(sizeof(int32), MAX_CLASSES,
                                               "pointers to classes");
#endif
    verblist = my_malloc(MAX_VERBSPACE, "register of verbs");
    verblist_p = verblist;
}

extern void tables_free_arrays(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    my_free(&vs, "verbs");
    my_free(&objects, "objects");
    my_free(&table_init, "static data");
    my_free(&actions, "actions");
    my_free(&preactions, "preactions");
    my_free(&adjectives, "adjectives");
    my_free(&adjcomps, "adj comps");
    my_free(&gvalues, "global values");
    my_free(&dict_places_list, "dictionary places list");
    my_free(&dict_places_back, "dictionary places back");
    my_free(&dict_places_inverse, "inverse of dictionary places");
    my_free(&dict_sorts, "dictionary sort codes");
    my_free(&class_pointer,"class table");
    my_free(&classes_here, "inherited classes list");
    my_free(&classes_at, "pointers to classes");
#endif
    my_free(&verblist,"register of verbs");
}

/* ------------------------------------------------------------------------- */
/*  A routine using an unusual ANSI library function:                        */
/*                                                                           */
/*  write_serialnumber writes today's date in the form YYMMDD as a string    */
/*    (as can be seen, the VAX doesn't know it)                              */
/* ------------------------------------------------------------------------- */

static void write_serialnumber(char *buffer)
{   time_t tt;  tt=time(0);
    if (time_set==0)
#ifdef TIME_UNAVAILABLE
      sprintf(buffer,"940000");
#else
      strftime(buffer,10,"%y%m%d",localtime(&tt));
#endif
    else
      sprintf(buffer,"%06s",time_given);
}

/* ------------------------------------------------------------------------- */
/*   Dictionary table builder                                                */
/*   The dictionary is, so to speak, thumb-indexed: the beginning of each    */
/*   letter in the double-linked-list is marked.  Experiments with           */
/*   increasing the number of markers (to the first two letters, say) result */
/*   in extra bureaucracy which cancels out any speed gain.                  */
/* ------------------------------------------------------------------------- */

#define NUMBER_DICT_MARKERS 26

static int total_dict_entries;
static dict_word letter_keys[NUMBER_DICT_MARKERS];
static int letter_starts[NUMBER_DICT_MARKERS];
static int start_list;
static dict_word prepared_sort;
static int initial_letter;

static void dictionary_begin_pass(void)
{   int i, j;
    dict_p=dictionary+7;
    total_dict_entries=dict_entries; dict_entries=0;
    if (pass_number==1)
    {   start_list=0; dict_places_list[0]= -2; dict_places_back[0]= -1;
        for (i=0; i<NUMBER_DICT_MARKERS; i++)
        {   letter_keys[i].b[0]=0xff;
            letter_starts[i]= -1;
        }
    }
    else
    {   for (j=start_list, i=0; i<total_dict_entries; i++)
        {   dict_places_inverse[j]=i; j=dict_places_list[j]; }
    }
}

static int compare_sorts(dict_word d1, dict_word d2)
{   int i;
    for (i=0; i<6; i++) if (d1.b[i]!=d2.b[i]) return(d1.b[i]-d2.b[i]);
/*  since memcmp(d1.b, d2.b, 6); doesn't work in the Unix port - ? */
    return(0);
}

static dict_word dictionary_prepare(char *dword)
{   int i, j, k, wd[10]; int32 tot;
    for (i=0, j=0; (i<9)&&(dword[j]!=0); i++, j++)
    {   k=(int) dword[j];
        if (k==(int) '\'')
            obsolete_warning("use the ^ character for an apostrophe \
in a dictionary word, e.g. 'peter^s'");
        if (k==(int) '^') k=(int) '\'';
        k=chars_lookup[k];
        if ((k>=26)&&(k<2*26)) k=k-26;
        if ((k/26)!=0) wd[i++]=3+(k/26);
        wd[i]=6+(k%26);
    }
    for (; i<9; i++) wd[i]=5;
    InV3 { wd[6]=5; wd[7]=5; wd[8]=5;
           if ((wd[5]==3)||(wd[5]==4)) wd[5]=5;
         }
    InV5 { if ((wd[8]==3)||(wd[8]==4)) wd[8]=5; }

    initial_letter = wd[0]-6;
    if (initial_letter<0)
    {   error("Dictionary words must begin with a letter of the alphabet");
        initial_letter=0;
    }
    /* Note... this doesn't depend on A to Z being contiguous in the
       machine's character set  */

    tot = wd[2] + wd[1]*(1<<5) + wd[0]*(1<<10);
    prepared_sort.b[1]=tot%0x100;
    prepared_sort.b[0]=(tot/0x100)%0x100;
    tot = wd[5] + wd[4]*(1<<5) + wd[3]*(1<<10);
    prepared_sort.b[3]=tot%0x100;
    prepared_sort.b[2]=(tot/0x100)%0x100;
    tot = wd[8] + wd[7]*(1<<5) + wd[6]*(1<<10);
    prepared_sort.b[5]=tot%0x100;
    prepared_sort.b[4]=(tot/0x100)%0x100;
    InV3 { prepared_sort.b[2]+=0x80; }
    InV5 { prepared_sort.b[4]+=0x80; }

    return(prepared_sort);
}

extern int dictionary_find(char *dword, int scope)
{   int32 j, j2, k, jlim;
    dict_word i;
    i=dictionary_prepare(dword);
    if (scope==1) jlim=dict_entries; else jlim=total_dict_entries;

    if (pass_number==1)
    {   for (j=0; j<jlim; j++)
            if (compare_sorts(i,dict_sorts[j])==0) return((int) j+1);
        return(0);
    }
    if ((k=letter_starts[initial_letter])==-1) return(0);
    j=initial_letter+1;
    while ((j<NUMBER_DICT_MARKERS)&&((j2=letter_starts[j])==-1)) j++;
    if (j==NUMBER_DICT_MARKERS) { j2= -2; }
    while (k!=j2)
    {
        if ((compare_sorts(i,dict_sorts[k])==0)&&(k<jlim))
            return(dict_places_inverse[k]+1);
        k=dict_places_list[k];
    }
    return(0);
}        

static void show_letter(int code)
{
    if (code<6) { printf("."); return; }
    printf("%c",(alphabet[0])[code-6]);
}

extern void show_dictionary(void)
{   int i, j; char *p;
    int res=((version_number==3)?4:6);
    printf("Dictionary contains %d entries:\n",dict_entries);
    for (i=0; i<dict_entries; i++)
    {   p=(char *)dictionary+7+(3+res)*i;
        if (dict_entries==0)
          printf("Entry %03d (%03d > %03d) at %04x: ",
            i,dict_places_back[i],dict_places_list[i],
            dictionary_offset+7+(3+res)*i);
        else
          printf("Entry %03d at %04x: ",i,dictionary_offset+7+(3+res)*i);
        show_letter( (((int) p[0])&0x7c)/4 );
        show_letter( 8*(((int) p[0])&0x3) + (((int) p[1])&0xe0)/32 );
        show_letter( ((int) p[1])&0x1f );
        show_letter( (((int) p[2])&0x7c)/4 );
        show_letter( 8*(((int) p[2])&0x3) + (((int) p[3])&0xe0)/32 );
        show_letter( ((int) p[3])&0x1f );
        printf("  ");
        for (j=0; j<3+res; j++) printf("%02x ",p[j]);
        printf("\n");
    }
}

static void dictionary_set_verb_number(char *dword, int to)
{   int i; zip *p;
    int res=((version_number==3)?4:6);
    i=dictionary_find(dword,1);
    if (i!=0)
    {   p=dictionary+7+(i-1)*(3+res)+res; p[1]=to;
    }
}

extern int dictionary_add(char *dword, int x, int y, int z)
{   int off, i, k, l; zip *p; dict_word pcomp, qcomp;
    int res=((version_number==3)?4:6);

    if (dict_entries==MAX_DICT_ENTRIES)
    {   memoryerror("MAX_DICT_ENTRIES",MAX_DICT_ENTRIES); }

    i=dictionary_find(dword,1);
    if (i!=0)
    {   p=dictionary+7+(i-1)*(3+res)+res;
        p[0]=(p[0])|x; p[1]=(p[1])|y; p[2]=(p[2])|z;
        return(dictionary_offset+7+(3+res)*(i-1));
    }

    if (pass_number==1) i=dict_entries;
    else { i=dict_places_inverse[dict_entries]; }

    off=(3+res)*i+7;
    p=dictionary+off;

    p[0]=prepared_sort.b[0]; p[1]=prepared_sort.b[1];
    p[2]=prepared_sort.b[2]; p[3]=prepared_sort.b[3];
    InV5 { p[4]=prepared_sort.b[4]; p[5]=prepared_sort.b[5]; }

    p[res]=x; p[res+1]=y; p[res+2]=z;

    if (pass_number==1)
    {   pcomp=prepared_sort;
        if (dict_entries==0)
        {   dict_places_list[0]= -2; dict_places_list[1]= -1;
            goto PlaceFound;
        }
        l=initial_letter; do { k=letter_starts[l--]; } while ((l>=0)&&(k==-1));
        if (k==-1) k=start_list;
        for (; k!=-2; k=dict_places_list[k])
        {   qcomp=dict_sorts[k];
            if (compare_sorts(pcomp,qcomp)<0)
            {   l=dict_places_back[k];
                if (l==-1)
                {   dict_places_list[dict_entries]=start_list;
                    dict_places_back[dict_entries]= -1;
                    dict_places_back[k]=dict_entries;
                    start_list=dict_entries; goto PlaceFound;
                }
                dict_places_list[l]=dict_entries;
                dict_places_back[k]=dict_entries;
                dict_places_list[dict_entries]=k;
                dict_places_back[dict_entries]=l;
                goto PlaceFound;
            }
            l=k;
        }
        dict_places_list[l]=dict_entries;
        dict_places_back[dict_entries]=l;
        dict_places_list[dict_entries]= -2;
        PlaceFound: dict_sorts[dict_entries]=pcomp;
        if (compare_sorts(pcomp,letter_keys[initial_letter])<0)
        {   letter_keys[initial_letter]=pcomp;
            letter_starts[initial_letter]=dict_entries;
        }        
    }

    dict_entries++; dict_p+=res+3;
    /* show_dictionary();  */
    return(dictionary_offset+off);
}

extern void list_object_tree(void)
{   int i;
    printf("obj   par nxt chl   Object tree:\n");
    for (i=0; i<no_objects; i++)
        printf("%3d   %3d %3d %3d\n",
            i+1,objects[i].parent,objects[i].next, objects[i].child);
}

extern void list_verb_table(void)
{   int i, j, k;
    for (i=0; i<no_verbs; i++)
    {   printf("Verb entry %2d  [%d]\n",i,vs[i].lines);
        for (j=0; j<vs[i].lines; j++)
        {   for (k=0; k<8; k++) printf("%03d ",vs[i].l[j].e[k]);
            printf("\n");
        }
    }
}

/* ------------------------------------------------------------------------- */
/*   Keep track of actions                                                   */
/* ------------------------------------------------------------------------- */

static int make_action(int32 addr)
{   int i;
    if (no_actions>=MAX_ACTIONS)
      memoryerror("MAX_ACTIONS",MAX_ACTIONS);
    for (i=0; i<no_actions; i++) if (actions[i]==addr) return(i);
    actions[no_actions]=addr; preactions[no_actions]= -1;
    return(no_actions++);
}

extern int find_action(int32 addr)
{   int i;
    for (i=0; i<fp_no_actions; i++) if (actions[i]==addr) return(i);
    if (pass_number==2)
        error("Action given in constant does not exist");
    return(0);
}

extern void new_action(char *b, int c)
{   if ((printprops_mode==0)||(pass_number==2)) return;
    printf("Action '%s' is numbered %d\n",b,c);
}

/* ------------------------------------------------------------------------- */
/*   Parsing the grammar table, and making new adjectives and verbs          */
/*   (see the documentation for what the verb table it makes looks like)     */
/* ------------------------------------------------------------------------- */

static int make_adjective(char *c)
{   int i; dict_word acomp;

    acomp=dictionary_prepare(c);
    for (i=0; i<no_adjectives; i++)
    {   if (compare_sorts(acomp,adjcomps[i])==0) return(0xff-i);
    }
    adjectives[no_adjectives]=dictionary_add(c,8,0,0xff-no_adjectives);
    adjcomps[no_adjectives]=acomp;
    return(0xff-no_adjectives++);
}

static int find_verb(char *verb)
{   char *p;
    p=verblist;
    while (p<verblist_p)
    {   if (strcmp(verb, p+2)==0) return(p[1]);
        p=p+p[0];
    }
    return(-1);
}

static int verbspace_consumed = 0;
static void register_verb(char *verb, int number)
{   if (find_verb(verb)>=0)
    {   error_named("Two different verb definitions refer to", verb);
        return;
    }
    verbspace_consumed += strlen(verb)+3;
    if (verbspace_consumed >= MAX_VERBSPACE)
        memoryerror("MAX_VERBSPACE",MAX_VERBSPACE);
    strcpy(verblist_p+2,verb);
    verblist_p[1]=number;
    verblist_p[0]=3+strlen(verb);
    verblist_p += verblist_p[0];
}

static int32 parsing_routines[64];
static int no_prs;
static int debug_acts_writ[256];
static int action_exists[256];

static int grammar_line(int verbnum, int line, int i, char *b)
{   int j, l, flag=0; int32 k;
    int vargs, vtokens, vinsert;

    if (line>=MAX_LINES_PER_VERB)
    {   error("Too many lines of grammar for verb: increase \
#define MAX_LINES_PER_VERB"); return(0); }

        word(b,i++); flag=2;
        if (b[0]==0) return(0);
        if (strcmp(b,"*")!=0)
        {   error_named("'*' divider expected, but found",b); return(0); }
        vtokens=1; vargs=0; for (j=0; j<8; j++) vs[verbnum].l[line].e[j]=0;
        do
        {   word(b,i++);
            if (b[0]==0) { error("'->' clause missing"); return(0); }
            if (strcmp(b,"->")==0) break;
            if (b[0]=='\"')
            {   textword(b,i-1); vinsert=make_adjective(b);
            }
            else On_("noun")        { vargs++; vinsert=0; }
            else On_("held")        { vargs++; vinsert=1; }
            else On_("multi")       { vargs++; vinsert=2; }
            else On_("multiheld")   { vargs++; vinsert=3; }
            else On_("multiexcept") { vargs++; vinsert=4; }
            else On_("multiinside") { vargs++; vinsert=5; }
            else On_("creature")    { vargs++; vinsert=6; }
            else On_("special")     { vargs++; vinsert=7; }
            else On_("number")      { vargs++; vinsert=8; }
            else On_("scope")
                 {   word(b,i++);
                     if (strcmp(b,"=")!=0)
                     {   error_named("Expected '=' after 'scope' but found",b);
                         return(0);
                     }
                     word(b,i++);
                     j=find_symbol(b);
                     if ((j<0) || (stypes[j]!=1))
                     {   error_named(
                             "Expected routine after 'scope=' but found",b);
                         return(0);
                     }

                     for (l=0; (l<no_prs)
                              && parsing_routines[l]!=svals[j]; l++) ;
                     parsing_routines[l]=svals[j]; if (l==no_prs) no_prs++;
                     vargs++; vinsert=l+80;
                 }
            else On_("=")
                 {   if ((vtokens==0)||(vs[verbnum].l[line].e[vtokens-1]!=0))
                     {   error("'=' is only legal here as 'noun=Routine'");
                         return(0);
                     }
                     word(b,i++);
                     j=find_symbol(b);
                     if ((j<0) || (stypes[j]!=1))
                     {   error_named(
                             "Expected routine after 'noun=' but found", b);
                         return(0);
                     }
                     vtokens--;

                     for (l=0; (l<no_prs)
                              && parsing_routines[l]!=svals[j]; l++) ;
                     parsing_routines[l]=svals[j]; if (l==no_prs) no_prs++;
                     vinsert=l+16;
                 }
            else
            {   j=find_symbol(b);
                if ((j<0) || ((stypes[j]!=7)&&(stypes[j]!=1)))
                {   error_named("No such token as",b); return(0); }
                if (stypes[j]==7)
                {   vargs++; vinsert=svals[j]+128;
                }
                else
                {   vargs++;
                    for (l=0; (l<no_prs)
                              && parsing_routines[l]!=svals[j]; l++) ;
                    parsing_routines[l]=svals[j]; if (l==no_prs) no_prs++;
                    vinsert=l+48;
                }
            }
            vs[verbnum].l[line].e[vtokens]=vinsert;
            vtokens++;
        } while (1==1);

        word(b,i++);
        j=strlen(b)-3;
        if ((j<0)||(b[j]!='S')||(b[j+1]!='u')||(b[j+2]!='b'))
            sprintf(b+j+3,"Sub");
        j=find_symbol(b);
        if ((j==-1)&&(pass_number==2))
        {   error_named("No such action routine as",b);
            return(0);
        }
        if (j==-1) k=0;
        else
        {   if (stypes[j]!=1) { error_named("Not an action:",b); return(0); }
            k=svals[j];
        }
        vs[verbnum].l[line].e[0]=vargs;
        vs[verbnum].l[line].e[7]=make_action(k);

        j=find_action(k);
        if (action_exists[j]==0)
        {   action_exists[j]=1; new_action(b,j);
        }

        if ((debugging_file==1)&&(k!=0)
            &&(debug_acts_writ[j]==0))
        {   debug_pass=2;
            b[strlen(b)-3]=0;
            write_debug_byte(8); write_debug_byte(j);
            write_debug_string(b);
            debug_pass=1;
            if (pass_number==2) debug_acts_writ[j]=1;
        }

    return(i);
}

static int get_verb(char *b, int wn)
{   int j;
    textword(b,wn); j=find_verb(b);
    if (j==-1)
        error_named("There is no previous grammar for the verb", b);
    return(j);
}

extern void make_verb(char *b)
{   int i, i2, x=-1, flag=0;
    int lines=0;
    i=2;
    word(b,i);
    On_("meta") { i++; flag=2; }

    i2=i;
    do word(b,i2++); while (b[0]=='\"');
    if (b[0]=='=') x=get_verb(b,i2++);
    else
    {   x=no_verbs;
        if (no_verbs==MAX_VERBS)
            memoryerror("MAX_VERBS",MAX_VERBS);
    }

    do
    {   word(b,i);
        if (b[0]!='\"') break;
        textword(b,i++);
        dictionary_add(b,0x41+flag,0xff-x,0);
        if (pass_number==1) register_verb(b,x);
    } while (1==1);

    if (x!=no_verbs)
    {   word(b,i2);
        if (b[0]!=0)
        error_named("Expected end of command but found",b);
        return;
    }

    do
    {   i=grammar_line(no_verbs, lines++,i,b);
    } while (i!=0);

    vs[no_verbs].lines=--lines;
    no_verbs++;
}

extern void extend_verb(char *b)
{   int i, j, k, l, lines, mode;
    word(b,2);
    if (strcmp(b,"only")==0)
    {   i=3; l=-1;
        if (no_verbs==MAX_VERBS)
            memoryerror("MAX_VERBS",MAX_VERBS);
        do
        {   j=get_verb(b,i++);
            if (j==-1) return;
            if ((l!=-1) && (j!=l) && (pass_number==1))
            {   warning_named("Verb disagrees with previous verbs", b);
            }
            l=j;
            dictionary_set_verb_number(b,0xff-no_verbs);
            word(b,i);
        } while (b[0]=='\"');
        vs[no_verbs]=vs[j];
        j=no_verbs++;
    }
    else { j=get_verb(b,2); if (j==-1) return; i=3; }
    word(b,i); mode=3;
    if (strcmp(b,"*")!=0)
    {   mode=0;
        if (strcmp(b,"replace")==0) mode=1;
        if (strcmp(b,"first")==0) mode=2;
        if (strcmp(b,"last")==0) mode=3;
        if (mode==0)
        {   error_named(
                "Expected 'replace', 'last' or 'first' but found",b);
            mode=3;
        }
        i++;
    }
    l=vs[j].lines; if (mode<=2) lines=0; else lines=l;
    do
    {   if (mode==2)
            for (k=l; k>0; k--)
                 vs[j].l[k+lines]=vs[j].l[k-1+lines];
        i=grammar_line(j, lines++,i,b);
    } while (i!=0);

    if (mode==2)
    {   vs[j].lines = l+lines-1;
        for (k=0; k<l; k++)
            vs[j].l[k+lines-1]=vs[j].l[k+lines];
    }
    else vs[j].lines=--lines;
}

/* ------------------------------------------------------------------------- */
/*   Object manufacture.  Note that property lists are not kept for each     */
/*   object, only written in game-file format and then forgotten; but the    */
/*   object tree structure so far, is kept                                   */
/* ------------------------------------------------------------------------- */

static int class_flag;
static int no_c_here;

static int properties(int w)
{   int i, k, x, y, pnw, flag=0; int32 j;

    do
    {   pnw=w;
        word(sub_buffer,w++);
        if (sub_buffer[0]==0) return(w-1);
/*        if (strcmp(sub_buffer,",")==0) return(w-1); */
        if (strcmp(sub_buffer,"has")==0) return(w-1);
        if (strcmp(sub_buffer,"class")==0) return(w-1);
        i=find_symbol(sub_buffer);
        if (i==-1)
        {   error_named("No such property as",sub_buffer);
            return(w);
        }
        if (stypes[i]!=12)
        {   error_named("Not a property:",sub_buffer);
            return(w);
        }
        i=svals[i];
        x=full_object.l++;
        full_object.pp[x].num=i;
        y=0;
        do
        {   word(sub_buffer,w++);
            if (strcmp(sub_buffer,",")==0) break;
            if (strcmp(sub_buffer,"has")==0) { w--; break; }
            if (strcmp(sub_buffer,"class")==0) { w--; break; }
            if (sub_buffer[0]==0) break;

            if (strcmp(sub_buffer,"[")==0)
            {   flag=1;
                sprintf(sub_buffer,"Mb__%d",embedded_routine);
            }
            if ((i==1)&&(sub_buffer[0]=='\"'))
            {   textword(sub_buffer,w-1);
                j=dictionary_add(sub_buffer,0x80,0,0);
            }
            else
            {   if ((pass_number==2)&&(y!=0))
                {   k=find_symbol(sub_buffer);
                    if ((k!=-1)&&(stypes[k]==12))
                    {   warning_named(
            "Missing ','?  Property data seems to contain the property name",
                            sub_buffer);
                    }
                }
                j=constant_value(sub_buffer);
                if (j==0)
                {   if (prop_defaults[i]>=256) j=0x1000;
                }
            }
            if ((j>=256)||(prop_longflag[i]==1))
                full_object.pp[x].p[y++]=j/256;
            full_object.pp[x].p[y++]=j%256;
        } while (flag==0);
        if (y==0) { full_object.pp[x].p[y++]=0;
                    full_object.pp[x].p[y++]=0;
                  }
        full_object.pp[x].l=y;
        InV3
        {   if (y>8)
            {   word(sub_buffer, pnw);
                if (pass_number==2)
       warning_named("Standard-game limit of 8 bytes per property exceeded \
(use Advanced to get 64), so truncating property", sub_buffer);
                full_object.pp[x].l=8;
            }
        }
    } while (flag==0);
    sprintf(buffer,"[ Mb__%d",embedded_routine++);
    do
    {   word(sub_buffer,w++);
        sprintf(buffer+strlen(buffer)," %s",sub_buffer);
    } while (sub_buffer[0]!=0);
    stack_line(buffer);
    return(-1);
}

static int attributes(int w)
{   int i, negate_flag;
    do
    {   word(sub_buffer,w++);
        if (sub_buffer[0]==0) return(w-1);
        if ((strcmp(sub_buffer,"with")==0)
            || (strcmp(sub_buffer,",")==0)
            || (strcmp(sub_buffer,"class")==0)) return(w-1);
        if (sub_buffer[0]=='~')
        {   negate_flag=1; i=find_symbol(sub_buffer+1); }
        else
        {   negate_flag=0; i=find_symbol(sub_buffer); }
        if ((i==-1)||(stypes[i]!=7))
        { error_named("No such attribute as",sub_buffer); return(w); }
        i=svals[i];
        if (negate_flag==0)
            full_object.atts[i/8] =
                full_object.atts[i/8] | (1 << (7-i%8));
        else
            full_object.atts[i/8] =
                full_object.atts[i/8] & ~(1 << (7-i%8));
    } while (1==1);
    return(0);
}

static int classes(int w)
{   int i; zip *p;
    do
    {   word(sub_buffer,w++);
        if (sub_buffer[0]==0) return(w-1);
        if ((strcmp(sub_buffer,",")==0)
            || (strcmp(sub_buffer,"with")==0)
            || (strcmp(sub_buffer,"has")==0)) return(w-1);
        i=find_symbol(sub_buffer);
        if ((i==-1)||(stypes[i]!=13))
        { error_named("No such class as",sub_buffer); return(w); }
        classes_here[no_c_here++]=svals[i];
/*        printf("Inheriting attrs from %d\n",svals[i]); */
        p=(zip *)classes_at[svals[i]-1];
        for (i=0; i<6; i++) full_object.atts[i] |= p[i];
/*        for (i=0; i<6; i++) printf("Data %d  %x\n",i,p[i]); */
    } while (1==1);
    return(0);
}

static int write_properties(zip *p, char *shortname)
{   int i, l, j, k, count, number; zip *tmp; zip *from;
    int32 props=0;

    if (class_flag==0)
    {   for (i=0;i<6;i++)
            objects[no_objects].atts[i]=full_object.atts[i];
 
        tmp=translate_text(p+1,shortname);
        props=subtract_pointers(tmp,p);
        p[0]=(props-1)/2;
    }
    else
    {   p=class_pointer+classes_size;
        classes_at[no_classes]=(int32 *) p;
        /* printf("Class data at %08x\n",p); */
        for (i=0;i<6;i++) p[i]=full_object.atts[i];
        p+=6;
    }

    for (l=0; l<no_c_here; l++)
    {   j=0; from=6+(zip *)classes_at[classes_here[l]-1];
        /* printf("Inheriting from %d at %08x\n",classes_here[l],from); */

        /* for (j=0; j<32; j++) printf("%02x ",from[j]); printf("\n"); j=0; */

        while (from[j]!=0)
        {   InV3
            {   number=from[j]%32; count=1+from[j++]/32;
            }
            InV5
            {   number=from[j]%64; count=1+from[j++]/64;
                if (count>2) count=from[j++]%64;
            }
            /* printf("Inheriting no %d count %d\n",number,count); */
            for (k=0; k<full_object.l; k++)
            {   if (full_object.pp[k].num == number)
                {   if ((number==1) || (prop_additive[number]!=0))
                    {   /* printf("Appending %d\n",number); */
                        for (i=full_object.pp[k].l;
                             i<full_object.pp[k].l+count; i++)
                            full_object.pp[k].p[i]=from[j++];
                        full_object.pp[k].l += count;
                    }
                    else
                    {   j+=count;
                        /* printf("Ignoring %d\n",number); */
                    }
                    goto DunInheriting;
                }
            }
            k=full_object.l++;
            full_object.pp[k].num = number;
            full_object.pp[k].l = count;
            for (i=0; i<count; i++)
                full_object.pp[k].p[i]=from[j++];
            /* printf("Creating %d\n",number); */
            DunInheriting: ;
        }
    }

    for (l=((version_number==3)?31:63); l>0; l--)
    {   for (j=0; j<full_object.l; j++)
        {   if (full_object.pp[j].num == l)
            {   count=full_object.pp[j].l; number=full_object.pp[j].num;
                InV3
                {   p[props++] = number + (count - 1)*32;
                }
                InV5
                {   switch(count)
                    {   case 1:
                          p[props++] = number; break;
                        case 2:
                          p[props++] = number + 0x40; break;
                        default:
                          p[props++] = number + 0x80;
                          p[props++] = count + 0x80;
                          break;
                    }
                }

                for (k=0; k<full_object.pp[j].l; k++)
                {   p[props++]=full_object.pp[j].p[k];
                }
            }
        }
    }

    p[props]=0; props++;

    if (class_flag==1)
    {
        classes_size+=props+6;
        if (classes_size >= MAX_CLASS_TABLE_SIZE)
            memoryerror("MAX_CLASS_TABLE_SIZE",MAX_CLASS_TABLE_SIZE);
    }
    else
    {
        properties_size+=props;
        if (properties_size >= MAX_PROP_TABLE_SIZE)
            memoryerror("MAX_PROP_TABLE_SIZE",MAX_PROP_TABLE_SIZE);
    }

    return(props);
}

static char shortname_buffer[256];

extern void finish_object(void)
{   int j;
    if (class_flag == 0)
    {   j=objects[no_objects].propsize;
        objects[no_objects].propsize=
            write_properties((zip *) (properties_table+properties_size),
            shortname_buffer);
        if (pass_number==2)
        {   if (j != objects[no_objects].propsize)
            {   error("Object has altered in memory usage between passes: \
perhaps an attempt to use a routine name as value of a small property");
            }
        }
        no_objects++;
        if (pass_number==1) max_no_objects++;
    }
    else { write_properties(NULL,NULL); no_classes++; }

    if (debugging_file==1)
    {   write_present_linenum();
    }
}

extern void resume_object(char *b, int j)
{   int flag=0, commas=0;
    if (j==1)
    {   word(b,1); flag=1;
        if ((strcmp(b,"has")==0)
            || (strcmp(b,",")==0)
            || (strcmp(b,"class")==0)
            || (strcmp(b,"with")==0)) flag=0;
    }
    do
    {   RO_Comma:
        if (flag==0)
        {   word(b,j++);
            if (b[0]==0)
            {   if (commas>0)
                    error("Object/class definition finishes with ','");
                break;
            }
        }
        if (strcmp(b,",")==0) { commas++; goto RO_Comma; }
        if (commas>1)
            error("Two commas ',' in a row in object/class definition");
        commas=0;
        if ((flag==1)||(strcmp(b,"with")==0))
        {   j=properties(j);
            if (j==-1) { resobj_flag=1; return; }
        }
        else if (strcmp(b,"has")==0) j=attributes(j);
        else if (strcmp(b,"class")==0) j=classes(j);
        else error_named("Expected 'with', 'has' or \
'class' in object/class definition but found",b);
        flag=0;
    } while (1==1);
    finish_object();
}

extern void make_class(char *b)
{   class_flag = 1; no_c_here=0;

    if (no_classes==MAX_CLASSES)
        memoryerror("MAX_CLASSES", MAX_CLASSES);

    word(b,2); new_symbol(b,no_classes+1,13);

    if (debugging_file==1)
    {   write_debug_byte(2); write_debug_linenum();
        write_debug_string(b);
    }
  
    full_object.l=0;
    full_object.atts[0]=0;
    full_object.atts[1]=0;
    full_object.atts[2]=0;
    full_object.atts[3]=0;
    full_object.atts[4]=0;
    full_object.atts[5]=0;
    resume_object(b,3);
}

static int near_obj;

extern void make_object(char *b, int nearby_flag)
{   int i, j, k, non=0, later, subof;

    if (no_objects==MAX_OBJECTS)
        memoryerror("MAX_OBJECTS", MAX_OBJECTS);
    
    class_flag = 0; no_c_here=0;

    word(b,2);
    if (b[0]=='\"')
    {   textword(b,2);
        error_named(
            "Expected an (internal) name for object, but found the string", b);
        sprintf(b,"failedobj");
    }
    new_symbol(b,no_objects+1,9);
    if (debugging_file==1)
    {   write_debug_byte(3);
        write_debug_byte((no_objects+1)/256);
        write_debug_byte((no_objects+1)%256);
        write_debug_linenum(); write_debug_string(b);
    }

    do
    {   word(b,3+non);
        if (b[0]!='\"')
        {   new_symbol(b,no_objects+1,9); non++;
            obsolete_warning("an object should only have one internal name");
        }
    } while (b[0]!='\"');
    later=non+5;

    if (nearby_flag==0)
    {   word(b,4+non);
        subof=0;
        if ((b[0]==0)
            || (strcmp(b,",")==0)
            || (strcmp(b,"with")==0)
            || (strcmp(b,"has")==0)
            || (strcmp(b,"class")==0)) later--;
        else
        {   i=find_symbol(b);
            if (i<0)
            {   error_named("An object must be defined \
after the one which contains it: (so far) there is no such object as",b);
                return;
            }
            if (stypes[i]!=9)
            {   error_named("Not an object:",b); return; }
            subof=svals[i];
        }
        near_obj=no_objects+1;
    }
    else
    {   subof=near_obj; later--;
    }
    full_object.atts[0]=0;
    full_object.atts[1]=0;
    full_object.atts[2]=0;
    full_object.atts[3]=0;
    full_object.atts[4]=0;
    full_object.atts[5]=0;
    objects[no_objects].parent=subof;
    objects[no_objects].next=0;
    objects[no_objects].child=0;
    full_object.l=0;

    if (subof>0)
    {   j=subof-1; k=objects[j].child;
        if (k==0)
        {   objects[j].child=no_objects+1; }
        else
        {   while(objects[k-1].next!=0) { k=objects[k-1].next; }
            objects[k-1].next=no_objects+1;
        }
    }

    textword(b,3+non);
    if ((listobjects_mode==1) && ((pass_number==2)||(bothpasses_mode==1)))
        printf("%3d \"%s\"\n",no_objects+1,b);
    strcpy(shortname_buffer,b);
    resume_object(b, later);
}

/* ------------------------------------------------------------------------- */
/*   Making and initialising variables and arrays                            */
/* ------------------------------------------------------------------------- */

#define NON_VALUE (int32)0x100000L

extern void check_globals(void)
{   int i;
    for (i=0;i<no_globals;i++)
        if (gvalues[i]==NON_VALUE)
        {   error("An initialised global variable was defined only in Pass 1");
            return;
        }
}

static int multiplier;
static int32 table_posn;
static int array_base;
int doing_table_mode;

static void finish_array(int32 i)
{   doing_table_mode=0;
    if ((pass_number==1)&&(array_base!=globals_size))
    {   if (multiplier==2)
        {   table_init[array_base]   = i/256;
            table_init[array_base+1] = i%256;
        }
        else
        {   if (i>=256)
                error("A 'string' array can have at most 256 entries");
            table_init[array_base] = i;
        }
    }
    globals_size+=i*multiplier;
}

static void array_entry(int32 i, int32 j, char *b)
{   if (globals_size+i*multiplier >= MAX_STATIC_DATA)
        memoryerror("MAX_STATIC_DATA", MAX_STATIC_DATA);
    if (multiplier==1)
    {   table_init[globals_size+i]=j;
        if ((pass_number==2)&&(j>=256))
            warning_named("Array entry too large for a byte:",b);
    }
    else
    {   table_init[globals_size+2*i]=j/256;
        table_init[globals_size+2*i+1]=j%256;
    }
}

extern void assemble_table(char *b, int from)
{   int32 j; int i;
    for (i=from;1==1;i++)
    {   word(b,i); if (b[0]==0) return;
        if (strcmp(b,"]")==0)
        {   finish_array(table_posn);
            return;
        }
        if (strcmp(b,"[")==0)
        {   stack_line("]");
            error("Expected ']' but found '['"); return;
        }
        j=constant_value(b);
        array_entry(table_posn++, j, b);
    }
    return;
}

extern void make_global(char *b, int array_flag)
{   int32 i, j; int array_type, data_type, one_word;

    word(b,2);
    if (array_flag==0)
    {   if (no_globals==235)
        {   error("All 235 global variables already declared");
            return;
        }
        if (pass_number==1)
        {   new_symbol(b,no_globals,2);
            if (debugging_file==1)
            {   write_debug_byte(4); write_debug_byte(no_globals);
                write_debug_string(b);
            }
        }
        else
        {   i=find_symbol(b);
            no_globals=svals[i];
        }
        gvalues[no_globals]=0; no_globals++;
    }
    else
    {   IfPass2
        {   j=find_symbol(b);
            svals[j]=variables_offset+globals_size;
        }
        else new_symbol(b,0x800+globals_size,8);
    }
    word(b,3);
    if (b[0]==0)
    {   if (array_flag==1) error("Missing array definition");
        return;
    }
    if ((pass_number==1) && (array_flag==0))
        gvalues[no_globals-1]=NON_VALUE;

    if ((strcmp(b,"=")==0) && (array_flag==0))
    {   word(b,4); i=constant_value(b);
        if (pass_number==2) gvalues[no_globals-1]=i;
        return;
    }

    if ((pass_number==2) && (array_flag==0))
       gvalues[no_globals-1]=globals_size+variables_offset;

    array_type=0; data_type=-1;

         if ((array_flag==0)&&(strcmp(b,"data")==0))    { data_type=0; }
    else if ((array_flag==0)&&(strcmp(b,"initial")==0)) { data_type=1; }
    else if ((array_flag==0)&&(strcmp(b,"initstr")==0)) { data_type=2; }

    else if (strcmp(b,"->")==0)      array_type=0;
    else if (strcmp(b,"-->")==0)     array_type=1;
    else if (strcmp(b,"string")==0)  array_type=2;
    else if (strcmp(b,"table")==0)   array_type=3;

    else {   if (array_flag==0)
    error_named("Expected '=', '->', '-->', 'string' or 'table' but found",b);
             else
    error_named("Expected '->', '-->', 'string' or 'table' but found",b);
             return;
         }

    multiplier=1; if ((array_type==1) || (array_type==3)) multiplier=2;

    word(b,5);
    one_word=0; if (b[0]==0) one_word=1;
    word(b,4);
    if (b[0]==0) { error("Missing array definition"); return; }

    switch(data_type)
    {   case -1:
            if (b[0]=='[') data_type=3;
            else
            {   if (one_word==1)
                {   if (b[0]=='\"') data_type=2; else data_type=0;
                }
                else data_type=1;
            }
            break;
        case 0: obsolete_warning("use '->' instead of 'data'"); break;
        case 1: obsolete_warning("use '->' instead of 'initial'"); break;
        case 2: obsolete_warning("use '->' instead of 'initstr'"); break;
    }

    array_base=globals_size;
    if ((array_type==2) || (array_type==3)) globals_size+=multiplier;

    switch(data_type)
    {   case 0:
            word(b,4); j=constant_value(b);
            for (i=0; i<j; i++) array_entry(i, 0, b);
            break;
        case 1:
            i=0;
            do
            {   word(b,4+i); if (b[0]==0) break;
                if ((strcmp(b,"]")==0) || (strcmp(b,"[")==0))
                {   error_named("Misplaced ",b); return; }
                j=constant_value(b);
                array_entry(i, j, b);
                i++;
            } while (1==1);
            break;
        case 2:
            textword(b,4);
            for (i=0; b[i]!=0; i++) array_entry(i, b[i], b);
            break;
        case 3:
            doing_table_mode=1; table_posn=0;
            if (one_word==0) assemble_table(b,5);
            return;
    }

    finish_array(i);
}

/* ------------------------------------------------------------------------- */
/*   Construct story file up as far as code area                             */
/*   (see documentation for description of what goes on here)                */
/* ------------------------------------------------------------------------- */

static void percentage(char *name, int32 x, int32 total)
{   printf("   %-20s %2d.%d%%\n",name,x*100/total,(x*1000/total)%10);
}

static int alloc_done_flag=0;

static char *version_name(int v)
{   switch(v)
    {   case 3: return "Standard";
        case 4: return "Plus";
        case 5: return "Advanced";
        case 6: return "graphical";
    }
    return "extended format";
}

extern void construct_storyfile(void)
{   char *p; unsigned char *u; int32 i, j, k; int32 excess;
    int32 syns, objs, props, vars, parse, code, strs, dict, nparse,
        actshere, preactshere;
    int32 synsat, glpat, objat, propat, parsat;
    int32 code_length, strings_length, alloc_size;
    int32 limit;
    int extend_offset;

    if (alloc_done_flag==0)
    {   alloc_done_flag=1;
        alloc_size=0x4000 + subtract_pointers(dict_p,dictionary);
        for (i=0; i<no_objects; i++) alloc_size+=objects[i].propsize;
        output_p=my_malloc(alloc_size, "output buffer");
    }

    p=(char *)output_p; u=(unsigned char *) p;

    for (i=0; i<=0x3f; i++) p[i]=0;

    p[0]=actual_version; p[1]=statusline_flag*2;
    p[2]=(release_number/256); p[3]=(release_number%256);
    p[16]=0; p[17]=0;

    write_serialnumber(buffer);
    for (i=0; i<6; i++) p[18+i]=buffer[i];

    syns=0x40;
    u[syns]=0x80; p[syns+1]=0; syns+=2;
    for (i=0; i+low_strings<low_strings_p; syns++, i++)
        p[0x42+i]=low_strings[i];

    p[24]=syns/256; p[25]=syns%256; synsat=syns;
    for (i=0; i<3*32; i++)
    {   p[syns++]=0; p[syns++]=0x20;
    }

    for (i=0; i<no_abbrevs; i++)
    {   j=abbrev_values[i];
        p[synsat+64+2*i]=j/256;
        p[synsat+65+2*i]=j%256;
    }
    objs=syns;

    p[10]=objs/256; p[11]=objs%256; glpat=objs;
    p[objs]=0; p[objs+1]=0;

    for (i=2; i< ((version_number==3)?32:64); i++)
    {   p[objs+2*i-2]=prop_defaults[i]/256;
        p[objs+2*i-1]=prop_defaults[i]%256;
    }
    objs+=2*i-2;
    props=objs+((version_number==3)?9:14)*no_objects;
    objat=objs; propat=props;

    for (i=0; i<properties_size; i++)
        p[props+i]=properties_table[i];

    for (i=0; i<no_objects; i++)
    {
        InV3
        {   p[objs]=objects[i].atts[0];
            p[objs+1]=objects[i].atts[1];
            p[objs+2]=objects[i].atts[2];
            p[objs+3]=objects[i].atts[3];
            p[objs+4]=objects[i].parent;
            p[objs+5]=objects[i].next;
            p[objs+6]=objects[i].child;
            p[objs+7]=props/256;
            p[objs+8]=props%256;
            objs+=9;
        }
        InV5
        {   p[objs]=objects[i].atts[0];
            p[objs+1]=objects[i].atts[1];
            p[objs+2]=objects[i].atts[2];
            p[objs+3]=objects[i].atts[3];
            p[objs+4]=objects[i].atts[4];
            p[objs+5]=objects[i].atts[5];
            p[objs+6]=(objects[i].parent)/256;
            p[objs+7]=(objects[i].parent)%256;
            p[objs+8]=(objects[i].next)/256;
            p[objs+9]=(objects[i].next)%256;
            p[objs+10]=(objects[i].child)/256;
            p[objs+11]=(objects[i].child)%256;
            p[objs+12]=props/256;
            p[objs+13]=props%256;
            objs+=14;
        }
        props+=objects[i].propsize;
    }

    vars=props;

    p[12]=(vars/256); p[13]=(vars%256);

    for (i=vars; i<vars+globals_size; i++) p[i]=table_init[i-vars];
    for (i=0; i<240; i++)
    {   j=gvalues[i];
        p[vars+i*2]   = j/256;
        p[vars+i*2+1] = j%256;
    }

    parse=vars+globals_size;

    p[14]=(parse/256); p[15]=(parse%256);  parsat=parse;
    nparse=parse+no_verbs*2;
    for (i=0; i<no_verbs; i++)
    {   p[parse]=(nparse/256); p[parse+1]=(nparse%256);
        parse+=2;
        p[nparse]=vs[i].lines; nparse++;
        for (j=0; j<vs[i].lines; j++)
        {   for (k=0; k<8; k++) p[nparse+k]=vs[i].l[j].e[k];
            nparse+=8;
        }
    }

    actshere=nparse; nparse+=2*no_actions;
    preactshere=nparse; nparse+=2*no_actions;

    p[nparse]=0; p[nparse+1]=no_adjectives; nparse+=2;

    dict=nparse+4*no_adjectives;

    adjectives_offset=nparse;

    for (i=0; i<no_adjectives; i++)
    {   j=adjectives[no_adjectives-i-1];
        p[nparse]=j/256; p[nparse+1]=j%256; p[nparse+2]=0;
        p[nparse+3]=(256-no_adjectives+i); nparse+=4;
    }

    dictionary[0]=3; dictionary[1]='.'; dictionary[2]=','; dictionary[3]='"';
    dictionary[4]=(version_number==3)?7:9;
    dictionary[5]=(dict_entries/256); dictionary[6]=(dict_entries%256);

    p[8]=(dict/256);     p[9]=(dict%256);
    for (i=0; i+dictionary<dict_p; i++) p[dict+i]=dictionary[i];
    code=dict+i;
    while ((code%scale_factor) != 0) p[code++]=0;

    p[4]=(code/256);     p[5]=(code%256);
    p[6]=((code+1)/256); p[7]=((code+1)%256);


    Write_Code_At = code;
    code_length=subtract_pointers(zcode_p,zcode);

    strs=code+code_length;
    while ((strs%scale_factor) != 0) strs++;

    Write_Strings_At = strs;
    strings_length=subtract_pointers(strings_p,strings);

    Out_Size=strs+strings_length;

    if (actual_version==3)
    { excess=Out_Size-((int32) 0x20000L); limit=128; }
    if ((actual_version==4)||(actual_version==5))
    { excess=Out_Size-((int32) 0x40000L); limit=256; }
    if ((actual_version==6)||(actual_version==8))
    { excess=Out_Size-((int32) 0x80000L); limit=512; }
    if (actual_version==7) { excess=0; limit=320; }

    if (excess>0)
    {   sprintf(buffer,
          "Story file exceeds version-%d limit (%dK) by %d bytes",
          version_number, limit, excess);
        fatalerror(buffer);
    }

    extend_offset=256;
    if (no_objects+5 > extend_offset) extend_offset=no_objects+5;

    if (extend_memory_map==1) code_offset = extend_offset*scale_factor;
    else code_offset = code;
    if (extend_memory_map==1)
        strings_offset = code_offset + (strs-code);
    else strings_offset = strs;

    dictionary_offset = dict;
    variables_offset = vars;
    actions_offset = actshere;
    preactions_offset = preactshere;

    j=Out_Size/scale_factor;

    p[26]=j/256; p[27]=j%256; p[28]=0; p[29]=0;

    for (i=0; i<no_actions; i++)
    {   j=(actions[i]+code_offset)/scale_factor;
        p[actshere+i*2]=j/256; p[actshere+i*2+1]=j%256;

        if (i<no_prs) j=(parsing_routines[i]+code_offset)/scale_factor;
        else if (preactions[i]==-1) j=0;
        else j=(preactions[i]+code_offset)/scale_factor;
        p[preactshere+i*2]=j/256; p[preactshere+i*2+1]=j%256;
    }

    if (extend_memory_map==1)
    {   j=Write_Code_At/scale_factor - extend_offset;
        p[40]=j/256; p[41]=j%256;
        j=Write_Code_At/scale_factor - extend_offset;
        p[42]=j/256; p[43]=j%256;
    }

    /*  From here on, it's all reportage: construction is finished  */

    if (statistics_mode==1)
    {   int32 k_long, rate; char *k_str="";
        k_long=(Out_Size/1024);
        if ((Out_Size-1024*k_long) >= 512) { k_long++; k_str=""; }
        else if ((Out_Size-1024*k_long) > 0) { k_str=".5"; }
        rate=total_bytes_trans*1000/total_chars_trans;
        if ((pass_number==2)||(bothpasses_mode==1))
        {   printf("Input %d lines (%d statements, %d chars)",
                total_source_line,internal_line,marker_in_file);
            if (total_files_read > 1) { printf(" from %d files",
                total_files_read); }
            printf(
"\nVersion %d (%s) story file\n\
%4d objects (maximum %3d)     %4d dictionary entries (maximum %d)\n\
%4d attributes (maximum %2d)   %4d properties (maximum %2d)\n\
%4d adjectives (maximum 240)  %4d verbs (maximum %d)\n\
%4d actions (maximum %3d)     %4d abbreviations (maximum %d)\n",
                   actual_version, version_name(actual_version),
                   no_objects, ((version_number==3)?255:(MAX_OBJECTS-1)),
                   dict_entries, MAX_DICT_ENTRIES,
                   no_attributes, ((version_number==3)?32:48),
                   no_properties-2, ((version_number==3)?30:62),
                   no_adjectives,
                   no_verbs, MAX_VERBS,
                   no_actions, MAX_ACTIONS,
                   no_abbrevs, MAX_ABBREVS);

            printf(
"%4d globals (maximum 240)     %4d variable space (maximum %d)\n\
%4d symbols (maximum %4d)    %4d routines (maximum %d)\n\
%4d classes (maximum %2d)      %4d fake actions (unlimited)\n\
%4ld characters of text (compressed to %ld bytes, rate 0.%3ld)\n\
Output story file is %3ld%sK long (maximum %ldK)\n",
                   no_globals,
                   globals_size, MAX_STATIC_DATA,
                   no_symbols, MAX_SYMBOLS,
                   no_routines, MAX_ROUTINES,
                   no_classes, MAX_CLASSES,
                   no_fake_actions,
                   (long int) total_chars_trans,
                   (long int) total_bytes_trans,
                   (long int) rate,
                   (long int) k_long, k_str, (long int) limit);
        }
    }

    if ((debugging_file==1)&&(pass_number==2))
    {   debug_pass=2;
        write_debug_byte(13);
        write_debug_address(vars+480);
        write_debug_address(parsat);
        write_debug_address(dict);
        write_debug_address(code);
        write_debug_address(strs);
        debug_pass=1;
    }

    if (offsets_mode==1)
    {   if ((pass_number==2)||(bothpasses_mode==1))
        {   printf(
"\nOffsets in story file:\n\
%05lx Synonyms     %05lx Defaults     %05lx Objects    %05lx Properties\n\
%05lx Variables    %05lx Parse table  %05lx Actions    %05lx Preactions\n\
%05lx Adjectives   %05lx Dictionary   %05lx Code       %05lx Strings\n\n",
   (long int) synsat, (long int) glpat, (long int) objat, (long int) propat,
   (long int) vars, (long int) parsat, (long int) actshere,
   (long int) preactshere, (long int) adjectives_offset, (long int) dict,
   (long int) code, (long int) strs);
        }
    }
    if (memory_map_mode==1)
    {   if ((pass_number==2)||(bothpasses_mode==1))
        {
printf("Dynamic +---------------------+   00000\n");
printf("memory  |       header        |\n");
printf("        +---------------------+   00040\n");
printf("        |   synonym strings   |\n");
printf("        + - - - - - - - - - - +   %05lx\n", (long int) synsat);
printf("        |    synonym table    |\n");
printf("        +---------------------+   %05lx\n", (long int) glpat);
printf("        |  property defaults  |\n");
printf("        + - - - - - - - - - - +   %05lx\n", (long int) objat);
printf("        |      objects        |\n");
printf("        + - - - - - - - - - - +   %05lx\n", (long int) propat);
printf("        | object short names  |\n");
printf("        |   and properties    |\n");
printf("        +---------------------+   %05lx\n", (long int) vars);
printf("        |  global variables   |\n");
printf("        + - - - - - - - - - - +   %05lx\n", ((long int) vars)+480L);
printf("        |       arrays        |\n");
printf("        +=====================+   %05lx\n", (long int) parsat);
printf("Static  |    grammar table    |\n");
printf("cached  + - - - - - - - - - - +   %05lx\n", (long int) actshere);
printf("data    |       actions       |\n");
printf("        + - - - - - - - - - - +   %05lx\n", (long int) preactshere);
printf("        |     preactions      |\n");
printf("        + - - - - - - - - - - +   %05lx\n",
                                              (long int) adjectives_offset);
printf("        |     adjectives      |\n");
printf("        +---------------------+   %05lx\n", (long int) dict);
printf("        |     dictionary      |\n");
printf("        +---------------------+   %05lx\n", (long int) code);
printf("Static  |       Z-code        |\n");
printf("paged   +---------------------+   %05lx\n", (long int) strs);
printf("data    |       strings       |\n");
printf("        +---------------------+   %05lx\n", (long int) Out_Size);
        }
    }
    if (percentages_mode==1)
    {   if ((pass_number==2)||(bothpasses_mode==1))
        {   printf("Approximate percentage breakdown of story file:\n");
            percentage("Z-code",code_length,Out_Size);
            percentage("Static strings",strings_length,Out_Size);
            percentage("Dictionary",code-dict,Out_Size);
            percentage("Objects",vars-glpat,Out_Size);
            percentage("Globals",parsat-vars,Out_Size);
            percentage("Parsing tables",dict-parsat,Out_Size);
            percentage("Header and synonyms",glpat,Out_Size);
            percentage("Total of save area",parsat,Out_Size);
            percentage("Total of text",total_bytes_trans,Out_Size);
        }
    }
    if (frequencies_mode==1)
    {   if ((pass_number==2)||(bothpasses_mode==1))
        {   printf("How frequently abbreviations were used, and roughly\n");
            printf("how many bytes they saved:  ('_' denotes spaces)\n");
            for (i=0; i<no_abbrevs; i++)
            {   sprintf(sub_buffer,
                    (char *)abbreviations_at+i*MAX_ABBREV_LENGTH);
                for (j=0; sub_buffer[j]!=0; j++)
                    if (sub_buffer[j]==' ') sub_buffer[j]='_';
                printf("%10s %5d/%5d   ",sub_buffer,abbrev_freqs[i],
                    2*((abbrev_freqs[i]-1)*abbrev_quality[i])/3);
                if ((i%3)==2) printf("\n");
            }
            if ((i%3)!=0) printf("\n");
            if (no_abbrevs==0) printf("None were declared.\n");
        }
    }
    if (((statistics_mode==1)||(economy_mode==1))&&(pass_number==2))
    {   printf("Essential size %ld bytes: %ld remaining\n",
            (long int) Out_Size,
            (long int) (((long int) (limit*1024L)) - ((long int) Out_Size)));
    }
}

/* ------------------------------------------------------------------------- */
/*  Begin pass                                                               */
/* ------------------------------------------------------------------------- */

extern void tables_begin_pass(void)
{   int i;

    for (i=0; i<256; i++) debug_acts_writ[i]=0;

    dictionary_begin_pass();

    properties_size=0; classes_size=0;

    no_objects=0; no_classes=0; embedded_routine=0;

    no_verbs=0; fp_no_actions=no_actions; no_actions=0; no_adjectives=0;
    no_properties=2; no_attributes=0; no_fake_actions=0;
    no_globals=0; globals_size=0x1e0;

    no_prs=0;

    objects[0].parent=0; objects[0].child=0; objects[0].next=0;

    doing_table_mode = 0;
}

extern void init_tables_vars(void)
{
    no_fake_actions = 0;
    release_number = 1;
    statusline_flag = 0;
    max_no_objects = 0;
    resobj_flag = 0;
    no_verbs = 0;
    no_actions = 0;
    no_adjectives = 0;
    no_objects = 0;
    no_classes = 0;
    classes_size = 0;
    fp_no_actions = 0;
    embedded_routine = 0;
    verbspace_consumed = 0;
    alloc_done_flag = 0;

#ifdef ALLOCATE_BIG_ARRAYS
    vs = NULL;
    objects = NULL;
    table_init = NULL;
    actions = NULL;
    preactions = NULL;
    adjectives = NULL;
    adjcomps = NULL;
    gvalues = NULL;
    dict_places_list = NULL;
    dict_places_back = NULL;
    dict_places_inverse = NULL;
    dict_sorts = NULL;
    class_pointer = NULL;
    classes_here = NULL;
    classes_at = NULL;
#endif
    verblist = NULL;
    output_p = NULL;
}
