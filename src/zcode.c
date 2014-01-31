/* ------------------------------------------------------------------------- */
/*   "zcode" : Z-opcodes, text translation and abbreviation routines         */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/*   On non-ASCII machines, translate_to_ascii must be altered               */
/* ------------------------------------------------------------------------- */

#include "header.h"

int32 total_chars_trans, total_bytes_trans, trans_length;
int chars_lookup[256];
int abbrevs_lookup[256], almade_flag=0;

extern int translate_to_ascii(char c)
{   return((int) c);
}

const char *alphabet[3] = {
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    " ^0123456789.,!?_#'~/\\-:()"
};

extern void make_lookup(void)
{   int i, j, k;
    for (j=0; j<256; j++)
    {   chars_lookup[j]=127; abbrevs_lookup[j]= -1; }
    for (j=0; j<3; j++)
        for (k=0; k<26; k++)
        {   i=(int) ((alphabet[j])[k]);
            chars_lookup[i]=k+j*26;
        }
}

extern void make_abbrevs_lookup(void)
{   int i, j, k, l; char p[MAX_ABBREV_LENGTH]; char *p1, *p2;
    do
    { for (i=0, j=0; j<no_abbrevs; j++)
        for (k=j+1; k<no_abbrevs; k++)
        {   p1=(char *)abbreviations_at+j*MAX_ABBREV_LENGTH;
            p2=(char *)abbreviations_at+k*MAX_ABBREV_LENGTH;
            if (strcmp(p1,p2)<0)
            {   i=1; strcpy(p,p1); strcpy(p1,p2); strcpy(p2,p);
                l=abbrev_values[j]; abbrev_values[j]=abbrev_values[k];
                abbrev_values[k]=l;
                l=abbrev_quality[j]; abbrev_quality[j]=abbrev_quality[k];
                abbrev_quality[k]=l;
            }
        }
    } while (i==1);
    for (j=no_abbrevs-1; j>=0; j--)
    {   p1=(char *)abbreviations_at+j*MAX_ABBREV_LENGTH;
        abbrevs_lookup[p1[0]]=j;
        abbrev_freqs[j]=0;
    }
    almade_flag=1;
}

static int z_chars[3], uptothree;
static int total_zchars_trans;
static unsigned char *text_pc;

static void write_z_char(int i)
{   uint32 j;
    total_zchars_trans++;
    z_chars[uptothree++]=(i%32);
    if (uptothree!=3) return;
    j= z_chars[0]*0x0400 + z_chars[1]*0x0020 + z_chars[2];
    text_pc[0] = j/256;
    text_pc[1] = j%256;
    uptothree=0; text_pc+=2;
    total_bytes_trans+=2;
}

static void end_z_chars(void)
{   unsigned char *p;
    trans_length=total_zchars_trans-trans_length;
    while (uptothree!=0) write_z_char(5);
    p=(unsigned char *) text_pc;
    *(p-2)= *(p-2)+128;
}


static int try_abbreviations_from(unsigned char *text, int i, int from)
{   int j, k; char *p, c;
    c=text[i];
    for (j=from, p=(char *)abbreviations_at+from*MAX_ABBREV_LENGTH;
         (j<no_abbrevs)&&(c==p[0]); j++, p+=MAX_ABBREV_LENGTH)
    {   if (text[i+1]==p[1])
        {   for (k=2; p[k]!=0; k++)
                if (text[i+k]!=p[k]) goto NotMatched;
            for (k=0; p[k]!=0; k++) text[i+k]=1;
            abbrev_freqs[j]++;
            return(j);
            NotMatched: ; 
        }
    }
    return(-1);
}

static int32 text_transcribed = 0;

extern zip *translate_text(zip *p, char *s_text)
{   int i, j, k, newa, cc, value, value2;
    unsigned char *text;

    if (s_text[0]==0) error("The empty string \"\" is illegal");

    trans_length=total_zchars_trans;

    if ((almade_flag==0)&&(no_abbrevs!=0)&&(abbrev_mode!=0))
        make_abbrevs_lookup();

    if ((abbrev_mode!=0)&&(store_the_text==1))
    {   if (pass_number==1)
        {   text_transcribed += strlen(s_text)+2;
            if (text_transcribed >= MAX_TRANSCRIPT_SIZE)
                memoryerror("MAX_TRANSCRIPT_SIZE", MAX_TRANSCRIPT_SIZE);
        }
        else
        {   sprintf(all_text_p, "%s\n\n", s_text);
            all_text_p+=strlen(all_text_p);
        }
    }

    text=(unsigned char *) s_text;

    uptothree=0; text_pc=(unsigned char *) p;
    for (i=0; text[i]!=0; i++)
    {   total_chars_trans++;
        if (double_spaced==1)
        {   if ((text[i]=='.')&&(text[i+1]==' ')&&(text[i+2]==' '))
                text[i+2]=1;
        }
        if ((economy_mode==1)&&(abbrev_mode!=0)
            &&((k=abbrevs_lookup[text[i]])!=-1))
        {   if ((j=try_abbreviations_from(text, i, k))!=-1)
            {   if (j<32) { write_z_char(2); write_z_char(j); }
                else { write_z_char(3); write_z_char(j-32); }
            }
        }
        if (text[i]=='@')
        {   if (text[i+1]=='@')
            {   i+=2;
                value=atoi((char *) (text+i));
                write_z_char(5); write_z_char(6);
                write_z_char(value/32); write_z_char(value%32);
                while (isdigit(text[i])) i++; i--;
            }
            else
            {   value= -1;
                switch(text[i+1])
                {   case '0': value=0; break;
                    case '1': value=1; break;
                    case '2': value=2; break;
                    case '3': value=3; break;
                    case '4': value=4; break;
                    case '5': value=5; break;
                    case '6': value=6; break;
                    case '7': value=7; break;
                    case '8': value=8; break;
                    case '9': value=9; break;
                }
                value2= -1;
                switch(text[i+2])
                {   case '0': value2=0; break;
                    case '1': value2=1; break;
                    case '2': value2=2; break;
                    case '3': value2=3; break;
                    case '4': value2=4; break;
                    case '5': value2=5; break;
                    case '6': value2=6; break;
                    case '7': value2=7; break;
                    case '8': value2=8; break;
                    case '9': value2=9; break;
                }
                if ((value!=-1)&&(value2!=-1))
                {   i++; i++;
                    write_z_char(1); write_z_char(value*10+value2);
                }
            }
        }
        else
        {   if (text[i]!=1)
            {   if (text[i]==' ') write_z_char(0);
                else
                {   cc=chars_lookup[(int) (text[i])];
                    if (cc==127)
                    {   write_z_char(5); write_z_char(6);
                        j=translate_to_ascii(text[i]);
                        write_z_char(j/32); write_z_char(j%32);
                    }
                    else
                    {   newa=cc/26; value=cc%26;
                        if (newa==1) write_z_char(4);
                        if (newa==2) write_z_char(5);
                        write_z_char(value+6);
                    }
                }
            }
        }
    }
    end_z_chars();
    return((zip *) text_pc);
}

/* ------------------------------------------------------------------------- */
/*   The (static) Z-code database (using a table adapted from that in the    */
/*   InfoToolkit disassembler "txd")                                         */
/* ------------------------------------------------------------------------- */

static opcode the_opcode(int i,char *s,int k,int l,int m)
{   opcode op; op.name=(zip *)s; op.code=i; op.type1=k; op.type2=l; op.no=m;
    return(op);
}

extern opcode opcs(int32 i)
{ 
   switch(i)
    {
    case  0: return(the_opcode(0x01, "je",              BRANCH,   NONE, TWO));
    case  1: return(the_opcode(0x02, "jl",              BRANCH,   NONE, TWO));
    case  2: return(the_opcode(0x03, "jg",              BRANCH,   NONE, TWO));
    case  3: return(the_opcode(0x04, "dec_chk",         BRANCH,    VAR, TWO));
    case  4: return(the_opcode(0x05, "inc_chk",         BRANCH,    VAR, TWO));
    case  5: return(the_opcode(0x06, "jin",             BRANCH,   NONE, TWO));
    case  6: return(the_opcode(0x07, "test",            BRANCH,   NONE, TWO));
    case  7: return(the_opcode(0x08, "or",               STORE,   NONE, TWO));
    case  8: return(the_opcode(0x09, "and",              STORE,   NONE, TWO));
    case  9: return(the_opcode(0x0A, "test_attr",       BRANCH,   NONE, TWO));
    case 10: return(the_opcode(0x0B, "set_attr",          NONE,   NONE, TWO));
    case 11: return(the_opcode(0x0C, "clear_attr",        NONE,   NONE, TWO));
    case 12: return(the_opcode(0x0D, "store",             NONE,    VAR, TWO));
    case 13: return(the_opcode(0x0D, "store",             NONE,    VAR, VARI));
    case 14: return(the_opcode(0x0E, "insert_obj",        NONE,   NONE, TWO));
    case 15: return(the_opcode(0x0F, "loadw",            STORE,   NONE, TWO));
    case 16: return(the_opcode(0x10, "loadb",            STORE,   NONE, TWO));
    case 17: return(the_opcode(0x11, "get_prop",         STORE,   NONE, TWO));
    case 18: return(the_opcode(0x12, "get_prop_addr",    STORE,   NONE, TWO));
    case 19: return(the_opcode(0x13, "get_next_prop",    STORE,   NONE, TWO));
    case 20: return(the_opcode(0x14, "add",              STORE,   NONE, TWO));
    case 21: return(the_opcode(0x15, "sub",              STORE,   NONE, TWO));
    case 22: return(the_opcode(0x16, "mul",              STORE,   NONE, TWO));
    case 23: return(the_opcode(0x17, "div",              STORE,   NONE, TWO));
    case 24: return(the_opcode(0x18, "mod",              STORE,   NONE, TWO));

    case 25: return(the_opcode(0x01, "je",              BRANCH,   NONE, VARI));

    case 26: return(the_opcode(0x20, "call",              CALL,   NONE, VARI));
    case 27: return(the_opcode(0x20, "call",             STORE,   NONE, VARI));
    case 28: return(the_opcode(0x21, "storew",            NONE,   NONE, VARI));
    case 29: return(the_opcode(0x22, "storeb",            NONE,   NONE, VARI));
    case 30: return(the_opcode(0x23, "put_prop",          NONE,   NONE, VARI));
    case 32: return(the_opcode(0x25, "print_char",       PCHAR,   NONE, VARI));
    case 33: return(the_opcode(0x26, "print_num",         NONE,   NONE, VARI));
    case 34: return(the_opcode(0x27, "random",           STORE,   NONE, VARI));
    case 35: return(the_opcode(0x28, "push",              NONE,   NONE, VARI));
    case 36: return(the_opcode(0x29, "pull",              NONE,    VAR, VARI));
    case 37: return(the_opcode(0x2A, "split_window",      NONE,   NONE, VARI));
    case 38: return(the_opcode(0x2B, "set_window",        NONE,   NONE, VARI));
    case 39: return(the_opcode(0x33, "output_stream",     NONE,   NONE, VARI));
    case 40: return(the_opcode(0x34, "input_stream",      NONE,   NONE, VARI));
    case 41: return(the_opcode(0x35, "sound_effect",      NONE,   NONE, VARI));

    case 42: return(the_opcode(0x00, "jz",              BRANCH,   NONE, ONE));
    case 43: return(the_opcode(0x01, "get_sibling",      STORE, OBJECT, ONE));
    case 44: return(the_opcode(0x02, "get_child",        STORE, OBJECT, ONE));
    case 45: return(the_opcode(0x03, "get_parent",       STORE,   NONE, ONE));
    case 46: return(the_opcode(0x04, "get_prop_len",     STORE,   NONE, ONE));
    case 47: return(the_opcode(0x05, "inc",               NONE,    VAR, ONE));
    case 48: return(the_opcode(0x06, "dec",               NONE,    VAR, ONE));
    case 49: return(the_opcode(0x07, "print_addr",        NONE,   NONE, ONE));

    case 50: return(the_opcode(0x09, "remove_obj",        NONE,   NONE, ONE));
    case 51: return(the_opcode(0x0A, "print_obj",         NONE,   NONE, ONE));
    case 52: return(the_opcode(0x0B, "ret",             RETURN,   NONE, ONE));
    case 53: return(the_opcode(0x0C, "jump",              JUMP,   NONE, ONE));
    case 54: return(the_opcode(0x0D, "print_paddr",       NONE,   NONE, ONE));
    case 55: return(the_opcode(0x0E, "load",             STORE,    VAR, ONE));

    case 57: return(the_opcode(0x00, "rtrue",           RETURN,   NONE, ZERO));
    case 58: return(the_opcode(0x01, "rfalse",          RETURN,   NONE, ZERO));
    case 59: return(the_opcode(0x02, "print",             NONE,   TEXT, ZERO));
    case 60: return(the_opcode(0x03, "print_ret",       RETURN,   TEXT, ZERO));
    case 63: return(the_opcode(0x07, "restart",           NONE,   NONE, ZERO));
    case 64: return(the_opcode(0x08, "ret_popped",      RETURN,   NONE, ZERO));
    case 65: return(the_opcode(0x09, "pop",               NONE,   NONE, ZERO));
    case 66: return(the_opcode(0x0A, "quit",              NONE,   NONE, ZERO));
    case 67: return(the_opcode(0x0B, "new_line",          NONE,   NONE, ZERO));
    case 69: return(the_opcode(0x0D, "verify",          BRANCH,   NONE, ZERO));
  }

  if ((version_number==3) && (i==68))
             return(the_opcode(0x0C, "show_status",       NONE,   NONE, ZERO));

  if ((version_number==3) || (version_number==4))
  { switch(i)
    {
    case 31: return(the_opcode(0x24, "read",              NONE,   NONE, VARI));
    case 56: return(the_opcode(0x0F, "not",              STORE,   NONE, ONE));
    case 61: return(the_opcode(0x05, "save",            BRANCH,   NONE, ZERO));
    case 62: return(the_opcode(0x06, "restore",         BRANCH,   NONE, ZERO));
    }
  }

  if (version_number>=4)
  { switch(i)
    {
    case 70: return(the_opcode(0x19, "call_2s",           CALL,   NONE, TWO));

    case 72: return(the_opcode(0x20, "call_vs",           CALL,   NONE, VARI));
    case 89: return(the_opcode(0x2C, "call_vs2",          CALL,   NONE, MANY));
    case 75: return(the_opcode(0x2D, "erase_window",      NONE,   NONE, VARI));
    case 76: return(the_opcode(0x2E, "erase_line",        NONE,   NONE, VARI));
    case 77: return(the_opcode(0x2F, "set_cursor",        NONE,   NONE, VARI));
    case 78: return(the_opcode(0x31, "set_text_style",    NONE,   NONE, VARI));
    case 79: return(the_opcode(0x32, "buffer_mode",       NONE,   NONE, VARI));
    case 96: return(the_opcode(0x35, "sound_effect",      NONE,   NONE, VARI));
    case 80: return(the_opcode(0x36, "read_char",        STORE,   NONE, VARI));
    case 81: return(the_opcode(0x37, "scan_table",       STORE, OBJECT, VARI));

    case 98: return(the_opcode(0x04, "nop",               NONE,   NONE, ZERO));

    case 82: return(the_opcode(0x08, "call_1s",           CALL,   NONE, ONE));
    }
  }

  if (version_number>=5)
  { switch(i)
    {
    case 71: return(the_opcode(0x1a, "call_2n",          NCALL,   NONE, TWO));
    case 99: return(the_opcode(0x1b, "set_colour",        NONE,   NONE, TWO));
   case 100: return(the_opcode(0x1c, "throw",             NONE,   NONE, TWO));
    case 90: return(the_opcode(0x24, "read",             STORE,   NONE, VARI));

    case 61: return(the_opcode(0x00, "save",             STORE,   NONE, EXTD));
    case 62: return(the_opcode(0x01, "restore",          STORE,   NONE, EXTD));
    case 85: return(the_opcode(0x02, "log_shift",        STORE,   NONE, EXTD));
    case 91: return(the_opcode(0x03, "art_shift",        STORE,   NONE, EXTD));
    case 86: return(the_opcode(0x04, "set_font",         STORE,   NONE, EXTD));
    case 87: return(the_opcode(0x09, "save_undo",        STORE,   NONE, EXTD));
    case 88: return(the_opcode(0x0A, "restore_undo",     STORE,   NONE, EXTD));

    case 56: return(the_opcode(0x38, "not",               NONE,   NONE, VARI));
    case 73: return(the_opcode(0x39, "call_vn",          NCALL,   NONE, VARI));
    case 74: return(the_opcode(0x3a, "call_vn2",         NCALL,   NONE, MANY));
    case 97: return(the_opcode(0x3b, "tokenise",          NONE,   NONE, VARI));
   case 101: return(the_opcode(0x3c, "encode_text",       NONE,   NONE, VARI));
   case 102: return(the_opcode(0x3d, "copy_table",        NONE,   NONE, VARI));
   case 103: return(the_opcode(0x3e, "print_table",       NONE,   NONE, VARI));
    case 84: return(the_opcode(0x3f, "check_arg_count", BRANCH,   NONE, VARI));

    case 83: return(the_opcode(0x0F, "call_1n",          NCALL,   NONE, ONE));

   case 104: return(the_opcode(0x09, "catch",            STORE,   NONE, ZERO));
   case 105: return(the_opcode(0x0F, "piracy",          BRANCH,   NONE, ZERO));
    }
  }

  if (version_number==6)
  { switch(i)
    {
    case 92: return(the_opcode(0x05, "draw_picture",      NONE,   NONE, EXTD));
    case 93: return(the_opcode(0x06, "picture_data",    BRANCH,   NONE, EXTD));
    case 94: return(the_opcode(0x07, "erase_picture",     NONE,   NONE, EXTD));
    case 106: return(the_opcode(0x08, "set_margins",       NONE,   NONE, EXTD));

    case 107: return(the_opcode(0x10, "move_window",       NONE,   NONE, EXTD));
    case 108: return(the_opcode(0x11, "window_size",       NONE,   NONE, EXTD));
    case 109: return(the_opcode(0x12, "window_style",      NONE,   NONE, EXTD));
    case 110: return(the_opcode(0x13, "get_wind_prop",    STORE,   NONE, EXTD));
    case 111: return(the_opcode(0x14, "scroll_window",     NONE,   NONE, EXTD));
    case 112: return(the_opcode(0x15, "pop_stack",         NONE,   NONE, EXTD));
    case 113: return(the_opcode(0x16, "read_mouse",        NONE,   NONE, EXTD));
    case 114: return(the_opcode(0x17, "mouse_window",      NONE,   NONE, EXTD));
    case 115: return(the_opcode(0x18, "push_stack",      BRANCH,   NONE, EXTD));
    case 116: return(the_opcode(0x19, "put_wind_prop",     NONE,   NONE, EXTD));
    case 117: return(the_opcode(0x1a, "print_form",        NONE,   NONE, EXTD));
    case 118: return(the_opcode(0x1b, "make_menu",       BRANCH,   NONE, EXTD));
    case 119: return(the_opcode(0x1c, "picture_table",     NONE,   NONE, EXTD));
    }
  }

  return(the_opcode(0xff,"???",INVALID,NONE,ZERO));
}

/* ------------------------------------------------------------------------- */
/*   Creating reserved words                                                 */
/* ------------------------------------------------------------------------- */

#define CreateD_(x,y) prim_new_symbol(x,y,14,6)
#define CreateC_(x,y) prim_new_symbol(x,y,15,6)
#define CreateB_(x,y,z) prim_new_symbol(x,z+y*100,16,6)
#define CreateA_(x,y) prim_new_symbol(x,y,17,6)

extern void stockup_symbols(void)
{   char *r1="RET#TRUE", *r2="RET#FALSE";

    new_symbol("nothing",0,9);

    new_symbol("sp",0,4);    new_symbol("ret#true",1,4);
    new_symbol("rtrue",1,4); new_symbol("ret#false",2,4);
    new_symbol("rfalse",2,4);

    new_symbol("=",1,10);    new_symbol("==",1,10);
    new_symbol(">",2,10);    new_symbol("<",3,10);
    new_symbol("has",4,10);
    new_symbol("near",5,10);
    new_symbol("~=",6,10);   new_symbol("<=",7,10);
    new_symbol(">=",8,10);
    new_symbol("hasnt",9,10);  new_symbol("far",10,10);

    new_symbol("name",1,12);

    new_symbol("temp_global",239,2);
    new_symbol("temp_global2",238,2);
    new_symbol("temp_global3",237,2);
    new_symbol("sys_glob0",0,2);
    new_symbol("sys_glob1",1,2);
    new_symbol("sys_glob2",2,2);

    CreateD_("ABBREVIATE", ABBREVIATE_CODE);
    CreateD_("ATTRIBUTE",  ATTRIBUTE_CODE);
    CreateD_("CONSTANT",   CONSTANT_CODE);
    CreateD_("DICTIONARY", DICTIONARY_CODE);
    CreateD_("END",        END_CODE);
    CreateD_("INCLUDE",    INCLUDE_CODE);
    CreateD_("GLOBAL",     GLOBAL_CODE);
    CreateD_("OBJECT",     OBJECT_CODE);
    CreateD_("PROPERTY",   PROPERTY_CODE);
    CreateD_("RELEASE",    RELEASE_CODE);
    CreateD_("SWITCHES",   SWITCHES_CODE);
    CreateD_("STATUSLINE", STATUSLINE_CODE); 
    CreateD_("VERB",       VERB_CODE);
    CreateD_("TRACE",      TRACE_CODE);
    CreateD_("NOTRACE",    NOTRACE_CODE);
    CreateD_("ETRACE",     ETRACE_CODE);
    CreateD_("NOETRACE",   NOETRACE_CODE);
    CreateD_("BTRACE",     BTRACE_CODE);
    CreateD_("NOBTRACE",   NOBTRACE_CODE);
    CreateD_("LTRACE",     LTRACE_CODE);
    CreateD_("NOLTRACE",   NOLTRACE_CODE);
    CreateD_("ATRACE",     ATRACE_CODE);
    CreateD_("NOATRACE",   NOATRACE_CODE);
    CreateD_("LISTSYMBOLS", LISTSYMBOLS_CODE);
    CreateD_("LISTOBJECTS", LISTOBJECTS_CODE);
    CreateD_("LISTVERBS",  LISTVERBS_CODE);
    CreateD_("LISTDICT",   LISTDICT_CODE);
    CreateD_("[",          OPENBLOCK_CODE);
    CreateD_("]",          CLOSEBLOCK_CODE);
    CreateD_("SERIAL",     SERIAL_CODE);
    CreateD_("DEFAULT",    DEFAULT_CODE);
    CreateD_("STUB",       STUB_CODE);
    CreateD_("VERSION",    VERSION_CODE);
    CreateD_("IFV3",       IFV3_CODE);
    CreateD_("IFV5",       IFV5_CODE);
    CreateD_("IFDEF",      IFDEF_CODE);
    CreateD_("IFNDEF",     IFNDEF_CODE);
    CreateD_("ENDIF",      ENDIF_CODE);
    CreateD_("IFNOT",      IFNOT_CODE);
    CreateD_("LOWSTRING",  LOWSTRING_CODE);
    CreateD_("CLASS",      CLASS_CODE);
    CreateD_("FAKE_ACTION", FAKE_ACTION_CODE);
    CreateD_("NEARBY",     NEARBY_CODE);
    CreateD_("SYSTEM_FILE", SYSTEM_CODE);
    CreateD_("REPLACE",    REPLACE_CODE);
    CreateD_("EXTEND",     EXTEND_CODE);
    CreateD_("ARRAY",      ARRAY_CODE);

    CreateB_("PRINT_ADDR", PRINT_ADDR_CODE,   49);
    CreateB_("PRINT_CHAR", PRINT_CHAR_CODE,   32);
    CreateB_("PRINT_PADDR", PRINT_PADDR_CODE, 54);
    CreateB_("PRINT_OBJ",  PRINT_OBJ_CODE,    51);
    CreateB_("PRINT_NUM",  PRINT_NUM_CODE,    33);
    CreateB_("RESTORE",    RESTORE_CODE,      62);
    CreateB_("SAVE",       SAVE_CODE,         61);
    CreateB_("PRINT",      PRINT_CODE,        59);
    CreateB_("PRINT_RET",  PRINT_RET_CODE,    60);
    CreateB_("JUMP",       JUMP_CODE,         53);
    
    CreateC_("REMOVE",     REMOVE_CODE);
    CreateC_("RETURN",     RETURN_CODE);
    CreateC_("DO",         DO_CODE);
    CreateC_("FOR",        FOR_CODE);
    CreateC_("IF",         IF_CODE);
    CreateC_("OBJECTLOOP", OBJECTLOOP_CODE);
    CreateC_("UNTIL",      UNTIL_CODE);
    CreateC_("WHILE",      WHILE_CODE);
    CreateC_("BREAK",      BREAK_CODE);
    CreateC_("ELSE",       ELSE_CODE);
    CreateC_("GIVE",       GIVE_CODE);
    CreateC_("INVERSION",  INVERSION_CODE);
    CreateC_("MOVE",       MOVE_CODE);
    CreateC_("PUT",        PUT_CODE);
    CreateC_("WRITE",      WRITE_CODE);
    CreateC_("STRING",     STRING_CODE);
    CreateC_("FONT",       FONT_CODE);
    CreateC_("READ",       READ_CODE);
    CreateC_("STYLE",      STYLE_CODE);
    CreateC_("SPACES",     SPACES_CODE);
    CreateC_("BOX",        BOX_CODE);
    CreateC_("SWITCH",     SWITCH_CODE);

    CreateA_("JE",0);
    CreateA_("JL",1);
    CreateA_("JG",2);
    CreateA_("JZ",42);
    CreateA_("SREAD",31);
    CreateA_("RANDOM",34);
    CreateA_("RET",52);
    CreateA_(r1,57);
    CreateA_(r2,58);
    CreateA_("RTRUE",57);
    CreateA_("RFALSE",58);
    CreateA_("RESTART",63);
    CreateA_("RETSP",64);
    CreateA_("REMOVE_OBJ",50);
    CreateA_("PUT_PROP",30);
    CreateA_("PUSH",35);
    CreateA_("PULL",36);
    CreateA_("POP",65);
    CreateA_("GET_SIBLING",43);
    CreateA_("GET_CHILD",44);
    CreateA_("GET_PARENT",45);
    CreateA_("GET_PROP_LEN",46);
    CreateA_("GET_PROP",17);
    CreateA_("GET_PROP_ADDR",18);
    CreateA_("GET_NEXT_PROP",19);
    CreateA_("SET_ATTR",10);
    CreateA_("STORE",12);
    CreateA_("SUB",21);
    CreateA_("STOREW",28);
    CreateA_("STOREB",29);
    CreateA_("SPLIT_WINDOW",37);
    CreateA_("SET_WINDOW",38);
    CreateA_("OUTPUT_STREAM",39);
    CreateA_("SOUND_EFFECT",41);
    CreateA_("SHOW_SCORE",68);
    CreateA_("DEC_CHK",3);
    CreateA_("INC_CHK",4);
    CreateA_("COMPARE_POBJ",5);
    CreateA_("TEST",6);
    CreateA_("OR",7);
    CreateA_("AND",8);
    CreateA_("TEST_ATTR",9);
    CreateA_("CLEAR_ATTR",11);
    CreateA_("LSTORE",13);
    CreateA_("INSERT_OBJ",14);
    CreateA_("LOADW",15);
    CreateA_("LOADB",16);
    CreateA_("ADD",20);
    CreateA_("MUL",22);
    CreateA_("DIV",23);
    CreateA_("MOD",24);
    CreateA_("VJE",25);
    CreateA_("CALL",26);
    CreateA_("ICALL",27);
    CreateA_("INPUT_STREAM",40);
    CreateA_("INC",47);
    CreateA_("DEC",48);
    CreateA_("LOAD",55);
    CreateA_("NOT",56);
    CreateA_("QUIT",66);
    CreateA_("NEW_LINE",67);
    CreateA_("VERIFY",69);

    CreateA_("CALL_2S",70);
    CreateA_("CALL_2N",71);
    CreateA_("CALL_VS",72);
    CreateA_("CALL_VN",73);
    CreateA_("CALL_VN2",74);
    CreateA_("ERASE_WINDOW",75);
    CreateA_("ERASE_LINE",76);
    CreateA_("SET_CURSOR",77);
    CreateA_("SET_TEXT_STYLE",78);
    CreateA_("BUFFER_MODE",79);
    CreateA_("READ_CHAR",80);
    CreateA_("SCANW",81);
    CreateA_("CALL_1S",82);
    CreateA_("CALL_1N",83);
    CreateA_("CHECK_NO_ARGS",84);

    CreateA_("LOG_SHIFT",85);
    CreateA_("SET_FONT",86);
    CreateA_("SAVE_UNDO",87);
    CreateA_("RESTORE_UNDO",88);

    CreateA_("CALL_VS2",89);

    CreateA_("AREAD",90);
    CreateA_("ART_SHIFT",91);
    CreateA_("DRAW_PICTURE",92);
    CreateA_("PICTURE_DATA",93);
    CreateA_("ERASE_PICTURE",94);
    CreateA_("INPUT_STREAM",95);
    CreateA_("BEEP",96);
    CreateA_("APARSE",97);

    CreateA_("NOP",98);
    CreateA_("COLOUR",99);
    CreateA_("THROW",100);
    CreateA_("ENCRYPT",101);
    CreateA_("COPY_TABLE",102);
    CreateA_("PRINT_TABLE",103);
    CreateA_("CATCH",104);
    CreateA_("PIRACY",105);
    CreateA_("SET_MARGINS",106);
    CreateA_("MOVE_WINDOW",107);
    CreateA_("WINDOW_SIZE",108);
    CreateA_("WINDOW_STYLE",109);
    CreateA_("GET_WIND_PROP",110);
    CreateA_("SCROLL_WINDOW",111);
    CreateA_("POP_STACK",112);
    CreateA_("READ_MOUSE",113);
    CreateA_("MOUSE_WINDOW",114);
    CreateA_("PUSH_STACK",115);
    CreateA_("PUT_WIND_PROP",116);
    CreateA_("PRINT_FORM",117);
    CreateA_("MAKE_MENU",118);
    CreateA_("PICTURE_TABLE",119);

    CreateA_("IO_BUFFER_MODE",79);  /* Alias for BUFFER_MODE */
    CreateA_("JIN",5);              /* for COMPARE_POBJ */
    CreateA_("SET_COLOUR",99);      /* for COLOUR */
    CreateA_("RET_POPPED",64);      /* for RETSP */
    CreateA_("SHOW_STATUS",68);     /* for SHOW_SCORE */
    CreateA_("SCAN_TABLE",81);      /* for SCANW */
    CreateA_("TOKENISE",97);        /* for APARSE */
    CreateA_("ENCODE_TEXT",101);    /* for ENCRYPT */
    CreateA_("CHECK_ARG_COUNT",84); /* for CHECK_NO_ARGS */

}

/* ------------------------------------------------------------------------- */
/*   The abbreviations optimiser                                             */
/* ------------------------------------------------------------------------- */

typedef struct tlb_s
{   char text[4];
    int32 intab, occurrences;
} tlb;
static tlb *tlbtab;
static int32 no_occs;

static int32 *grandtable;
static int32 *grandflags;
typedef struct optab_s
{   int32  length;
    int32  popularity;
    int32  score;
    int32  location;
    char text[64];
} optab;
static optab *bestyet, *bestyet2;

static int pass_no=0;

static void optimise_pass(void)
{   int32 i; int t1, t2;
    int32 j, j2, k, nl, matches, noflags, score, min, minat, x, scrabble, c;
    for (i=0; i<256; i++) bestyet[i].length=0;
    for (i=0; i<no_occs; i++)
    {   if ((i!=(int) '\n')&&(tlbtab[i].occurrences!=0))
        {   printf("Pass %d, %4ld/%ld '%s' (%ld occurrences) ",
                pass_no, (long int) i, (long int) no_occs, tlbtab[i].text,
                (long int) tlbtab[i].occurrences);
            t1=(int) (time(0));
            for (j=0; j<tlbtab[i].occurrences; j++)
            {   for (j2=0; j2<tlbtab[i].occurrences; j2++) grandflags[j2]=1;
                nl=2; noflags=tlbtab[i].occurrences;
                while ((noflags>=2)&&(nl<=62))
                {   nl++;
                    for (j2=0; j2<nl; j2++)
                        if (all_text[grandtable[tlbtab[i].intab+j]+j2]=='\n')
                            goto FinishEarly;
                    matches=0;
                    for (j2=j; j2<tlbtab[i].occurrences; j2++)
                    {   if (grandflags[j2]==1)
                        {   x=grandtable[tlbtab[i].intab+j2]
                              - grandtable[tlbtab[i].intab+j];
                         if (((x>-nl)&&(x<nl))
                            || (memcmp(all_text+grandtable[tlbtab[i].intab+j],
                                       all_text+grandtable[tlbtab[i].intab+j2],
                                       nl)!=0))
                            {   grandflags[j2]=0; noflags--; }
                            else matches++;
                        }
                    }
                    scrabble=0;
                    for (k=0; k<nl; k++)
                    {   scrabble++;
                        c=all_text[grandtable[tlbtab[i].intab+j+k]];
                        if (c!=(int) ' ')
                        {   if (chars_lookup[c]==127)
                                scrabble+=2;
                            else
                                if (chars_lookup[c]>=26)
                                    scrabble++;
                        }
                    }
                    score=(matches-1)*(scrabble-2);
                    min=score;
                    for (j2=0; j2<256; j2++)
                    {   if ((nl==bestyet[j2].length)
                                && (memcmp(all_text+bestyet[j2].location,
                                       all_text+grandtable[tlbtab[i].intab+j],
                                       nl)==0))
                        {   j2=256; min=score; }
                        else    
                        {   if (bestyet[j2].score<min)
                            {   min=bestyet[j2].score; minat=j2;
                            }
                        }
                    }
                    if (min!=score)
                    {   bestyet[minat].score=score;
                        bestyet[minat].length=nl;
                        bestyet[minat].location=grandtable[tlbtab[i].intab+j];
                        bestyet[minat].popularity=matches;
                        for (j2=0; j2<nl; j2++) sub_buffer[j2]=
                            all_text[bestyet[minat].location+j2];
                        sub_buffer[nl]=0;
                    }
                }
                FinishEarly: ;
            }
            t2=((int) time(0)) - t1;
            printf(" (%d seconds)\n",t2);
        }
    }
}

static int any_overlap(char *s1, char *s2)
{   int a, b, i, j, flag;
    a=strlen(s1); b=strlen(s2);
    for (i=1-b; i<a; i++)
    {   flag=0;
        for (j=0; j<b; j++)
            if ((0<=i+j)&&(i+j<=a-1))
                if (s1[i+j]!=s2[j]) flag=1;
        if (flag==0) return(1);
    }
    return(0);
}

#define MAX_TLBS 8000

extern void optimise_abbreviations(void)
{   int32 i, j, t, max=0, MAX_GTABLE;
    int32 j2, selected, available, maxat, nl;
    tlb test;

    printf("Beginning calculation of optimal abbreviations...\n");

    tlbtab=my_calloc(sizeof(tlb), MAX_TLBS, "tlb table"); no_occs=0;
    sub_buffer=my_calloc(sizeof(char), BUFFER_LENGTH, "sub_buffer");
    for (i=0; i<MAX_TLBS; i++) tlbtab[i].occurrences=0;

    bestyet=my_calloc(sizeof(optab), 256, "bestyet");
    bestyet2=my_calloc(sizeof(optab), 64, "bestyet2");

            bestyet2[0].text[0]='.';
            bestyet2[0].text[1]=' ';
            bestyet2[0].text[2]=' ';
            bestyet2[0].text[3]=0;

            bestyet2[1].text[0]=',';
            bestyet2[1].text[1]=' ';
            bestyet2[1].text[2]=0;

    for (i=0, t=0; all_text+i<all_text_p; i++)
    {
        if ((all_text[i]=='.') && (all_text[i+1]==' ') && (all_text[i+2]==' '))
        {   all_text[i]='\n'; all_text[i+1]='\n'; all_text[i+2]='\n';
            bestyet2[0].popularity++;
        }

        if ((all_text[i]=='.') && (all_text[i+1]==' '))
        {   all_text[i]='\n'; all_text[i+1]='\n';
            bestyet2[0].popularity++;
        }

        if ((all_text[i]==',') && (all_text[i+1]==' '))
        {   all_text[i]='\n'; all_text[i+1]='\n';
            bestyet2[1].popularity++;
        }
    }

    MAX_GTABLE=subtract_pointers(all_text_p,all_text)+1;
    grandtable=my_calloc(4*sizeof(int32), MAX_GTABLE/4, "grandtable");

    for (i=0, t=0; all_text+i<all_text_p; i++)
    {   test.text[0]=all_text[i];
        test.text[1]=all_text[i+1];
        test.text[2]=all_text[i+2];
        test.text[3]=0;
        if ((test.text[0]=='\n')||(test.text[1]=='\n')||(test.text[2]=='\n'))
            goto DontKeep;
        for (j=0; j<no_occs; j++)
            if (strcmp(test.text,tlbtab[j].text)==0)
                goto DontKeep;
        test.occurrences=0;
        for (j=i+3; all_text+j<all_text_p; j++)
        {   if ((all_text[i]==all_text[j])
                 && (all_text[i+1]==all_text[j+1])
                 && (all_text[i+2]==all_text[j+2]))
                 {   grandtable[t+test.occurrences]=j;
                     test.occurrences++;
                     if (t+test.occurrences==MAX_GTABLE)
                     {   printf("All %ld cross-references used\n",
                             (long int) MAX_GTABLE);
                         goto Built;
                     }
                 }
        }
        if (test.occurrences>=2)
        {   tlbtab[no_occs]=test;
            tlbtab[no_occs].intab=t; t+=tlbtab[no_occs].occurrences;
            if (max<tlbtab[no_occs].occurrences)
                max=tlbtab[no_occs].occurrences;
            no_occs++;
            if (no_occs==MAX_TLBS)
            {   printf("All %d three-letter-blocks used\n",
                    MAX_TLBS);
                goto Built;
            }
        }
        DontKeep: ;
    }

    Built:
    grandflags=my_calloc(sizeof(int), max, "grandflags");


    printf("Cross-reference table (%ld entries) built...\n",
        (long int) no_occs);
    /*  for (i=0; i<no_occs; i++)
            printf("%4d %4d '%s' %d\n",i,tlbtab[i].intab,tlbtab[i].text,
                tlbtab[i].occurrences);
    */

    for (i=0; i<64; i++) bestyet2[i].length=0; selected=2;
    available=256;
    while ((available>0)&&(selected<64))
    {   printf("Pass %d\n", ++pass_no);

        optimise_pass();
        available=0;
        for (i=0; i<256; i++)
            if (bestyet[i].score!=0)
            {   available++;
                nl=bestyet[i].length;
                for (j2=0; j2<nl; j2++) bestyet[i].text[j2]=
                    all_text[bestyet[i].location+j2];
                bestyet[i].text[nl]=0;
            }

/*      printf("End of pass results:\n");
        printf("\nno   score  freq   string\n");
        for (i=0; i<256; i++)
            if (bestyet[i].score>0)
                printf("%02d:  %4d   %4d   '%s'\n", i, bestyet[i].score,
                    bestyet[i].popularity, bestyet[i].text);
*/

        do
        {   max=0;
            for (i=0; i<256; i++)
                if (max<bestyet[i].score)
                {   max=bestyet[i].score;
                    maxat=i;
                }

            if (max>0)
            {   bestyet2[selected++]=bestyet[maxat];

                printf("Selection %2ld: '%s' (repeated %ld times, scoring %ld)\n",
                    (long int) selected,bestyet[maxat].text,
                    (long int) bestyet[maxat].popularity,
                    (long int) bestyet[maxat].score);

                test.text[0]=bestyet[maxat].text[0];
                test.text[1]=bestyet[maxat].text[1];
                test.text[2]=bestyet[maxat].text[2];
                test.text[3]=0;

                for (i=0; i<no_occs; i++)
                    if (strcmp(test.text,tlbtab[i].text)==0)
                        break;

                for (j=0; j<tlbtab[i].occurrences; j++)
                {   if (memcmp(bestyet[maxat].text,
                               all_text+grandtable[tlbtab[i].intab+j],
                               bestyet[maxat].length)==0)
                    {   for (j2=0; j2<bestyet[maxat].length; j2++)
                            all_text[grandtable[tlbtab[i].intab+j]+j2]='\n';
                    }
                }

                for (i=0; i<256; i++)
                    if ((bestyet[i].score>0)&&
                        (any_overlap(bestyet[maxat].text,bestyet[i].text)==1))
                    {   bestyet[i].score=0;
                       /* printf("Discarding '%s' as overlapping\n",
                            bestyet[i].text); */
                    }
            }
        } while ((max>0)&&(available>0)&&(selected<64));
    }

    printf("\nChosen abbreviations (in Inform syntax):\n\n");
    for (i=0; i<selected; i++)
        printf("Abbreviate \"%s\";\n", bestyet2[i].text);

    zcode_free_arrays();
}

extern void zcode_free_arrays(void)
{   my_free (&tlbtab,"tlb table");
    my_free (&sub_buffer,"sub_buffer");
    my_free (&bestyet,"bestyet");
    my_free (&bestyet2,"bestyet2");
    my_free (&grandtable,"grandtable");
    my_free (&grandflags,"grandflags");
}

extern void init_zcode_vars(void)
{   almade_flag = 0;
    pass_no = 0;
    bestyet = NULL;
    bestyet2 = NULL;
    tlbtab = NULL;
    grandtable = NULL;
    grandflags = NULL;
    text_transcribed = 0;
}
