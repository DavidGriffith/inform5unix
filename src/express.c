/* ------------------------------------------------------------------------- */
/*   "express" :  The expression evaluator                                   */
/*                                                                           */
/*   Part of Inform release 5                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include "header.h"

int sys_functions[16];

extern int replace_sf(char *b)
{   int x=-1;
    word(b,2);
    On_("parent")  x=0;
    On_("sibling") x=1;
    On_("younger") x=2;
    On_("child")   x=3;
    On_("eldest")  x=4;
    On_("random")  x=5;
    On_("prop_len") x=6;
    On_("prop_addr") x=7;
    On_("prop")    x=8;
    On_("children") x=9;
    On_("youngest") x=10;
    On_("elder") x=11;
    On_("indirect") x=12;
    if (x==-1) return(0);
    sys_functions[x]=1;
    return(1);
}

/* ------------------------------------------------------------------------- */
/*   Definition of expression tree branch                                    */
/* ------------------------------------------------------------------------- */

typedef struct treenode {
    int up;                     /* Node above                                */
    int word;                   /* Token number in source line               */
    int type;                   /* Type of node (there are eight: see above) */
    int priority;               /* Eg, + has a low priority, * a higher one  */
    int label_number;           /* For code blocks in && and || expressions  */
    int yes_label;              /* Condition true                            */
    int no_label;               /* Condition false                           */
    int arity;                  /* Number of branches expected               */
    int b[MAX_ARITY];           /* Nodes below (the branches)...             */
    int branches_made;          /* ...from 0 up to this-1                    */
    char *op;                   /* Name of opcode to achieve this operation  */
} treenode;

#ifndef ALLOCATE_BIG_ARRAYS
  static treenode   tree[MAX_EXPRESSION_NODES];
#else
  static treenode   *tree;
#endif

extern void express_allocate_arrays(void)
{
#ifdef ALLOCATE_BIG_ARRAYS
    tree      = my_calloc(sizeof(treenode),   MAX_EXPRESSION_NODES,
                "expression tree");
#endif
}

extern void express_free_arrays(void)
{   
#ifdef ALLOCATE_BIG_ARRAYS
    my_free(&tree, "expression tree");
#endif
}

/* ------------------------------------------------------------------------- */
/*   Compiler expression and assignment evaluator                            */
/* ------------------------------------------------------------------------- */
/*                                                                           */
/*   This is easily the most complicated algorithm in Inform, for two        */
/*   reasons - firstly, because I invented it in a hurry and know absolutely */
/*   nothing about compiler theory, and second, because it tries to avoid    */
/*   recursion which uses the C stack.  (Some machines to which Inform has   */
/*   been ported have had problems with the size of the C stack.)            */
/*                                                                           */
/*   The code generator is also complicated by some awkward features of the  */
/*   Z-machine: for instance, the safe way to extract property lengths is    */
/*   tricky to get right.  Anyway, enough excuses:                           */
/*                                                                           */
/*   The algorithm first makes a tree out of the expression and then         */
/*   clears it off again by clipping off nodes and stacking up corresponding */
/*   assembly lines.  This needs to be in the right order, since the stack   */
/*   can't very conveniently be re-ordered.                                  */
/*                                                                           */
/*   The algorithm is iterative rather than recursive.  In the first phase,  */
/*   we imagine a tree gradually growing as a dryad scampers over it.  She   */
/*   is either resting on a leaf already grown, or else hovering in midair   */
/*   over the space where one will grow.  Every so often she casts an easy   */
/*   spell to grow a leaf where she is, or a more powerful one to grow her   */
/*   leaf into a branch, and she then clambers along or down.                */
/*                                                                           */
/*   The dryad is also sometimes awaiting a comma, since she knows that the  */
/*   line 45+1, ... ends with the comma, whereas i=fn(j,k) does not.  Sadly  */
/*   she can get distracted while waiting, for instance in the expression    */
/*   (after reading j) i=fn(j+5*(l+1),2), but being conscientious she ties   */
/*   her veil where she was so as to be able to find the place again later.  */
/*                                                                           */
/*   To follow her movements, try Informing with full expression tracing on  */
/*   (using "etrace full"): to look simply at the tree she grows, and the    */
/*   code it results in, try just "etrace".                                  */
/*                                                                           */
/*   The main data structure (treenode) is commented at the top of the file. */
/*   (Most of the variables have been more sensibly named since early        */
/*   releases: in particular now you can't see the woods[] for the tree[].)  */
/* ------------------------------------------------------------------------- */

#define ROOT_NODE    -4
#define BLANK_NODE   -3
#define CDONE_NODE   -2
#define SP_NODE      -1
#define LEAF_NODE     0
#define STORE_NODE    1
#define ARITH_NODE    2
#define FCALL_NODE    3
#define COND_NODE     4
#define NCOND_NODE    5
#define LOGICAND_NODE 6
#define LOGICOR_NODE  7
#define OR_NODE       8
#define ALTERA_NODE   9
#define ALTERB_NODE  10

static int tree_size, dryad, resting, comma_mode, dryad_veil=-1,
           sofar_empty;

/* ------------------------------------------------------------------------- */
/*   Pretty printing (for diagnostics)                                       */
/* ------------------------------------------------------------------------- */

static void show_leaf(int ln, int depth)
{   char sl_buffer[100];
    int j;
    for (j=0; j<2*depth+2; j++) printf(" ");
    if (ln==-1) { printf("..\n"); return; }
    else if (etrace_mode==1) printf("%02d ",ln);
    switch(tree[ln].type)
    {   case ROOT_NODE:  printf("<root>"); break;
        case BLANK_NODE: printf("<blank>"); break;
        case CDONE_NODE: printf("<condition done>"); break;
        case SP_NODE:    printf("<sp>"); break;
        case LEAF_NODE:  word(sl_buffer,tree[ln].word);
                         printf("%s",sl_buffer); break;
        default:         printf("%s",tree[ln].op); break;
    }
    if (etrace_mode==1)
    {   if (tree[ln].priority!=0)
        { printf(" (%d)",tree[ln].priority); }
        if (tree[ln].type>=STORE_NODE)
        {   switch(tree[ln].type)
            {   case STORE_NODE:    printf(" [assignment] ");        break;
                case ARITH_NODE:    printf(" [arithmetic] ");        break;
                case FCALL_NODE:    printf(" [function call] ");     break;
                case COND_NODE:     printf(" [condition] ");         break;
                case NCOND_NODE:    printf(" [negated condition] "); break;
                case LOGICAND_NODE: printf(" [conjunction] ");       break;
                case LOGICOR_NODE:  printf(" [disjunction] ");       break;
                case OR_NODE:       printf(" [or] ");                break;
                case ALTERA_NODE:    printf(" [read then alter] ");  break;
                case ALTERB_NODE:    printf(" [alter then read] ");  break;
            }
        }
        printf(" ^%d ",tree[ln].up);
        if (ln==dryad) printf("(dryad here) ");
    }
    if ((tree[ln].type==LOGICAND_NODE)
        ||(tree[ln].type==LOGICOR_NODE))
        printf(" {%d/%d/%d} ",tree[ln].label_number,
            tree[ln].yes_label, tree[ln].no_label);
    printf("\n");
    for (j=0; j<tree[ln].arity; j++)
    {   if (j<tree[ln].branches_made) show_leaf(tree[ln].b[j],depth+1);
        else show_leaf(-1,depth+1);
    }
}

static void show_tree(char *c)
{   printf("%s\n",c); show_leaf(tree[0].b[0],0);
    if (etrace_mode==1)
    {   printf("The dryad is at %d and is %s%s\n",
            dryad,(resting==1)?"resting":"hovering in midair",
            (comma_mode==0)?"":"\nwhile waiting for a comma");
        if (dryad_veil!=-1)
            printf("with her veil tied to %d\n",dryad_veil);
    }
}

/* ------------------------------------------------------------------------- */
/*   Growing the tree                                                        */
/* ------------------------------------------------------------------------- */

static void grow_branch(int a, int wn, int type, char *opcde, int prio)
{   int i, above_dryad, rflag=0, prefix=0, postfix=0;

    if ((a>=5)&&(actual_version==3))
    {   error("A function may be called with at most 3 arguments");
        return;
    }
    if ((a>=9)&&(actual_version>3))
    {   error("A function may be called with at most 7 arguments");
        return;
    }

    above_dryad=tree[dryad].up;

    if ((a==1)&&(type!=FCALL_NODE)&&(sofar_empty==1))
    {   prefix=1;
        if (type==ALTERA_NODE) type=ALTERB_NODE;
    }
    if (type==ALTERA_NODE) postfix=1;
    if ((a==1)&&(type==FCALL_NODE)) postfix=1;

    if (etrace_mode==1) printf("%s%s%s operator '%s'\n",
        (prefix==1)?"prefix":"",(postfix==1)?"postfix":"",
        ((prefix==0)&&(postfix==0))?"infix":"",opcde);
    sofar_empty=1;

    if ((resting==0)&&(comma_mode==0)&&(prefix==0))
    {   error("Operator has too few arguments"); return;
    }

    if (type==OR_NODE)
    {   if ((strcmp(tree[above_dryad].op,"je")==0)
            || (strcmp(tree[above_dryad].op,"vje")==0))
        {   if (tree[above_dryad].arity < 5)
            {   tree[above_dryad].arity++; rflag=1; }
            tree[above_dryad].op="vje";
            if (tree[above_dryad].arity==5)
            {   error("At most three values can be separated by 'or'");
                rflag=0;
            }
            if (rflag==1)
            {   if (tree_size==MAX_EXPRESSION_NODES) goto TooComplex;
                dryad=tree_size++;
                tree[dryad].up=above_dryad;
                tree[dryad].type=BLANK_NODE;
                tree[above_dryad].b[tree[above_dryad].arity-1]=dryad;
                resting=0;
                return;
            }
        }
        else
            error("'or' can only be used with the conditions '==' and '~='");
    }

    if (comma_mode==1)
    {   comma_mode=0;
        if (prefix==0) dryad_veil=dryad;
    }

    while ((tree[above_dryad].priority>prio)
           || ((tree[above_dryad].priority==prio)
               && (type==ARITH_NODE)
               && (tree[above_dryad].type==ARITH_NODE)))
    {   dryad=above_dryad; above_dryad=tree[dryad].up;
    }

    if (prefix==0)
    {   while (tree[dryad].type== BLANK_NODE)
        {   for (i=1; i<tree[above_dryad].arity; i++)
                if (tree[above_dryad].b[i]==dryad)
                { dryad=tree[above_dryad].b[i-1]; break; }
        }
    }

    if (prefix==1) tree[above_dryad].branches_made++;

    for (i=0; i<tree[above_dryad].arity; i++)
        if (tree[above_dryad].b[i]==dryad)
            tree[above_dryad].b[i]=tree_size;

    tree[dryad].up=tree_size;
    tree[tree_size].b[0]=dryad;
    tree[tree_size].up=above_dryad; 

    if (tree_size==MAX_EXPRESSION_NODES) goto TooComplex;
    dryad=tree_size++;

    tree[dryad].arity=a;
    tree[dryad].word=wn;
    tree[dryad].branches_made=1;
    if (prefix==1) tree[dryad].branches_made=0;
    tree[dryad].type=type;
    tree[dryad].op=opcde;
    tree[dryad].priority=prio;
    tree[dryad].no_label=-2;
    tree[dryad].yes_label=-2;
    if ((type==LOGICAND_NODE) || (type==LOGICOR_NODE))
        tree[dryad].label_number=no_dummy_labels++;

    for (i=1; i<a; i++)
    {   tree[dryad].b[i]=tree_size;
        tree[tree_size].type= BLANK_NODE;
        tree[tree_size].arity=0;
        tree[tree_size].branches_made=0;
        tree[tree_size].priority=0;
        tree[tree_size++].up=dryad;
    }
    if ((prefix==0)&&(postfix==0))
    {       if (etrace_mode==1) printf("Magic = %d\n",dryad);
        dryad=tree[dryad].b[1]; resting=0; }
    if (prefix==1)
    {   dryad=tree[dryad].b[0]; resting=0; }
    if (postfix==1)
    {   dryad=tree[dryad].b[0]; resting=1; }

    if (etrace_mode==1) show_tree("grow_branch to");
    return;

    TooComplex:
        memoryerror("MAX_EXPRESSION_NODES", MAX_EXPRESSION_NODES);
}

static void grow_leaf(int wn)
{   int above_dryad;

    above_dryad=tree[dryad].up;

    tree[dryad].arity=0;
    tree[dryad].word=wn;
    tree[dryad].branches_made=0;
    tree[dryad].type=LEAF_NODE;
    tree[dryad].priority=0;

    tree[above_dryad].branches_made++;
    if (tree[above_dryad].branches_made<tree[above_dryad].arity)
    { dryad=tree[above_dryad].b[tree[above_dryad].branches_made];
      resting=0;
    }
    else resting=1;

    if (etrace_mode==1) show_tree("grow_leaf to");
    return;
}

static void fix_yesno_labels(int n, int flag)
{   int i;
    if ((tree[n].type!=LOGICOR_NODE)&&(tree[n].type!=LOGICAND_NODE))
        return;
    if (flag==1)
    {   
            tree[n].yes_label = tree[tree[n].up].yes_label;
            tree[n].no_label  = tree[tree[n].up].no_label;
    }
    else
    switch(tree[tree[n].up].type)
    {   case ROOT_NODE:
            tree[n].yes_label = no_dummy_labels-1;
            tree[n].no_label  = -1;
            break;
        case LOGICAND_NODE:
            tree[n].yes_label = tree[n].label_number;
            tree[n].no_label  = tree[tree[n].up].no_label;
            break;
        case LOGICOR_NODE:
            tree[n].yes_label = tree[tree[n].up].yes_label;
            tree[n].no_label  = tree[n].label_number;
            break;
    }
    for (i=0; i<tree[n].branches_made; i++)
        fix_yesno_labels(tree[n].b[i], (i==tree[n].branches_made-1)?1:0);
}

/* ------------------------------------------------------------------------- */
/*   Main expression (and assignment) evaluator routine:                     */
/*                                                                           */
/*   expression(from) compiles code to evaluate the expression starting at   */
/*     token from, leaves next_token at the first not part of the expression */
/*     (or -1 if the line was all used up); and returns                      */
/*                                                                           */
/*     -3  if the result was thrown away (eg. a void context function call)  */
/*     -2  if there is no result (eg, from "i=4" there is none)              */
/*     -1  if the result is on the stack                                     */
/*     n   if the result is the constant term in token n                     */
/* ------------------------------------------------------------------------- */

static void eword(char *b, int bn)
{   if (tree[bn].type==SP_NODE) strcpy(b,"sp");
    else word(b,tree[bn].word);
    /* printf("Eword %d -> %d <%s>\n",bn,tree[bn].word,b);  */
}

#define Op_(type,name,priority) grow_branch(2,token-1,type,name,\
priority+current_priority)
#define Op1_(type,name,priority) grow_branch(1,token-1,type,name,\
priority+current_priority)
#define OnCS_(x) if ((condition_context==1)&&(strcmp(sub_buffer,x)==0))
#define OnBS_(x) if ((bracket_level>0)&&(strcmp(sub_buffer,x)==0))

int next_token, void_context, condition_context=0,
           assign_context=0, lines_compiled;
char condition_label[64];
static int brackets[MAX_EXPRESSION_BRACKETS], bracket_level;

static void estack_line(char *rewrite)
{   if (etrace_mode>=1) printf("%2d  %s\n",++lines_compiled,rewrite+1);
    stack_line(rewrite);
}

extern int expression(int fromword)
{   int c, i, j, k, t, token, current_priority=0, wd_tk,
        sysfun_arity, dummy_branch_flag,
        call_level, call_flag, call_args, call_i, tosp_flag,
        revise_flag, last_was_leaf,
        its_void=0, thrown_away_flag=0, indirect_flag=0,
        last_was_comma=0, last_was_openb=0;

    /* --------------------------------------------------------------------- */
    /*  1. A little optimisation: can we tell at once this will not be a     */
    /*     full-blown expression, but just a single constant term?           */
    /* --------------------------------------------------------------------- */

    t=word_token(fromword);
    if ((t!=OPENB_SEP) && (t!=MINUS_SEP) && (condition_context==0)
        && (t!=DEC_SEP) && (t!=INC_SEP))
    {   word(sub_buffer,fromword+1); c=sub_buffer[0];
        if ((c==0) || (isalpha(c)) || (isdigit(c)) ||
            (c=='#') || (c==',') || (c==':'))
        {   next_token=fromword+1; if (c==0) next_token=-1;
            return(fromword);
        }
    }

    /* --------------------------------------------------------------------- */
    /*  2. Initially the tree is just a root node plus one blank branch      */
    /* --------------------------------------------------------------------- */

    tree[0].up= -1;  tree[0].type= ROOT_NODE;   tree[0].b[0]=1;
    tree[0].arity=1; tree[0].branches_made=0;   tree[0].priority=0;

    tree[1].up=0;    tree[1].type= BLANK_NODE;  tree[1].priority=0;

    tree_size=2; comma_mode=0;
    dryad=1; resting=0; sofar_empty=1; last_was_leaf=0;

    bracket_level=0;
    token=fromword;
    revise_flag=0;

    if (etrace_mode==1) printf("\n++++++++++++++++++++++++++++++\n");
    if (etrace_mode>=1)
    {   printf("\nEvaluating expression %s%s%sat:\n  ",
            (void_context==1)?"(in void context) ":"",
            (condition_context==1)?"(in condition context) ":"",
            (assign_context==1)?"(in assignment context) ":"");
        for (i=0; (i==0)||(sub_buffer[0]!=0); i++)
        {   word(sub_buffer,fromword+i); printf("%s ",sub_buffer);
        }
        printf("\n");
    }

    lines_compiled=0;

    /* --------------------------------------------------------------------- */
    /*  3. Grow the tree                                                     */
    /* --------------------------------------------------------------------- */

    do
    {   token++;
        wd_tk = word_token(token-1); sub_buffer[0]=0;
        if (wd_tk==-1) { word(sub_buffer,token-1);
                         if (sub_buffer[0]==0) break;
                       }

        if (last_was_comma>0) last_was_comma--;
        if (last_was_openb>0) last_was_openb--;

        if (etrace_mode==1)
          printf("Dryad reads '%s'\nSofar_empty=%d\n",sub_buffer,sofar_empty);

        switch(wd_tk)
        {
             case SETEQUALS_SEP:
                                   Op_(STORE_NODE,  "store",         3);
                 revise_flag=1; its_void=1; last_was_openb=2; break;

             case PLUS_SEP:        Op_(ARITH_NODE,  "add",           4); break;

             case MINUS_SEP:
                 if (sofar_empty==0)
                                   Op_(ARITH_NODE,  "sub",           4);
                 else
                                  Op1_(ARITH_NODE,  "sub 0",         8);
                 break;

             case TIMES_SEP:       Op_(ARITH_NODE,  "mul",           5); break;

             case DIVIDE_SEP:      Op_(ARITH_NODE,  "div",           5); break;

             case REMAINDER_SEP:   Op_(ARITH_NODE,  "mod",           5); break;

             case ARTAND_SEP:      Op_(ARITH_NODE,  "and",           5); break;

             case ARTOR_SEP:       Op_(ARITH_NODE,  "or",            5); break;

             case ARROW_SEP:       Op_(ARITH_NODE,  "loadb",         6); break;

             case DARROW_SEP:      Op_(ARITH_NODE,  "loadw",         6); break;

             case PROPERTY_SEP:    Op_(ARITH_NODE,  "get_prop",      7); break;

             case PROPADD_SEP:     Op_(ARITH_NODE,  "get_prop_addr", 7); break;

             case PROPNUM_SEP:     Op_(ARITH_NODE,  "_get_prop_len", 7);
                 revise_flag=1; break;

             case INC_SEP:        Op1_(ALTERA_NODE, "inc",           9); break;

             case DEC_SEP:        Op1_(ALTERA_NODE, "dec",           9); break;

             case OPENB_SEP:
                 current_priority+=10;
                 last_was_openb=2;
                 if (last_was_leaf==0)
                 {   if (bracket_level==MAX_EXPRESSION_BRACKETS)
                     {   error("Brackets '(' too deeply nested");
                         goto BigBreak; }
                     brackets[bracket_level++]=1;
                 }
                 else
                 {   sofar_empty=1; brackets[bracket_level++]=0;
                     j=token; call_args=2; call_level=bracket_level;
                     word(sub_buffer,token);
                     OnS_(")") call_args=1;
                     else
                     do { word(sub_buffer,j++);
                          if (sub_buffer[0]==0)
                          {   error("Missing bracket ')' in function call");
                              goto BigBreak;
                          }
                          if ((strcmp(sub_buffer,",")==0)
                              &&(bracket_level==call_level))
                              call_args++;
                          OnS_("(") call_level++;
                          OnS_(")") call_level--;
                        } while (call_level>=bracket_level);
                     grow_branch(call_args,token-1,FCALL_NODE,"call",
                                 1+current_priority);
                     comma_mode=1;
                 }
                 sofar_empty=1;
                 break;

             case CLOSEB_SEP:
                 if (bracket_level--==0) goto BigBreak;
                 current_priority-=10;
                 break;

             case COMMA_SEP:
                 if (bracket_level<=0) goto DefaultCase;
                 if (brackets[bracket_level-1]==1)
                     error("Spurious comma ','");
                 comma_mode=1; sofar_empty=1; last_was_comma=2;
                 if (tree[dryad].type!=BLANK_NODE)
                 {   while (tree[dryad].type!=ROOT_NODE)
                     {   while (tree[dryad].type!=FCALL_NODE)
                             dryad=tree[dryad].up;
                         if (tree[dryad].branches_made<tree[dryad].arity)
                         {   dryad=tree[dryad].b[tree[dryad].branches_made];
                             goto FoundNewPlace;
                         }
                         dryad=tree[dryad].up;
                     }
                     error("Misplaced comma ','"); return(-2);
                     FoundNewPlace: resting=0;
                 }
                 if (etrace_mode==1) show_tree("Comma to");
                 break;

             default:
                 DefaultCase:
                     goto TryFurther;
        }
        goto AtomDone;

        TryFurther:

        if (condition_context==1)
        {   switch(wd_tk)
            {   case GREATER_SEP:  Op_(COND_NODE,   "jg",            3); break;
                case LESS_SEP:     Op_(COND_NODE,   "jl",            3); break;
                case CONDEQUALS_SEP:
                                   Op_(COND_NODE,   "je",            3); break;
                case NOTEQUAL_SEP: Op_(NCOND_NODE,  "je",            3); break;
                case GE_SEP:       Op_(NCOND_NODE,  "jl",            3); break;
                case LE_SEP:       Op_(NCOND_NODE,  "jg",            3); break;
                case LOGAND_SEP:   Op_(LOGICAND_NODE, "&&",          2); break;
                case LOGOR_SEP:    Op_(LOGICOR_NODE, "||",           2); break;
                default:

                     OnS_("or")    Op_(OR_NODE,     "or",            2);
                else OnS_("in")    Op_(COND_NODE,   "jin",           3);
                else OnS_("notin") Op_(NCOND_NODE,  "jin",           3);
                else OnS_("has")   Op_(COND_NODE,   "test_attr",     3);
                else OnS_("hasnt") Op_(NCOND_NODE,  "test_attr",     3);
                else OnS_("far")   error("'far' is obselete and withdrawn");
                else OnS_("near")  error("'near' is obselete and withdrawn");
                else goto TryYetFurther;
            }
            last_was_openb=2;
            goto AtomDone;
        }

        TryYetFurther:

        if (resting==1)
        {   if (sub_buffer[0]=='-')
            {   Op_(ARITH_NODE,"add",4);
                grow_leaf(token-1); sofar_empty=0;
            }
            else
            {   if (bracket_level>0)
                error("Operator has too many arguments");
                goto BigBreak;
            }
        }
        else if (tree[dryad].type==BLANK_NODE)
             {   
                 if ((sub_buffer[0]=='-')
                     &&(last_was_openb==0)&&(last_was_comma==0))
                 {   Op_(ARITH_NODE,"add",4);
                     grow_leaf(token-1); sofar_empty=0;
                     goto AtomDone;
                 }

                 sofar_empty=0; grow_leaf(token-1); last_was_leaf=1;
                 goto UnlessItWas;
             }
             else
             if (sub_buffer[0]=='-')
                  {   Op_(ARITH_NODE,"add",4);
                      grow_leaf(token-1); sofar_empty=0;
                  }
                  else goto BigBreak;


        AtomDone:
        last_was_leaf=0;
        UnlessItWas: ;
    } while (1==1);
 
    BigBreak:
    if ((wd_tk==-1)&&(sub_buffer[0]==0)) next_token= -1;
    else next_token=token-1;

    if (bracket_level>0) error("Too many brackets '(' in expression");

    /* --------------------------------------------------------------------- */
    /*  4. If necessary, revise the tree to allow for get_prop_len and the   */
    /*     different contexts of the "=" assignment                          */
    /* --------------------------------------------------------------------- */

    if (revise_flag==1)
    {
        for (i=0; i<tree_size; i++)
        {   j=tree[i].b[0];
            if ((tree[i].type==STORE_NODE)&&(tree[j].type==ARITH_NODE)
                &&(tree[i].op!=NULL)&&(strcmp(tree[i].op,"store")==0))
            {   k=0;
                if (tree[j].op!=NULL)
                {   if (strcmp(tree[j].op,"get_prop")==0) k=1;
                    if (strcmp(tree[j].op,"loadb")==0) k=2;
                    if (strcmp(tree[j].op,"loadw")==0) k=3;
                }
                if (k!=0)
                {   
                    if (etrace_mode==1)
                        printf("Sub type %d nodes %d and %d\n",k,i,j);
                    if (etrace_mode==1) show_tree("Substituting from");
                    tree[j].b[tree[j].arity++] = tree[i].b[1];
                    tree[j].branches_made++;
                    tree[j].type=STORE_NODE;
                    tree[j].up=tree[i].up;
                    tree[i]=tree[j];
                    switch(k)
                    {   case 1: tree[i].op="put_prop"; break;
                        case 2: tree[i].op="storeb"; break;
                        case 3: tree[i].op="storew"; break;
                    }
                    if (etrace_mode==1) show_tree("Substituting to");
                }
            }
            if ((tree[i].op!=NULL)&&(strcmp(tree[i].op,"_get_prop_len")==0))
            {
                tree[i].op="get_prop_addr";
                tree[tree_size]=tree[i];
                tree[tree_size].up=i;
                tree[i].b[0]=tree_size;
                tree[i].arity=1;
                tree[i].branches_made=1;
                tree[i].op="get_prop_len";
                if (tree_size==MAX_EXPRESSION_NODES) goto TooComplexToo;
                tree_size++;
                if (etrace_mode==1) show_tree("Fixing a _get_prop_len to");
            }

            if (((tree[i].type==COND_NODE)||(tree[i].type==NCOND_NODE))
                &&(tree[i].op!=NULL)&&(strcmp(tree[i].op,"in")==0))
            {   if (etrace_mode==1) show_tree("Substituting 'in' from");
                j=tree[i].b[0];
                tree[i].op="je";
                tree[tree_size]=tree[j];
                tree[tree_size].up=j;
                tree[j].b[0]=tree_size;
                tree[j].arity=1;
                tree[j].type=ARITH_NODE;
                tree[j].branches_made=1;
                tree[j].op="get_parent";
                if (tree_size==MAX_EXPRESSION_NODES) goto TooComplexToo;
                tree_size++;
                if (etrace_mode==1) show_tree("Substituting to");
            }
        }
    }

    if (condition_context==1)
    {   fix_yesno_labels(tree[0].b[0],0);
    }

    if (etrace_mode>=1) show_tree("Made the tree:");

    /* --------------------------------------------------------------------- */
    /*  5. Recursively cut off branches into assembly language               */
    /* --------------------------------------------------------------------- */

    if (etrace_mode==2) printf("Compiling code:\n");

    do
    {   i=0;
        DownDown:
        if ((tree[i].type!=LOGICAND_NODE)&&(tree[i].type!=LOGICOR_NODE))
        {   for (j=tree[i].branches_made-1; j>=0; j--)
            {   t=tree[tree[i].b[j]].type;
                if ((t!=LEAF_NODE)&&(t!=SP_NODE)&&(t!=CDONE_NODE))
                {   i=tree[i].b[j]; goto DownDown;
                }
            }
        }
        else
        {   for (j=0; j<tree[i].branches_made; j++)
            {   t=tree[tree[i].b[j]].type;
                if ((t!=LEAF_NODE)&&(t!=SP_NODE)&&(t!=CDONE_NODE))
                {   i=tree[i].b[j]; goto DownDown;
                }
            }
        }

        if (etrace_mode==1) printf("Detaching %d\n",i);

        if (i==0)
        {   if (tree[tree[0].b[0]].type==SP_NODE) j=-1;
            else if (tree[tree[0].b[0]].type==CDONE_NODE) j=-3;
            else j=tree[tree[0].b[0]].word;
            if (its_void==1) j=-2;

            if ((condition_context==0)&&(j==-3))
            {   error("Unexpected condition"); return(-2); }

            if ((condition_context==1)&&(j!=-3))
            {   error("Expected condition but found expression");
                return(-2);
            }

            if (etrace_mode>=1)
            {   if (next_token==-1) printf("Completed: line used up: ");
                else
                {   word(sub_buffer,next_token);
                    printf("Completed: next word '%s': ",sub_buffer);
                }
                if (j>=0) word(sub_buffer,j);
                if (thrown_away_flag==1) printf("result thrown away\n");
                else if (j==-1) printf("result on stack\n");
                else if (j==-2) printf("no resulting value\n");
                else if (j==-3) printf("a condition\n");
                else printf("result in %s\n",sub_buffer);
            }
            if (thrown_away_flag==1) return(-3);
            return(j);
        }

        if (tree[i].branches_made<tree[i].arity)
            error("Operator has too few arguments");

        sysfun_arity=0; dummy_branch_flag=0;
        if (tree[i].type==FCALL_NODE)
        {   indirect_flag=0;
            InV5
            {   tree[i].op="call_***";
                call_args=tree[i].branches_made-1;
            }
            call_i=i; call_flag=1;
            eword(sub_buffer,tree[i].b[0]);

#define SysF_(x,n) if ((strcmp(sub_buffer,x)==0)&&(sys_functions[n]==0))

            SysF_("parent",0)    { sysfun_arity=1; tree[i].op="get_parent";    }
            SysF_("sibling",1)   { sysfun_arity=1; tree[i].op="get_sibling";
                                    dummy_branch_flag=1; }
            SysF_("younger",2)   { sysfun_arity=1; tree[i].op="get_sibling";
                                    dummy_branch_flag=1; }
            SysF_("child",3)     { sysfun_arity=1; tree[i].op="get_child";
                                    dummy_branch_flag=1; }
            SysF_("eldest",4)    { sysfun_arity=1; tree[i].op="get_child";
                                    dummy_branch_flag=1; }
            SysF_("random",5)    { sysfun_arity=1; tree[i].op="random";        }
            SysF_("prop_len",6)  { sysfun_arity=1; tree[i].op="get_prop_len";  }
            SysF_("prop_addr",7) { sysfun_arity=2; tree[i].op="get_prop_addr"; }
            SysF_("prop",8)      { sysfun_arity=2; tree[i].op="get_prop";      }
            SysF_("children",9)
            {   if (2!=tree[i].branches_made)
                    error("'children' takes a single argument");
                estack_line(         "@store temp_global 0");
                eword(sub_buffer,
                  tree[i].b[1]);
                sprintf(rewrite,     "@get_child %s sp ~_x%d",
                  sub_buffer,
                  no_dummy_labels+1);
                estack_line(rewrite);
                sprintf(rewrite,     "@._x%d",
                  no_dummy_labels++);
                estack_line(rewrite);
                estack_line(         "@inc temp_global");
                sprintf(rewrite,     "@get_sibling sp sp _x%d",
                  no_dummy_labels-1);
                estack_line(rewrite);
                sprintf(rewrite,     "@._x%d",
                  no_dummy_labels++);
                estack_line(rewrite);
                sprintf(rewrite,     "@add sp temp_global sp");
                estack_line(rewrite);
                tree[i].type=SP_NODE;
                tree[i].word=-1;
                goto Detached;
            }
            SysF_("youngest",10)
            {   if (2!=tree[i].branches_made)
                    error("'youngest' takes a single argument");
                eword(sub_buffer,tree[i].b[1]);
                sprintf(rewrite,     "@get_child %s temp_global ~_x%d",
                  sub_buffer,
                  no_dummy_labels+1);
                estack_line(rewrite);
                estack_line(         "@push temp_global");
                sprintf(rewrite,     "@._x%d",
                  no_dummy_labels++);
                estack_line(rewrite);
                estack_line(         "@store temp_global sp");
                sprintf(rewrite,     "@get_sibling temp_global sp _x%d",
                    no_dummy_labels-1);
                estack_line(rewrite);
                sprintf(rewrite,     "@._x%d",no_dummy_labels++);
                estack_line(rewrite);
                sprintf(rewrite,     "@push temp_global");
                estack_line(rewrite);
                tree[i].type=SP_NODE;
                tree[i].word=-1;
                goto Detached;
            }
            SysF_("elder",11)
            {   if (2!=tree[i].branches_made)
                    error("'elder' takes a single argument");
                eword(sub_buffer,tree[i].b[1]);
                sprintf(rewrite,     "@store temp_global %s",
                  sub_buffer);
                estack_line(rewrite);
                estack_line(         "@store temp_global3 0");
                estack_line(         "@get_parent temp_global sp");
                sprintf(rewrite,     "@get_child sp temp_global2 _x%d",
                  no_dummy_labels);
                estack_line(rewrite);
                sprintf(rewrite,     "@._x%d",no_dummy_labels++);
                estack_line(rewrite);
                sprintf(rewrite,     "@je temp_global temp_global2 _x%d",
                  no_dummy_labels);
                estack_line(rewrite);
                estack_line(         "@store temp_global3 temp_global2");
                sprintf(rewrite, "@get_sibling temp_global2 temp_global2 _x%d",
                  no_dummy_labels-1);
                estack_line(rewrite);
                sprintf(rewrite,     "@._x%d",
                  no_dummy_labels++);
                estack_line(rewrite);
                sprintf(rewrite,     "@push temp_global3");
                estack_line(rewrite);
                tree[i].type=SP_NODE;
                tree[i].word=-1;
                goto Detached;
            }

            SysF_("indirect",12)
            {   if (tree[i].branches_made<2)
                { error("'indirect' takes at least one argument"); }
                sysfun_arity=1; tree[i].op="icall"; indirect_flag=1;
            }
            else
            if (sysfun_arity!=0)
            {   if (sysfun_arity+1!=tree[i].branches_made)
                {   error("Wrong number of arguments to system function");
                }
            }
        }
        else call_flag=0;

        t=tree[i].type;

        if ((t==LOGICAND_NODE)||(t==LOGICOR_NODE))
        {   for (j=0; j<tree[i].branches_made; j++)
            {   if (tree[tree[i].b[j]].type != CDONE_NODE)
                {   error("Expected condition but found expression");
                    return(-2);
                }
            }
            sprintf(rewrite,"@._x%d",tree[i].label_number);
            estack_line(rewrite);
            tree[i].type=CDONE_NODE;
            goto Detached;
        }

        if (t==ALTERB_NODE)
        {   if (tree[tree[i].b[0]].type!=LEAF_NODE)
            {   error("'++' and '--' can only apply directly to variables");
                return(-2);
            }
            eword(sub_buffer,tree[i].b[0]);
            sprintf(rewrite,"@%s %s",tree[i].op,sub_buffer);
            estack_line(rewrite);
            tree[i].type=LEAF_NODE;
            tree[i].word=tree[tree[i].b[0]].word;
            tree[i].arity=0;
            tree[i].branches_made=0;
            if (tree[i].up==0)
                if (assign_context==1) its_void=1;
            goto Detached;
        }
        if (t==ALTERA_NODE)
        {   if (tree[tree[i].b[0]].type!=LEAF_NODE)
            {   error("'++' and '--' can only apply directly to variables");
                return(-2);
            }
            if ((tree[i].up==0)&&(assign_context==1))
            {   eword(sub_buffer,tree[i].b[0]);
                sprintf(rewrite,"@%s %s",tree[i].op,sub_buffer);
                estack_line(rewrite);
                tree[i].type=LEAF_NODE;
                tree[i].word=tree[tree[i].b[0]].word;
                tree[i].arity=0;
                tree[i].branches_made=0;
                if (assign_context==1) its_void=1;
                goto Detached;
            }
            eword(sub_buffer,tree[i].b[0]);
            sprintf(rewrite,"@push %s",sub_buffer);
            estack_line(rewrite);
            sprintf(rewrite,"@%s %s",tree[i].op,sub_buffer);
            estack_line(rewrite);
            tree[i].type=SP_NODE;
            tree[i].arity=0;
            tree[i].branches_made=0;
            goto Detached;
        }

        if (strcmp(tree[i].op,"get_prop_len")==0)
        {   eword(sub_buffer,tree[i].b[0]);
            sprintf(rewrite,"@store temp_global %s",sub_buffer);
            estack_line(rewrite);
            sprintf(rewrite,"@jz temp_global _x%d",no_dummy_labels);
            estack_line(rewrite);
            sprintf(rewrite,"@get_prop_len temp_global temp_global");
            estack_line(rewrite);
            sprintf(rewrite,"@._x%d",no_dummy_labels++);
            estack_line(rewrite);

            if ((tree[tree[i].up].op!=NULL) &&
                (strcmp(tree[tree[i].up].op,"store")==0))
            {   word(sub_buffer,tree[tree[tree[i].up].b[0]].word);
                i=tree[i].up; t=STORE_NODE;
                sprintf(rewrite,"@store %s temp_global",sub_buffer);
            }
            else sprintf(rewrite,"@push temp_global");

        }
        else
        {
            sprintf(rewrite,"@%s ",tree[i].op);

            for (j=(sysfun_arity>0)?1:0; j<tree[i].branches_made; j++)
            {   eword(sub_buffer,tree[i].b[j]);
                sprintf(rewrite+strlen(rewrite),"%s ",sub_buffer);
            }

            if ((sub_buffer[0]=='-') && (tree[i].op!=NULL)
                && (strcmp(tree[i].op,"add")==0))
            {   eword(sub_buffer,tree[i].b[0]);
                sprintf(rewrite,"@sub %s ",sub_buffer);
                eword(sub_buffer,tree[i].b[1]);
                sprintf(rewrite+strlen(rewrite),"%s ",sub_buffer+1);
            }

            if ((t==ARITH_NODE)||(t==FCALL_NODE))
            {   if ((tree[tree[i].up].op!=NULL) &&
                    (strcmp(tree[tree[i].up].op,"store")==0))
                {   word(sub_buffer,tree[tree[tree[i].up].b[0]].word);
                    i=tree[i].up; t=STORE_NODE;
                }
                else sprintf(sub_buffer,"sp");
                sprintf(rewrite+strlen(rewrite),"%s",sub_buffer);
            }
        }

        if (dummy_branch_flag==1)
        {   sprintf(rewrite+strlen(rewrite)," _x%d",no_dummy_labels);
        }
        if ((t==COND_NODE)||(t==NCOND_NODE))
        {   int pflag, lnumber, lat, tup;
            tup=tree[i].up; lat=tree[tup].label_number;
            switch(tree[tup].type)
            {   case ROOT_NODE: lnumber=-1; pflag=0; break;
                case LOGICAND_NODE:
                         if (tree[tup].b[tree[tup].branches_made-1]!=i)
                         {   lnumber=tree[tup].no_label; pflag=0;  }
                         else
                         {   if (lat==tree[tup].yes_label)
                             {   lnumber=tree[tup].no_label; pflag=0;  }
                             else
                             {   lnumber=tree[tup].yes_label; pflag=1; }
                         }
                         break;
                case LOGICOR_NODE:
                         if (tree[tup].b[tree[tup].branches_made-1]!=i)
                         {   lnumber=tree[tup].yes_label; pflag=1;  }
                         else
                         {   if (lat==tree[tup].no_label)
                             {   lnumber=tree[tup].yes_label; pflag=1;  }
                             else
                             {   lnumber=tree[tup].no_label; pflag=0; }
                         }
                         break;
                default:
                    error("Attempt to use a condition as a value");
                    return(-2);
            }
            if (pflag==0)
                sprintf(rewrite+strlen(rewrite),"?%s",
                    (t==COND_NODE)?"~":"");
            else
                sprintf(rewrite+strlen(rewrite),"?%s",
                    (t!=COND_NODE)?"~":"");
            if (lnumber==-1)
                sprintf(rewrite+strlen(rewrite),"%s",
                    condition_label);
            else
                sprintf(rewrite+strlen(rewrite),"_x%d",
                    lnumber);
        }

        if (call_flag==1)
        {   tosp_flag=0;
            if (strcmp(sub_buffer,"sp")==0) tosp_flag=1;

            InV3
            {
                if ((tosp_flag==1)&&(void_context==1)&&(tree[call_i].up==0))
                {   sprintf(rewrite+(strlen(rewrite)-2), "temp_global");
                    thrown_away_flag=1;
                }
            }

            if (actual_version == 4)
            {
                if (sysfun_arity==0)
                {   rewrite[7]='s';
                    switch(call_args)
                    {   case 0: rewrite[6]='1'; break;
                        case 1: rewrite[6]='2'; break;
                        default: rewrite[6]='v'; break;
                    }
                    if (call_args>3) rewrite[8]='2'; else rewrite[8]=' ';
                    if ((tosp_flag==1)&&(void_context==1)&&(tree[call_i].up==0))
                    {   sprintf(rewrite+(strlen(rewrite)-2), "temp_global");
                        thrown_away_flag=1;
                    }
                }
                if (indirect_flag==1)
                    if ((tosp_flag==1)&&(void_context==1)&&(tree[call_i].up==0))
                    {   sprintf(rewrite+(strlen(rewrite)-2), "temp_global");
                        thrown_away_flag=1;
                    }
            }

            if (actual_version >= 5)
            {
                if (sysfun_arity==0)
                {   rewrite[7]='s';
                    switch(call_args)
                    {   case 0: rewrite[6]='1'; break;
                        case 1: rewrite[6]='2'; break;
                        default: rewrite[6]='v'; break;
                    }
                    if (call_args>3) rewrite[8]='2'; else rewrite[8]=' ';
                    if ((tosp_flag==1)&&(void_context==1)&&(tree[call_i].up==0))
                    {   rewrite[7]='n';
                        rewrite[strlen(rewrite)-3]=0;
                        thrown_away_flag=1;
                    }
                }
                if (indirect_flag==1)
                    if ((tosp_flag==1)&&(void_context==1)&&(tree[call_i].up==0))
                    {   sprintf(rewrite+(strlen(rewrite)-2), "temp_global");
                        thrown_away_flag=1;
                    }
            }
        }

        estack_line(rewrite);

        if (dummy_branch_flag==1)
        {   sprintf(rewrite,"@._x%d",no_dummy_labels++);
            estack_line(rewrite);
        }

        if (t==STORE_NODE) tree[i]=tree[tree[i].b[0]];
        else
        {   tree[i].arity=0;
            tree[i].type= SP_NODE;
            tree[i].branches_made=0;
        }
        switch(t)
        {   default: tree[i].type = SP_NODE; break;
            case COND_NODE:
            case NCOND_NODE:
            case LOGICOR_NODE:
            case LOGICAND_NODE: tree[i].type = CDONE_NODE; break;
        }

        Detached:
        if (etrace_mode==1) show_tree("to");
    } while (1==1);
    return(1);

    TooComplexToo:
        memoryerror("MAX_EXPRESSION_NODES", MAX_EXPRESSION_NODES);
    return(1);
}

extern void assignment(int from, int flag)
{   int i;
    next_token=from;
    do
    {   assign_context=1; i=expression(next_token); assign_context=0;
        if ((i!=-2)&&(i!=from))
        {   error("Expected an assignment but found an expression");
            return;
        }
        if (i==from)
        {   word(sub_buffer,from);
            if (flag==1)
                error_named("Expected an assignment, command, directive \
or opcode but found",sub_buffer);
            else
                error_named("Expected an assignment but found",sub_buffer);
            return;
        }
        if (next_token==-1) return;
        word(sub_buffer,next_token);
        if (strcmp(sub_buffer,",")!=0) return;
        next_token++;
    } while (1==1);
}

extern void init_express_vars(void)
{   int i;
    dryad_veil = -1;
    condition_context = 0;
    assign_context = 0;
    for (i=0;i<16;i++) sys_functions[i]=0;
#ifdef ALLOCATE_BIG_ARRAYS
    tree = NULL;
#endif
}
