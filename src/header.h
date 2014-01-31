/* ------------------------------------------------------------------------- */
/*   Header file for Inform:  Infocom game ("Z-code") compiler               */
/*   (should be #included in the others)                                     */
/*                                                                           */
/*   Inform 5   Revision 5.5                                                 */
/*                                                                           */
/*   This header file and the others making up the Inform source code are    */
/*   copyright (c) Graham Nelson 1993, 1994, 1995                            */
/*                                                                           */
/*   Manuals for this language are available from the if-archive at          */
/*   ftp.gmd.de.                                                             */
/*   For notes on how this program may legally be used, see the Designer's   */
/*   Manual introduction.  (Any recreational use is fine, and so is some     */
/*   commercial use.)  There is also a Technical Manual which may be of      */
/*   interest to porters and hackers but probably not Inform programmers.    */
/* ------------------------------------------------------------------------- */

#define RELEASE_STRING "Release 5.5 (July 3rd 1995)"
#define RELEASE_NUMBER 1502

/* ------------------------------------------------------------------------- */
/*   Our machine for today is...                                             */
/*                                                                           */
/*   [ Inform should compile (possibly with warnings) and work safely        */
/*     if you just:                                                          */
/*                                                                           */
/*     #define AMIGA       -  for the Commodore Amiga under SAS/C            */
/*     #define ARCHIMEDES  -  for the Acorn Archimedes under Norcroft C      */
/*     #define ATARIST     -  for the Atari ST                               */
/*     #define LINUX       -  for Linux under gcc (essentially as Unix)      */
/*     #define MACINTOSH   -  for the Apple Mac under Think C or Codewarrior */
/*     #define MAC_68K     -  for 68K Macs, under Think C or Codewarrior     */
/*     #define MAC_MPW     -  for MPW under Codewarrior (and maybe Think C)  */
/*     #define OS2         -  for OS/2 32-bit mode under IBM's C Set++       */
/*     #define PC          -  for 386+ IBM PCs, eg. Microsoft Visual C/C++   */
/*     #define PC_QUICKC   -  for small IBM PCs under QuickC                 */
/*     #define UNIX        -  for Unix under gcc (or big IBM PC under djgpp) */
/*     #define VMS         -  for VAX or ALPHA under DEC C, but not VAX C    */
/*                                                                           */
/*     See the notes below.  Executables may already be available by ftp.]   */
/*                                                                           */
/*   (If no machine is defined, then cautious #defines will be made.)        */
/* ------------------------------------------------------------------------- */

/* #define GRAHAM */

/* ------------------------------------------------------------------------- */
/*   The other #definable options (some of them set by the above) are:       */
/*                                                                           */
/*   USE_TEMPORARY_FILES - use scratch files for workspace, not memory       */
/*   PROMPT_INPUT        - prompt input (don't use Unix-style command line)  */
/*   TIME_UNAVAILABLE    - don't use ANSI time routines to work out today's  */
/*                         date                                              */
/*   US_POINTERS         - make some of the checksum routine pointers        */
/*                         unsigned char *'s, not char *'s: this should be   */
/*                         used if your compiler does un-ANSI things with    */
/*                         casts between these types                         */
/* ------------------------------------------------------------------------- */
/*   Hello, Porter!                                                          */
/*   For notes on making a new port of Inform, see the Technical Manual.     */
/* ------------------------------------------------------------------------- */
/*   ARCHIMEDES "port" (the original)                                        */
/*     ...incorporates three Archimedes-only #define'able options:           */
/*                                                                           */
/*   ARC_PROFILING       - do clumsy profiling                               */
/*   ARC_THROWBACK       - compile to be useful under the DDE as well as the */
/*                         command line, including throwback of errors into  */
/*                         !SrcEdit (code donated by Robin Watts)            */
/*   GRAHAM              - for Graham's machine only                         */
/*                                                                           */
/*   (link with ansilib only, not RISC_OSlib)                                */
/* ------------------------------------------------------------------------- */
/*   UNIX port (by Dilip Sequeira):  A "vanilla" makefile for gcc is:        */
/*                                                                           */
/*     OBJECTS = files.o asm.o inputs.o symbols.o zcode.o tables.o inform.o  */
/*               express.o                                                   */
/*     CC = gcc                                                              */
/*     CFLAGS = -O2 -finline-functions -fomit-frame-pointer                  */
/*                                                                           */
/*     inform: $(OBJECTS)                                                    */
/*      $(CC) -o inform $(OBJECTS)                                           */
/*                                                                           */
/*     $(OBJECTS): header.h Makefile                                         */
/*                                                                           */
/*   (making an executable of size about 100K on a SparcStation II).         */
/*   The temporary files option is not on by default, but if it is used,     */
/*     file names are used which contain the process ID.                     */
/* ------------------------------------------------------------------------- */
/*   AMIGA port (by Christopher A. Wichura)                                  */
/*                                                                           */
/*     (Because of the way this compiler handles unsigned char *'s the       */
/*        #define AMIGA option forces #define US_POINTERS.)                  */
/*                                                                           */
/*     "I am compiling with SAS/C v6.51.  To support building a GST file     */
/*     (essentially a pre-compiled header file), I had to add the file       */
/*     amiheadermaker.c, which I have also included.  Finally, I am also     */
/*     including a copy of the SCOPTIONS file."                              */
/*                                                                           */
/*      --- BEGIN smakefile ---                                              */
/*      # smake file for Inform, the Infocom compiler                        */
/*      # created 1/24/94 by Christopher A. Wichura (caw@miroc.chi.il.us)    */
/*                                                                           */
/*      PROGNAME = Inform                                                    */
/*                                                                           */
/*      CFLAGS = nostkchk strmerge parms=reg utillib optimize                */
/*      LFLAGS = smallcode smalldata stripdebug                              */
/*                                                                           */
/*      HDR = $(PROGNAME).gst                                                */
/*                                                                           */
/*      OBJS = asm.o files.o inform.o inputs.o symbols.o tables.o zcode.o    */
/*             express.o                                                     */
/*      LIBS = LIB:sc.lib LIB:debug.lib LIB:amiga.lib                        */
/*                                                                           */
/*      .c.o                                                                 */
/*       sc $(CFLAGS) gst=$(HDR) $*                                          */
/*                                                                           */
/*      $(PROGNAME): $(OBJS)                                                 */
/*       slink with lib:utillib.with <WITH < (withfile.lnk)                  */
/*      FROM LIB:cres.o $(OBJS)                                              */
/*      TO $(PROGNAME)                                                       */
/*      LIB $(LIBS)                                                          */
/*      $(LFLAGS)                                                            */
/*      MAP $(PROGNAME).map fhlsx plain                                      */
/*      <                                                                    */
/*                                                                           */
/*      # clean target                                                       */
/*      clean:                                                               */
/*       delete \#?.o $(PROGNAME).gst $(PROGNAME).map                        */
/*                                                                           */
/*      # dependancies of various modules                                    */
/*      $(HDR): header.h                                                     */
/*       sc $(CFLAGS) noobjname makegst=$(HDR) amiheadermaker.c              */
/*                                                                           */
/*      asm.o: asm.c $(HDR)                                                  */
/*                                                                           */
/*      files.o: files.c $(HDR)                                              */
/*                                                                           */
/*      inform.o: inform.c $(HDR)                                            */
/*                                                                           */
/*      express.o: express.c $(HDR)                                          */
/*                                                                           */
/*      inputs.o: inputs.c $(HDR)                                            */
/*                                                                           */
/*      symbols.o: symbols.c $(HDR)                                          */
/*                                                                           */
/*      tables.o: tables.c $(HDR)                                            */
/*                                                                           */
/*      zcode.o: zcode.c $(HDR)                                              */
/*                                                                           */
/*      --- END smakefile ---                                                */
/*                                                                           */
/*      --- BEGIN amiheadermaker.c ---                                       */
/*      // file used to build the Amiga GST file.                            */
/*      #include "header.h"                                                  */
/*                                                                           */
/*      --- END amiheadermaker.c ---                                         */
/*                                                                           */
/*      --- BEGIN SCOPTIONS                                                  */
/*      MemorySize=HUGE                                                      */
/*      IncludeDir=SINCLUDE:                                                 */
/*      OptimizerComplexity=15                                               */
/*      OptimizerDepth=5                                                     */
/*      OptimizerRecurDepth=15                                               */
/*      OptimizerSchedule                                                    */
/*      OptimizerInLocal                                                     */
/*      Verbose                                                              */
/*      --- END SCOPTIONS                                                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */
/*   Microsoft Visual C/C++ port (for the PC) (by Toby Nelson)               */
/*                                                                           */
/*  "The following makefile is for the "Microsoft Visual C++ V1.00"          */
/*  development environment, of which the important bit is the "Microsoft    */
/*  C/C++ Optimizing Compiler Version 8.00". It may well work for previous   */
/*  versions of the compiler too.                                            */
/*                                                                           */
/*        CFLAGS = /nologo /Ox /W0 /AL                                       */
/*        goal: inform.exe                                                   */
/*                                                                           */
/*        .obj: .c                                                           */
/*        cl *.c                                                             */
/*                                                                           */
/*        inform.exe: asm.obj files.obj inform.obj inputs.obj symbols.obj \  */
/*            tables.obj zcode.obj express.obj                               */
/*        link /STACK:16384 asm files inform inputs symbols tables \         */
/*            zcode express, inform.exe,,llibce.lib,,                        */
/*                                                                           */
/*  (NB: The compiler options used here are:                                 */
/*                                                                           */
/*        /nologo = no copyright message,                                    */
/*        /Ox = optimize for speed,                                          */
/*        /W0 = turn off all compiler warnings, and                          */
/*        /AL = compile for the large memory model.)"                        */
/*                                                                           */
/* ------------------------------------------------------------------------- */
/*   Quick C port (for the PC) (by Bob Newell): Makefile...                  */
/*                                                                           */
/*        CFLAGS =   /DPC_QUICKC /AL  /DJ                                    */
/*        CC = qcl                                                           */
/*                                                                           */
/*        inform.exe:  asm.obj files.obj inform.obj inputs.obj symbols.obj \ */
/*            tables.obj zcode.obj express.obj                               */
/*            qlink /STACK:16384 asm.obj files.obj inform.obj inputs.obj \   */
/*             symbols.obj tables.obj zcode.obj express.obj, inform.exe,, \  */
/*             llibce.lib,,                                                  */
/* ------------------------------------------------------------------------- */
/*   Atari ST port (by Charles Briscoe-Smith): Makefile...                   */
/*                                                                           */
/*        OBJS = files.o asm.o inputs.o symbols.o zcode.o tables.o inform.o  */
/*               express.o                                                   */
/*                                                                           */
/*        CC = gcc                                                           */
/*                                                                           */
/*        # I didn't use the optimisation flags, because of memory shortage. */
/*        # If you have enough memory, try uncommenting the "CFLAGS=" line.  */
/*                                                                           */
/*        #CFLAGS = -O2 -finline-functions -fomit-frame-pointer              */
/*                                                                           */
/*        inform.ttp : $(OBJS)                                               */
/*              $(CC) $(CFLAGS) -o inform.ttp $(OBJS)                        */
/*                                                                           */
/*        $(OBJS): header.h                                                  */
/*                                                                           */
/*   This port contains a TOSFS option: if defined, then temporary files are */
/*   named in DOS style, and if not then in Unix style (in any case, this    */
/*   only matters if USE_TEMPORARY_FILES is set).                            */
/* ------------------------------------------------------------------------- */
/*   OS/2 port (by John W. Kennedy):                                         */
/*                                                                           */
/*   "The program compiles and runs correctly as-is... I found it convenient */
/*   to create a dummy h.version file, to prevent problems with makemake.    */
/*   Since the file is not actually used, the contents don't matter...       */
/*   ...it can be assumed that later this year it [Inform] will port         */
/*   successfully to WorkPlace OS on the PowerPC, as well."                  */
/* ------------------------------------------------------------------------- */
/*   Makefile for Linux (by Spedge, aka Dark Mage)                           */
/*                                                                           */
/*    CC      = gcc                                                          */
/*    CFLAGS  = -Dlinux                                                      */
/*    LDFLAGS =                                                              */
/*    LIBS    =                                                              */
/*                                                                           */
/*    PROG    = inform                                                       */
/*    SUBDIRS = data                                                         */
/*    BINDIR  = /usr/local/games/infocom                                     */
/*    DATADIR = /usr/local/games/infocom/data                                */
/*    MANDIR  = /usr/local/man/man1                                          */
/*    INSTALL = install                                                      */
/*    RM      = rm                                                           */
/*    CD      = cd                                                           */
/*    CP      = cp                                                           */
/*                                                                           */
/*    INC     = header.h                                                     */
/*    OBJS    = inform.o asm.o inputs.o files.o symbols.o tables.o zcode.o   */
/*              express.o                                                    */
/*                                                                           */
/*    all: $(PROG)                                                           */
/*                                                                           */
/*    inform : $(OBJS)                                                       */
/*            $(CC) -o $(PROG) $(LDFLAGS) $(OBJS) $(LIBS)                    */
/*                                                                           */
/*    $(OBJS) : $(INC)                                                       */
/*                                                                           */
/*    install: $(PROG)                                                       */
/*            $(INSTALL) $(PROG) $(BINDIR)                                   */
/*      @for d in $(SUBDIRS); do (cd $$d && $(MAKE) install) || exit; done   */
/*                                                                           */
/*    install.man:                                                           */
/*            $(CP) $(PROG).1 $(MANDIR)                                      */
/*                                                                           */
/*    clean :                                                                */
/*            $(RM) -f *.o $(PROG)                                           */
/*                                                                           */
/* ------------------------------------------------------------------------- */
/*   Apple Macintosh port (by Robert Pelak):                                 */
/*                                                                           */
/*      A working version of Inform can be produced using either THINK C     */
/*      or Codewarrior.  Inform will need to use a console package (the      */
/*      THINK C console or Sioux are adequate) for input and output, but     */
/*      these are usually added automatically.  In older versions of THINK   */
/*      C, FAR DATA needs to be selected before compilation.  Also, I've     */
/*      experienced some difficulty with one set of pointers when working    */
/*      under THINK C; you can correct for this by issuing a                 */
/*                                                                           */
/*               #define MAC_68K                                             */
/*                                                                           */
/*      just after #define MACINTOSH.                                        */
/*                                                                           */
/*      (Defining the MAC_FACE option as well provides compatibility with    */
/*      Robert's Inform front-end - GN.)                                     */
/*                                                                           */
/*   For support for the Macintosh Programmer's Workshop (added by Robert    */
/*   Stone), #define MAC_MPW.                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */
/*   VMS port (VAX or ALPHA, DEC C only) (by David Wagner):                  */
/*                                                                           */
/*      Compile all the .c files, and link.                                  */
/*      To run, define a foreign symbol:                                     */
/*                                                                           */
/*         $ INFORM :== $disk:[directory]INFORM.EXE                          */
/*                                                                           */
/*      then, e.g.,                                                          */
/*                                                                           */
/*         $ INFORM -dx shell                                                */
/*                                                                           */
/*      (Enclose the options in quotes if some are in upper case)            */
/* ------------------------------------------------------------------------- */
/*                                                                           */
/*   The modification history and source code map have been moved to the     */
/*   Technical Manual.                                                       */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#define ALLOCATE_BIG_ARRAYS

#define LARGE_SIZE   1
#define SMALL_SIZE   2
#define HUGE_SIZE    3

/* ------------------------------------------------------------------------- */
/*   By setting up the prefixes and extensions in the definitions below, you */
/*   should be able to get something sensible for your filing system.        */
/*   In the last resort, the clumsy "z3", "z5" etc prefixes below are chosen */
/*   to cause least offense to different filing systems.                     */
/*   Note that if both Code_Prefix and Code_Extension are empty, then Inform */
/*   may overwrite its source code with the object code... so don't allow    */
/*   this.                                                                   */
/*   (For Unix and PCs the extension is ".z3" or ".z5" rather than ".zip"    */
/*   to avoid looking like the file compression trailer...)                  */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#ifdef GRAHAM
#define ARCHIMEDES
#define DEFAULT_MEMORY_SIZE LARGE_SIZE
#define ARC_THROWBACK
#endif

#define VNUMBER RELEASE_NUMBER

#ifdef ARCHIMEDES
#define MACHINE_STRING   "Archimedes"
#define Source_Prefix    "inform."
#define Source_Extension ""
#define Include_Prefix   "library."
#define Code_Prefix      "games."
#define Code_Extension   ""
#define Transcript_Name  "Game_Text"
#define Debugging_Name   "Game_Debug"
#define USE_TEMPORARY_FILES
#define Temp1_Name temporary_name(1)
#define Temp2_Name temporary_name(2)
#ifdef ARC_PROFILING
     extern int _fmapstore(char *);
#endif
#endif

#ifdef UNIX
  #ifdef LINUX
  #define MACHINE_STRING "Linux"
  #else
  #define MACHINE_STRING   "Unix"
  #endif
#define Source_Prefix    ""
#define Source_Extension ".inf"
#define Include_Extension ".h"
#define Code_Prefix      ""
#define Code_Extension   ".z3"
#define V4Code_Extension ".z4"
#define V5Code_Extension ".z5"
#define V6Code_Extension ".z6"
#define V7Code_Extension ".z7"
#define V8Code_Extension ".z8"
#define Transcript_Name "game.txt"
#define Debugging_Name  "game.dbg"
extern char Temp1_Name[], Temp2_Name[];
#define Temp1_Hdr "/tmp/InformTemp1"
#define Temp2_Hdr "/tmp/InformTemp2"
#define DEFAULT_MEMORY_SIZE LARGE_SIZE
#define US_POINTERS
#endif

#ifdef PC_QUICKC
#define PC
#endif

#ifdef PC
#define MACHINE_STRING   "PC"
#define Source_Prefix    ""
#define Source_Extension ".inf"
#define Include_Extension ".h"
#define Code_Prefix      ""
#define Code_Extension   ".z3"
#define V4Code_Extension ".z4"
#define V5Code_Extension ".z5"
#define V6Code_Extension ".z6"
#define V7Code_Extension ".z7"
#define V8Code_Extension ".z8"
#define Temp1_Name "Inftmp1.tmp"
#define Temp2_Name "Inftmp2.tmp"
#define Transcript_Name "game.txt"
#define Debugging_Name  "game.dbg"
#define USE_TEMPORARY_FILES
#define US_POINTERS
#define DEFAULT_ERROR_FORMAT 1
#endif

#ifdef __VMS
#define VMS
#endif
#ifdef VMS
#ifdef __ALPHA
#define MACHINE_STRING "Alpha/VMS"
#else
#define MACHINE_STRING "VAX/VMS"
#endif
#define Source_Prefix    ""
#define Source_Extension ".inf"
#define Include_Extension ".h"
#define Code_Prefix      ""
#define Code_Extension   ".zip"
#define V4Code_Extension ".zip"
#define V5Code_Extension ".zip"
#define V6Code_Extension ".zip"
#define V7Code_Extension ".zip"
#define V8Code_Extension ".zip"
#define Transcript_Name "game.txt"
#define Debugging_Name  "game.dbg"
#define Temp1_Name "Inftmp1.tmp"
#define Temp2_Name "Inftmp2.tmp"
#define DEFAULT_MEMORY_SIZE LARGE_SIZE
#endif

#ifdef AMIGA
#define MACHINE_STRING   "Amiga"
#define Source_Prefix    ""
#define Source_Extension ".inf"
#define Include_Extension ".h"
#define Code_Prefix      ""
#define Code_Extension   ".z3"
#define V4Code_Extension ".z4"
#define V5Code_Extension ".z5"
#define V6Code_Extension ".z6"
#define V7Code_Extension ".z7"
#define V8Code_Extension ".z8"
#define Transcript_Name "game.txt"
#define Debugging_Name  "game.dbg"
extern char Temp1_Name[], Temp2_Name[];
#define Temp1_Hdr "T:InformTemp1"
#define Temp2_Hdr "T:InformTemp2"
#define __USE_SYSBASE
#include <proto/exec.h>
#define US_POINTERS
#define DEFAULT_MEMORY_SIZE LARGE_SIZE
#endif

#ifdef ATARIST
#define MACHINE_STRING   "Atari ST"
#define Source_Prefix    ""
#define Source_Extension ".inf"
#define Include_Extension ".h"
#define Code_Prefix      ""
#define Code_Extension   ".z3"
#define V4Code_Extension ".z4"
#define V5Code_Extension ".z5"
#define V6Code_Extension ".z6"
#define V7Code_Extension ".z7"
#define V8Code_Extension ".z8"
#define Transcript_Name "game.txt"
#define Debugging_Name  "game.dbg"
#ifdef TOSFS
#define Temp1_Name "Inftmp1.tmp"
#define Temp2_Name "Inftmp2.tmp"
#else
char Temp1_Name[50], Temp2_Name[50];
#define Temp1_Hdr "/tmp/InformTemp1"
#define Temp2_Hdr "/tmp/InformTemp2"
#endif
#endif /* ATARIST */

#ifdef OS2
#define MACHINE_STRING   "OS/2"
#define Source_Prefix    ""
#define Source_Extension ".inf"
#define Include_Extension ".h"
#define Code_Prefix      ""
#define Code_Extension   ".z3"
#define V4Code_Extension ".z4"
#define V5Code_Extension ".z5"
#define V6Code_Extension ".z6"
#define V7Code_Extension ".z7"
#define V8Code_Extension ".z8"
#define Temp1_Name "Inftemp1"
#define Temp2_Name "Inftemp2"
#define Transcript_Name "game.txt"
#define Debugging_Name  "game.dbg"
#define DEFAULT_MEMORY_SIZE LARGE_SIZE
#endif

#ifdef MAC_MPW
#define MACINTOSH
#endif

#ifdef MACINTOSH
#ifdef MAC_MPW
#define MACHINE_STRING   "Macintosh Programmer's Workshop"
#define Include_Extension ".h"
#else
#define MACHINE_STRING   "Macintosh"
#define Include_Extension ""
#endif
#define Source_Prefix    ""
#define Code_Prefix      ""
#define Temp1_Name "Inftemp1"
#define Temp2_Name "Inftemp2"
#define US_POINTERS
#define DEFAULT_MEMORY_SIZE LARGE_SIZE
#ifndef MAC_FACE
#ifndef MAC_MPW
#define PROMPT_INPUT
#endif
#define Transcript_Name "game.text"
#define Debugging_Name  "game.debug"
#define Source_Extension ".inf"
#define Code_Extension   ".z"
#define V4Code_Extension ".z"
#define V5Code_Extension ".z"
#define V6Code_Extension ".z"
#define V7Code_Extension ".z"
#define V8Code_Extension ".z"
#endif
#ifdef MAC_FACE
#define Source_Extension ""
#define Code_Prefix      ""
#define V5Code_Extension ""
#define V6Code_Extension ""
#define V7Code_Extension ""
#define V8Code_Extension ""
extern char *Transcript_Name;
extern char *Debugging_Name;
#endif
#endif

/* Default settings: */

#ifndef Source_Prefix
#define Source_Prefix    ""
#define Source_Extension ""
#define Code_Prefix      "z3"
#define V5Code_Prefix    "z5"
#define Code_Extension   ""
#define Temp1_Name "Inftemp1"
#define Temp2_Name "Inftemp2"
#endif

#ifndef Include_Prefix
#define Include_Prefix Source_Prefix
#endif
#ifndef Include_Extension
#define Include_Extension Source_Extension
#endif

#ifndef V4Code_Prefix
#define V4Code_Prefix Code_Prefix
#endif
#ifndef V4Code_Extension
#define V4Code_Extension Code_Extension
#endif
#ifndef V5Code_Prefix
#define V5Code_Prefix Code_Prefix
#endif
#ifndef V5Code_Extension
#define V5Code_Extension Code_Extension
#endif
#ifndef V6Code_Prefix
#define V6Code_Prefix Code_Prefix
#endif
#ifndef V6Code_Extension
#define V6Code_Extension Code_Extension
#endif
#ifndef V7Code_Prefix
#define V7Code_Prefix Code_Prefix
#endif
#ifndef V7Code_Extension
#define V7Code_Extension Code_Extension
#endif
#ifndef V8Code_Prefix
#define V8Code_Prefix Code_Prefix
#endif
#ifndef V8Code_Extension
#define V8Code_Extension Code_Extension
#endif

#ifndef MAC_FACE
#ifndef Transcript_Name
#define Transcript_Name "thetext"
#endif

#ifndef Debugging_Name
#define Debugging_Name "debuginf"
#endif
#endif

#ifndef DEFAULT_MEMORY_SIZE
#define DEFAULT_MEMORY_SIZE SMALL_SIZE
#endif

#ifndef DEFAULT_ERROR_FORMAT
#define DEFAULT_ERROR_FORMAT 0
#endif

/* ------------------------------------------------------------------------- */
/*   Inclusions and some macro definitions...                                */
/*   (At this point in earlier releases, there were many #define's for       */
/*    memory limits and sizes of arrays: see "files" for the new code.)      */
/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#ifdef MAC_FACE
#include <setjmp.h>
#endif
#ifdef MAC_MPW
#include <CursorCtl.h>
#endif

#define  MAX_ERRORS 100
#define  MAX_BLOCK_NESTING 32
#define  MAX_EXPRESSION_BRACKETS 32
#define  MAX_ARITY 9
#define  MAX_IFDEF_DEPTH 32
#define  MAX_IDENTIFIER_LENGTH 32
#define  MAX_ABBREV_LENGTH 64
#define  MAX_INCLUSION_DEPTH 4

/* ------------------------------------------------------------------------- */
/*   Twisting the C compiler's arm to get a convenient 32-bit integer type   */
/*   Warning: chars are presumed unsigned in this code, which I think is     */
/*   ANSI std; but they were presumed signed by K&R, so confusion reigns.    */
/*   Anyway a compiler ought to be able to cast either way as needed.        */
/*   Subtracting pointers is in a macro here for convenience: if even 32 bit */
/*   ints won't reliably hold pointers on your machine, rewrite properly     */
/*   using ptrdiff_t                                                         */
/* ------------------------------------------------------------------------- */

#ifndef VAX
#if   SCHAR_MAX >= 0x7FFFFFFFL && SCHAR_MIN <= -0x7FFFFFFFL
      typedef signed char       int32; 
      typedef unsigned char     uint32; 
#elif SHRT_MAX >= 0x7FFFFFFFL  && SHRT_MIN <= -0x7FFFFFFFL
      typedef signed short int  int32;
      typedef unsigned short int uint32;
#elif INT_MAX >= 0x7FFFFFFFL   && INT_MIN <= -0x7FFFFFFFL
      typedef signed int        int32;
      typedef unsigned int      uint32;
#elif LONG_MAX >= 0x7FFFFFFFL  && LONG_MIN <= -0x7FFFFFFFL
      typedef signed long int   int32;
      typedef unsigned long int uint32;
#else
      #error No type large enough to support 32-bit integers.
#endif
#else
      typedef int int32;
      typedef unsigned int uint32;
#endif

#ifdef PC_QUICKC
    void _huge * halloc(long, size_t);
    void hfree(void *);
#define subtract_pointers(p1,p2) (long)((char _huge *)p1-(char _huge *)p2)
#else
#define subtract_pointers(p1,p2) (((int32) p1)-((int32) p2))
#endif

#ifdef US_POINTERS
    typedef unsigned char zip;
#else
    typedef char zip;
#endif

extern int BUFFER_LENGTH;
extern int MAX_QTEXT_SIZE;
extern int MAX_SYMBOLS;
extern int MAX_BANK_SIZE;
extern int SYMBOLS_CHUNK_SIZE;
extern int BANK_CHUNK_SIZE;
extern int HASH_TAB_SIZE;

extern int MAX_OBJECTS;

extern int MAX_ACTIONS;
extern int MAX_ADJECTIVES;
extern int MAX_DICT_ENTRIES;
extern int MAX_STATIC_DATA;

extern int MAX_TOKENS;
extern int MAX_OLDEPTH;
extern int MAX_ROUTINES;
extern int MAX_GCONSTANTS;

extern int MAX_PROP_TABLE_SIZE;

extern int MAX_FORWARD_REFS;

extern int STACK_SIZE;
extern int STACK_LONG_SLOTS;
extern int STACK_SHORT_LENGTH;

extern int MAX_ABBREVS;

extern int MAX_EXPRESSION_NODES;
extern int MAX_VERBS;
extern int MAX_VERBSPACE;

extern int32 MAX_STATIC_STRINGS;
extern int32 MAX_ZCODE_SIZE;

extern int MAX_LOW_STRINGS;

extern int32 MAX_TRANSCRIPT_SIZE;

extern int MAX_CLASSES;
extern int MAX_CLASS_TABLE_SIZE;

/* ------------------------------------------------------------------------- */
/*   If your compiler doesn't recognise \t, and you use ASCII, you could     */
/*   define T_C as (char) 9; failing that, it _must_ be defined as a space   */
/*   and is _not_ allowed to be 0 or any recognisable character.             */
/* ------------------------------------------------------------------------- */

#define TAB_CHARACTER '\t'

/* ------------------------------------------------------------------------- */
/*  This hideous line is here only for checking on my machine that Inform    */
/*  runs properly when int is 16-bit                                         */
/* ------------------------------------------------------------------------- */

/* #define int short int */

/* ------------------------------------------------------------------------- */
/*   Structure definitions (there are a few others local to files)           */
/* ------------------------------------------------------------------------- */

typedef struct sourcefile
{   FILE *handle;
    char filename[64];
    int  source_line;
    int  sys_flag;
    int  file_no;
    int32 chars_read;
    int32 line_start;
} Sourcefile;

typedef struct opcode
{   zip *name;
    int code;
    int32 offset;
    int type1, type2, no;
} opcode;

typedef struct operand_t
{   int32 value; int type;
} operand_t;

typedef struct verbl {
    unsigned char e[8];
} verbl;

#define MAX_LINES_PER_VERB 20

typedef struct verbt {
    int lines;
    verbl l[MAX_LINES_PER_VERB];
} verbt;

typedef struct prop {
    unsigned char l, num, p[64];
} prop;

typedef struct propt {
    char l;
    prop pp[32];
} propt;

typedef struct fpropt {
    unsigned char atts[6];
    char l;
    prop pp[32];
} fpropt;

typedef struct objectt {
    unsigned char atts[6];
    int parent, next, child;
    int propsize;
} objectt;

typedef struct dict_word {
    unsigned char b[6];
} dict_word;

typedef struct dbgl_s
{   int b1, b2, b3;
} dbgl;

/* ------------------------------------------------------------------------- */
/*   Opcode type definitions                                                 */
/* ------------------------------------------------------------------------- */

#define NONE    0
#define STORE   1
#define BRANCH  2
#define CALL    3
#define JUMP    4
#define RETURN  5
#define NCALL   6
#define PCHAR   7
#define VATTR   8
#define ILLEGAL 9
#define INDIR  10

#define VAR     1
#define TEXT    2
#define OBJECT  3

#define VARI   -1
#define ZERO    0
#define ONE     1
#define TWO     2
#define EXTD    3
#define MANY    4

#define INVALID 100

/* ------------------------------------------------------------------------- */
/*   Inform code definitions                                                 */
/* ------------------------------------------------------------------------- */

#define ABBREVIATE_CODE  0
#define ATTRIBUTE_CODE   1
#define CONSTANT_CODE    2
#define DICTIONARY_CODE  3
#define END_CODE         4
#define INCLUDE_CODE     5
#define GLOBAL_CODE      6
#define OBJECT_CODE      7
#define PROPERTY_CODE    8
#define RELEASE_CODE     9
#define SWITCHES_CODE    10
#define STATUSLINE_CODE  11
#define VERB_CODE        12
#define TRACE_CODE       13
#define NOTRACE_CODE     14
#define ETRACE_CODE      15
#define NOETRACE_CODE    16
#define BTRACE_CODE      17
#define NOBTRACE_CODE    18
#define LTRACE_CODE      19
#define NOLTRACE_CODE    20
#define ATRACE_CODE      21
#define NOATRACE_CODE    22
#define LISTSYMBOLS_CODE 23
#define LISTOBJECTS_CODE 24
#define LISTVERBS_CODE   25
#define LISTDICT_CODE    26
#define OPENBLOCK_CODE   27
#define CLOSEBLOCK_CODE  28
#define SERIAL_CODE      29
#define DEFAULT_CODE     30
#define STUB_CODE        31
#define VERSION_CODE     32
#define IFV3_CODE        33
#define IFV5_CODE        34
#define IFDEF_CODE       35
#define IFNDEF_CODE      36
#define ENDIF_CODE       37
#define IFNOT_CODE       38
#define LOWSTRING_CODE   39
#define CLASS_CODE       40
#define FAKE_ACTION_CODE 41
#define NEARBY_CODE      42
#define SYSTEM_CODE      43
#define REPLACE_CODE     44
#define EXTEND_CODE      45
#define ARRAY_CODE       46

#define PRINT_ADDR_CODE  0
#define PRINT_CHAR_CODE  1
#define PRINT_PADDR_CODE 2
#define PRINT_OBJ_CODE   3 
#define PRINT_NUM_CODE   4
#define REMOVE_CODE      5
#define RETURN_CODE      6
#define DO_CODE          7
#define FOR_CODE         8
#define IF_CODE          9
#define OBJECTLOOP_CODE  10
#define UNTIL_CODE       11
#define WHILE_CODE       12
#define BREAK_CODE       13
#define ELSE_CODE        14
#define GIVE_CODE        15
#define INVERSION_CODE   16
#define MOVE_CODE        17
#define PUT_CODE         18
#define WRITE_CODE       19
#define STRING_CODE      20
#define FONT_CODE        21
#define READ_CODE        22
#define STYLE_CODE       23
#define RESTORE_CODE     24
#define SAVE_CODE        25
#define PRINT_CODE       26
#define SPACES_CODE      27
#define PRINT_RET_CODE   28
#define BOX_CODE         29
#define SWITCH_CODE      30
#define JUMP_CODE        31

#define ASSIGNMENT_CODE  100
#define FUNCTION_CODE    101

#define ARROW_SEP        0
#define DARROW_SEP       1
#define DEC_SEP          2
#define MINUS_SEP        3
#define INC_SEP          4
#define PLUS_SEP         5
#define TIMES_SEP        6
#define DIVIDE_SEP       7
#define REMAINDER_SEP    8
#define LOGOR_SEP        9
#define ARTOR_SEP       10
#define LOGAND_SEP      11
#define ARTAND_SEP      12
#define CONDEQUALS_SEP  13
#define SETEQUALS_SEP   14
#define NOTEQUAL_SEP    15
#define GE_SEP          16
#define GREATER_SEP     17
#define LE_SEP          18
#define LESS_SEP        19
#define OPENB_SEP       20
#define CLOSEB_SEP      21
#define COMMA_SEP       22
#define PROPADD_SEP     23
#define PROPNUM_SEP     24
#define PROPERTY_SEP    25
#define COLON_SEP       26

/* ------------------------------------------------------------------------- */
/*   Useful macros                                                           */
/* ------------------------------------------------------------------------- */

#define On_(x)   if (strcmp(b,x)==0)
#define OnS_(x)  if (strcmp(sub_buffer,x)==0)
#define IfPass2  if (pass_number==2)

#define InV3     if (version_number==3)
#define InV5     if (version_number==5)

/* ------------------------------------------------------------------------- */
/*   Initialisation extern definitions                                       */
/* ------------------------------------------------------------------------- */

extern void init_inform_vars(void);
extern void init_express_vars(void);
extern void init_zcode_vars(void);
extern void zcode_free_arrays(void);
extern void init_files_vars(void);
extern void init_symbols_vars(void);
extern void init_inputs_vars(void);
extern void init_tables_vars(void);
extern void init_asm_vars(void);

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "inform"                                         */
/* ------------------------------------------------------------------------- */

#ifdef ALLOCATE_BIG_ARRAYS
    extern int  *abbrev_values;
    extern int  *abbrev_quality;
    extern int  *abbrev_freqs;
#else
    extern int  abbrev_values[];
    extern int  abbrev_quality[];
    extern int  abbrev_freqs[];
#endif

extern int
    version_number,      override_version,
    no_abbrevs,          no_routines,      no_symbols,
    no_errors,           no_warnings,      endofpass_flag,
    no_dummy_labels,
    pass_number,         brace_sp,         no_locals,
    process_filename_flag,                 actual_version;
extern int32 scale_factor;
extern int
    statistics_mode,     offsets_mode,     tracing_mode,
    ignoreswitches_mode, bothpasses_mode,  hash_mode,
    percentages_mode,    trace_mode,       ltrace_mode,
    etrace_mode,         listing_mode,     concise_mode,
    nowarnings_mode,     frequencies_mode, ignoring_mode,
    double_spaced,       economy_mode,     memout_mode,
    transcript_mode,     optimise_mode,    store_the_text,
    abbrev_mode,         memory_map_mode,  withdebug_mode,
    listobjects_mode,    printprops_mode,  debugging_file,
    obsolete_mode,       extend_memory_map;
extern int error_format;
extern zip
    *zcode,      *zcode_p,     *utf_zcode_p,
    *symbols_p,  *symbols_top,
    *strings,    *strings_p,
    *low_strings, *low_strings_p,
    *dictionary, *dict_p,
    *output_p,   *abbreviations_at;

    extern char *all_text, *all_text_p;

#ifdef ALLOCATE_BIG_ARRAYS
    extern char *buffer, *sub_buffer, *parse_buffer, *rewrite;
#else
    extern char buffer[], sub_buffer[], parse_buffer[], rewrite[];
#endif

extern int32 Write_Code_At, Write_Strings_At;

extern int32
    code_offset,
    actions_offset,
    preactions_offset,
    dictionary_offset,
    adjectives_offset,
    variables_offset,
    strings_offset;

extern int32 Out_Size;

extern void make_lower_case(char *str);
extern void make_upper_case(char *str);
extern void free_remaining_arrays(void);
extern void switches(char *, int);

extern int call_for_br_flag;
extern void compile_box_routine(void);

#ifdef MAC_FACE
extern int inform_main(char *, char *, char *, char *);
extern jmp_buf mac_env;
#endif

/* ------------------------------------------------------------------------- */
/*   Extern definitions for Archimedes DDE throwback (see above)             */
/* ------------------------------------------------------------------------- */

#ifdef ARC_THROWBACK
extern void throwback(int severity, char * error);
extern void throwback_start(void);
extern void throwback_end(void);
extern int throwbackflag;
#endif

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "express"                                        */
/* ------------------------------------------------------------------------- */

extern int  next_token, void_context, condition_context,
            assign_context, lines_compiled;
extern char condition_label[];

extern int replace_sf(char *b);
extern void express_allocate_arrays(void);
extern void express_free_arrays(void);
extern int expression(int fromword);
extern void assignment(int from, int flag);

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "zcode"                                          */
/* ------------------------------------------------------------------------- */

extern int  almade_flag;
extern int32 total_chars_trans, total_bytes_trans, trans_length;
extern const char *alphabet[];
extern int  chars_lookup[];

extern int  translate_to_ascii(char c);
extern void make_lookup(void);
extern void make_abbrevs_lookup(void);
extern zip *translate_text(zip *p, char *s_text);
extern opcode opcs(int32 i);
extern void stockup_symbols(void);
extern void optimise_abbreviations(void);

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "files"                                          */
/* ------------------------------------------------------------------------- */

extern int  override_error_line, total_files_read, input_file;
extern int32 malloced_bytes;
extern char Code_Name[];
extern FILE *Temp1_fp, *Temp2_fp;
extern int  debug_pass;
extern int  total_sl_count;
extern int  include_path_set;
extern char include_path[];

extern void *my_malloc(int32 size, char *whatfor);
extern void *my_calloc(int32 size, int32 howmany, char *whatfor);
extern void my_free(void *pointer, char *whatitwas);
extern void fatalerror(char *s);
extern void memoryerror(char *s, int32 size);
extern void open_temporary_files(void);
extern void check_temp_files(void);
extern void remove_temp_files(void);
extern void open_debug_file(void);
extern void write_debug_byte(int i);
extern void keep_chars_read(void);
extern void write_chars_read(void);
extern void make_debug_linenum(void);
extern void keep_debug_linenum(void);
extern void write_debug_linenum(void);
extern void write_kept_linenum(void);
extern void keep_routine_linenum(void);
extern void write_routine_linenum(void);
extern void keep_re_linenum(void);
extern void write_re_linenum(void);
extern void write_debug_address(int32 i);
extern void write_present_linenum(void);
extern void write_dbgl(dbgl x);
extern void write_debug_string(char *s);
extern void close_debug_file(void);
extern void print_error_line(void);
extern int  file_end(int32 marker);
extern int  file_char(int32 marker);
extern void add_to_checksum(void *address);
extern int  current_source_line(void);
extern void load_sourcefile(char *story_name, int style);
extern void close_all_source(void);
extern void advance_line(void);
extern void output_file(void);
extern void declare_systemfile(void);
extern int  is_systemfile(void);

extern void set_memory_sizes(int size_flag);
extern void memory_command(char *command);

#ifdef ARCHIMEDES
extern char tname_pre[];
#endif

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "symbols"                                        */
/* ------------------------------------------------------------------------- */

extern int routine_starts_line, used_local_variable[];
extern zip *local_varname[];

#ifndef ALLOCATE_BIG_ARRAYS
  extern int32 svals[];
#ifdef VAX
    extern char stypes[];
#else
    extern signed char stypes[];
#endif
#else
  extern int32 *svals;
#ifdef VAX
    extern char *stypes;
#else
    extern signed char *stypes;
#endif
#endif

extern void symbols_allocate_arrays(void);
extern void symbols_free_arrays(void);
extern void init_symbol_banks(void);
extern void prim_new_symbol(char *p, int32 value, int type, int bank);
extern void new_symbol(char *p, int32 value, int type);
extern int prim_find_symbol(char *q, int bank);
extern int find_symbol(char *q);
extern int local_find_symbol(char *q);
extern void new_symbol(char *p, int32 value, int type);
extern void list_symbols(void);
extern void lose_local_symbols(void);

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "inputs"                                         */
/* ------------------------------------------------------------------------- */

extern char *tokens;
extern char *forerrors_buff;
extern int  forerrors_line;
extern int  total_source_line, internal_line;
extern int32 marker_in_file;

extern void inputs_allocate_arrays(void);
extern void inputs_free_arrays(void);
extern void input_begin_pass(void);
extern void dequote_text(char *b1);
extern void word(char *b1, int32 w);
extern int word_token(int w);
extern void textword(char *b1, int w);
extern void no_such_label(char *lname);
extern void warning(char *s);
extern void warning_named(char *s1, char *s2);
extern void obsolete_warning(char *s1);
extern void error(char *s);
extern void error_named(char *s1, char *s2);
extern void stack_create(void);
extern void stack_line(char *p);
extern int  get_next_line(void);
extern void tokenise_line(void);
extern void make_s_grid(void);

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "tables"                                         */
/* ------------------------------------------------------------------------- */

extern int no_attributes, no_properties, no_globals, max_no_objects,
           no_fake_actions;
extern int dict_entries;
extern int release_number, statusline_flag;
extern int time_set;
extern int resobj_flag;
extern char time_given[];
extern int globals_size, properties_size;
extern int32 prop_defaults[];
extern int prop_longflag[];
extern int prop_additive[];
extern char *properties_table;
extern int doing_table_mode;

extern void tables_allocate_arrays(void);
extern void tables_free_arrays(void);
extern void tables_begin_pass(void);
extern int  dictionary_find(char *dword, int scope);
extern int  dictionary_add(char *dword, int x, int y, int z);
extern int  find_action(int32 addr);
extern void new_action(char *name, int code);
extern void make_global(char *b, int array_flag);
extern void assemble_table(char *b, int word_from);
extern void check_globals(void);
extern void make_object(char *b, int flag);
extern void make_class(char *b);
extern void resume_object(char *b, int j);
extern void finish_object(void);
extern void make_verb(char *b);
extern void extend_verb(char *b);
extern void list_object_tree(void);
extern void list_verb_table(void);
extern void show_dictionary(void);
extern void construct_storyfile(void);

/* ------------------------------------------------------------------------- */
/*   Extern definitions for "asm"                                            */
/* ------------------------------------------------------------------------- */

extern int  no_stubbed;
extern int  in_routine_flag;
extern int  ignoring_routine;
extern int  a_from_one;
extern int  line_done_flag;
extern int  return_flag;

extern void asm_allocate_arrays(void);
extern void asm_free_arrays(void);
extern int32 constant_value(char *b);
extern void args_begin_pass(void);
extern int  assemble_opcode(char *b, int32 offset, int internal_code);
extern void assemble_label(int32 offset, char *b);
extern void assemble_directive(char *b, int32 offset, int32 code);
