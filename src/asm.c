/* ------------------------------------------------------------------------- */
/*   "asm" : The Inform assembler                                            */
/*                                                                           */
/*   Part of Inform release 5                                                */
/* ------------------------------------------------------------------------- */

#include "header.h"

int in_routine_flag, no_stubbed;

int ignoring_routine;

int return_flag;
static int stub_flags[32];

static int lower_strings_flag=0;

static int ifdef_stack[MAX_IFDEF_DEPTH];
static int ifdef_sp=0;
static int started_ignoring=0;

#ifndef ALLOCATE_BIG_ARRAYS
  static long int pa_forwards[MAX_FORWARD_REFS];
#else
  static long int *pa_forwards;
#endif

extern void asm_allocate_arrays(void)
{
#ifdef ALLOCATE_BIG_ARRAYS
    pa_forwards = (long int *) my_calloc(sizeof(long int),   MAX_FORWARD_REFS,
                  "forward references");
#endif
}

extern void asm_free_arrays(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    my_free(&pa_forwards, "forward references");
#endif
}

/* ------------------------------------------------------------------------- */
/*   Decode arguments as constants and variables                             */
/*   (Gratuitous Space:1999 reference by Mr Dilip Sequeira of Edinburgh      */
/*   University)                                                             */
/* ------------------------------------------------------------------------- */

static int constant_was_string;

extern int32 constant_value(char *b)
{   int32 i, j, k, action_flag=0, rv, base=10, moon, alpha, victor=0, eagle;

    constant_was_string=0;
    if (b[0]=='#') b++;

    if (b[0]=='\'')
    {   if ((b[2]=='\'') && (b[3]==0)) return(translate_to_ascii(b[1]));
        i=strlen(b);
        if ((i>=4)&&(b[i-1]=='\''))
        {   b[i-1]=0; rv=dictionary_add(b+1,0x80,0,0); return(rv);
        }
    }
    if (b[0]=='#')
    {   i=find_symbol(b+1);
        if ((i>=0)&&(stypes[i]==18)) return(svals[i]);
        if (strlen(b)>60)
        {   error_named("Action name over 60 characters long:",b);
            return(0);
        }
        sprintf(b+strlen(b),"Sub");
        action_flag=1; b++; goto actionfound;

    }
    if (b[0]=='\"') goto stringfound;

    if (*b=='$')
    {   if (*++b=='$') { b++; base=2; }
        else base=16;
    }
    else if (*b=='-')  { b++; victor=1; }
    else if (b[1]=='$') goto dollarform;
    else for (i=0; b[i]; i++) if (!isdigit(b[i])) goto nonumber;

    for(moon=0;*b;b++) 
    {  alpha=isalpha(*b)?(tolower(*b)-'a'+10):*b-'0';
       if(alpha>=base || alpha<0) break; else moon=moon*base+alpha;
    }
    if (victor==0) return(moon);
    eagle=((int32) 0x10000L)-moon;
    return(eagle);

  dollarform:
    switch(b[0])
    {   case 'a':
            obsolete_warning("'#a$Action' is now superceded by '##Action'");
            b+=2; action_flag=1; goto actionfound;
        case 'w':
            obsolete_warning("'#w$word' is now superceded by ''word''");
            k=dictionary_find(b+2,2);
            rv=dictionary_offset+7+((version_number==3)?7:9)*(k-1);
            if ((k==0)&&(pass_number==2))
                error_named("Dictionary word not found for constant",b);
            return(rv);
        case 'n':
            if (strlen(b)>3)
                obsolete_warning("'#n$word' is now superceded by ''word''");
            rv=dictionary_add(b+2,0x80,0,0); return(rv);
        case 'r':
            i=find_symbol(b+2);
            if ((i<0)||(stypes[i]!=1))
            {   if (pass_number==2)
                    error_named("No such routine as",b);
                return(256);
            }
            rv=svals[i]; goto constantfound;
    }
    error_named("There is no such # constant form as",b);
    return(0);

  nonumber:

    for (j=0,i=0; b[i]; i++) if (b[i]=='_') j=1;

    if (j==1)
    {   if (strcmp(b,"adjectives_table")==0) return(adjectives_offset);
        if (strcmp(b,"preactions_table")==0) return(preactions_offset);
        if (strcmp(b,"actions_table")==0) return(actions_offset);
        if (strcmp(b,"version_number")==0) return(actual_version);
        if (strcmp(b,"largest_object")==0) return(256+max_no_objects-1);
        if (strcmp(b,"strings_offset")==0) return(strings_offset/scale_factor);
        if (strcmp(b,"code_offset")==0) return(code_offset/scale_factor);
        if (strcmp(b,"dict_par1")==0) return((version_number==3)?4:6);
        if (strcmp(b,"dict_par2")==0) return((version_number==3)?5:7);
        if (strcmp(b,"dict_par3")==0) return((version_number==3)?6:8);
    }

  actionfound:
    i=find_symbol(b);
    if (i<0)
    {   if (pass_number==2)
        {   if (action_flag==1)
                error_named("No such action routine as",b);
            else
                error_named("No such constant as",b);
        }
        return(0);
    }
    rv=svals[i];

  constantfound:
    switch(stypes[i])
    {   case 1: rv=(rv+code_offset)/scale_factor;
                break;
        case 2:
        case 3: error_named("Not a constant:",b); return(0);
        case 4:
        case 10: error_named("Reserved word as constant:",b); return(0);
    }
    if (action_flag==0) return(rv);
    j=find_action(svals[i]);
    return(j);

  stringfound:

    dequote_text(b);
    constant_was_string=1;

    if (lower_strings_flag==1)
    {   j=subtract_pointers(low_strings_p,low_strings);
        low_strings_p=translate_text(low_strings_p,b);
        i= subtract_pointers(low_strings_p,low_strings);
        if (i>MAX_LOW_STRINGS)
            memoryerror("MAX_LOW_STRINGS", MAX_LOW_STRINGS);
        return(0x21+(j/2));
    }

    j= subtract_pointers(strings_p,strings);

#ifdef USE_TEMPORARY_FILES
    {   zip *c;
        c=translate_text(strings,b);
        i=subtract_pointers(c,strings);
        strings_p+=i;
        if (pass_number==2)
            for (c=strings; c<strings+i; c++)
            {   fputc(*c,Temp1_fp); add_to_checksum((void *) c);
            }
    }
#else
    strings_p=translate_text(strings_p,b);
    i=subtract_pointers(strings_p,strings);
    if (i>MAX_STATIC_STRINGS)
        memoryerror("MAX_STATIC_STRINGS",MAX_STATIC_STRINGS);
#endif

    while ((i%scale_factor)!=0)
    {   i+=2;
#ifdef USE_TEMPORARY_FILES
        strings_p+=2;
        if (pass_number==2) { fputc(0,Temp1_fp); fputc(0,Temp1_fp); }
#else
        *(strings_p++)=0; *(strings_p++)=0;
#endif
    }

    return((strings_offset+j)/scale_factor);
}

/* ------------------------------------------------------------------------- */
/*   parse_argument returns: 1000+n   Constant value n                       */
/*                                    long_form_flag set if long form vital  */
/*                                n   Variable n                             */
/* ------------------------------------------------------------------------- */

static char known_unknowns[MAX_IDENTIFIER_LENGTH*16];
static int  no_knowns;

static long int pa_id;
static int pa_ref;
static int max_pa_ref;
static int long_form_flag;

static char one_letter_locals[16];

extern void args_begin_pass(void)
{   if (pass_number==2) max_pa_ref=pa_ref;
    pa_ref=0; pa_id=0;
}

static int32 parse_argument(char *b)
{   int i, j, flag=0;
    long_form_flag=0;
    i=b[0];
    if (b[1]==0)
    {   for (j=0;j<16;j++)
            if (i==one_letter_locals[j])
            {   used_local_variable[j]=1;
                return(j+1);
            }
        if (i=='0') return(1000);
        if (i=='1') return(1001);
        if (i=='2') return(1002);
        if (i=='3') return(1003);
        if (i=='4') return(1004);
        if (i=='5') return(1005);
        if (i=='6') return(1006);
        if (i=='7') return(1007);
        if (i=='8') return(1008);
        if (i=='9') return(1009);
    }
    if ((i=='#')||(i=='-')||(i=='$')||(i=='\"')||(i=='\''))
        return(1000+constant_value(b));
    for (i=0; b[i]!=0; i++)
        if (isdigit(b[i])==0) { flag=1; break; }
    if (flag==0) return(1000+constant_value(b));
    if (in_routine_flag==1)
    {   i=local_find_symbol(b);
        if (i>=0) return(1+svals[i]);
    }
    pa_id++;

    i=find_symbol(b);
    if (i>=0)
    {   switch(stypes[i])
        {   case 1: return(1000+svals[i]);
            case 2: return(16+svals[i]);
            case 4: if (svals[i]==0) return(0);
            case 7:
            case 12:
            case 13:
            case 18:
            case 8:
            case 11:
            case 9: if (pass_number==2)
                    {   while ((pa_ref<max_pa_ref)
                           &&(pa_id>pa_forwards[pa_ref])) pa_ref++;
                        if ((pa_ref<max_pa_ref)
                            &&(pa_id==pa_forwards[pa_ref]))
                        {  long_form_flag=1;
/*  printf("'%s' ref %d id %d seems forward\n",b,pa_ref, pa_id); */
                        }
                    }
                    return(1000+constant_value(b));
            default: error_named("Type mismatch in argument",b);
              return(0);
        }
    }

    if (pass_number==1)
    {   if (pa_ref==MAX_FORWARD_REFS)
            memoryerror("MAX_FORWARD_REFS", MAX_FORWARD_REFS);
        pa_forwards[pa_ref++]=pa_id;
/*  printf("'%s' ref %d id %d is forward\n",b,pa_ref, pa_id); */
        return(1256);
    }

    for (i=0; i<no_knowns; i++)
        if (strcmp(known_unknowns+i*MAX_IDENTIFIER_LENGTH,b)==0)
            return(0);
    if (no_knowns<16)
        strcpy(known_unknowns+(no_knowns++)*MAX_IDENTIFIER_LENGTH,b);
    error_named("No such variable as",b); return(0);
}

/* ------------------------------------------------------------------------- */
/*   Assembler of individual lines                                           */
/* ------------------------------------------------------------------------- */

static void byteout(int32 i)
{   *zcode_p=(unsigned char) i; zcode_p++;
#ifdef USE_TEMPORARY_FILES
    utf_zcode_p++;
#endif
    if (subtract_pointers(zcode_p,zcode) >= MAX_ZCODE_SIZE)
    {   memoryerror("MAX_ZCODE_SIZE",MAX_ZCODE_SIZE);
    }
}

static void write_operand(operand_t op)
{   int32 j;
    j=op.value;
    if (op.type!=0) byteout(j);
    else { byteout(j/256); byteout(j%256); }
}

static operand_t parse_operand(int32 number, opcode opco, char *b)
{   int32 j; int opt, type=opco.type1; operand_t oper;

    word(b,number+2);

    if (((type==CALL)||(type==NCALL))&&(number==0))
    {   if (pass_number==2)
        {   
            j=find_symbol(b);
            if (j==-1) no_such_label(b);
            else if (stypes[j]!=1) error_named("Not a label:",b);
            else oper.value=(code_offset+svals[j])/scale_factor;
            oper.type=0;
        }
        else
        {   oper.value=0x1000; oper.type=0;
        }
        return(oper);
    }

         if ((b[0]=='s')&&(b[1]=='p')&&(b[2]==0)) j=0;
    else j=parse_argument(b);

    if (j>=1256) { opt=0; j=j-1000; }
    else if (j>=1000) { opt=1; j=j-1000; }
    else opt=2;

    if ((opt==1) && (long_form_flag==1)) opt=0;

    if ((opco.type2==VAR)&&(opt==2)&&(number==0)) opt=1;

    oper.value=j; oper.type=opt;
    return(oper);
}

int a_from_one=0;
int line_done_flag = 0;

extern int assemble_opcode(char *b, int32 offset, int internal_number)
{
    zip *opc, *opcname, *ac;
    int32 j, topbits, addr, cargs, ccode, ccode2, oldccode, oldccode2,
          multi, mask, flag, longf, branchword, offset_here;
    operand_t oper1, oper2;
    opcode opco;

    opco = opcs(internal_number);

    if (opco.type1==INVALID)
    {   if (version_number==3)
            warning_named("Ignoring Advanced-game opcode",b);
        else
            warning_named("Ignoring Standard-game opcode",b);
        return(1);
    }
    return_flag=0;
    if ((opco.type1==RETURN) || (opco.type1==JUMP)) return_flag=1;
    if (opco.type1==INDIR)
    {   byteout(0xE0); byteout(0xBF); byteout(0);  byteout(0);
        goto Line_Done;
    }
    opcname=opco.name;
    switch(opco.no)
    {   case MANY:
        case VARI: topbits=0xc0; break;
        case ZERO: topbits=0xb0; break;
        case ONE:  topbits=0x80; break;
        case TWO:  topbits=0x00; break;
    }

    opc=zcode_p;

    if (opco.no!=EXTD)
    {   byteout(topbits+opco.code);
    }
    else
    {   byteout(0xbe); byteout(opco.code); opco.no=VARI;
    }

    if (opco.type1==JUMP)
    {   word(b,2);
        if (pass_number==1) addr=0;
        else
        {   j=find_symbol(b);
            if (j<0) { no_such_label(b); return(1); }
            if ((stypes[j]!=6)&&(stypes[j]!=20))
            { error_named("Not a label:",b); return(1); }
            addr=svals[j]-offset-1;
            if (addr<0) addr+=(int32) 0x10000L;
        }
        byteout(addr/256); byteout(addr%256);
        goto Line_Done;
    }

    if (opco.type2==TEXT)
    {   zip *tmp;
        if (a_from_one==1) textword(b,1); else textword(b,2);
        tmp=zcode_p; zcode_p=translate_text(zcode_p,b);
        j=subtract_pointers(zcode_p,tmp);
#ifdef USE_TEMPORARY_FILES
        utf_zcode_p+=j;  
#endif
        goto Line_Done;
    }

    switch(opco.no)
    {   case VARI:
            ac=zcode_p; byteout(0);
            cargs= -1; ccode=0xff;
            while (word(b,(++cargs)+2),(b[0]!=0))
            {   if (b[0]=='?') { branchword=cargs+2; break; }
                switch(cargs)
                {   case 0: multi=0x40; mask=0xc0; break;
                    case 1: multi=0x10; mask=0x30; break;
                    case 2: multi=0x04; mask=0x0c; break;
                    case 3: multi=0x01; mask=0x03; break;
                    case 4: multi=0;
                            if ((opco.type1!=CALL)&&(opco.type1!=STORE))
                                error("Too many arguments");
                            break;
                    default: error("Too many arguments"); break;
                }

                oper1=parse_operand(cargs, opco, b);

                write_operand(oper1);
                oldccode=ccode; ccode = (ccode & (~mask)) + oper1.type*multi;
            }
            if ((opco.type1==CALL)||(opco.type1==STORE))
            {   if (oper1.type!=2)
                { error("Can't store to that (no such variable)"); }
                *ac=oldccode;
            }
            else *ac=ccode;
            break;
        case MANY:
            ac=zcode_p; byteout(0); byteout(0);
            cargs= -1; ccode=0xff; ccode2=0xff;
            while (word(b,(++cargs)+2),(b[0]!=0))
            {   if (b[0]=='?') { branchword=cargs+2; break; }
                switch(cargs)
                {   case 0: case 4: multi=0x40; mask=0xc0; break;
                    case 1: case 5: multi=0x10; mask=0x30; break;
                    case 2: case 6: multi=0x04; mask=0x0c; break;
                    case 3: case 7: multi=0x01; mask=0x03; break;
                    case 8: multi=0;
                            if ((opco.type1!=CALL)&&(opco.type1!=STORE))
                                error("Too many arguments");
                            break;
                    default: error("Too many arguments"); break;
                }

                oper1=parse_operand(cargs, opco, b);

                write_operand(oper1);
                oldccode=ccode; oldccode2=ccode2;
                if (cargs<4)
                    ccode = (ccode & (~mask)) + oper1.type*multi;
                else
                    ccode2 = (ccode2 & (~mask)) + oper1.type*multi;
            }
            if ((opco.type1==CALL)||(opco.type1==STORE))
            {   if (oper1.type!=2)
                { error("Can't store to that (no such variable)"); }
                *ac=oldccode; *(ac+1)=oldccode2;
            }
            else { *ac=ccode; *(ac+1)=ccode2; }
            break;
        case ONE:
            oper1=parse_operand(0,opco,b);
            *opc=(*opc) + oper1.type*0x10;
            write_operand(oper1);
            break;
        case TWO:
            oper1=parse_operand(0,opco,b);
            oper2=parse_operand(1,opco,b);

            if ((oper1.type==0)||(oper2.type==0))
            {   *opc=(*opc) + 0xc0;
                byteout(oper1.type*0x40 + oper2.type*0x10 + 0x0f);
            }
            else
            {   if (oper1.type==2) *opc=(*opc) + 0x40;
                if (oper2.type==2) *opc=(*opc) + 0x20;
            }
            write_operand(oper1);
            write_operand(oper2);
            break;                    
        case ZERO:
            break;
    }

    if ((opco.no==ONE) || (opco.no==TWO))
    {   if ((opco.type1==STORE)||(opco.type1==CALL))
        {   if (opco.no==ONE) oper1=parse_operand(1,opco,b);
            if (opco.no==TWO) oper1=parse_operand(2,opco,b);
            if (oper1.type!=2)
            { error("Can't store to that (no such variable)"); }
            byteout(oper1.value);
        }
    }

    if ((opco.type1==BRANCH)||(opco.type2==OBJECT))
    {   int o=0; int32 pca;
        pca= subtract_pointers(zcode_p,opc) -1;
        switch(opco.no)
        {   case ZERO: word(b,2); break;
            case ONE:  if (opco.type2!=OBJECT) { word(b,3); break; }
            case TWO:  word(b,4); break;
            case VARI: word(b,branchword); break;
        }
        if (b[0]=='?') { longf=1; o++; }
        else
        {   int o2=0;
            if (b[0]=='~') o2=1;
            if (pass_number==1)
            {   j=find_symbol(b+o2);
                if (j<0) longf=0;
                else longf=1;
            }
            else
            {   j=find_symbol(b+o2);
                if (j<0) longf=0;
                else
                {   if (offset-svals[j]>0) longf=1;
                    else longf=0;
                    if ((svals[j]-offset-pca)>30)
                    { error("Branch too far forward: use '?'"); return(1); }
                }
            }
        }
        /* printf("Branch at %04x has longf=%d\n",offset,longf);  */
        if (pass_number==1) { byteout(0); if (longf==1) byteout(0); }
        else
        {   if (b[o]=='~') { flag=0; o++; } else flag=1;
            j=find_symbol(b+o);
            if (j<0) { no_such_label(b+o); return(1); }
            switch(stypes[j])
            {   case 4:
                  switch(svals[j])
                  {   case 1: addr=1; longf=0; break;
                      case 2: addr=0x20; longf=0; break;
                      default: error("No such return condition for branch");
                               return(1);
                  }
                  break;
                case 1: error("Can't branch to a routine, only to a label");
                        return(1);
                case 6:
                case 20:
                  if (longf==1) pca++;
                  addr=svals[j]-offset-pca;
                  if (addr<0) addr+=(int32) 0x10000L; break;
                default: error_named("Not a label:",b+o); return(1);
            }
            addr=addr&0x3fff;
            if (longf==1)
            {   byteout(flag*0x80 + addr/256); byteout(addr%256); }
            else
                byteout(flag*0x80+ 0x40 + (addr&0x3f));
        }
    }

    Line_Done:

    if (trace_mode==1)
    {   printf("%04d %05lx  %-14s  ", current_source_line(),
            ((long int) offset)+((long int) code_offset),opcname);
        for (j=0;opc<zcode_p; j++, opc++)
        {   printf("%02x ", *opc);
            if (j%16==15) printf("\n                           ");
        }
        printf("\n");
    }

    if ((debugging_file==1)&&(pass_number==2)
        &&(line_done_flag==0))
    {   debug_pass=2; line_done_flag=1;
        write_debug_byte(10); write_debug_linenum();
        offset_here = offset+code_offset;
        write_debug_byte((int)((offset_here/256)/256));
        write_debug_byte((int)((offset_here/256)%256));
        write_debug_byte((int)(offset_here%256));
        debug_pass=1;
    }

#ifdef USE_TEMPORARY_FILES
      {
        zip *c;
        int32 i;
        i= subtract_pointers(zcode_p,zcode);
        if (pass_number==2)
        {   for (c=zcode; c<zcode+i; c++)
            {   fputc(*c,Temp2_fp);
                add_to_checksum((void *) c);
            }
        }
        zcode_p=zcode;
      }
#endif

    return(1);
}

/* ------------------------------------------------------------------------- */
/*   Making attributes and properties (incorporating "alias" code suggested  */
/*   by Art Dyer)                                                            */
/* ------------------------------------------------------------------------- */

static void trace_s(char *name, int32 code, int f)
{   if (debugging_file==1)
    {   write_debug_byte(5+f); write_debug_byte((int)code);
        write_debug_string(name);
    }
    if ((printprops_mode==0)||(pass_number==2)) return;
    printf("%s  %02ld  ",(f==0)?"Attr":"Prop",(long int) code);
    if (f==0) printf("  ");
    else      printf("%s%s",(prop_longflag[code]==1)?"L":" ",
                            (prop_additive[code]==1)?"A":" ");
    printf("  %s\n",name);
}

static void make_attribute(char *b)
{   int i;

    word(b,3);
    if (strcmp(b,"alias")==0)
    {   
        word(b,4);
        if (b[0]==0)
        {   error("Expected an attribute name after 'alias'");
            return;
        }
        if (((i=find_symbol(b))==-1)||(stypes[i]!=7))
        {   error_named("'alias' refers to undefined attribute",b);
            return;
        }
        word(b,2);
        trace_s(b,svals[i],0);
        new_symbol(b,svals[i],7);
        return;
    }
    InV3
    {   if (no_attributes==32)
        {   error("All 32 attributes already declared (compile as Advanced \
game to get an extra 16)"); return; }
    }
    InV5
    {   if (no_attributes==48)
        {   error("All 48 attributes already declared"); return; }
    }
    word(b,2);
    trace_s(b,no_attributes,0);
    new_symbol(b,no_attributes++,7);
    return;
}

static void make_property(char *b)
{   int32 def=0;
    int i, fl, addflag=0;

    fl=2;
    do
    {   word(b,fl++);
        if (strcmp(b,"long")==0)
            obsolete_warning("all properties are now automatically 'long'");
        else if (strcmp(b,"additive")==0) addflag=1;
        else break;
    } while (1==1);

    word(b,fl);
    if (strcmp(b,"alias")==0)
    {   if (addflag)
        {   error("'alias' incompatible with 'additive'"); return; }
        word(b,4);
        if (b[0]==0)
        {   error("Expected a property name after 'alias'"); return; }
        i=find_symbol(b);
        if (((i=find_symbol(b))==-1)||(stypes[i]!=12))
        {   error_named("'alias' refers to undefined property",b);
            return;
        }
        word(b,2);
        trace_s(b,svals[i],1);
        new_symbol(b,svals[i],12);
        return;
    }
    InV3
    {   if (no_properties==32)
        { error("All 30 properties already declared (compile as Advanced \
game to get an extra 32)"); return; }
    }
    InV5
    {   if (no_properties==64)
        { error("All 62 properties already declared"); return; }
    }
    if (b[0]!=0) def=constant_value(b);
    word(b,fl-1);
    prop_defaults[no_properties]=def;
    InV3 { prop_longflag[no_properties]=1; }
    InV5 { prop_longflag[no_properties]=1; }
    prop_additive[no_properties]=addflag;
    trace_s(b,no_properties,1);
    new_symbol(b,no_properties++,12);
    return;
}

static void make_fake_action(char *b)
{   int i;
    i=256+no_fake_actions++;
    word(b,2);
    new_symbol(b,i,18);
    new_action(b,i);
    if (debugging_file==1)
    {   write_debug_byte(7); write_debug_byte(i);
        write_debug_string(b);
    }
    return;
}

/* ------------------------------------------------------------------------- */
/*   Assembler directives: for diagnosis, and making the non-code part of    */
/*     the file                                                              */
/* ------------------------------------------------------------------------- */

static int lmoved_flag=0;

extern void assemble_label(int32 offset, char *b)
{   int i;
    if (pass_number==1) new_symbol(b,offset,6);
    else
    {   i=find_symbol(b);
        if ((stypes[i]==6)&&(svals[i]!=offset))
            if (lmoved_flag==0)
            {   lmoved_flag=1;
                if (no_errors==0)
    error("An error has occurred just before this point \
with what Inform thought was a constant.  Perhaps it was a global \
variable used before its definition in the code.");
            }
    }
    if (trace_mode==1) printf(".%s\n",b);
    return_flag=0;
}

static void stack_sline(char *s1, char *b)
{   char rw[100];
    sprintf(rw,s1,b); stack_line(rw);
}

static int constant_made_flag = 0;

extern void assemble_directive(char *b, int32 offset, int32 code)
{   int i, j, condition=0, debugged_routine; int32 k;

  switch(code)
  { 
    case IFDEF_CODE:
        word(b,2);
        if ((b[0]=='V')&&(b[1]=='N')&&(b[2]=='_')&&(strlen(b)==7))
        {   i=atoi(b+3); if (VNUMBER>=i) condition=1;
            goto generic_if;
        }
        i=find_symbol(b);
        if (i>=0) condition=1;
        goto generic_if;
    case IFNDEF_CODE:
        word(b,2);
        if ((b[0]=='V')&&(b[1]=='N')&&(b[2]=='_')&&(strlen(b)==7))
        {   i=atoi(b+3); if (VNUMBER<i) condition=1;
            goto generic_if;
        }
        i=find_symbol(b);
        if (i==-1) condition=1;
        goto generic_if;
    case IFV3_CODE:
        if (version_number==3) condition=1; goto generic_if;
    case IFV5_CODE:
        if (version_number==5) condition=1; goto generic_if;

    generic_if:

        if (ifdef_sp==MAX_IFDEF_DEPTH)
        { error("'#IF' nested too deeply: increase #define MAX_IFDEF_DEPTH");
          return;
        }
        ifdef_stack[ifdef_sp++]=ignoring_mode;
        if (ignoring_mode==0)
        {   ignoring_mode=1-condition;
            started_ignoring=ifdef_sp;
        }
        return;

    case ENDIF_CODE:
        if (ifdef_sp==0)
        { error("'#ENDIF' without matching '#IF'"); return; }
        ignoring_mode=(ifdef_stack[--ifdef_sp])%2;
        return;

    case IFNOT_CODE:
        if (ifdef_sp==0)
        { error("'#IFNOT' without matching '#IF'"); return; }
        if (ifdef_stack[ifdef_sp-1]>=2)
        { error("Two '#IFNOT's in the same '#IF'"); return; }
        ifdef_stack[ifdef_sp-1]+=2;
        if (ignoring_mode==1)
        {   if (ifdef_sp==started_ignoring)
                ignoring_mode=0;
        }
        else
        {   ignoring_mode=1;
        }
        return;

    case END_CODE:
        endofpass_flag=1;
        if (trace_mode==1) printf("<end>\n");

        if (ifdef_sp>0)
        { error("End of file reached inside '#IF...'"); }
        break;

    case OPENBLOCK_CODE:
        if (is_systemfile()==1)
        {   strcpy(b,"Rep__");
            word(b+5,2);
            i=find_symbol(b);
            if (i!=-1)
            {   ignoring_routine=1; /* printf("Ignoring %s\n",b+5); */ break; }
        }
        word(b,2);
        ignoring_routine=0; debugged_routine=0;
        while ((offset%scale_factor)!=0) { byteout(0); offset++; }
        new_symbol(b,offset,1);
        if (trace_mode==1) printf("<Routine %d, '%s' begins at %04x; ",
          no_routines,b,offset);

        if (debugging_file==1)
        {   debug_pass=2;
            write_debug_byte(11);
            write_debug_byte(no_routines/256);
            write_debug_byte(no_routines%256);
            write_routine_linenum(); write_debug_string(b);
        }

        lose_local_symbols();

        no_locals= -1; no_knowns=0;
        for (i=0;i<16;i++) one_letter_locals[i]='\'';
        routine_starts_line=current_source_line();
        no_routines++;
        if (no_routines>=MAX_ROUTINES)
            memoryerror("MAX_ROUTINES", MAX_ROUTINES);
        in_routine_flag=1; return_flag=0;
        word(b,3); if (strcmp(b,"*")==0) debugged_routine=1;
        while (word(b,(++no_locals)+3+debugged_routine),(b[0]!=0))
        {   if ( ((b[0]==',')&&(b[1]==0)) || ((b[0]==':')&&(b[1]==0)))
            {   error("Expected local variable but found ',' or ':' \
(probably the ';' after the '[ ...' line was forgotten)");
                break;
            }
            if ((b[0]=='s')&&(b[1]=='p')&&(b[2]==0))
            {   error("'sp' means the stack pointer, and can't be used \
as the name of a local variable"); break;
            }
            if (b[1]==0) one_letter_locals[no_locals]=b[0];
            new_symbol(b,no_locals,3);
            if (debugging_file==1) write_debug_string(b);
        }

        if (debugging_file==1) { write_debug_byte(0); debug_pass=1; }
        byteout(no_locals);

        if (actual_version<5)
        {   for (i=0; i<no_locals; i++) { byteout(0); byteout(0); }
        }
        if (trace_mode==1) printf("%d locals>\n",no_locals);
        if (no_locals>15) error("Routine has more than 15 local variables");
        if ((no_routines==1)&&(pass_number==1))
        {   word(b,2); make_lower_case(b);
            if (strcmp(b,"main")!=0)
            {
warning_named("Since it is defined before inclusion of the library, game-play \
will begin not at 'Main' but at the routine",b);
            }
            if (no_locals!=0)
 error("The earliest-defined routine is not allowed to have local variables");
        }
        if (nowarnings_mode==0)
            for (i=0; i<16; i++) used_local_variable[i]=0;

        if ((withdebug_mode==1) || (debugged_routine==1))
        {   word(b,2); sprintf(sub_buffer,"print \"[%s\"",b);
            for (i=0; (i<no_locals)&&(i<3); i++)
            {   word(b, 3+i+debugged_routine);
               sprintf(sub_buffer+strlen(sub_buffer), ", \", %s=\", %s", b, b);
            }
            sprintf(sub_buffer+strlen(sub_buffer), ", \"]^\"");
            stack_line(sub_buffer);
        }
        break;

    case CLOSEBLOCK_CODE:

        word(b,2);
        if (b[0]!=0)
        {   if ((resobj_flag==0) || (strcmp(b,",")!=0))
            {
                if (resobj_flag>0)
                error_named(
                    "Expected ',' or ';' after ']' but found",
                    parse_buffer);
                else
                error_named(
                    "Expected ';' after ']' but found",
                    parse_buffer);
            }
        }

        if (trace_mode==1)  printf("<Routine ends>\n");
        if (debugging_file==1)
        {   debug_pass=2;
            write_debug_byte(14);
            write_debug_byte((no_routines-1)/256);
            write_debug_byte((no_routines-1)%256);
            write_re_linenum(); debug_pass=1;
        }

        if (return_flag==0)
        {   if (resobj_flag==0) stack_line("@rtrue");
                           else stack_line("@rfalse");
        }
        if (resobj_flag==1) resobj_flag=3;

        if (brace_sp>0)
        {   error("Brace mismatch in previous routine");
            brace_sp=0;
        }
        in_routine_flag=0;
        if ((nowarnings_mode==0)&&(pass_number==1))
        {   for (i=0; i<no_locals; i++)
            {   if (used_local_variable[i]==0)
                {   override_error_line = routine_starts_line;
                    warning_named("Local variable unused:",
                      (char *) (local_varname[i]));
                }
            }
        }
        break;

    case ABBREVIATE_CODE:
        i=2;
        while (1>0)
        {  word(b,i); if (b[0]==0) break; textword(b,i);
           if (pass_number==1)
           {   if (no_abbrevs==MAX_ABBREVS)
               {   error("Too many abbreviations declared"); break; }
               if (almade_flag==1) 
               {   error("All abbreviations must be declared together");
                   break;
               }
               if (strlen(b)<2)
               {   error_named("It's not worth abbreviating",b); break; }
           }
           strcpy((char *)abbreviations_at+no_abbrevs*MAX_ABBREV_LENGTH, b);
           word(b,i++);
           abbrev_mode=0; lower_strings_flag=1;
           abbrev_values[no_abbrevs]=constant_value(b);
           abbrev_quality[no_abbrevs++]=trans_length-2;
           abbrev_mode=1; lower_strings_flag=0;
        }
        break;

    case ATTRIBUTE_CODE:
        make_attribute(b); break;

    case CONSTANT_CODE:
        constant_made_flag=1;
        word(b,3); k=constant_value(b);
        word(b,2);
        IfPass2
        {   i=find_symbol(b); svals[i]=k; }
        else
        {   if (constant_was_string==1) new_symbol(b,k,11);
            else new_symbol(b,k,8);
        }
        break;

    case LOWSTRING_CODE:
        word(b,3);
        lower_strings_flag=1;
        k=constant_value(b);
        lower_strings_flag=0;
        word(b,2);
        IfPass2
        {   i=find_symbol(b);
        }
        else
        {   if (constant_was_string==1) new_symbol(b,k,11);
            else new_symbol(b,i,8);
        }
        break;

    case DEFAULT_CODE:
        word(b,3);
        IfPass2
        {
        }
        else
        {   i=constant_value(b);
            word(b,2);
            if (find_symbol(b)==-1)
            {   if (constant_was_string==1)
                { error("Defaulted constants can't be strings"); return; }
                new_symbol(b,i,8);
            }
        }
        break;

    case STUB_CODE:
        i=0;
        IfPass2 { if (stub_flags[no_stubbed++]==1) i=1; }
        else
        {   word(b,2); if (find_symbol(b)==-1) i=1;
            stub_flags[no_stubbed++]=i;
        }
        if (i==1)
        {
            word(b,3); i=constant_value(b); word(b,2);
            switch(i)
            {   case 0: stack_sline("[ %s",b); stack_line("rfalse");
                        stack_line("]"); break;
                case 1: stack_sline("[ %s x1",b); stack_line("@store x1 0");
                        stack_line("rfalse"); stack_line("]"); break;
                case 2: stack_sline("[ %s x1 x2",b); stack_line("@store x1 0");
                        stack_line("@store x2 0");
                        stack_line("rfalse"); stack_line("]"); break;
                case 3: stack_sline("[ %s x1 x2 x3",b);
                        stack_line("@store x1 0"); stack_line("@store x2 0");
                        stack_line("@store x3 0");
                        stack_line("rfalse"); stack_line("]"); break;
                default:
                    error("Must specify 0 to 3 variables in 'stub' routine");
                        return;
            }
        }
        break;

    case DICTIONARY_CODE:
        obsolete_warning("use 'word' as a constant dictionary address");
        textword(b,3); i=dictionary_add(b,4,0,0);
        word(b,2);
        IfPass2
        { j=find_symbol(b); svals[j]=i; } else new_symbol(b,i,8);
        break;

    case FAKE_ACTION_CODE:
        make_fake_action(b); break;

    case GLOBAL_CODE: make_global(b,0); break;

    case ARRAY_CODE: make_global(b,1); break;

    case INCLUDE_CODE:
        word(b,3);
        if (b[0]!=0)
            error_named("Expected ';' after 'include <file>' but found",b);
        word(b,2);
        textword(b,2);
        if (b[0]!='>') load_sourcefile(b,0);
        else load_sourcefile(b+1,1);
        break;

    case NEARBY_CODE: make_object(b,1); break;

    case OBJECT_CODE: make_object(b,0); break;

    case CLASS_CODE: make_class(b); break;

    case PROPERTY_CODE: make_property(b); break;

    case RELEASE_CODE:
        word(b,2); release_number=constant_value(b); break;
        
    case SWITCHES_CODE:
        if (ignoreswitches_mode==0)
        {   if ((pass_number==1)&&(constant_made_flag==1))
                error("A 'switches' directive must \
must come before constant definitions");
            word(b,2); switches(b,0);
        }
        break;

    case STATUSLINE_CODE:
        word(b,2);
        On_("score") { statusline_flag=0; break; }
        On_("time")  { statusline_flag=1; break; }
        error_named("Expected 'score' or 'time' after 'statusline' but found",
            b); break;

    case SERIAL_CODE:
        textword(b,2);
        if (strlen(b)!=6)
        {   error("The serial number must be a 6-digit date in double-quotes");
            break; }
        for (i=0; i<6; i++)
          if (isdigit(b[i])==0)
          { error("The serial number must be a 6-digit date in double-quotes");
            break; }
        strcpy(time_given,b); time_set=1; break;

    case SYSTEM_CODE: 
        declare_systemfile(); break;

    case REPLACE_CODE:
        if (replace_sf(b)==1) break;
        sprintf(b, "Rep__");
        word(b+5, 2);
        new_symbol(b, 0, 19);
        break;

    case VERB_CODE: make_verb(b); break;

    case EXTEND_CODE: extend_verb(b); break;

    case VERSION_CODE:
        if (override_version) break;
        word(b,2); actual_version=constant_value(b);
        if (actual_version==3) version_number=3; else version_number=5;
        if (version_number==3) scale_factor=2; else scale_factor=4;
        if ((actual_version==6)||(actual_version==8)) scale_factor=8;
        if (actual_version==7) extend_memory_map=1;
        if ((actual_version>=3)&&(actual_version<=8)) break;
        error("The version number must be in the range 3 to 8"); break;

    case TRACE_CODE:     IfPass2 trace_mode=1;  break;
    case NOTRACE_CODE:   IfPass2 trace_mode=tracing_mode;  break;
    case ETRACE_CODE:
        IfPass2
        {   word(b,2); if (b[0]==0) { etrace_mode=2; break; }
            if (strcmp(b,"full")==0) { etrace_mode=1; break; }
        error_named("Expected 'full' or nothing after 'etrace' but found", b);
            break;
        }
    case NOETRACE_CODE:  IfPass2 etrace_mode=0; break; 
    case BTRACE_CODE:            trace_mode=1;  break; 
    case NOBTRACE_CODE:          trace_mode=0;  break; 
    case LTRACE_CODE:    IfPass2 ltrace_mode=1; break; 
    case NOLTRACE_CODE:  IfPass2 ltrace_mode=listing_mode; break; 
    case ATRACE_CODE:    IfPass2 ltrace_mode=2; break; 
    case NOATRACE_CODE:  IfPass2 ltrace_mode=listing_mode; break; 

    case LISTSYMBOLS_CODE:  IfPass2 list_symbols();     break;
    case LISTOBJECTS_CODE:  IfPass2 list_object_tree(); break;
    case LISTVERBS_CODE:    IfPass2 list_verb_table();  break;
    case LISTDICT_CODE:     IfPass2 show_dictionary();  break;

    default: error("Internal error - unknown directive code");
  }
    return;
}

extern void init_asm_vars(void)
{
    lower_strings_flag = 0;
    ifdef_sp = 0;
    started_ignoring = 0;
    a_from_one = 0;
    line_done_flag = 0;
    lmoved_flag = 0;
    constant_made_flag = 0;

#ifdef ALLOCATE_BIG_ARRAYS
    pa_forwards = NULL;
#endif
}
