/* ------------------------------------------------------------------------- */
/*   "symbols" :  The symbols table                                          */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include "header.h"

int used_local_variable[16];
zip *local_varname[16];
int routine_starts_line;

/* ------------------------------------------------------------------------- */
/*   Symbols table arrays                                                    */
/* ------------------------------------------------------------------------- */

#ifndef ALLOCATE_BIG_ARRAYS
  static char *  symbs[MAX_SYMBOLS];
  int32   svals[MAX_SYMBOLS];
#ifdef VAX
  char    stypes[MAX_SYMBOLS];
#else
  signed char  stypes[MAX_SYMBOLS];
#endif
  static int     bank1_next[MAX_BANK_SIZE];
  static int32   bank1_hash[HASH_TAB_SIZE];
  static int     bank6_next[MAX_BANK_SIZE];
  static int32   bank6_hash[HASH_TAB_SIZE];
  static int     routine_keys[MAX_ROUTINES+1];
#else
  static int32 *  *symbs;
  int32   *svals;
#ifdef VAX
  char    *stypes;
#else
  signed char  *stypes;
#endif
  static int     *routine_keys;
  static int     *bank1_next;
  static int32   *bank1_hash;
  static int     *bank6_next;
  static int32   *bank6_hash;
#endif

static int banksize[7];

static int bank_chunks_made[7];
static int32 *bank_chunks[7][32];

static char** symbs_ptrs;
static int no_symbs_ptrs;

extern void symbols_allocate_arrays(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    symbs      = my_calloc(sizeof(char *),  MAX_SYMBOLS, "symbols");
    svals      = my_calloc(sizeof(int32),   MAX_SYMBOLS, "symbol values");
    stypes     = my_calloc(sizeof(char),    MAX_SYMBOLS, "symbol types");
    bank1_next = my_calloc(sizeof(int),     MAX_BANK_SIZE, "bank 1 next");
    bank1_hash = my_calloc(sizeof(int32),   MAX_BANK_SIZE, "bank 1 hash");
    bank6_next = my_calloc(sizeof(int),     MAX_BANK_SIZE, "bank 6 next");
    bank6_hash = my_calloc(sizeof(int32),   MAX_BANK_SIZE, "bank 6 hash");
    routine_keys = my_calloc(sizeof(int),   MAX_ROUTINES+1,  "routine keys");
    symbs_ptrs = my_calloc(sizeof(char *),  MAX_SYMBOLS/MAX_BANK_SIZE+100,
                                            "symbol pointers");
#endif
}

extern void symbols_free_arrays(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    int i, j;

    for (i=0; i<no_symbs_ptrs; i++)
            my_free(&(symbs_ptrs[i]), "symbol pointer");
    my_free(&symbs_ptrs, "symbol pointer array");

    my_free(&symbs, "symbols");
    my_free(&svals, "symbol values");
    my_free(&stypes, "symbol types");
    my_free(&bank1_next, "bank 1 next");
    my_free(&bank1_hash, "bank 1 hash");
    my_free(&bank6_next, "bank 6 next");
    my_free(&bank6_hash, "bank 6 hash");
    my_free(&routine_keys, "routine keys");

    for (i=0; i<7; i++)
        for (j=0; j<=bank_chunks_made[i]; j++)
            my_free(&(bank_chunks[i][j]), "symbols banks chunk");

    if (memout_mode==1)
    {   for (i=0; i<7; i++)
            printf("Bank %d: %d entries, %d chunks (size %d bytes)\n",
                i,banksize[i],bank_chunks_made[i]+1,
                (bank_chunks_made[i]+1)*BANK_CHUNK_SIZE);
    }

#endif
}

/* ------------------------------------------------------------------------- */
/*   Symbols table and address fixing                                        */
/* ------------------------------------------------------------------------- */

extern void init_symbol_banks(void)
{   int i;

    for (i=0; i<7; i++) { banksize[i]=0; bank_chunks_made[i]=-1; }
    
    for (i=0; i<MAX_ROUTINES+1; i++) routine_keys[i]= -1;
    for (i=0;i<HASH_TAB_SIZE;i++) { bank1_hash[i]= -1; bank6_hash[i]= -1; }
}

static int read_banks(int number, int32 entry)
{   int *x, chunk=-1, i;


    chunk = entry/BANK_CHUNK_SIZE;
    i = entry%BANK_CHUNK_SIZE;

/*    printf("RB %d, entry %d, chunk %d, slot %d\n",
             number, entry, chunk, i);
*/

    if (bank_chunks_made[number] < chunk) return(-1);

    x= (int *) bank_chunks[number][chunk];
    return x[i];
}

static void write_banks(int number, int32 entry, int value)
{   int *x, chunk=-1, i, j, k;

    chunk = entry/BANK_CHUNK_SIZE;
    i = entry%BANK_CHUNK_SIZE;

/*    printf("WB %d, entry %d, chunk %d, slot %d -> %d\n",
             number, entry, chunk, i, value);
*/
    while (bank_chunks_made[number] < chunk)
    {   j=++bank_chunks_made[number];
        if (j==32)
            memoryerror("BANK_CHUNK_SIZE", BANK_CHUNK_SIZE);

        bank_chunks[number][j] = my_calloc(sizeof(int), BANK_CHUNK_SIZE,
                                           "symbols banks chunk");
        for (k=0; k<BANK_CHUNK_SIZE; k++)
            *( (int *)bank_chunks[number][j] + k) = -1;
    }

    x=(int *) bank_chunks[number][chunk];
    x[i] = value;
}

static char reserveds_buffer[32];

extern void prim_new_symbol(char *p, int32 value, int type, int bank)
{   int32 i, j, this, last, key, start; char *r;

    if (p[0]==0) { error("Symbol name expected"); return; }
    if (bank==6)
    {   strcpy(reserveds_buffer,p); p=reserveds_buffer; }
    make_lower_case(p);
    if (bank==0)
    {   start=routine_keys[no_routines]; if (start<0) goto NotDupl;
        for (i=start; i<banksize[0]; i++)
        {   j=read_banks(0,i);
            if (strcmp((char *)symbs[j],p)==0)
            {   error_named("Duplicated symbol name:",p);
                return;
            }
        }
        NotDupl: j=banksize[0]++;
    }
    else
    if (bank==1)
    {   for(r=p,key=0; *r; r++) key=(key*67+*r)%HASH_TAB_SIZE;
        for(this=bank1_hash[key], j= -1, last= -1;
           this!=-1 && (j=strcmp((char *)symbs[read_banks(1,this)],p))<0;
           last=this, this=bank1_next[this]);
        if(!j)
        { if (pass_number==1)
          { error_named("Duplicated symbol name:",p); return;}
          return;
        }
        j=banksize[1]++;
    }
    else
    if (bank==6)
    {   for(r=p,key=0; *r; r++) key=(key*67+*r)%HASH_TAB_SIZE;
        for(this=bank6_hash[key], j= -1, last= -1;
           this!=-1 && (j=strcmp((char *)symbs[read_banks(6,this)],p))<0;
           last=this, this=bank6_next[this]);
        if(!j)
        { if (pass_number==1)
          { error_named("Duplicated symbol name:",p); return;}
          return;
        }
        j=banksize[6]++;
    }
    else
    {   j=atoi(p+2);
        if (read_banks(bank,j)!=-1)
        {   error_named("Duplicated system symbol name:",p);
            return;
        }
    }

    if (j>=MAX_BANK_SIZE)
        memoryerror("MAX_BANK_SIZE", MAX_BANK_SIZE);

    write_banks(bank,j,no_symbols);

    if (bank==0)
    {   if (routine_keys[no_routines]==-1) routine_keys[no_routines]=j;
        if (nowarnings_mode==0)
        {   local_varname[value]=symbols_p;
        }
    }
    if (bank==1) 
    { if (last==-1) {bank1_next[j]=bank1_hash[key];bank1_hash[key]=j;}
      else          {bank1_next[j]=this; bank1_next[last]=j;}
    }
    if (bank==6) 
    { if (last==-1) {bank6_next[j]=bank6_hash[key];bank6_hash[key]=j;}
      else          {bank6_next[j]=this; bank6_next[last]=j;}
    }

    if (no_symbols==MAX_SYMBOLS)
        memoryerror("MAX_SYMBOLS", MAX_SYMBOLS);

    if (symbols_p+strlen(p)+1 >= symbols_top)
    {   symbols_p=my_malloc(SYMBOLS_CHUNK_SIZE,"symbols table chunk");
        symbols_top=symbols_p+SYMBOLS_CHUNK_SIZE;
        symbs_ptrs[no_symbs_ptrs] =  (char *)symbols_p;
        no_symbs_ptrs++;
    }

    strcpy((char *)symbols_p,p); symbs[no_symbols]=(int32 *) symbols_p;
    symbols_p+=strlen((char *)symbols_p)+1;

    svals[no_symbols]=value; stypes[no_symbols]=type;

    no_symbols++;
}

extern int prim_find_symbol(char *q, int bank)
{   char c[50], *r; int i, j, start, finish=banksize[bank];
    int32 key, this;
    if (strlen(q)>49) return -1;
    strcpy(c,q); make_lower_case(c);

    if (bank==0)
    {   start=routine_keys[no_routines]; if (start<0) return -1;
        i=routine_keys[no_routines+1]; if (i>=0) finish=i;
        if (finish>start+15) finish=start+15;
        for (i=start; i<finish; i++)
        {   j=read_banks(0,i);
            if (strcmp((char *) symbs[j],c)==0)
            {   used_local_variable[svals[j]]=1;
                return(j);
            }
        }
        return(-1);
    }
    else
    if (bank==1)
    { for(r=c, key=0; *r; r++) key=(key*67+*r)%HASH_TAB_SIZE;
      for(this=bank1_hash[key],j= -1;
          this!=-1 && (j=strcmp((char *)symbs[read_banks(1,this)],c))<0;
          this=bank1_next[this]);
      if(!j) return read_banks(1,this);
      return(-1);
    }
    else
    if (bank==6)
    { for(r=c, key=0; *r; r++) key=(key*67+*r)%HASH_TAB_SIZE;
      for(this=bank6_hash[key],j= -1;
          this!=-1 && (j=strcmp((char *)symbs[read_banks(6,this)],c))<0;
          this=bank6_next[this]);
      if(!j) return read_banks(6,this);
      return(-1);
    }

    j=atoi(c+2);
    return(read_banks(bank,j));
}

extern int find_symbol(char *q)
{   if (q[0]!='_') return(prim_find_symbol(q,1));
    if (q[1]=='s') return(prim_find_symbol(q,2));
    if (q[1]=='S') return(prim_find_symbol(q,2));
    if (q[1]=='w') return(prim_find_symbol(q,3));
    if (q[1]=='W') return(prim_find_symbol(q,3));
    if (q[1]=='f') return(prim_find_symbol(q,4));
    if (q[1]=='F') return(prim_find_symbol(q,4));
    if (q[1]=='x') return(prim_find_symbol(q,5));
    if (q[1]=='X') return(prim_find_symbol(q,5));
    error("Names are not permitted to start with an _");
    return(-1);
}

extern int local_find_symbol(char *q)
{   return(prim_find_symbol(q,0));
}

extern void new_symbol(char *p, int32 value, int type)
{   if (pass_number==2) return;
    if (strlen(p)>MAX_IDENTIFIER_LENGTH)
    {   error_named("Symbol name is too long:",p);
        return;
    }
    if (type==3) { prim_new_symbol(p,value,type,0); return; }
    if (p[0]!='_') { prim_new_symbol(p,value,type,1); return; }
    if (p[1]=='s') { prim_new_symbol(p,value,type,2); return; }
    if (p[1]=='S') { prim_new_symbol(p,value,type,2); return; }
    if (p[1]=='w') { prim_new_symbol(p,value,type,3); return; }
    if (p[1]=='W') { prim_new_symbol(p,value,type,3); return; }
    if (p[1]=='f') { prim_new_symbol(p,value,type,4); return; }
    if (p[1]=='F') { prim_new_symbol(p,value,type,4); return; }
    if (p[1]=='x') { prim_new_symbol(p,value,type,5); return; }
    if (p[1]=='X') { prim_new_symbol(p,value,type,5); return; }
    error("Symbol names are not permitted to start with an '_'");
}

extern void lose_local_symbols(void)
{   /* For future consideration */
}

/* ------------------------------------------------------------------------- */
/*   Printing diagnostics                                                    */
/* ------------------------------------------------------------------------- */

static char *typename(int type)
{   switch(type)
    {   case 1: return("Global label");
        case 2: return("Global variable");
        case 3: return("Local variable");
        case 4: return("Reserved word");
        case 5: return("Static string");
        case 6: return("Local label");
        case 7: return("Attribute");
        case 8: return("Integer constant");
        case 9: return("Object");
        case 10: return("Condition");
        case 11: return("Constant string address");
        case 12: return("Property");
        case 13: return("Class");
        case 14: return("Assembler directive");
        case 15: return("Compiler-modified opcode");
        case 16: return("Compiled command");
        case 17: return("Opcode");
        case 18: return("Fake action");
        case 19: return("Replacement name");
        case 20: return("Compiler-moved label");
        default: return("(Unknown type)");
    }
}

extern void list_symbols(void)
{   int i, j, k;

    for (j=0; j<2; j++)
    {   printf("In bank %d\n", j);
        for (i=0; i<banksize[j]; i++)
        {   k=read_banks(j,i);
            printf("%4d  %-16s  %04x  %s\n",
                k,symbs[k],svals[k],typename(stypes[k]));
        }
    }
    for (j=2; j<6; j++)
    {   printf("In bank %d\n", j);
        for (i=0; i<MAX_BANK_SIZE; i++)
        {   k=read_banks(j,i);
            if (k!=-1)
            {   printf("%4d  %-16s  %04x  %s\n",
                    k,symbs[k],svals[k],typename(stypes[k]));
            }
        }
    }
    printf("Full list:\n");
    for (i=0; i<no_symbols; i++)
        printf("(%08x) %-16s  %04x  %s\n",
            (int) symbs[i],symbs[i],svals[i],typename(stypes[i]));
}

extern void init_symbols_vars(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    int i,j;
    symbs = NULL;
    svals = NULL;
    stypes = NULL;
    bank1_next = NULL;
    bank1_hash = NULL;
    bank6_next = NULL;
    bank6_hash = NULL;
    routine_keys = NULL;
    symbs_ptrs = NULL;

    for (i=0; i<7; i++)
        for (j=0; j<32; j++)
            bank_chunks[i][j] = NULL;
#endif
    no_symbs_ptrs = 0;
}
