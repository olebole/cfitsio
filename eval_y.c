/* A Bison parser, made from eval.y
   by GNU bison 1.35.  */

#define FFBISON 1  /* Identify Bison output.  */

# define	BOOLEAN	257
# define	LONG	258
# define	DOUBLE	259
# define	STRING	260
# define	BITSTR	261
# define	FUNCTION	262
# define	BFUNCTION	263
# define	GTIFILTER	264
# define	REGFILTER	265
# define	COLUMN	266
# define	BCOLUMN	267
# define	SCOLUMN	268
# define	BITCOL	269
# define	ROWREF	270
# define	NULLREF	271
# define	SNULLREF	272
# define	OR	273
# define	AND	274
# define	EQ	275
# define	NE	276
# define	GT	277
# define	LT	278
# define	LTE	279
# define	GTE	280
# define	POWER	281
# define	NOT	282
# define	INTCAST	283
# define	FLTCAST	284
# define	UMINUS	285

#line 1 "eval.y"

/************************************************************************/
/*                                                                      */
/*                       CFITSIO Lexical Parser                         */
/*                                                                      */
/* This file is one of 3 files containing code which parses an          */
/* arithmetic expression and evaluates it in the context of an input    */
/* FITS file table extension.  The CFITSIO lexical parser is divided    */
/* into the following 3 parts/files: the CFITSIO "front-end",           */
/* eval_f.c, contains the interface between the user/CFITSIO and the    */
/* real core of the parser; the FLEX interpreter, eval_l.c, takes the   */
/* input string and parses it into tokens and identifies the FITS       */
/* information required to evaluate the expression (ie, keywords and    */
/* columns); and, the BISON grammar and evaluation routines, eval_y.c,  */
/* receives the FLEX output and determines and performs the actual      */
/* operations.  The files eval_l.c and eval_y.c are produced from       */
/* running flex and bison on the files eval.l and eval.y, respectively. */
/* (flex and bison are available from any GNU archive: see www.gnu.org) */
/*                                                                      */
/* The grammar rules, rather than evaluating the expression in situ,    */
/* builds a tree, or Nodal, structure mapping out the order of          */
/* operations and expression dependencies.  This "compilation" process  */
/* allows for much faster processing of multiple rows.  This technique  */
/* was developed by Uwe Lammers of the XMM Science Analysis System,     */
/* although the CFITSIO implementation is entirely code original.       */
/*                                                                      */
/*                                                                      */
/* Modification History:                                                */
/*                                                                      */
/*   Kent Blackburn      c1992  Original parser code developed for the  */
/*                              FTOOLS software package, in particular, */
/*                              the fselect task.                       */
/*   Kent Blackburn      c1995  BIT column support added                */
/*   Peter D Wilson   Feb 1998  Vector column support added             */
/*   Peter D Wilson   May 1998  Ported to CFITSIO library.  User        */
/*                              interface routines written, in essence  */
/*                              making fselect, fcalc, and maketime     */
/*                              capabilities available to all tools     */
/*                              via single function calls.              */
/*   Peter D Wilson   Jun 1998  Major rewrite of parser core, so as to  */
/*                              create a run-time evaluation tree,      */
/*                              inspired by the work of Uwe Lammers,    */
/*                              resulting in a speed increase of        */
/*                              10-100 times.                           */
/*   Peter D Wilson   Jul 1998  gtifilter(a,b,c,d) function added       */
/*   Peter D Wilson   Aug 1998  regfilter(a,b,c,d) function added       */
/*   Peter D Wilson   Jul 1999  Make parser fitsfile-independent,       */
/*                              allowing a purely vector-based usage    */
/*  Craig B Markwardt Jun 2004  Add MEDIAN() function                   */
/*  Craig B Markwardt Jun 2004  Add SUM(), and MIN/MAX() for bit arrays */
/*  Craig B Markwardt Jun 2004  Allow subscripting of nX bit arrays     */
/*  Craig B Markwardt Jun 2004  Implement statistical functions         */
/*                              NVALID(), AVERAGE(), and STDDEV()       */
/*                              for integer and floating point vectors  */
/*  Craig B Markwardt Jun 2004  Use NULL values for range errors instead*/
/*                              of throwing a parse error               */
/*                                                                      */
/************************************************************************/

#define  APPROX 1.0e-7
#include "eval_defs.h"
#include "region.h"
#include <time.h>

#include <stdlib.h>

#ifndef alloca
#define alloca malloc
#endif

   /*  Shrink the initial stack depth to keep local data <32K (mac limit)  */
   /*  yacc will allocate more space if needed, though.                    */
#define  FFINITDEPTH   100

/***************************************************************/
/*  Replace Bison's BACKUP macro with one that fixes a bug --  */
/*  must update state after popping the stack -- and allows    */
/*  popping multiple terms at one time.                        */
/***************************************************************/

#define FFNEWBACKUP(token, value)                               \
   do								\
     if (ffchar == FFEMPTY )   					\
       { ffchar = (token);                                      \
         memcpy( &fflval, &(value), sizeof(value) );            \
         ffchar1 = FFTRANSLATE (ffchar);			\
         while (fflen--) FFPOPSTACK;				\
         ffstate = *ffssp;					\
         goto ffbackup;						\
       }							\
     else							\
       { fferror ("syntax error: cannot back up"); FFERROR; }	\
   while (0)

/***************************************************************/
/*  Useful macros for accessing/testing Nodes                  */
/***************************************************************/

#define TEST(a)        if( (a)<0 ) FFERROR
#define SIZE(a)        gParse.Nodes[ a ].value.nelem
#define TYPE(a)        gParse.Nodes[ a ].type
#define PROMOTE(a,b)   if( TYPE(a) > TYPE(b) )                  \
                          b = New_Unary( TYPE(a), 0, b );       \
                       else if( TYPE(a) < TYPE(b) )             \
	                  a = New_Unary( TYPE(b), 0, a );

/*****  Internal functions  *****/

#ifdef __cplusplus
extern "C" {
#endif

static int  Alloc_Node    ( void );
static void Free_Last_Node( void );
static void Evaluate_Node ( int thisNode );

static int  New_Const ( int returnType, void *value, long len );
static int  New_Column( int ColNum );
static int  New_Offset( int ColNum, int offset );
static int  New_Unary ( int returnType, int Op, int Node1 );
static int  New_BinOp ( int returnType, int Node1, int Op, int Node2 );
static int  New_Func  ( int returnType, funcOp Op, int nNodes,
			int Node1, int Node2, int Node3, int Node4, 
			int Node5, int Node6, int Node7 );
static int  New_Deref ( int Var,  int nDim,
			int Dim1, int Dim2, int Dim3, int Dim4, int Dim5 );
static int  New_GTI   ( char *fname, int Node1, char *start, char *stop );
static int  New_REG   ( char *fname, int NodeX, int NodeY, char *colNames );
static int  New_Vector( int subNode );
static int  Close_Vec ( int vecNode );
static int  Locate_Col( Node *this );
static int  Test_Dims ( int Node1, int Node2 );
static void Copy_Dims ( int Node1, int Node2 );

static void Allocate_Ptrs( Node *this );
static void Do_Unary     ( Node *this );
static void Do_Offset    ( Node *this );
static void Do_BinOp_bit ( Node *this );
static void Do_BinOp_str ( Node *this );
static void Do_BinOp_log ( Node *this );
static void Do_BinOp_lng ( Node *this );
static void Do_BinOp_dbl ( Node *this );
static void Do_Func      ( Node *this );
static void Do_Deref     ( Node *this );
static void Do_GTI       ( Node *this );
static void Do_REG       ( Node *this );
static void Do_Vector    ( Node *this );

static long Search_GTI   ( double evtTime, long nGTI, double *start,
			   double *stop, int ordered );

static char  saobox (double xcen, double ycen, double xwid, double ywid,
		     double rot,  double xcol, double ycol);
static char  ellipse(double xcen, double ycen, double xrad, double yrad,
		     double rot, double xcol, double ycol);
static char  circle (double xcen, double ycen, double rad,
		     double xcol, double ycol);
static char  bnear  (double x, double y, double tolerance);
static char  bitcmp (char *bitstrm1, char *bitstrm2);
static char  bitlgte(char *bits1, int oper, char *bits2);

static void  bitand(char *result, char *bitstrm1, char *bitstrm2);
static void  bitor (char *result, char *bitstrm1, char *bitstrm2);
static void  bitnot(char *result, char *bits);

static void  fferror(char *msg);

#ifdef __cplusplus
    }
#endif


#line 174 "eval.y"
#ifndef FFSTYPE
typedef union {
    int    Node;        /* Index of Node */
    double dbl;         /* real value    */
    long   lng;         /* integer value */
    char   log;         /* logical value */
    char   str[256];    /* string value  */
} ffstype;
# define FFSTYPE ffstype
# define FFSTYPE_IS_TRIVIAL 1
#endif
#ifndef FFDEBUG
# define FFDEBUG 0
#endif



#define	FFFINAL		276
#define	FFFLAG		-32768
#define	FFNTBASE	51

/* FFTRANSLATE(FFLEX) -- Bison token number corresponding to FFLEX. */
#define FFTRANSLATE(x) ((unsigned)(x) <= 285 ? fftranslate[x] : 59)

/* FFTRANSLATE[FFLEX] -- Bison token number corresponding to FFLEX. */
static const char fftranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      47,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    36,    40,     2,
      49,    50,    37,    34,    19,    35,     2,    38,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    21,     2,
       2,    20,     2,    24,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    46,     2,    48,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    22,    39,    23,    29,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    25,    26,    27,    28,    30,    31,    32,
      33,    41,    42,    43,    44,    45
};

#if FFDEBUG
static const short ffprhs[] =
{
       0,     0,     1,     4,     6,     9,    12,    15,    18,    21,
      24,    28,    31,    35,    39,    43,    46,    49,    51,    53,
      58,    62,    66,    70,    75,    82,    91,   102,   115,   118,
     122,   124,   126,   128,   133,   135,   137,   141,   145,   149,
     153,   157,   161,   164,   167,   171,   175,   179,   185,   191,
     197,   200,   204,   208,   212,   216,   222,   227,   234,   243,
     254,   267,   270,   273,   276,   279,   281,   283,   288,   292,
     296,   300,   304,   308,   312,   316,   320,   324,   328,   332,
     336,   340,   344,   348,   352,   356,   360,   364,   368,   372,
     376,   380,   386,   392,   396,   400,   404,   410,   418,   430,
     446,   449,   453,   459,   469,   473,   481,   491,   496,   503,
     512,   523,   536,   539,   543,   545,   547,   552,   554,   558,
     562,   568
};
static const short ffrhs[] =
{
      -1,    51,    52,     0,    47,     0,    55,    47,     0,    56,
      47,     0,    58,    47,     0,    57,    47,     0,     1,    47,
       0,    22,    56,     0,    53,    19,    56,     0,    22,    55,
       0,    54,    19,    55,     0,    54,    19,    56,     0,    53,
      19,    55,     0,    54,    23,     0,    53,    23,     0,     7,
       0,    15,     0,    15,    22,    55,    23,     0,    57,    40,
      57,     0,    57,    39,    57,     0,    57,    34,    57,     0,
      57,    46,    55,    48,     0,    57,    46,    55,    19,    55,
      48,     0,    57,    46,    55,    19,    55,    19,    55,    48,
       0,    57,    46,    55,    19,    55,    19,    55,    19,    55,
      48,     0,    57,    46,    55,    19,    55,    19,    55,    19,
      55,    19,    55,    48,     0,    42,    57,     0,    49,    57,
      50,     0,     4,     0,     5,     0,    12,     0,    12,    22,
      55,    23,     0,    16,     0,    17,     0,    55,    36,    55,
       0,    55,    34,    55,     0,    55,    35,    55,     0,    55,
      37,    55,     0,    55,    38,    55,     0,    55,    41,    55,
       0,    34,    55,     0,    35,    55,     0,    49,    55,    50,
       0,    55,    37,    56,     0,    56,    37,    55,     0,    56,
      24,    55,    21,    55,     0,    56,    24,    56,    21,    55,
       0,    56,    24,    55,    21,    56,     0,     8,    50,     0,
       8,    56,    50,     0,     8,    58,    50,     0,     8,    57,
      50,     0,     8,    55,    50,     0,     8,    55,    19,    55,
      50,     0,    55,    46,    55,    48,     0,    55,    46,    55,
      19,    55,    48,     0,    55,    46,    55,    19,    55,    19,
      55,    48,     0,    55,    46,    55,    19,    55,    19,    55,
      19,    55,    48,     0,    55,    46,    55,    19,    55,    19,
      55,    19,    55,    19,    55,    48,     0,    43,    55,     0,
      43,    56,     0,    44,    55,     0,    44,    56,     0,     3,
       0,    13,     0,    13,    22,    55,    23,     0,    57,    27,
      57,     0,    57,    28,    57,     0,    57,    31,    57,     0,
      57,    32,    57,     0,    57,    30,    57,     0,    57,    33,
      57,     0,    55,    30,    55,     0,    55,    31,    55,     0,
      55,    33,    55,     0,    55,    32,    55,     0,    55,    29,
      55,     0,    55,    27,    55,     0,    55,    28,    55,     0,
      58,    27,    58,     0,    58,    28,    58,     0,    58,    30,
      58,     0,    58,    33,    58,     0,    58,    31,    58,     0,
      58,    32,    58,     0,    56,    26,    56,     0,    56,    25,
      56,     0,    56,    27,    56,     0,    56,    28,    56,     0,
      55,    20,    55,    21,    55,     0,    56,    24,    56,    21,
      56,     0,     9,    55,    50,     0,     9,    56,    50,     0,
       9,    58,    50,     0,     8,    56,    19,    56,    50,     0,
       9,    55,    19,    55,    19,    55,    50,     0,     9,    55,
      19,    55,    19,    55,    19,    55,    19,    55,    50,     0,
       9,    55,    19,    55,    19,    55,    19,    55,    19,    55,
      19,    55,    19,    55,    50,     0,    10,    50,     0,    10,
       6,    50,     0,    10,     6,    19,    55,    50,     0,    10,
       6,    19,    55,    19,     6,    19,     6,    50,     0,    11,
       6,    50,     0,    11,     6,    19,    55,    19,    55,    50,
       0,    11,     6,    19,    55,    19,    55,    19,     6,    50,
       0,    56,    46,    55,    48,     0,    56,    46,    55,    19,
      55,    48,     0,    56,    46,    55,    19,    55,    19,    55,
      48,     0,    56,    46,    55,    19,    55,    19,    55,    19,
      55,    48,     0,    56,    46,    55,    19,    55,    19,    55,
      19,    55,    19,    55,    48,     0,    42,    56,     0,    49,
      56,    50,     0,     6,     0,    14,     0,    14,    22,    55,
      23,     0,    18,     0,    49,    58,    50,     0,    58,    34,
      58,     0,    56,    24,    58,    21,    58,     0,     8,    58,
      19,    58,    50,     0
};

#endif

#if FFDEBUG
/* FFRLINE[FFN] -- source line where rule number FFN was defined. */
static const short ffrline[] =
{
       0,   223,   224,   227,   228,   234,   240,   246,   252,   255,
     257,   270,   272,   285,   296,   310,   314,   318,   323,   325,
     334,   337,   340,   343,   345,   347,   349,   351,   353,   356,
     360,   362,   364,   366,   375,   377,   379,   382,   385,   388,
     391,   394,   397,   399,   401,   403,   407,   411,   430,   449,
     468,   478,   489,   501,   523,   591,   643,   645,   647,   649,
     651,   653,   655,   657,   659,   663,   665,   667,   676,   679,
     682,   685,   688,   691,   694,   697,   700,   703,   706,   709,
     712,   715,   718,   721,   724,   727,   730,   733,   735,   737,
     739,   742,   749,   766,   779,   792,   803,   819,   837,   858,
     885,   889,   893,   896,   900,   904,   907,   911,   913,   915,
     917,   919,   921,   923,   927,   930,   932,   941,   943,   945,
     948,   960
};
#endif


#if (FFDEBUG) || defined FFERROR_VERBOSE

/* FFTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const fftname[] =
{
  "$", "error", "$undefined.", "BOOLEAN", "LONG", "DOUBLE", "STRING", 
  "BITSTR", "FUNCTION", "BFUNCTION", "GTIFILTER", "REGFILTER", "COLUMN", 
  "BCOLUMN", "SCOLUMN", "BITCOL", "ROWREF", "NULLREF", "SNULLREF", "','", 
  "'='", "':'", "'{'", "'}'", "'?'", "OR", "AND", "EQ", "NE", "'~'", "GT", 
  "LT", "LTE", "GTE", "'+'", "'-'", "'%'", "'*'", "'/'", "'|'", "'&'", 
  "POWER", "NOT", "INTCAST", "FLTCAST", "UMINUS", "'['", "'\\n'", "']'", 
  "'('", "')'", "lines", "line", "bvector", "vector", "expr", "bexpr", 
  "bits", "sexpr", 0
};
#endif

/* FFR1[FFN] -- Symbol number of symbol that rule FFN derives. */
static const short ffr1[] =
{
       0,    51,    51,    52,    52,    52,    52,    52,    52,    53,
      53,    54,    54,    54,    54,    55,    56,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    58,    58,    58,    58,    58,    58,
      58,    58
};

/* FFR2[FFN] -- Number of symbols composing right hand side of rule FFN. */
static const short ffr2[] =
{
       0,     0,     2,     1,     2,     2,     2,     2,     2,     2,
       3,     2,     3,     3,     3,     2,     2,     1,     1,     4,
       3,     3,     3,     4,     6,     8,    10,    12,     2,     3,
       1,     1,     1,     4,     1,     1,     3,     3,     3,     3,
       3,     3,     2,     2,     3,     3,     3,     5,     5,     5,
       2,     3,     3,     3,     3,     5,     4,     6,     8,    10,
      12,     2,     2,     2,     2,     1,     1,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     5,     5,     3,     3,     3,     5,     7,    11,    15,
       2,     3,     5,     9,     3,     7,     9,     4,     6,     8,
      10,    12,     2,     3,     1,     1,     4,     1,     3,     3,
       5,     5
};

/* FFDEFACT[S] -- default rule to reduce with in state S when FFTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short ffdefact[] =
{
       1,     0,     0,    65,    30,    31,   114,    17,     0,     0,
       0,     0,    32,    66,   115,    18,    34,    35,   117,     0,
       0,     0,     0,     0,     0,     3,     0,     2,     0,     0,
       0,     0,     0,     0,     8,    50,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   100,     0,     0,     0,     0,
       0,    11,     9,     0,    42,     0,    43,     0,   112,    28,
      61,    62,    63,    64,     0,     0,     0,     0,     0,    16,
       0,    15,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     4,     0,     0,
       0,     0,     0,     0,     0,     5,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     7,     0,     0,     0,
       0,     0,     0,     0,     6,     0,    54,     0,    51,    53,
       0,    52,     0,    93,    94,    95,     0,   101,     0,   104,
       0,     0,     0,     0,    44,   113,    29,   118,    14,    10,
      12,    13,     0,    79,    80,    78,    74,    75,    77,    76,
      37,    38,    36,    39,    45,    40,    41,     0,     0,     0,
       0,    88,    87,    89,    90,    46,     0,     0,     0,    68,
      69,    72,    70,    71,    73,    22,    21,    20,     0,    81,
      82,    83,    85,    86,    84,   119,     0,     0,     0,     0,
       0,     0,    33,    67,   116,    19,     0,     0,    56,     0,
       0,     0,     0,   107,    28,     0,     0,    23,    55,    96,
     121,     0,     0,   102,     0,    91,     0,    47,    49,    48,
      92,   120,     0,     0,     0,     0,     0,     0,    57,     0,
     108,     0,    24,     0,    97,     0,     0,   105,     0,     0,
       0,     0,     0,     0,     0,    58,     0,   109,     0,    25,
       0,   103,   106,     0,     0,     0,     0,     0,    59,     0,
     110,     0,    26,     0,    98,     0,     0,     0,     0,    60,
     111,    27,     0,     0,    99,     0,     0
};

static const short ffdefgoto[] =
{
       1,    27,    28,    29,    57,    55,    42,    53
};

static const short ffpact[] =
{
  -32768,   290,   -44,-32768,-32768,-32768,-32768,-32768,   337,   385,
      -5,     7,    -6,     5,     6,    31,-32768,-32768,-32768,   385,
     385,   385,   385,   385,   385,-32768,   385,-32768,    71,    89,
     968,   160,  1267,  1288,-32768,-32768,   411,    15,  1212,   131,
     435,   165,  1313,   333,   -17,-32768,   -15,   385,   385,   385,
     385,  1195,  1265,  1360,   -32,  1265,   -32,  1195,    24,    51,
     -32,    24,   -32,    24,   555,   209,  1233,   382,   385,-32768,
     385,-32768,   385,   385,   385,   385,   385,   385,   385,   385,
     385,   385,   385,   385,   385,   385,   385,-32768,   385,   385,
     385,   385,   385,   385,   385,-32768,     2,     2,     2,     2,
       2,     2,     2,     2,     2,   385,-32768,   385,   385,   385,
     385,   385,   385,   385,-32768,   385,-32768,   385,-32768,-32768,
     385,-32768,   385,-32768,-32768,-32768,   385,-32768,   385,-32768,
    1071,  1091,  1111,  1131,-32768,-32768,-32768,-32768,  1195,  1265,
    1195,  1265,  1153,  1330,  1330,  1330,  1343,  1343,  1343,  1343,
      22,    22,    22,    45,    24,    45,    45,   627,  1175,   289,
     247,   -16,   -22,    52,    52,    45,   650,     2,     2,    -8,
      -8,    -8,    -8,    -8,    -8,    18,    51,    51,   673,    69,
      69,    73,    73,    73,    73,-32768,   579,   226,  1254,   991,
     459,  1011,-32768,-32768,-32768,-32768,   385,   385,-32768,   385,
     385,   385,   385,-32768,    51,    16,   385,-32768,-32768,-32768,
  -32768,   385,    99,-32768,   385,  1296,   696,  1296,  1265,  1296,
    1265,  1360,   719,   742,   483,    96,   507,   385,-32768,   385,
  -32768,   385,-32768,   385,-32768,   111,   112,-32768,   765,   788,
     811,  1031,    70,    74,   385,-32768,   385,-32768,   385,-32768,
     385,-32768,-32768,   834,   857,   880,   531,   385,-32768,   385,
  -32768,   385,-32768,   385,-32768,   902,   924,   946,  1051,-32768,
  -32768,-32768,   385,   603,-32768,   119,-32768
};

static const short ffpgoto[] =
{
  -32768,-32768,-32768,-32768,    -1,    87,   121,    28
};


#define	FFLAST		1394


static const short fftable[] =
{
      30,    44,   126,    34,   128,    91,    92,    36,    40,     7,
      90,    91,    92,    46,    86,    93,    47,    15,    51,    54,
      56,    93,    60,    62,    94,    64,   102,    48,    49,    33,
      94,   103,   104,   127,   117,   129,    39,    43,   105,    88,
      89,    90,    91,    92,   167,    45,   130,   131,   132,   133,
     102,   168,    93,    50,    67,   103,   104,   103,   104,    83,
      84,    94,   105,    85,   105,   118,   136,   138,    86,   140,
      94,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   155,   156,   157,    85,   158,    31,    93,
      68,    86,   165,   166,    69,    37,    41,   105,    94,   109,
     110,   111,   112,   113,   178,   225,    52,   113,    70,    58,
      61,    63,    71,    65,   186,   235,   160,   242,   243,   276,
     251,   189,    32,     0,   252,   190,     0,   191,     0,    38,
       0,     0,     0,     0,     0,   179,   180,   181,   182,   183,
     184,   185,     0,    59,     0,     0,     0,    66,   188,     0,
     120,     0,     0,     0,     0,   139,     0,   141,   107,   108,
       0,   109,   110,   111,   112,   113,     0,     0,     0,     0,
     154,     0,     0,     0,     0,   159,   161,   162,   163,   164,
       0,   121,     0,     0,    88,    89,    90,    91,    92,    88,
      89,    90,    91,    92,     0,   215,   216,    93,   217,   219,
       0,   222,    93,     0,   187,   223,    94,    95,     0,     0,
     224,    94,     0,   226,     0,   124,     0,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   238,     0,   239,   221,
     240,     0,   241,    88,    89,    90,    91,    92,     0,     0,
       0,     0,     0,   253,     0,   254,    93,   255,     0,   256,
      88,    89,    90,    91,    92,    94,   265,     0,   266,   135,
     267,     0,   268,    93,     0,     0,     0,     0,   201,     0,
       0,   273,    94,     0,   107,   108,   209,   109,   110,   111,
     112,   113,     0,     0,     0,     0,   218,   220,   204,   205,
     275,     2,     0,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,     0,
     200,     0,    19,    88,    89,    90,    91,    92,     0,     0,
       0,     0,     0,     0,    20,    21,    93,     0,     0,     0,
       0,     0,    22,    23,    24,    94,     0,    25,     0,    26,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,     0,     0,     0,    19,
     107,   108,     0,   109,   110,   111,   112,   113,     0,     0,
       0,    20,    21,     0,     0,     0,     0,     0,     0,    22,
      23,    24,     0,   125,     0,     0,    26,    35,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,     0,     0,     0,    19,     0,   107,
     108,     0,   109,   110,   111,   112,   113,     0,     0,    20,
      21,     0,     0,     0,     0,     0,     0,    22,    23,    24,
     115,    72,   137,     0,    26,     0,     0,     0,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,     0,    85,     0,   122,    72,     0,    86,     0,     0,
       0,   116,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,     0,     0,    85,     0,   212,    72,
       0,    86,     0,     0,     0,   123,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,     0,     0,
      85,     0,   233,    72,     0,    86,     0,     0,     0,   213,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,     0,     0,    85,     0,   236,    72,     0,    86,
       0,     0,     0,   234,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,     0,     0,    85,     0,
     263,    72,     0,    86,     0,     0,     0,   237,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,     0,    85,     0,     0,    72,     0,    86,     0,     0,
       0,   264,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,     0,     0,    85,     0,     0,    72,
       0,    86,     0,     0,     0,   134,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,     0,     0,
      85,     0,     0,    72,     0,    86,     0,     0,     0,   208,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,     0,     0,    85,     0,   197,    72,     0,    86,
       0,     0,     0,   274,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,     0,     0,    85,   202,
      72,     0,     0,    86,     0,   198,     0,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,     0,
       0,    85,   206,    72,     0,     0,    86,     0,   203,     0,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,     0,     0,    85,   227,    72,     0,     0,    86,
       0,   207,     0,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,     0,     0,    85,   229,    72,
       0,     0,    86,     0,   228,     0,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,     0,     0,
      85,   231,    72,     0,     0,    86,     0,   230,     0,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,     0,     0,    85,   244,    72,     0,     0,    86,     0,
     232,     0,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,     0,     0,    85,   246,    72,     0,
       0,    86,     0,   245,     0,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,     0,     0,    85,
     248,    72,     0,     0,    86,     0,   247,     0,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,     0,    85,   257,    72,     0,     0,    86,     0,   249,
       0,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,     0,     0,    85,   259,    72,     0,     0,
      86,     0,   258,     0,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,     0,     0,    85,   261,
      72,     0,     0,    86,     0,   260,     0,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,     0,
       0,    85,    72,     0,     0,     0,    86,     0,   262,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,     0,     0,    85,    72,     0,     0,     0,    86,     0,
     269,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,     0,     0,    85,    72,     0,     0,     0,
      86,     0,   270,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,     0,     0,    85,    72,     0,
       0,     0,    86,     0,   271,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,     0,     0,    85,
     211,    72,     0,     0,    86,    87,     0,     0,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
     214,    72,    85,     0,     0,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
     250,    72,    85,     0,     0,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
     272,    72,    85,     0,     0,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,    72,    85,     0,   192,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,    72,    85,     0,   193,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,    72,    85,     0,   194,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,    72,    85,     0,   195,     0,     0,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       0,     0,    85,    72,   196,     0,     0,    86,     0,     0,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,     0,     0,    85,    72,   199,     0,     0,    86,
       0,     0,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,     0,    72,    85,     0,     0,     0,
       0,    86,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,     0,     0,    85,     0,     0,    96,
      97,    86,    98,    99,   100,   101,   102,     0,     0,     0,
       0,   103,   104,     0,     0,     0,     0,     0,   105,     0,
      96,    97,   119,    98,    99,   100,   101,   102,     0,     0,
       0,     0,   103,   104,     0,     0,     0,     0,     0,   105,
       0,   107,   108,   136,   109,   110,   111,   112,   113,    88,
      89,    90,    91,    92,    96,    97,     0,    98,    99,   100,
     101,   102,    93,     0,   210,     0,   103,   104,     0,     0,
       0,    94,     0,   105,   106,   107,   108,     0,   109,   110,
     111,   112,   113,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,   114,     0,    85,     0,     0,
      96,    97,    86,    98,    99,   100,   101,   102,     0,     0,
       0,     0,   103,   104,     0,     0,     0,     0,     0,   105,
      76,    77,    78,    79,    80,    81,    82,    83,    84,     0,
       0,    85,     0,     0,     0,     0,    86,    80,    81,    82,
      83,    84,     0,     0,    85,     0,     0,   107,   108,    86,
     109,   110,   111,   112,   113
};

static const short ffcheck[] =
{
       1,     6,    19,    47,    19,    27,    28,     8,     9,     7,
      26,    27,    28,     6,    46,    37,    22,    15,    19,    20,
      21,    37,    23,    24,    46,    26,    34,    22,    22,     1,
      46,    39,    40,    50,    19,    50,     8,     9,    46,    24,
      25,    26,    27,    28,    42,    50,    47,    48,    49,    50,
      34,    49,    37,    22,    26,    39,    40,    39,    40,    37,
      38,    46,    46,    41,    46,    50,    50,    68,    46,    70,
      46,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    41,    88,     1,    37,
      19,    46,    93,    94,    23,     8,     9,    46,    46,    30,
      31,    32,    33,    34,   105,     6,    19,    34,    19,    22,
      23,    24,    23,    26,   115,    19,    88,     6,     6,     0,
      50,   122,     1,    -1,    50,   126,    -1,   128,    -1,     8,
      -1,    -1,    -1,    -1,    -1,   107,   108,   109,   110,   111,
     112,   113,    -1,    22,    -1,    -1,    -1,    26,   120,    -1,
      19,    -1,    -1,    -1,    -1,    68,    -1,    70,    27,    28,
      -1,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
      83,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      -1,    50,    -1,    -1,    24,    25,    26,    27,    28,    24,
      25,    26,    27,    28,    -1,   196,   197,    37,   199,   200,
      -1,   202,    37,    -1,   117,   206,    46,    47,    -1,    -1,
     211,    46,    -1,   214,    -1,    50,    -1,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   227,    -1,   229,   201,
     231,    -1,   233,    24,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    -1,   244,    -1,   246,    37,   248,    -1,   250,
      24,    25,    26,    27,    28,    46,   257,    -1,   259,    50,
     261,    -1,   263,    37,    -1,    -1,    -1,    -1,    21,    -1,
      -1,   272,    46,    -1,    27,    28,    50,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    -1,   199,   200,   167,   168,
       0,     1,    -1,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    -1,
      21,    -1,    22,    24,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    37,    -1,    -1,    -1,
      -1,    -1,    42,    43,    44,    46,    -1,    47,    -1,    49,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    -1,    -1,    22,
      27,    28,    -1,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      43,    44,    -1,    50,    -1,    -1,    49,    50,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,    27,
      28,    -1,    30,    31,    32,    33,    34,    -1,    -1,    34,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,
      19,    20,    50,    -1,    49,    -1,    -1,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    41,    -1,    19,    20,    -1,    46,    -1,    -1,
      -1,    50,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    41,    -1,    19,    20,
      -1,    46,    -1,    -1,    -1,    50,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      41,    -1,    19,    20,    -1,    46,    -1,    -1,    -1,    50,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    41,    -1,    19,    20,    -1,    46,
      -1,    -1,    -1,    50,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    41,    -1,
      19,    20,    -1,    46,    -1,    -1,    -1,    50,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    41,    -1,    -1,    20,    -1,    46,    -1,    -1,
      -1,    50,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    41,    -1,    -1,    20,
      -1,    46,    -1,    -1,    -1,    50,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      41,    -1,    -1,    20,    -1,    46,    -1,    -1,    -1,    50,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    41,    -1,    19,    20,    -1,    46,
      -1,    -1,    -1,    50,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    41,    19,
      20,    -1,    -1,    46,    -1,    48,    -1,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    41,    19,    20,    -1,    -1,    46,    -1,    48,    -1,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    41,    19,    20,    -1,    -1,    46,
      -1,    48,    -1,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    41,    19,    20,
      -1,    -1,    46,    -1,    48,    -1,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      41,    19,    20,    -1,    -1,    46,    -1,    48,    -1,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    41,    19,    20,    -1,    -1,    46,    -1,
      48,    -1,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    41,    19,    20,    -1,
      -1,    46,    -1,    48,    -1,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    41,
      19,    20,    -1,    -1,    46,    -1,    48,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    41,    19,    20,    -1,    -1,    46,    -1,    48,
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    41,    19,    20,    -1,    -1,
      46,    -1,    48,    -1,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    41,    19,
      20,    -1,    -1,    46,    -1,    48,    -1,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    41,    20,    -1,    -1,    -1,    46,    -1,    48,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    41,    20,    -1,    -1,    -1,    46,    -1,
      48,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    41,    20,    -1,    -1,    -1,
      46,    -1,    48,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    41,    20,    -1,
      -1,    -1,    46,    -1,    48,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    41,
      19,    20,    -1,    -1,    46,    47,    -1,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      19,    20,    41,    -1,    -1,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      19,    20,    41,    -1,    -1,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      19,    20,    41,    -1,    -1,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    20,    41,    -1,    23,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    20,    41,    -1,    23,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    20,    41,    -1,    23,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    20,    41,    -1,    23,    -1,    -1,    46,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    41,    20,    21,    -1,    -1,    46,    -1,    -1,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    41,    20,    21,    -1,    -1,    46,
      -1,    -1,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    20,    41,    -1,    -1,    -1,
      -1,    46,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    41,    -1,    -1,    27,
      28,    46,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    39,    40,    -1,    -1,    -1,    -1,    -1,    46,    -1,
      27,    28,    50,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    39,    40,    -1,    -1,    -1,    -1,    -1,    46,
      -1,    27,    28,    50,    30,    31,    32,    33,    34,    24,
      25,    26,    27,    28,    27,    28,    -1,    30,    31,    32,
      33,    34,    37,    -1,    50,    -1,    39,    40,    -1,    -1,
      -1,    46,    -1,    46,    47,    27,    28,    -1,    30,    31,
      32,    33,    34,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    47,    -1,    41,    -1,    -1,
      27,    28,    46,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    39,    40,    -1,    -1,    -1,    -1,    -1,    46,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    41,    -1,    -1,    -1,    -1,    46,    34,    35,    36,
      37,    38,    -1,    -1,    41,    -1,    -1,    27,    28,    46,
      30,    31,    32,    33,    34
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with ff or FF, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (ffoverflow) || defined (FFERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if FFSTACK_USE_ALLOCA
#  define FFSTACK_ALLOC alloca
# else
#  ifndef FFSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define FFSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define FFSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef FFSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define FFSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define FFSIZE_T size_t
#  endif
#  define FFSTACK_ALLOC malloc
#  define FFSTACK_FREE free
# endif
#endif /* ! defined (ffoverflow) || defined (FFERROR_VERBOSE) */


#if (! defined (ffoverflow) \
     && (! defined (__cplusplus) \
	 || (FFLTYPE_IS_TRIVIAL && FFSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union ffalloc
{
  short ffss;
  FFSTYPE ffvs;
# if FFLSP_NEEDED
  FFLTYPE ffls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define FFSTACK_GAP_MAX (sizeof (union ffalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if FFLSP_NEEDED
#  define FFSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (FFSTYPE) + sizeof (FFLTYPE))	\
      + 2 * FFSTACK_GAP_MAX)
# else
#  define FFSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (FFSTYPE))				\
      + FFSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef FFCOPY
#  if 1 < __GNUC__
#   define FFCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define FFCOPY(To, From, Count)		\
      do					\
	{					\
	  register FFSIZE_T ffi;		\
	  for (ffi = 0; ffi < (Count); ffi++)	\
	    (To)[ffi] = (From)[ffi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables FFSIZE and FFSTACKSIZE give the old and new number of
   elements in the stack, and FFPTR gives the new location of the
   stack.  Advance FFPTR to a properly aligned location for the next
   stack.  */
# define FFSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	FFSIZE_T ffnewbytes;						\
	FFCOPY (&ffptr->Stack, Stack, ffsize);				\
	Stack = &ffptr->Stack;						\
	ffnewbytes = ffstacksize * sizeof (*Stack) + FFSTACK_GAP_MAX;	\
	ffptr += ffnewbytes / sizeof (*ffptr);				\
      }									\
    while (0)

#endif


#if ! defined (FFSIZE_T) && defined (__SIZE_TYPE__)
# define FFSIZE_T __SIZE_TYPE__
#endif
#if ! defined (FFSIZE_T) && defined (size_t)
# define FFSIZE_T size_t
#endif
#if ! defined (FFSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define FFSIZE_T size_t
# endif
#endif
#if ! defined (FFSIZE_T)
# define FFSIZE_T unsigned int
#endif

#define fferrok		(fferrstatus = 0)
#define ffclearin	(ffchar = FFEMPTY)
#define FFEMPTY		-2
#define FFEOF		0
#define FFACCEPT	goto ffacceptlab
#define FFABORT 	goto ffabortlab
#define FFERROR		goto fferrlab1
/* Like FFERROR except do call fferror.  This remains here temporarily
   to ease the transition to the new meaning of FFERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define FFFAIL		goto fferrlab
#define FFRECOVERING()  (!!fferrstatus)
#define FFBACKUP(Token, Value)					\
do								\
  if (ffchar == FFEMPTY && fflen == 1)				\
    {								\
      ffchar = (Token);						\
      fflval = (Value);						\
      ffchar1 = FFTRANSLATE (ffchar);				\
      FFPOPSTACK;						\
      goto ffbackup;						\
    }								\
  else								\
    { 								\
      fferror ("syntax error: cannot back up");			\
      FFERROR;							\
    }								\
while (0)

#define FFTERROR	1
#define FFERRCODE	256


/* FFLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When FFLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef FFLLOC_DEFAULT
# define FFLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* FFLEX -- calling `fflex' with the right arguments.  */

#if FFPURE
# if FFLSP_NEEDED
#  ifdef FFLEX_PARAM
#   define FFLEX		fflex (&fflval, &fflloc, FFLEX_PARAM)
#  else
#   define FFLEX		fflex (&fflval, &fflloc)
#  endif
# else /* !FFLSP_NEEDED */
#  ifdef FFLEX_PARAM
#   define FFLEX		fflex (&fflval, FFLEX_PARAM)
#  else
#   define FFLEX		fflex (&fflval)
#  endif
# endif /* !FFLSP_NEEDED */
#else /* !FFPURE */
# define FFLEX			fflex ()
#endif /* !FFPURE */


/* Enable debugging if requested.  */
#if FFDEBUG

# ifndef FFFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define FFFPRINTF fprintf
# endif

# define FFDPRINTF(Args)			\
do {						\
  if (ffdebug)					\
    FFFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int ffdebug;
#else /* !FFDEBUG */
# define FFDPRINTF(Args)
#endif /* !FFDEBUG */

/* FFINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	FFINITDEPTH
# define FFINITDEPTH 200
#endif

/* FFMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < FFSTACK_BYTES (FFMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if FFMAXDEPTH == 0
# undef FFMAXDEPTH
#endif

#ifndef FFMAXDEPTH
# define FFMAXDEPTH 10000
#endif

#ifdef FFERROR_VERBOSE

# ifndef ffstrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define ffstrlen strlen
#  else
/* Return the length of FFSTR.  */
static FFSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
ffstrlen (const char *ffstr)
#   else
ffstrlen (ffstr)
     const char *ffstr;
#   endif
{
  register const char *ffs = ffstr;

  while (*ffs++ != '\0')
    continue;

  return ffs - ffstr - 1;
}
#  endif
# endif

# ifndef ffstpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define ffstpcpy stpcpy
#  else
/* Copy FFSRC to FFDEST, returning the address of the terminating '\0' in
   FFDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
ffstpcpy (char *ffdest, const char *ffsrc)
#   else
ffstpcpy (ffdest, ffsrc)
     char *ffdest;
     const char *ffsrc;
#   endif
{
  register char *ffd = ffdest;
  register const char *ffs = ffsrc;

  while ((*ffd++ = *ffs++) != '\0')
    continue;

  return ffd - 1;
}
#  endif
# endif
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define FFPARSE_PARAM as the name of an argument to be passed
   into ffparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef FFPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define FFPARSE_PARAM_ARG void *FFPARSE_PARAM
#  define FFPARSE_PARAM_DECL
# else
#  define FFPARSE_PARAM_ARG FFPARSE_PARAM
#  define FFPARSE_PARAM_DECL void *FFPARSE_PARAM;
# endif
#else /* !FFPARSE_PARAM */
# define FFPARSE_PARAM_ARG
# define FFPARSE_PARAM_DECL
#endif /* !FFPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef FFPARSE_PARAM
int ffparse (void *);
# else
int ffparse (void);
# endif
#endif

/* FF_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to FFPARSE.  */

#define FF_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int ffchar;						\
							\
/* The semantic value of the lookahead symbol. */	\
FFSTYPE fflval;						\
							\
/* Number of parse errors so far.  */			\
int ffnerrs;

#if FFLSP_NEEDED
# define FF_DECL_VARIABLES			\
FF_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
FFLTYPE fflloc;
#else
# define FF_DECL_VARIABLES			\
FF_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !FFPURE
FF_DECL_VARIABLES
#endif  /* !FFPURE */

int
ffparse (FFPARSE_PARAM_ARG)
     FFPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if FFPURE
  FF_DECL_VARIABLES
#endif  /* !FFPURE */

  register int ffstate;
  register int ffn;
  int ffresult;
  /* Number of tokens to shift before error messages enabled.  */
  int fferrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int ffchar1 = 0;

  /* Three stacks and their tools:
     `ffss': related to states,
     `ffvs': related to semantic values,
     `ffls': related to locations.

     Refer to the stacks thru separate pointers, to allow ffoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	ffssa[FFINITDEPTH];
  short *ffss = ffssa;
  register short *ffssp;

  /* The semantic value stack.  */
  FFSTYPE ffvsa[FFINITDEPTH];
  FFSTYPE *ffvs = ffvsa;
  register FFSTYPE *ffvsp;

#if FFLSP_NEEDED
  /* The location stack.  */
  FFLTYPE fflsa[FFINITDEPTH];
  FFLTYPE *ffls = fflsa;
  FFLTYPE *fflsp;
#endif

#if FFLSP_NEEDED
# define FFPOPSTACK   (ffvsp--, ffssp--, fflsp--)
#else
# define FFPOPSTACK   (ffvsp--, ffssp--)
#endif

  FFSIZE_T ffstacksize = FFINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  FFSTYPE ffval;
#if FFLSP_NEEDED
  FFLTYPE ffloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int fflen;

  FFDPRINTF ((stderr, "Starting parse\n"));

  ffstate = 0;
  fferrstatus = 0;
  ffnerrs = 0;
  ffchar = FFEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  ffssp = ffss;
  ffvsp = ffvs;
#if FFLSP_NEEDED
  fflsp = ffls;
#endif
  goto ffsetstate;

/*------------------------------------------------------------.
| ffnewstate -- Push a new state, which is found in ffstate.  |
`------------------------------------------------------------*/
 ffnewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  ffssp++;

 ffsetstate:
  *ffssp = ffstate;

  if (ffssp >= ffss + ffstacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      FFSIZE_T ffsize = ffssp - ffss + 1;

#ifdef ffoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	FFSTYPE *ffvs1 = ffvs;
	short *ffss1 = ffss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if FFLSP_NEEDED
	FFLTYPE *ffls1 = ffls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if ffoverflow is a macro.  */
	ffoverflow ("parser stack overflow",
		    &ffss1, ffsize * sizeof (*ffssp),
		    &ffvs1, ffsize * sizeof (*ffvsp),
		    &ffls1, ffsize * sizeof (*fflsp),
		    &ffstacksize);
	ffls = ffls1;
# else
	ffoverflow ("parser stack overflow",
		    &ffss1, ffsize * sizeof (*ffssp),
		    &ffvs1, ffsize * sizeof (*ffvsp),
		    &ffstacksize);
# endif
	ffss = ffss1;
	ffvs = ffvs1;
      }
#else /* no ffoverflow */
# ifndef FFSTACK_RELOCATE
      goto ffoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (ffstacksize >= FFMAXDEPTH)
	goto ffoverflowlab;
      ffstacksize *= 2;
      if (ffstacksize > FFMAXDEPTH)
	ffstacksize = FFMAXDEPTH;

      {
	short *ffss1 = ffss;
	union ffalloc *ffptr =
	  (union ffalloc *) FFSTACK_ALLOC (FFSTACK_BYTES (ffstacksize));
	if (! ffptr)
	  goto ffoverflowlab;
	FFSTACK_RELOCATE (ffss);
	FFSTACK_RELOCATE (ffvs);
# if FFLSP_NEEDED
	FFSTACK_RELOCATE (ffls);
# endif
# undef FFSTACK_RELOCATE
	if (ffss1 != ffssa)
	  FFSTACK_FREE (ffss1);
      }
# endif
#endif /* no ffoverflow */

      ffssp = ffss + ffsize - 1;
      ffvsp = ffvs + ffsize - 1;
#if FFLSP_NEEDED
      fflsp = ffls + ffsize - 1;
#endif

      FFDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) ffstacksize));

      if (ffssp >= ffss + ffstacksize - 1)
	FFABORT;
    }

  FFDPRINTF ((stderr, "Entering state %d\n", ffstate));

  goto ffbackup;


/*-----------.
| ffbackup.  |
`-----------*/
ffbackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* ffresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  ffn = ffpact[ffstate];
  if (ffn == FFFLAG)
    goto ffdefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* ffchar is either FFEMPTY or FFEOF
     or a valid token in external form.  */

  if (ffchar == FFEMPTY)
    {
      FFDPRINTF ((stderr, "Reading a token: "));
      ffchar = FFLEX;
    }

  /* Convert token to internal form (in ffchar1) for indexing tables with */

  if (ffchar <= 0)		/* This means end of input. */
    {
      ffchar1 = 0;
      ffchar = FFEOF;		/* Don't call FFLEX any more */

      FFDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      ffchar1 = FFTRANSLATE (ffchar);

#if FFDEBUG
     /* We have to keep this `#if FFDEBUG', since we use variables
	which are defined only if `FFDEBUG' is set.  */
      if (ffdebug)
	{
	  FFFPRINTF (stderr, "Next token is %d (%s",
		     ffchar, fftname[ffchar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef FFPRINT
	  FFPRINT (stderr, ffchar, fflval);
# endif
	  FFFPRINTF (stderr, ")\n");
	}
#endif
    }

  ffn += ffchar1;
  if (ffn < 0 || ffn > FFLAST || ffcheck[ffn] != ffchar1)
    goto ffdefault;

  ffn = fftable[ffn];

  /* ffn is what to do for this token type in this state.
     Negative => reduce, -ffn is rule number.
     Positive => shift, ffn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (ffn < 0)
    {
      if (ffn == FFFLAG)
	goto fferrlab;
      ffn = -ffn;
      goto ffreduce;
    }
  else if (ffn == 0)
    goto fferrlab;

  if (ffn == FFFINAL)
    FFACCEPT;

  /* Shift the lookahead token.  */
  FFDPRINTF ((stderr, "Shifting token %d (%s), ",
	      ffchar, fftname[ffchar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (ffchar != FFEOF)
    ffchar = FFEMPTY;

  *++ffvsp = fflval;
#if FFLSP_NEEDED
  *++fflsp = fflloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (fferrstatus)
    fferrstatus--;

  ffstate = ffn;
  goto ffnewstate;


/*-----------------------------------------------------------.
| ffdefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
ffdefault:
  ffn = ffdefact[ffstate];
  if (ffn == 0)
    goto fferrlab;
  goto ffreduce;


/*-----------------------------.
| ffreduce -- Do a reduction.  |
`-----------------------------*/
ffreduce:
  /* ffn is the number of a rule to reduce with.  */
  fflen = ffr2[ffn];

  /* If FFLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets FFVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to FFVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that FFVAL may be used uninitialized.  */
  ffval = ffvsp[1-fflen];

#if FFLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  ffloc = fflsp[1-fflen];
  FFLLOC_DEFAULT (ffloc, (fflsp - fflen), fflen);
#endif

#if FFDEBUG
  /* We have to keep this `#if FFDEBUG', since we use variables which
     are defined only if `FFDEBUG' is set.  */
  if (ffdebug)
    {
      int ffi;

      FFFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 ffn, ffrline[ffn]);

      /* Print the symbols being reduced, and their result.  */
      for (ffi = ffprhs[ffn]; ffrhs[ffi] > 0; ffi++)
	FFFPRINTF (stderr, "%s ", fftname[ffrhs[ffi]]);
      FFFPRINTF (stderr, " -> %s\n", fftname[ffr1[ffn]]);
    }
#endif

  switch (ffn) {

case 3:
#line 227 "eval.y"
{}
    break;
case 4:
#line 229 "eval.y"
{ if( ffvsp[-1].Node<0 ) {
		     fferror("Couldn't build node structure: out of memory?");
		     FFERROR;  }
                  gParse.resultNode = ffvsp[-1].Node;
		}
    break;
case 5:
#line 235 "eval.y"
{ if( ffvsp[-1].Node<0 ) {
		     fferror("Couldn't build node structure: out of memory?");
		     FFERROR;  }
                  gParse.resultNode = ffvsp[-1].Node;
		}
    break;
case 6:
#line 241 "eval.y"
{ if( ffvsp[-1].Node<0 ) {
		     fferror("Couldn't build node structure: out of memory?");
		     FFERROR;  } 
                  gParse.resultNode = ffvsp[-1].Node;
		}
    break;
case 7:
#line 247 "eval.y"
{ if( ffvsp[-1].Node<0 ) {
		     fferror("Couldn't build node structure: out of memory?");
		     FFERROR;  }
                  gParse.resultNode = ffvsp[-1].Node;
		}
    break;
case 8:
#line 252 "eval.y"
{  fferrok;  }
    break;
case 9:
#line 256 "eval.y"
{ ffval.Node = New_Vector( ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 10:
#line 258 "eval.y"
{
                  if( gParse.Nodes[ffvsp[-2].Node].nSubNodes >= MAXSUBS ) {
		     ffvsp[-2].Node = Close_Vec( ffvsp[-2].Node ); TEST(ffvsp[-2].Node);
		     ffval.Node = New_Vector( ffvsp[-2].Node ); TEST(ffval.Node);
                  } else {
                     ffval.Node = ffvsp[-2].Node;
                  }
		  gParse.Nodes[ffval.Node].SubNodes[ gParse.Nodes[ffval.Node].nSubNodes++ ]
		     = ffvsp[0].Node;
                }
    break;
case 11:
#line 271 "eval.y"
{ ffval.Node = New_Vector( ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 12:
#line 273 "eval.y"
{
                  if( TYPE(ffvsp[-2].Node) < TYPE(ffvsp[0].Node) )
                     TYPE(ffvsp[-2].Node) = TYPE(ffvsp[0].Node);
                  if( gParse.Nodes[ffvsp[-2].Node].nSubNodes >= MAXSUBS ) {
		     ffvsp[-2].Node = Close_Vec( ffvsp[-2].Node ); TEST(ffvsp[-2].Node);
		     ffval.Node = New_Vector( ffvsp[-2].Node ); TEST(ffval.Node);
                  } else {
                     ffval.Node = ffvsp[-2].Node;
                  }
		  gParse.Nodes[ffval.Node].SubNodes[ gParse.Nodes[ffval.Node].nSubNodes++ ]
		     = ffvsp[0].Node;
                }
    break;
case 13:
#line 286 "eval.y"
{
                  if( gParse.Nodes[ffvsp[-2].Node].nSubNodes >= MAXSUBS ) {
		     ffvsp[-2].Node = Close_Vec( ffvsp[-2].Node ); TEST(ffvsp[-2].Node);
		     ffval.Node = New_Vector( ffvsp[-2].Node ); TEST(ffval.Node);
                  } else {
                     ffval.Node = ffvsp[-2].Node;
                  }
		  gParse.Nodes[ffval.Node].SubNodes[ gParse.Nodes[ffval.Node].nSubNodes++ ]
		     = ffvsp[0].Node;
                }
    break;
case 14:
#line 297 "eval.y"
{
                  TYPE(ffvsp[-2].Node) = TYPE(ffvsp[0].Node);
                  if( gParse.Nodes[ffvsp[-2].Node].nSubNodes >= MAXSUBS ) {
		     ffvsp[-2].Node = Close_Vec( ffvsp[-2].Node ); TEST(ffvsp[-2].Node);
		     ffval.Node = New_Vector( ffvsp[-2].Node ); TEST(ffval.Node);
                  } else {
                     ffval.Node = ffvsp[-2].Node;
                  }
		  gParse.Nodes[ffval.Node].SubNodes[ gParse.Nodes[ffval.Node].nSubNodes++ ]
		     = ffvsp[0].Node;
                }
    break;
case 15:
#line 311 "eval.y"
{ ffval.Node = Close_Vec( ffvsp[-1].Node ); TEST(ffval.Node); }
    break;
case 16:
#line 315 "eval.y"
{ ffval.Node = Close_Vec( ffvsp[-1].Node ); TEST(ffval.Node); }
    break;
case 17:
#line 319 "eval.y"
{
                  ffval.Node = New_Const( BITSTR, ffvsp[0].str, strlen(ffvsp[0].str)+1 ); TEST(ffval.Node);
		  SIZE(ffval.Node) = strlen(ffvsp[0].str);
		}
    break;
case 18:
#line 324 "eval.y"
{ ffval.Node = New_Column( ffvsp[0].lng ); TEST(ffval.Node); }
    break;
case 19:
#line 326 "eval.y"
{
                  if( TYPE(ffvsp[-1].Node) != LONG
		      || gParse.Nodes[ffvsp[-1].Node].operation != CONST_OP ) {
		     fferror("Offset argument must be a constant integer");
		     FFERROR;
		  }
                  ffval.Node = New_Offset( ffvsp[-3].lng, ffvsp[-1].Node ); TEST(ffval.Node);
                }
    break;
case 20:
#line 335 "eval.y"
{ ffval.Node = New_BinOp( BITSTR, ffvsp[-2].Node, '&', ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = ( SIZE(ffvsp[-2].Node)>SIZE(ffvsp[0].Node) ? SIZE(ffvsp[-2].Node) : SIZE(ffvsp[0].Node) );  }
    break;
case 21:
#line 338 "eval.y"
{ ffval.Node = New_BinOp( BITSTR, ffvsp[-2].Node, '|', ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = ( SIZE(ffvsp[-2].Node)>SIZE(ffvsp[0].Node) ? SIZE(ffvsp[-2].Node) : SIZE(ffvsp[0].Node) );  }
    break;
case 22:
#line 341 "eval.y"
{ ffval.Node = New_BinOp( BITSTR, ffvsp[-2].Node, '+', ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = SIZE(ffvsp[-2].Node) + SIZE(ffvsp[0].Node);                          }
    break;
case 23:
#line 344 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-3].Node, 1, ffvsp[-1].Node,  0,  0,  0,   0 ); TEST(ffval.Node); }
    break;
case 24:
#line 346 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-5].Node, 2, ffvsp[-3].Node, ffvsp[-1].Node,  0,  0,   0 ); TEST(ffval.Node); }
    break;
case 25:
#line 348 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-7].Node, 3, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,  0,   0 ); TEST(ffval.Node); }
    break;
case 26:
#line 350 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-9].Node, 4, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,   0 ); TEST(ffval.Node); }
    break;
case 27:
#line 352 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-11].Node, 5, ffvsp[-9].Node, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node ); TEST(ffval.Node); }
    break;
case 28:
#line 354 "eval.y"
{ ffval.Node = New_Unary( BITSTR, NOT, ffvsp[0].Node ); TEST(ffval.Node);     }
    break;
case 29:
#line 357 "eval.y"
{ ffval.Node = ffvsp[-1].Node; }
    break;
case 30:
#line 361 "eval.y"
{ ffval.Node = New_Const( LONG,   &(ffvsp[0].lng), sizeof(long)   ); TEST(ffval.Node); }
    break;
case 31:
#line 363 "eval.y"
{ ffval.Node = New_Const( DOUBLE, &(ffvsp[0].dbl), sizeof(double) ); TEST(ffval.Node); }
    break;
case 32:
#line 365 "eval.y"
{ ffval.Node = New_Column( ffvsp[0].lng ); TEST(ffval.Node); }
    break;
case 33:
#line 367 "eval.y"
{
                  if( TYPE(ffvsp[-1].Node) != LONG
		      || gParse.Nodes[ffvsp[-1].Node].operation != CONST_OP ) {
		     fferror("Offset argument must be a constant integer");
		     FFERROR;
		  }
                  ffval.Node = New_Offset( ffvsp[-3].lng, ffvsp[-1].Node ); TEST(ffval.Node);
                }
    break;
case 34:
#line 376 "eval.y"
{ ffval.Node = New_Func( LONG, row_fct,  0, 0, 0, 0, 0, 0, 0, 0 ); }
    break;
case 35:
#line 378 "eval.y"
{ ffval.Node = New_Func( LONG, null_fct, 0, 0, 0, 0, 0, 0, 0, 0 ); }
    break;
case 36:
#line 380 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, '%', ffvsp[0].Node );
		  TEST(ffval.Node);                                                }
    break;
case 37:
#line 383 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, '+', ffvsp[0].Node );
		  TEST(ffval.Node);                                                }
    break;
case 38:
#line 386 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, '-', ffvsp[0].Node ); 
		  TEST(ffval.Node);                                                }
    break;
case 39:
#line 389 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, '*', ffvsp[0].Node ); 
		  TEST(ffval.Node);                                                }
    break;
case 40:
#line 392 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, '/', ffvsp[0].Node ); 
		  TEST(ffval.Node);                                                }
    break;
case 41:
#line 395 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, POWER, ffvsp[0].Node );
		  TEST(ffval.Node);                                                }
    break;
case 42:
#line 398 "eval.y"
{ ffval.Node = ffvsp[0].Node; }
    break;
case 43:
#line 400 "eval.y"
{ ffval.Node = New_Unary( TYPE(ffvsp[0].Node), UMINUS, ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 44:
#line 402 "eval.y"
{ ffval.Node = ffvsp[-1].Node; }
    break;
case 45:
#line 404 "eval.y"
{ ffvsp[0].Node = New_Unary( TYPE(ffvsp[-2].Node), 0, ffvsp[0].Node );
                  ffval.Node = New_BinOp( TYPE(ffvsp[-2].Node), ffvsp[-2].Node, '*', ffvsp[0].Node ); 
		  TEST(ffval.Node);                                }
    break;
case 46:
#line 408 "eval.y"
{ ffvsp[-2].Node = New_Unary( TYPE(ffvsp[0].Node), 0, ffvsp[-2].Node );
                  ffval.Node = New_BinOp( TYPE(ffvsp[0].Node), ffvsp[-2].Node, '*', ffvsp[0].Node );
                  TEST(ffval.Node);                                }
    break;
case 47:
#line 412 "eval.y"
{
                  PROMOTE(ffvsp[-2].Node,ffvsp[0].Node);
                  if( ! Test_Dims(ffvsp[-2].Node,ffvsp[0].Node) ) {
                     fferror("Incompatible dimensions in '?:' arguments");
		     FFERROR;
                  }
                  ffval.Node = New_Func( 0, ifthenelse_fct, 3, ffvsp[-2].Node, ffvsp[0].Node, ffvsp[-4].Node,
                                 0, 0, 0, 0 );
                  TEST(ffval.Node);
                  if( SIZE(ffvsp[-2].Node)<SIZE(ffvsp[0].Node) )  Copy_Dims(ffval.Node, ffvsp[0].Node);
                  TYPE(ffvsp[-4].Node) = TYPE(ffvsp[-2].Node);
                  if( ! Test_Dims(ffvsp[-4].Node,ffval.Node) ) {
                     fferror("Incompatible dimensions in '?:' condition");
		     FFERROR;
                  }
                  TYPE(ffvsp[-4].Node) = BOOLEAN;
                  if( SIZE(ffval.Node)<SIZE(ffvsp[-4].Node) )  Copy_Dims(ffval.Node, ffvsp[-4].Node);
                }
    break;
case 48:
#line 431 "eval.y"
{
                  PROMOTE(ffvsp[-2].Node,ffvsp[0].Node);
                  if( ! Test_Dims(ffvsp[-2].Node,ffvsp[0].Node) ) {
                     fferror("Incompatible dimensions in '?:' arguments");
		     FFERROR;
                  }
                  ffval.Node = New_Func( 0, ifthenelse_fct, 3, ffvsp[-2].Node, ffvsp[0].Node, ffvsp[-4].Node,
                                 0, 0, 0, 0 );
                  TEST(ffval.Node);
                  if( SIZE(ffvsp[-2].Node)<SIZE(ffvsp[0].Node) )  Copy_Dims(ffval.Node, ffvsp[0].Node);
                  TYPE(ffvsp[-4].Node) = TYPE(ffvsp[-2].Node);
                  if( ! Test_Dims(ffvsp[-4].Node,ffval.Node) ) {
                     fferror("Incompatible dimensions in '?:' condition");
		     FFERROR;
                  }
                  TYPE(ffvsp[-4].Node) = BOOLEAN;
                  if( SIZE(ffval.Node)<SIZE(ffvsp[-4].Node) )  Copy_Dims(ffval.Node, ffvsp[-4].Node);
                }
    break;
case 49:
#line 450 "eval.y"
{
                  PROMOTE(ffvsp[-2].Node,ffvsp[0].Node);
                  if( ! Test_Dims(ffvsp[-2].Node,ffvsp[0].Node) ) {
                     fferror("Incompatible dimensions in '?:' arguments");
		     FFERROR;
                  }
                  ffval.Node = New_Func( 0, ifthenelse_fct, 3, ffvsp[-2].Node, ffvsp[0].Node, ffvsp[-4].Node,
                                 0, 0, 0, 0 );
                  TEST(ffval.Node);
                  if( SIZE(ffvsp[-2].Node)<SIZE(ffvsp[0].Node) )  Copy_Dims(ffval.Node, ffvsp[0].Node);
                  TYPE(ffvsp[-4].Node) = TYPE(ffvsp[-2].Node);
                  if( ! Test_Dims(ffvsp[-4].Node,ffval.Node) ) {
                     fferror("Incompatible dimensions in '?:' condition");
		     FFERROR;
                  }
                  TYPE(ffvsp[-4].Node) = BOOLEAN;
                  if( SIZE(ffval.Node)<SIZE(ffvsp[-4].Node) )  Copy_Dims(ffval.Node, ffvsp[-4].Node);
                }
    break;
case 50:
#line 469 "eval.y"
{ if (FSTRCMP(ffvsp[-1].str,"RANDOM(") == 0) {
                     srand( (unsigned int) time(NULL) );
                     ffval.Node = New_Func( DOUBLE, rnd_fct, 0, 0, 0, 0, 0, 0, 0, 0 );
                  } else {
                     fferror("Function() not supported");
		     FFERROR;
		  }
                  TEST(ffval.Node); 
                }
    break;
case 51:
#line 479 "eval.y"
{ if (FSTRCMP(ffvsp[-2].str,"SUM(") == 0) {
		     ffval.Node = New_Func( LONG, sum_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
                  } else if (FSTRCMP(ffvsp[-2].str,"NELEM(") == 0) {
                     ffval.Node = New_Const( LONG, &( SIZE(ffvsp[-1].Node) ), sizeof(long) );
		  } else {
                     fferror("Function(bool) not supported");
		     FFERROR;
		  }
                  TEST(ffval.Node); 
		}
    break;
case 52:
#line 490 "eval.y"
{ if (FSTRCMP(ffvsp[-2].str,"NELEM(") == 0) {
                     ffval.Node = New_Const( LONG, &( SIZE(ffvsp[-1].Node) ), sizeof(long) );
		  } else if (FSTRCMP(ffvsp[-2].str,"NVALID(") == 0) {
		     ffval.Node = New_Func( LONG, nonnull_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		  } else {
                     fferror("Function(str) not supported");
		     FFERROR;
		  }
                  TEST(ffval.Node); 
		}
    break;
case 53:
#line 502 "eval.y"
{ if (FSTRCMP(ffvsp[-2].str,"NELEM(") == 0) {
                     ffval.Node = New_Const( LONG, &( SIZE(ffvsp[-1].Node) ), sizeof(long) );
		} else if (FSTRCMP(ffvsp[-2].str,"NVALID(") == 0) { /* Bit arrays do not have NULL */
                     ffval.Node = New_Const( LONG, &( SIZE(ffvsp[-1].Node) ), sizeof(long) );
		} else if (FSTRCMP(ffvsp[-2].str,"SUM(") == 0) {
		     ffval.Node = New_Func( LONG, sum_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		} else if (FSTRCMP(ffvsp[-2].str,"MIN(") == 0) {
		     ffval.Node = New_Func( TYPE(ffvsp[-1].Node),  /* Force 1D result */
				    min1_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     SIZE(ffval.Node) = 1;
		} else if (FSTRCMP(ffvsp[-2].str,"MAX(") == 0) {
		     ffval.Node = New_Func( TYPE(ffvsp[-1].Node),  /* Force 1D result */
				    max1_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     SIZE(ffval.Node) = 1;
		} else {
                     fferror("Function(bits) not supported");
		     FFERROR;
		  }
                  TEST(ffval.Node); 
		}
    break;
case 54:
#line 524 "eval.y"
{ if (FSTRCMP(ffvsp[-2].str,"SUM(") == 0)
		     ffval.Node = New_Func( TYPE(ffvsp[-1].Node), sum_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		  else if (FSTRCMP(ffvsp[-2].str,"AVERAGE(") == 0)
		     ffval.Node = New_Func( DOUBLE, average_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		  else if (FSTRCMP(ffvsp[-2].str,"STDDEV(") == 0)
		     ffval.Node = New_Func( DOUBLE, stddev_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		  else if (FSTRCMP(ffvsp[-2].str,"MEDIAN(") == 0)
		     ffval.Node = New_Func( TYPE(ffvsp[-1].Node), median_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		  else if (FSTRCMP(ffvsp[-2].str,"NELEM(") == 0)
                     ffval.Node = New_Const( LONG, &( SIZE(ffvsp[-1].Node) ), sizeof(long) );
		  else if (FSTRCMP(ffvsp[-2].str,"NVALID(") == 0)
		     ffval.Node = New_Func( LONG, nonnull_fct, 1, ffvsp[-1].Node,
				    0, 0, 0, 0, 0, 0 );
		  else if (FSTRCMP(ffvsp[-2].str,"ABS(") == 0)
		     ffval.Node = New_Func( 0, abs_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
 		  else if (FSTRCMP(ffvsp[-2].str,"MIN(") == 0)
		     ffval.Node = New_Func( TYPE(ffvsp[-1].Node),  /* Force 1D result */
				    min1_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		  else if (FSTRCMP(ffvsp[-2].str,"MAX(") == 0)
		     ffval.Node = New_Func( TYPE(ffvsp[-1].Node),  /* Force 1D result */
				    max1_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
  		  else {  /*  These all take DOUBLE arguments  */
		     if( TYPE(ffvsp[-1].Node) != DOUBLE ) ffvsp[-1].Node = New_Unary( DOUBLE, 0, ffvsp[-1].Node );
                     if (FSTRCMP(ffvsp[-2].str,"SIN(") == 0)
			ffval.Node = New_Func( 0, sin_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"COS(") == 0)
			ffval.Node = New_Func( 0, cos_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"TAN(") == 0)
			ffval.Node = New_Func( 0, tan_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"ARCSIN(") == 0
			      || FSTRCMP(ffvsp[-2].str,"ASIN(") == 0)
			ffval.Node = New_Func( 0, asin_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"ARCCOS(") == 0
			      || FSTRCMP(ffvsp[-2].str,"ACOS(") == 0)
			ffval.Node = New_Func( 0, acos_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"ARCTAN(") == 0
			      || FSTRCMP(ffvsp[-2].str,"ATAN(") == 0)
			ffval.Node = New_Func( 0, atan_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"SINH(") == 0)
			ffval.Node = New_Func( 0, sinh_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"COSH(") == 0)
			ffval.Node = New_Func( 0, cosh_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"TANH(") == 0)
			ffval.Node = New_Func( 0, tanh_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"EXP(") == 0)
			ffval.Node = New_Func( 0, exp_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"LOG(") == 0)
			ffval.Node = New_Func( 0, log_fct,  1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"LOG10(") == 0)
			ffval.Node = New_Func( 0, log10_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"SQRT(") == 0)
			ffval.Node = New_Func( 0, sqrt_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"FLOOR(") == 0)
			ffval.Node = New_Func( 0, floor_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else if (FSTRCMP(ffvsp[-2].str,"CEIL(") == 0)
			ffval.Node = New_Func( 0, ceil_fct, 1, ffvsp[-1].Node, 0, 0, 0, 0, 0, 0 );
		     else {
			fferror("Function(expr) not supported");
			FFERROR;
		     }
		  }
                  TEST(ffval.Node); 
                }
    break;
case 55:
#line 592 "eval.y"
{ 
		   if (FSTRCMP(ffvsp[-4].str,"DEFNULL(") == 0) {
		      if( SIZE(ffvsp[-3].Node)>=SIZE(ffvsp[-1].Node) && Test_Dims( ffvsp[-3].Node, ffvsp[-1].Node ) ) {
			 PROMOTE(ffvsp[-3].Node,ffvsp[-1].Node);
			 ffval.Node = New_Func( 0, defnull_fct, 2, ffvsp[-3].Node, ffvsp[-1].Node, 0,
					0, 0, 0, 0 );
			 TEST(ffval.Node); 
		      } else {
			 fferror("Dimensions of DEFNULL arguments "
				 "are not compatible");
			 FFERROR;
		      }
		   } else if (FSTRCMP(ffvsp[-4].str,"ARCTAN2(") == 0) {
		     if( TYPE(ffvsp[-3].Node) != DOUBLE ) ffvsp[-3].Node = New_Unary( DOUBLE, 0, ffvsp[-3].Node );
		     if( TYPE(ffvsp[-1].Node) != DOUBLE ) ffvsp[-1].Node = New_Unary( DOUBLE, 0, ffvsp[-1].Node );
		     if( Test_Dims( ffvsp[-3].Node, ffvsp[-1].Node ) ) {
			ffval.Node = New_Func( 0, atan2_fct, 2, ffvsp[-3].Node, ffvsp[-1].Node, 0, 0, 0, 0, 0 );
			TEST(ffval.Node); 
			if( SIZE(ffvsp[-3].Node)<SIZE(ffvsp[-1].Node) ) Copy_Dims(ffval.Node, ffvsp[-1].Node);
		     } else {
			fferror("Dimensions of arctan2 arguments "
				"are not compatible");
			FFERROR;
		     }
		   } else if (FSTRCMP(ffvsp[-4].str,"MIN(") == 0) {
		      PROMOTE( ffvsp[-3].Node, ffvsp[-1].Node );
		      if( Test_Dims( ffvsp[-3].Node, ffvsp[-1].Node ) ) {
			ffval.Node = New_Func( 0, min2_fct, 2, ffvsp[-3].Node, ffvsp[-1].Node, 0, 0, 0, 0, 0 );
			TEST(ffval.Node);
			if( SIZE(ffvsp[-3].Node)<SIZE(ffvsp[-1].Node) ) Copy_Dims(ffval.Node, ffvsp[-1].Node);
		      } else {
			fferror("Dimensions of min(a,b) arguments "
				"are not compatible");
			FFERROR;
		      }
		   } else if (FSTRCMP(ffvsp[-4].str,"MAX(") == 0) {
		      PROMOTE( ffvsp[-3].Node, ffvsp[-1].Node );
		      if( Test_Dims( ffvsp[-3].Node, ffvsp[-1].Node ) ) {
			ffval.Node = New_Func( 0, max2_fct, 2, ffvsp[-3].Node, ffvsp[-1].Node, 0, 0, 0, 0, 0 );
			TEST(ffval.Node);
			if( SIZE(ffvsp[-3].Node)<SIZE(ffvsp[-1].Node) ) Copy_Dims(ffval.Node, ffvsp[-1].Node);
		      } else {
			fferror("Dimensions of max(a,b) arguments "
				"are not compatible");
			FFERROR;
		      }
		   } else {
		      fferror("Function(expr,expr) not supported");
		      FFERROR;
		   }
                }
    break;
case 56:
#line 644 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-3].Node, 1, ffvsp[-1].Node,  0,  0,  0,   0 ); TEST(ffval.Node); }
    break;
case 57:
#line 646 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-5].Node, 2, ffvsp[-3].Node, ffvsp[-1].Node,  0,  0,   0 ); TEST(ffval.Node); }
    break;
case 58:
#line 648 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-7].Node, 3, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,  0,   0 ); TEST(ffval.Node); }
    break;
case 59:
#line 650 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-9].Node, 4, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,   0 ); TEST(ffval.Node); }
    break;
case 60:
#line 652 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-11].Node, 5, ffvsp[-9].Node, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node ); TEST(ffval.Node); }
    break;
case 61:
#line 654 "eval.y"
{ ffval.Node = New_Unary( LONG,   INTCAST, ffvsp[0].Node );  TEST(ffval.Node);  }
    break;
case 62:
#line 656 "eval.y"
{ ffval.Node = New_Unary( LONG,   INTCAST, ffvsp[0].Node );  TEST(ffval.Node);  }
    break;
case 63:
#line 658 "eval.y"
{ ffval.Node = New_Unary( DOUBLE, FLTCAST, ffvsp[0].Node );  TEST(ffval.Node);  }
    break;
case 64:
#line 660 "eval.y"
{ ffval.Node = New_Unary( DOUBLE, FLTCAST, ffvsp[0].Node );  TEST(ffval.Node);  }
    break;
case 65:
#line 664 "eval.y"
{ ffval.Node = New_Const( BOOLEAN, &(ffvsp[0].log), sizeof(char) ); TEST(ffval.Node); }
    break;
case 66:
#line 666 "eval.y"
{ ffval.Node = New_Column( ffvsp[0].lng ); TEST(ffval.Node); }
    break;
case 67:
#line 668 "eval.y"
{
                  if( TYPE(ffvsp[-1].Node) != LONG
		      || gParse.Nodes[ffvsp[-1].Node].operation != CONST_OP ) {
		     fferror("Offset argument must be a constant integer");
		     FFERROR;
		  }
                  ffval.Node = New_Offset( ffvsp[-3].lng, ffvsp[-1].Node ); TEST(ffval.Node);
                }
    break;
case 68:
#line 677 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, EQ,  ffvsp[0].Node ); TEST(ffval.Node);
		  SIZE(ffval.Node) = 1;                                     }
    break;
case 69:
#line 680 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, NE,  ffvsp[0].Node ); TEST(ffval.Node); 
		  SIZE(ffval.Node) = 1;                                     }
    break;
case 70:
#line 683 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LT,  ffvsp[0].Node ); TEST(ffval.Node); 
		  SIZE(ffval.Node) = 1;                                     }
    break;
case 71:
#line 686 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LTE, ffvsp[0].Node ); TEST(ffval.Node); 
		  SIZE(ffval.Node) = 1;                                     }
    break;
case 72:
#line 689 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, GT,  ffvsp[0].Node ); TEST(ffval.Node); 
		  SIZE(ffval.Node) = 1;                                     }
    break;
case 73:
#line 692 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, GTE, ffvsp[0].Node ); TEST(ffval.Node); 
		  SIZE(ffval.Node) = 1;                                     }
    break;
case 74:
#line 695 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, GT,  ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 75:
#line 698 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LT,  ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 76:
#line 701 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, GTE, ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 77:
#line 704 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LTE, ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 78:
#line 707 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, '~', ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 79:
#line 710 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, EQ,  ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 80:
#line 713 "eval.y"
{ PROMOTE(ffvsp[-2].Node,ffvsp[0].Node); ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, NE,  ffvsp[0].Node );
                  TEST(ffval.Node);                                               }
    break;
case 81:
#line 716 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, EQ,  ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = 1; }
    break;
case 82:
#line 719 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, NE,  ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = 1; }
    break;
case 83:
#line 722 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, GT,  ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = 1; }
    break;
case 84:
#line 725 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, GTE, ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = 1; }
    break;
case 85:
#line 728 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LT,  ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = 1; }
    break;
case 86:
#line 731 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LTE, ffvsp[0].Node ); TEST(ffval.Node);
                  SIZE(ffval.Node) = 1; }
    break;
case 87:
#line 734 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, AND, ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 88:
#line 736 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, OR,  ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 89:
#line 738 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, EQ,  ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 90:
#line 740 "eval.y"
{ ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, NE,  ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 91:
#line 743 "eval.y"
{ PROMOTE(ffvsp[-4].Node,ffvsp[-2].Node); PROMOTE(ffvsp[-4].Node,ffvsp[0].Node); PROMOTE(ffvsp[-2].Node,ffvsp[0].Node);
		  ffvsp[-2].Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, LTE, ffvsp[-4].Node );
                  ffvsp[0].Node = New_BinOp( BOOLEAN, ffvsp[-4].Node, LTE, ffvsp[0].Node );
                  ffval.Node = New_BinOp( BOOLEAN, ffvsp[-2].Node, AND, ffvsp[0].Node );
                  TEST(ffval.Node);                                         }
    break;
case 92:
#line 750 "eval.y"
{
                  if( ! Test_Dims(ffvsp[-2].Node,ffvsp[0].Node) ) {
                     fferror("Incompatible dimensions in '?:' arguments");
		     FFERROR;
                  }
                  ffval.Node = New_Func( 0, ifthenelse_fct, 3, ffvsp[-2].Node, ffvsp[0].Node, ffvsp[-4].Node,
                                 0, 0, 0, 0 );
                  TEST(ffval.Node);
                  if( SIZE(ffvsp[-2].Node)<SIZE(ffvsp[0].Node) )  Copy_Dims(ffval.Node, ffvsp[0].Node);
                  if( ! Test_Dims(ffvsp[-4].Node,ffval.Node) ) {
                     fferror("Incompatible dimensions in '?:' condition");
		     FFERROR;
                  }
                  if( SIZE(ffval.Node)<SIZE(ffvsp[-4].Node) )  Copy_Dims(ffval.Node, ffvsp[-4].Node);
                }
    break;
case 93:
#line 767 "eval.y"
{
		   if (FSTRCMP(ffvsp[-2].str,"ISNULL(") == 0) {
		      ffval.Node = New_Func( 0, isnull_fct, 1, ffvsp[-1].Node, 0, 0,
				     0, 0, 0, 0 );
		      TEST(ffval.Node); 
                      /* Use expression's size, but return BOOLEAN */
		      TYPE(ffval.Node) = BOOLEAN;
		   } else {
		      fferror("Boolean Function(expr) not supported");
		      FFERROR;
		   }
		}
    break;
case 94:
#line 780 "eval.y"
{
		   if (FSTRCMP(ffvsp[-2].str,"ISNULL(") == 0) {
		      ffval.Node = New_Func( 0, isnull_fct, 1, ffvsp[-1].Node, 0, 0,
				     0, 0, 0, 0 );
		      TEST(ffval.Node); 
                      /* Use expression's size, but return BOOLEAN */
		      TYPE(ffval.Node) = BOOLEAN;
		   } else {
		      fferror("Boolean Function(expr) not supported");
		      FFERROR;
		   }
		}
    break;
case 95:
#line 793 "eval.y"
{
		   if (FSTRCMP(ffvsp[-2].str,"ISNULL(") == 0) {
		      ffval.Node = New_Func( BOOLEAN, isnull_fct, 1, ffvsp[-1].Node, 0, 0,
				     0, 0, 0, 0 );
		      TEST(ffval.Node); 
		   } else {
		      fferror("Boolean Function(expr) not supported");
		      FFERROR;
		   }
		}
    break;
case 96:
#line 804 "eval.y"
{
		   if (FSTRCMP(ffvsp[-4].str,"DEFNULL(") == 0) {
		      if( SIZE(ffvsp[-3].Node)>=SIZE(ffvsp[-1].Node) && Test_Dims( ffvsp[-3].Node, ffvsp[-1].Node ) ) {
			 ffval.Node = New_Func( 0, defnull_fct, 2, ffvsp[-3].Node, ffvsp[-1].Node, 0,
					0, 0, 0, 0 );
			 TEST(ffval.Node); 
		      } else {
			 fferror("Dimensions of DEFNULL arguments are not compatible");
			 FFERROR;
		      }
		   } else {
		      fferror("Boolean Function(expr,expr) not supported");
		      FFERROR;
		   }
		}
    break;
case 97:
#line 820 "eval.y"
{
		   if( SIZE(ffvsp[-5].Node)>1 || SIZE(ffvsp[-3].Node)>1 || SIZE(ffvsp[-1].Node)>1 ) {
		      fferror("Cannot use array as function argument");
		      FFERROR;
		   }
		   if( TYPE(ffvsp[-5].Node) != DOUBLE ) ffvsp[-5].Node = New_Unary( DOUBLE, 0, ffvsp[-5].Node );
		   if( TYPE(ffvsp[-3].Node) != DOUBLE ) ffvsp[-3].Node = New_Unary( DOUBLE, 0, ffvsp[-3].Node );
		   if( TYPE(ffvsp[-1].Node) != DOUBLE ) ffvsp[-1].Node = New_Unary( DOUBLE, 0, ffvsp[-1].Node );
		   if (FSTRCMP(ffvsp[-6].str,"NEAR(") == 0)
		      ffval.Node = New_Func( BOOLEAN, near_fct, 3, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,
				     0, 0, 0, 0 );
		   else {
		      fferror("Boolean Function not supported");
		      FFERROR;
		   }
                   TEST(ffval.Node); 
		}
    break;
case 98:
#line 838 "eval.y"
{
		   if( SIZE(ffvsp[-9].Node)>1 || SIZE(ffvsp[-7].Node)>1 || SIZE(ffvsp[-5].Node)>1 || SIZE(ffvsp[-3].Node)>1
		       || SIZE(ffvsp[-1].Node)>1 ) {
		      fferror("Cannot use array as function argument");
		      FFERROR;
		   }
		   if( TYPE(ffvsp[-9].Node) != DOUBLE ) ffvsp[-9].Node = New_Unary( DOUBLE, 0, ffvsp[-9].Node );
		   if( TYPE(ffvsp[-7].Node) != DOUBLE ) ffvsp[-7].Node = New_Unary( DOUBLE, 0, ffvsp[-7].Node );
		   if( TYPE(ffvsp[-5].Node) != DOUBLE ) ffvsp[-5].Node = New_Unary( DOUBLE, 0, ffvsp[-5].Node );
		   if( TYPE(ffvsp[-3].Node) != DOUBLE ) ffvsp[-3].Node = New_Unary( DOUBLE, 0, ffvsp[-3].Node );
		   if( TYPE(ffvsp[-1].Node)!= DOUBLE ) ffvsp[-1].Node= New_Unary( DOUBLE, 0, ffvsp[-1].Node);
                   if (FSTRCMP(ffvsp[-10].str,"CIRCLE(") == 0)
		      ffval.Node = New_Func( BOOLEAN, circle_fct, 5, ffvsp[-9].Node, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node,
				     ffvsp[-1].Node, 0, 0 );
		   else {
		      fferror("Boolean Function not supported");
		      FFERROR;
		   }
                   TEST(ffval.Node); 
		}
    break;
case 99:
#line 859 "eval.y"
{
		   if( SIZE(ffvsp[-13].Node)>1 || SIZE(ffvsp[-11].Node)>1 || SIZE(ffvsp[-9].Node)>1 || SIZE(ffvsp[-7].Node)>1
		       || SIZE(ffvsp[-5].Node)>1 || SIZE(ffvsp[-3].Node)>1 || SIZE(ffvsp[-1].Node)>1 ) {
		      fferror("Cannot use array as function argument");
		      FFERROR;
		   }
		   if( TYPE(ffvsp[-13].Node) != DOUBLE ) ffvsp[-13].Node = New_Unary( DOUBLE, 0, ffvsp[-13].Node );
		   if( TYPE(ffvsp[-11].Node) != DOUBLE ) ffvsp[-11].Node = New_Unary( DOUBLE, 0, ffvsp[-11].Node );
		   if( TYPE(ffvsp[-9].Node) != DOUBLE ) ffvsp[-9].Node = New_Unary( DOUBLE, 0, ffvsp[-9].Node );
		   if( TYPE(ffvsp[-7].Node) != DOUBLE ) ffvsp[-7].Node = New_Unary( DOUBLE, 0, ffvsp[-7].Node );
		   if( TYPE(ffvsp[-5].Node)!= DOUBLE ) ffvsp[-5].Node= New_Unary( DOUBLE, 0, ffvsp[-5].Node);
		   if( TYPE(ffvsp[-3].Node)!= DOUBLE ) ffvsp[-3].Node= New_Unary( DOUBLE, 0, ffvsp[-3].Node);
		   if( TYPE(ffvsp[-1].Node)!= DOUBLE ) ffvsp[-1].Node= New_Unary( DOUBLE, 0, ffvsp[-1].Node);
		   if (FSTRCMP(ffvsp[-14].str,"BOX(") == 0)
		      ffval.Node = New_Func( BOOLEAN, box_fct, 7, ffvsp[-13].Node, ffvsp[-11].Node, ffvsp[-9].Node, ffvsp[-7].Node,
				      ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node );
		   else if (FSTRCMP(ffvsp[-14].str,"ELLIPSE(") == 0)
		      ffval.Node = New_Func( BOOLEAN, elps_fct, 7, ffvsp[-13].Node, ffvsp[-11].Node, ffvsp[-9].Node, ffvsp[-7].Node,
				      ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node );
		   else {
		      fferror("SAO Image Function not supported");
		      FFERROR;
		   }
                   TEST(ffval.Node); 
		}
    break;
case 100:
#line 886 "eval.y"
{ /* Use defaults for all elements */
                   ffval.Node = New_GTI( "", -99, "*START*", "*STOP*" );
                   TEST(ffval.Node);                                        }
    break;
case 101:
#line 890 "eval.y"
{ /* Use defaults for all except filename */
                   ffval.Node = New_GTI( ffvsp[-1].str, -99, "*START*", "*STOP*" );
                   TEST(ffval.Node);                                        }
    break;
case 102:
#line 894 "eval.y"
{  ffval.Node = New_GTI( ffvsp[-3].str, ffvsp[-1].Node, "*START*", "*STOP*" );
                   TEST(ffval.Node);                                        }
    break;
case 103:
#line 897 "eval.y"
{  ffval.Node = New_GTI( ffvsp[-7].str, ffvsp[-5].Node, ffvsp[-3].str, ffvsp[-1].str );
                   TEST(ffval.Node);                                        }
    break;
case 104:
#line 901 "eval.y"
{ /* Use defaults for all except filename */
                   ffval.Node = New_REG( ffvsp[-1].str, -99, -99, "" );
                   TEST(ffval.Node);                                        }
    break;
case 105:
#line 905 "eval.y"
{  ffval.Node = New_REG( ffvsp[-5].str, ffvsp[-3].Node, ffvsp[-1].Node, "" );
                   TEST(ffval.Node);                                        }
    break;
case 106:
#line 908 "eval.y"
{  ffval.Node = New_REG( ffvsp[-7].str, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].str );
                   TEST(ffval.Node);                                        }
    break;
case 107:
#line 912 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-3].Node, 1, ffvsp[-1].Node,  0,  0,  0,   0 ); TEST(ffval.Node); }
    break;
case 108:
#line 914 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-5].Node, 2, ffvsp[-3].Node, ffvsp[-1].Node,  0,  0,   0 ); TEST(ffval.Node); }
    break;
case 109:
#line 916 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-7].Node, 3, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,  0,   0 ); TEST(ffval.Node); }
    break;
case 110:
#line 918 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-9].Node, 4, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node,   0 ); TEST(ffval.Node); }
    break;
case 111:
#line 920 "eval.y"
{ ffval.Node = New_Deref( ffvsp[-11].Node, 5, ffvsp[-9].Node, ffvsp[-7].Node, ffvsp[-5].Node, ffvsp[-3].Node, ffvsp[-1].Node ); TEST(ffval.Node); }
    break;
case 112:
#line 922 "eval.y"
{ ffval.Node = New_Unary( BOOLEAN, NOT, ffvsp[0].Node ); TEST(ffval.Node); }
    break;
case 113:
#line 924 "eval.y"
{ ffval.Node = ffvsp[-1].Node; }
    break;
case 114:
#line 928 "eval.y"
{ ffval.Node = New_Const( STRING, ffvsp[0].str, strlen(ffvsp[0].str)+1 ); TEST(ffval.Node);
                  SIZE(ffval.Node) = strlen(ffvsp[0].str);                            }
    break;
case 115:
#line 931 "eval.y"
{ ffval.Node = New_Column( ffvsp[0].lng ); TEST(ffval.Node); }
    break;
case 116:
#line 933 "eval.y"
{
                  if( TYPE(ffvsp[-1].Node) != LONG
		      || gParse.Nodes[ffvsp[-1].Node].operation != CONST_OP ) {
		     fferror("Offset argument must be a constant integer");
		     FFERROR;
		  }
                  ffval.Node = New_Offset( ffvsp[-3].lng, ffvsp[-1].Node ); TEST(ffval.Node);
                }
    break;
case 117:
#line 942 "eval.y"
{ ffval.Node = New_Func( STRING, null_fct, 0, 0, 0, 0, 0, 0, 0, 0 ); }
    break;
case 118:
#line 944 "eval.y"
{ ffval.Node = ffvsp[-1].Node; }
    break;
case 119:
#line 946 "eval.y"
{ ffval.Node = New_BinOp( STRING, ffvsp[-2].Node, '+', ffvsp[0].Node );  TEST(ffval.Node);
		  SIZE(ffval.Node) = SIZE(ffvsp[-2].Node) + SIZE(ffvsp[0].Node);                   }
    break;
case 120:
#line 949 "eval.y"
{
                  if( SIZE(ffvsp[-4].Node)!=1 ) {
                     fferror("Cannot have a vector string column");
		     FFERROR;
                  }
                  ffval.Node = New_Func( 0, ifthenelse_fct, 3, ffvsp[-2].Node, ffvsp[0].Node, ffvsp[-4].Node,
                                 0, 0, 0, 0 );
                  TEST(ffval.Node);
                  if( SIZE(ffvsp[-2].Node)<SIZE(ffvsp[0].Node) )  Copy_Dims(ffval.Node, ffvsp[0].Node);
                }
    break;
case 121:
#line 961 "eval.y"
{ 
		  if (FSTRCMP(ffvsp[-4].str,"DEFNULL(") == 0) {
		     ffval.Node = New_Func( 0, defnull_fct, 2, ffvsp[-3].Node, ffvsp[-1].Node, 0,
				    0, 0, 0, 0 );
		     TEST(ffval.Node); 
		     if( SIZE(ffvsp[-1].Node)>SIZE(ffvsp[-3].Node) ) SIZE(ffval.Node) = SIZE(ffvsp[-1].Node);
		  }
		}
    break;
}

#line 705 "/usr/share/bison/bison.simple"


  ffvsp -= fflen;
  ffssp -= fflen;
#if FFLSP_NEEDED
  fflsp -= fflen;
#endif

#if FFDEBUG
  if (ffdebug)
    {
      short *ffssp1 = ffss - 1;
      FFFPRINTF (stderr, "state stack now");
      while (ffssp1 != ffssp)
	FFFPRINTF (stderr, " %d", *++ffssp1);
      FFFPRINTF (stderr, "\n");
    }
#endif

  *++ffvsp = ffval;
#if FFLSP_NEEDED
  *++fflsp = ffloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  ffn = ffr1[ffn];

  ffstate = ffpgoto[ffn - FFNTBASE] + *ffssp;
  if (ffstate >= 0 && ffstate <= FFLAST && ffcheck[ffstate] == *ffssp)
    ffstate = fftable[ffstate];
  else
    ffstate = ffdefgoto[ffn - FFNTBASE];

  goto ffnewstate;


/*------------------------------------.
| fferrlab -- here on detecting error |
`------------------------------------*/
fferrlab:
  /* If not already recovering from an error, report this error.  */
  if (!fferrstatus)
    {
      ++ffnerrs;

#ifdef FFERROR_VERBOSE
      ffn = ffpact[ffstate];

      if (ffn > FFFLAG && ffn < FFLAST)
	{
	  FFSIZE_T ffsize = 0;
	  char *ffmsg;
	  int ffx, ffcount;

	  ffcount = 0;
	  /* Start FFX at -FFN if negative to avoid negative indexes in
	     FFCHECK.  */
	  for (ffx = ffn < 0 ? -ffn : 0;
	       ffx < (int) (sizeof (fftname) / sizeof (char *)); ffx++)
	    if (ffcheck[ffx + ffn] == ffx)
	      ffsize += ffstrlen (fftname[ffx]) + 15, ffcount++;
	  ffsize += ffstrlen ("parse error, unexpected ") + 1;
	  ffsize += ffstrlen (fftname[FFTRANSLATE (ffchar)]);
	  ffmsg = (char *) FFSTACK_ALLOC (ffsize);
	  if (ffmsg != 0)
	    {
	      char *ffp = ffstpcpy (ffmsg, "parse error, unexpected ");
	      ffp = ffstpcpy (ffp, fftname[FFTRANSLATE (ffchar)]);

	      if (ffcount < 5)
		{
		  ffcount = 0;
		  for (ffx = ffn < 0 ? -ffn : 0;
		       ffx < (int) (sizeof (fftname) / sizeof (char *));
		       ffx++)
		    if (ffcheck[ffx + ffn] == ffx)
		      {
			const char *ffq = ! ffcount ? ", expecting " : " or ";
			ffp = ffstpcpy (ffp, ffq);
			ffp = ffstpcpy (ffp, fftname[ffx]);
			ffcount++;
		      }
		}
	      fferror (ffmsg);
	      FFSTACK_FREE (ffmsg);
	    }
	  else
	    fferror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (FFERROR_VERBOSE) */
	fferror ("parse error");
    }
  goto fferrlab1;


/*--------------------------------------------------.
| fferrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
fferrlab1:
  if (fferrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (ffchar == FFEOF)
	FFABORT;
      FFDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  ffchar, fftname[ffchar1]));
      ffchar = FFEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  fferrstatus = 3;		/* Each real token shifted decrements this */

  goto fferrhandle;


/*-------------------------------------------------------------------.
| fferrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
fferrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  ffn = ffdefact[ffstate];
  if (ffn)
    goto ffdefault;
#endif


/*---------------------------------------------------------------.
| fferrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
fferrpop:
  if (ffssp == ffss)
    FFABORT;
  ffvsp--;
  ffstate = *--ffssp;
#if FFLSP_NEEDED
  fflsp--;
#endif

#if FFDEBUG
  if (ffdebug)
    {
      short *ffssp1 = ffss - 1;
      FFFPRINTF (stderr, "Error: state stack now");
      while (ffssp1 != ffssp)
	FFFPRINTF (stderr, " %d", *++ffssp1);
      FFFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| fferrhandle.  |
`--------------*/
fferrhandle:
  ffn = ffpact[ffstate];
  if (ffn == FFFLAG)
    goto fferrdefault;

  ffn += FFTERROR;
  if (ffn < 0 || ffn > FFLAST || ffcheck[ffn] != FFTERROR)
    goto fferrdefault;

  ffn = fftable[ffn];
  if (ffn < 0)
    {
      if (ffn == FFFLAG)
	goto fferrpop;
      ffn = -ffn;
      goto ffreduce;
    }
  else if (ffn == 0)
    goto fferrpop;

  if (ffn == FFFINAL)
    FFACCEPT;

  FFDPRINTF ((stderr, "Shifting error token, "));

  *++ffvsp = fflval;
#if FFLSP_NEEDED
  *++fflsp = fflloc;
#endif

  ffstate = ffn;
  goto ffnewstate;


/*-------------------------------------.
| ffacceptlab -- FFACCEPT comes here.  |
`-------------------------------------*/
ffacceptlab:
  ffresult = 0;
  goto ffreturn;

/*-----------------------------------.
| ffabortlab -- FFABORT comes here.  |
`-----------------------------------*/
ffabortlab:
  ffresult = 1;
  goto ffreturn;

/*---------------------------------------------.
| ffoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
ffoverflowlab:
  fferror ("parser stack overflow");
  ffresult = 2;
  /* Fall through.  */

ffreturn:
#ifndef ffoverflow
  if (ffss != ffssa)
    FFSTACK_FREE (ffss);
#endif
  return ffresult;
}
#line 971 "eval.y"


/*************************************************************************/
/*  Start of "New" routines which build the expression Nodal structure   */
/*************************************************************************/

static int Alloc_Node( void )
{
                      /* Use this for allocation to guarantee *Nodes */
   Node *newNodePtr;  /* survives on failure, making it still valid  */
                      /* while working our way out of this error     */

   if( gParse.nNodes == gParse.nNodesAlloc ) {
      if( gParse.Nodes ) {
	 gParse.nNodesAlloc += gParse.nNodesAlloc;
	 newNodePtr = (Node *)realloc( gParse.Nodes,
				       sizeof(Node)*gParse.nNodesAlloc );
      } else {
	 gParse.nNodesAlloc = 100;
	 newNodePtr = (Node *)malloc ( sizeof(Node)*gParse.nNodesAlloc );
      }	 

      if( newNodePtr ) {
	 gParse.Nodes = newNodePtr;
      } else {
	 gParse.status = MEMORY_ALLOCATION;
	 return( -1 );
      }
   }

   return ( gParse.nNodes++ );
}

static void Free_Last_Node( void )
{
   if( gParse.nNodes ) gParse.nNodes--;
}

static int New_Const( int returnType, void *value, long len )
{
   Node *this;
   int n;

   n = Alloc_Node();
   if( n>=0 ) {
      this             = gParse.Nodes + n;
      this->operation  = CONST_OP;             /* Flag a constant */
      this->DoOp       = NULL;
      this->nSubNodes  = 0;
      this->type       = returnType;
      memcpy( &(this->value.data), value, len );
      this->value.undef = NULL;
      this->value.nelem = 1;
      this->value.naxis = 1;
      this->value.naxes[0] = 1;
   }
   return(n);
}

static int New_Column( int ColNum )
{
   Node *this;
   int  n, i;

   n = Alloc_Node();
   if( n>=0 ) {
      this              = gParse.Nodes + n;
      this->operation   = -ColNum;
      this->DoOp        = NULL;
      this->nSubNodes   = 0;
      this->type        = gParse.varData[ColNum].type;
      this->value.nelem = gParse.varData[ColNum].nelem;
      this->value.naxis = gParse.varData[ColNum].naxis;
      for( i=0; i<gParse.varData[ColNum].naxis; i++ )
	 this->value.naxes[i] = gParse.varData[ColNum].naxes[i];
   }
   return(n);
}

static int New_Offset( int ColNum, int offsetNode )
{
   Node *this;
   int  n, i, colNode;

   colNode = New_Column( ColNum );
   if( colNode<0 ) return(-1);

   n = Alloc_Node();
   if( n>=0 ) {
      this              = gParse.Nodes + n;
      this->operation   = '{';
      this->DoOp        = Do_Offset;
      this->nSubNodes   = 2;
      this->SubNodes[0] = colNode;
      this->SubNodes[1] = offsetNode;
      this->type        = gParse.varData[ColNum].type;
      this->value.nelem = gParse.varData[ColNum].nelem;
      this->value.naxis = gParse.varData[ColNum].naxis;
      for( i=0; i<gParse.varData[ColNum].naxis; i++ )
	 this->value.naxes[i] = gParse.varData[ColNum].naxes[i];
   }
   return(n);
}

static int New_Unary( int returnType, int Op, int Node1 )
{
   Node *this, *that;
   int  i,n;

   if( Node1<0 ) return(-1);
   that = gParse.Nodes + Node1;

   if( !Op ) Op = returnType;

   if( (Op==DOUBLE || Op==FLTCAST) && that->type==DOUBLE  ) return( Node1 );
   if( (Op==LONG   || Op==INTCAST) && that->type==LONG    ) return( Node1 );
   if( (Op==BOOLEAN              ) && that->type==BOOLEAN ) return( Node1 );
   
   n = Alloc_Node();
   if( n>=0 ) {
      this              = gParse.Nodes + n;
      this->operation   = Op;
      this->DoOp        = Do_Unary;
      this->nSubNodes   = 1;
      this->SubNodes[0] = Node1;
      this->type        = returnType;

      that              = gParse.Nodes + Node1; /* Reset in case .Nodes mv'd */
      this->value.nelem = that->value.nelem;
      this->value.naxis = that->value.naxis;
      for( i=0; i<that->value.naxis; i++ )
	 this->value.naxes[i] = that->value.naxes[i];

      if( that->operation==CONST_OP ) this->DoOp( this );
   }
   return( n );
}

static int New_BinOp( int returnType, int Node1, int Op, int Node2 )
{
   Node *this,*that1,*that2;
   int  n,i,constant;

   if( Node1<0 || Node2<0 ) return(-1);

   n = Alloc_Node();
   if( n>=0 ) {
      this             = gParse.Nodes + n;
      this->operation  = Op;
      this->nSubNodes  = 2;
      this->SubNodes[0]= Node1;
      this->SubNodes[1]= Node2;
      this->type       = returnType;

      that1            = gParse.Nodes + Node1;
      that2            = gParse.Nodes + Node2;
      constant         = (that1->operation==CONST_OP
                          && that2->operation==CONST_OP);
      if( that1->type!=STRING && that1->type!=BITSTR )
	 if( !Test_Dims( Node1, Node2 ) ) {
	    Free_Last_Node();
	    fferror("Array sizes/dims do not match for binary operator");
	    return(-1);
	 }
      if( that1->value.nelem == 1 ) that1 = that2;

      this->value.nelem = that1->value.nelem;
      this->value.naxis = that1->value.naxis;
      for( i=0; i<that1->value.naxis; i++ )
	 this->value.naxes[i] = that1->value.naxes[i];

	 /*  Both subnodes should be of same time  */
      switch( that1->type ) {
      case BITSTR:  this->DoOp = Do_BinOp_bit;  break;
      case STRING:  this->DoOp = Do_BinOp_str;  break;
      case BOOLEAN: this->DoOp = Do_BinOp_log;  break;
      case LONG:    this->DoOp = Do_BinOp_lng;  break;
      case DOUBLE:  this->DoOp = Do_BinOp_dbl;  break;
      }
      if( constant ) this->DoOp( this );
   }
   return( n );
}

static int New_Func( int returnType, funcOp Op, int nNodes,
		     int Node1, int Node2, int Node3, int Node4, 
		     int Node5, int Node6, int Node7 )
/* If returnType==0 , use Node1's type and vector sizes as returnType, */
/* else return a single value of type returnType                       */
{
   Node *this, *that;
   int  i,n,constant;

   if( Node1<0 || Node2<0 || Node3<0 || Node4<0 || 
       Node5<0 || Node6<0 || Node7<0 ) return(-1);

   n = Alloc_Node();
   if( n>=0 ) {
      this              = gParse.Nodes + n;
      this->operation   = (int)Op;
      this->DoOp        = Do_Func;
      this->nSubNodes   = nNodes;
      this->SubNodes[0] = Node1;
      this->SubNodes[1] = Node2;
      this->SubNodes[2] = Node3;
      this->SubNodes[3] = Node4;
      this->SubNodes[4] = Node5;
      this->SubNodes[5] = Node6;
      this->SubNodes[6] = Node7;
      i = constant = nNodes;    /* Functions with zero params are not const */
      while( i-- )
         constant = ( constant &&
		      gParse.Nodes[ this->SubNodes[i] ].operation==CONST_OP );
      
      if( returnType ) {
	 this->type           = returnType;
	 this->value.nelem    = 1;
	 this->value.naxis    = 1;
	 this->value.naxes[0] = 1;
      } else {
	 that              = gParse.Nodes + Node1;
	 this->type        = that->type;
	 this->value.nelem = that->value.nelem;
	 this->value.naxis = that->value.naxis;
	 for( i=0; i<that->value.naxis; i++ )
	    this->value.naxes[i] = that->value.naxes[i];
      }
      if( constant ) this->DoOp( this );
   }
   return( n );
}

static int New_Deref( int Var,  int nDim,
		      int Dim1, int Dim2, int Dim3, int Dim4, int Dim5 )
{
   int n, idx, constant;
   long elem=0;
   Node *this, *theVar, *theDim[MAXDIMS];

   if( Var<0 || Dim1<0 || Dim2<0 || Dim3<0 || Dim4<0 || Dim5<0 ) return(-1);

   theVar = gParse.Nodes + Var;
   if( theVar->operation==CONST_OP || theVar->value.nelem==1 ) {
      fferror("Cannot index a scalar value");
      return(-1);
   }

   n = Alloc_Node();
   if( n>=0 ) {
      this              = gParse.Nodes + n;
      this->nSubNodes   = nDim+1;
      theVar            = gParse.Nodes + (this->SubNodes[0]=Var);
      theDim[0]         = gParse.Nodes + (this->SubNodes[1]=Dim1);
      theDim[1]         = gParse.Nodes + (this->SubNodes[2]=Dim2);
      theDim[2]         = gParse.Nodes + (this->SubNodes[3]=Dim3);
      theDim[3]         = gParse.Nodes + (this->SubNodes[4]=Dim4);
      theDim[4]         = gParse.Nodes + (this->SubNodes[5]=Dim5);
      constant          = theVar->operation==CONST_OP;
      for( idx=0; idx<nDim; idx++ )
	 constant = (constant && theDim[idx]->operation==CONST_OP);

      for( idx=0; idx<nDim; idx++ )
	 if( theDim[idx]->value.nelem>1 ) {
	    Free_Last_Node();
	    fferror("Cannot use an array as an index value");
	    return(-1);
	 } else if( theDim[idx]->type!=LONG ) {
	    Free_Last_Node();
	    fferror("Index value must be an integer type");
	    return(-1);
	 }

      this->operation   = '[';
      this->DoOp        = Do_Deref;
      this->type        = theVar->type;

      if( theVar->value.naxis == nDim ) { /* All dimensions specified */
	 this->value.nelem    = 1;
	 this->value.naxis    = 1;
	 this->value.naxes[0] = 1;
      } else if( nDim==1 ) { /* Dereference only one dimension */
	 elem=1;
	 this->value.naxis = theVar->value.naxis-1;
	 for( idx=0; idx<this->value.naxis; idx++ ) {
	    elem *= ( this->value.naxes[idx] = theVar->value.naxes[idx] );
	 }
	 this->value.nelem = elem;
      } else {
	 Free_Last_Node();
	 fferror("Must specify just one or all indices for vector");
	 return(-1);
      }
      if( constant ) this->DoOp( this );
   }
   return(n);
}

extern int ffGetVariable( char *varName, FFSTYPE *varVal );

static int New_GTI( char *fname, int Node1, char *start, char *stop )
{
   fitsfile *fptr;
   Node *this, *that0, *that1;
   int  type,i,n, startCol, stopCol, Node0;
   int  hdutype, hdunum, evthdu, samefile, extvers, movetotype, tstat;
   char extname[100];
   long nrows;
   double timeZeroI[2], timeZeroF[2], dt, timeSpan;
   char xcol[20], xexpr[20];
   FFSTYPE colVal;

   if( Node1==-99 ) {
      type = ffGetVariable( "TIME", &colVal );
      if( type==COLUMN ) {
	 Node1 = New_Column( (int)colVal.lng );
      } else {
	 fferror("Could not build TIME column for GTIFILTER");
	 return(-1);
      }
   }
   Node1 = New_Unary( DOUBLE, 0, Node1 );
   Node0 = Alloc_Node(); /* This will hold the START/STOP times */
   if( Node1<0 || Node0<0 ) return(-1);

   /*  Record current HDU number in case we need to move within this file  */

   fptr = gParse.def_fptr;
   ffghdn( fptr, &evthdu );

   /*  Look for TIMEZERO keywords in current extension  */

   tstat = 0;
   if( ffgkyd( fptr, "TIMEZERO", timeZeroI, NULL, &tstat ) ) {
      tstat = 0;
      if( ffgkyd( fptr, "TIMEZERI", timeZeroI, NULL, &tstat ) ) {
	 timeZeroI[0] = timeZeroF[0] = 0.0;
      } else if( ffgkyd( fptr, "TIMEZERF", timeZeroF, NULL, &tstat ) ) {
	 timeZeroF[0] = 0.0;
      }
   } else {
      timeZeroF[0] = 0.0;
   }

   /*  Resolve filename parameter  */

   switch( fname[0] ) {
   case '\0':
      samefile = 1;
      hdunum = 1;
      break;
   case '[':
      samefile = 1;
      i = 1;
      while( fname[i] != '\0' && fname[i] != ']' ) i++;
      if( fname[i] ) {
	 fname[i] = '\0';
	 fname++;
	 ffexts( fname, &hdunum, extname, &extvers, &movetotype,
		 xcol, xexpr, &gParse.status );
         if( *extname ) {
	    ffmnhd( fptr, movetotype, extname, extvers, &gParse.status );
	    ffghdn( fptr, &hdunum );
	 } else if( hdunum ) {
	    ffmahd( fptr, ++hdunum, &hdutype, &gParse.status );
	 } else if( !gParse.status ) {
	    fferror("Cannot use primary array for GTI filter");
	    return( -1 );
	 }
      } else {
	 fferror("File extension specifier lacks closing ']'");
	 return( -1 );
      }
      break;
   case '+':
      samefile = 1;
      hdunum = atoi( fname ) + 1;
      if( hdunum>1 )
	 ffmahd( fptr, hdunum, &hdutype, &gParse.status );
      else {
	 fferror("Cannot use primary array for GTI filter");
	 return( -1 );
      }
      break;
   default:
      samefile = 0;
      if( ! ffopen( &fptr, fname, READONLY, &gParse.status ) )
	 ffghdn( fptr, &hdunum );
      break;
   }
   if( gParse.status ) return(-1);

   /*  If at primary, search for GTI extension  */

   if( hdunum==1 ) {
      while( 1 ) {
	 hdunum++;
	 if( ffmahd( fptr, hdunum, &hdutype, &gParse.status ) ) break;
	 if( hdutype==IMAGE_HDU ) continue;
	 tstat = 0;
	 if( ffgkys( fptr, "EXTNAME", extname, NULL, &tstat ) ) continue;
	 ffupch( extname );
	 if( strstr( extname, "GTI" ) ) break;
      }
      if( gParse.status ) {
	 if( gParse.status==END_OF_FILE )
	    fferror("GTI extension not found in this file");
	 return(-1);
      }
   }

   /*  Locate START/STOP Columns  */

   ffgcno( fptr, CASEINSEN, start, &startCol, &gParse.status );
   ffgcno( fptr, CASEINSEN, stop,  &stopCol,  &gParse.status );
   if( gParse.status ) return(-1);

   /*  Look for TIMEZERO keywords in GTI extension  */

   tstat = 0;
   if( ffgkyd( fptr, "TIMEZERO", timeZeroI+1, NULL, &tstat ) ) {
      tstat = 0;
      if( ffgkyd( fptr, "TIMEZERI", timeZeroI+1, NULL, &tstat ) ) {
	 timeZeroI[1] = timeZeroF[1] = 0.0;
      } else if( ffgkyd( fptr, "TIMEZERF", timeZeroF+1, NULL, &tstat ) ) {
	 timeZeroF[1] = 0.0;
      }
   } else {
      timeZeroF[1] = 0.0;
   }

   n = Alloc_Node();
   if( n >= 0 ) {
      this                 = gParse.Nodes + n;
      this->nSubNodes      = 2;
      this->SubNodes[1]    = Node1;
      this->operation      = (int)gtifilt_fct;
      this->DoOp           = Do_GTI;
      this->type           = BOOLEAN;
      that1                = gParse.Nodes + Node1;
      this->value.nelem    = that1->value.nelem;
      this->value.naxis    = that1->value.naxis;
      for( i=0; i < that1->value.naxis; i++ )
	 this->value.naxes[i] = that1->value.naxes[i];

      /* Init START/STOP node to be treated as a "constant" */

      this->SubNodes[0]    = Node0;
      that0                = gParse.Nodes + Node0;
      that0->operation     = CONST_OP;
      that0->DoOp          = NULL;
      that0->value.data.ptr= NULL;

      /*  Read in START/STOP times  */

      if( ffgkyj( fptr, "NAXIS2", &nrows, NULL, &gParse.status ) )
	 return(-1);
      that0->value.nelem = nrows;
      if( nrows ) {

	 that0->value.data.dblptr = (double*)malloc( 2*nrows*sizeof(double) );
	 if( !that0->value.data.dblptr ) {
	    gParse.status = MEMORY_ALLOCATION;
	    return(-1);
	 }
	 
	 ffgcvd( fptr, startCol, 1L, 1L, nrows, 0.0,
		 that0->value.data.dblptr, &i, &gParse.status );
	 ffgcvd( fptr, stopCol, 1L, 1L, nrows, 0.0,
		 that0->value.data.dblptr+nrows, &i, &gParse.status );
	 if( gParse.status ) {
	    free( that0->value.data.dblptr );
	    return(-1);
	 }

	 /*  Test for fully time-ordered GTI... both START && STOP  */

	 that0->type = 1; /*  Assume yes  */
	 i = nrows;
	 while( --i )
	    if(    that0->value.data.dblptr[i-1]
                   >= that0->value.data.dblptr[i]
		|| that0->value.data.dblptr[i-1+nrows]
		   >= that0->value.data.dblptr[i+nrows] ) {
	       that0->type = 0;
	       break;
	    }
	 
	 /*  Handle TIMEZERO offset, if any  */
	 
	 dt = (timeZeroI[1] - timeZeroI[0]) + (timeZeroF[1] - timeZeroF[0]);
	 timeSpan = that0->value.data.dblptr[nrows+nrows-1]
	    - that0->value.data.dblptr[0];
	 
	 if( fabs( dt / timeSpan ) > 1e-12 ) {
	    for( i=0; i<(nrows+nrows); i++ )
	       that0->value.data.dblptr[i] += dt;
	 }
      }
      if( gParse.Nodes[Node1].operation==CONST_OP )
	 this->DoOp( this );
   }

   if( samefile )
      ffmahd( fptr, evthdu, &hdutype, &gParse.status );
   else
      ffclos( fptr, &gParse.status );

   return( n );
}

static int New_REG( char *fname, int NodeX, int NodeY, char *colNames )
{
   Node *this, *that0;
   int  type, n, Node0;
   int  Xcol, Ycol, tstat;
   WCSdata wcs;
   SAORegion *Rgn;
   char *cX, *cY;
   FFSTYPE colVal;

   if( NodeX==-99 ) {
      type = ffGetVariable( "X", &colVal );
      if( type==COLUMN ) {
	 NodeX = New_Column( (int)colVal.lng );
      } else {
	 fferror("Could not build X column for REGFILTER");
	 return(-1);
      }
   }
   if( NodeY==-99 ) {
      type = ffGetVariable( "Y", &colVal );
      if( type==COLUMN ) {
	 NodeY = New_Column( (int)colVal.lng );
      } else {
	 fferror("Could not build Y column for REGFILTER");
	 return(-1);
      }
   }
   NodeX = New_Unary( DOUBLE, 0, NodeX );
   NodeY = New_Unary( DOUBLE, 0, NodeY );
   Node0 = Alloc_Node(); /* This will hold the Region Data */
   if( NodeX<0 || NodeY<0 || Node0<0 ) return(-1);

   n = Alloc_Node();
   if( n >= 0 ) {
      this                 = gParse.Nodes + n;
      this->nSubNodes      = 3;
      this->SubNodes[0]    = Node0;
      this->SubNodes[1]    = NodeX;
      this->SubNodes[2]    = NodeY;
      this->operation      = (int)regfilt_fct;
      this->DoOp           = Do_REG;
      this->type           = BOOLEAN;
      this->value.nelem    = 1;
      this->value.naxis    = 1;
      this->value.naxes[0] = 1;

      /* Init Region node to be treated as a "constant" */

      that0                = gParse.Nodes + Node0;
      that0->operation     = CONST_OP;
      that0->DoOp          = NULL;

      /*  Identify what columns to use for WCS information  */

      Xcol = Ycol = 0;
      if( *colNames ) {
	 /*  Use the column names in this string for WCS info  */
	 while( *colNames==' ' ) colNames++;
	 cX = cY = colNames;
	 while( *cY && *cY!=' ' && *cY!=',' ) cY++;
	 if( *cY )
	    *(cY++) = '\0';
	 while( *cY==' ' ) cY++;
	 if( !*cY ) {
	    fferror("Could not extract valid pair of column names from REGFILTER");
	    Free_Last_Node();
	    return( -1 );
	 }
	 fits_get_colnum( gParse.def_fptr, CASEINSEN, cX, &Xcol,
			  &gParse.status );
	 fits_get_colnum( gParse.def_fptr, CASEINSEN, cY, &Ycol,
			  &gParse.status );
	 if( gParse.status ) {
	    fferror("Could not locate columns indicated for WCS info");
	    Free_Last_Node();
	    return( -1 );
	 }

      } else {
	 /*  Try to find columns used in X/Y expressions  */
	 Xcol = Locate_Col( gParse.Nodes + NodeX );
	 Ycol = Locate_Col( gParse.Nodes + NodeY );
	 if( Xcol<0 || Ycol<0 ) {
	    fferror("Found multiple X/Y column references in REGFILTER");
	    Free_Last_Node();
	    return( -1 );
	 }
      }

      /*  Now, get the WCS info, if it exists, from the indicated columns  */
      wcs.exists = 0;
      if( Xcol>0 && Ycol>0 ) {
	 tstat = 0;
	 ffgtcs( gParse.def_fptr, Xcol, Ycol,
		 &wcs.xrefval, &wcs.yrefval,
		 &wcs.xrefpix, &wcs.yrefpix,
		 &wcs.xinc,    &wcs.yinc,
		 &wcs.rot,      wcs.type,
		 &tstat );
	 if( tstat==NO_WCS_KEY ) {
	    wcs.exists = 0;
	 } else if( tstat ) {
	    gParse.status = tstat;
	    Free_Last_Node();
	    return( -1 );
	 } else {
	    wcs.exists = 1;
	 }
      }

      /*  Read in Region file  */

      fits_read_rgnfile( fname, &wcs, &Rgn, &gParse.status );
      if( gParse.status ) {
	 Free_Last_Node();
	 return( -1 );
      }

      that0->value.data.ptr = Rgn;

      if( gParse.Nodes[NodeX].operation==CONST_OP
	  && gParse.Nodes[NodeY].operation==CONST_OP )
	 this->DoOp( this );
   }

   return( n );
}

static int New_Vector( int subNode )
{
   Node *this, *that;
   int n;

   n = Alloc_Node();
   if( n >= 0 ) {
      this              = gParse.Nodes + n;
      that              = gParse.Nodes + subNode;
      this->type        = that->type;
      this->nSubNodes   = 1;
      this->SubNodes[0] = subNode;
      this->operation   = '{';
      this->DoOp        = Do_Vector;
   }

   return( n );
}

static int Close_Vec( int vecNode )
{
   Node *this;
   int n, nelem=0;

   this = gParse.Nodes + vecNode;
   for( n=0; n < this->nSubNodes; n++ ) {
      if( TYPE( this->SubNodes[n] ) != this->type ) {
	 this->SubNodes[n] = New_Unary( this->type, 0, this->SubNodes[n] );
	 if( this->SubNodes[n]<0 ) return(-1);
      }
      nelem += SIZE(this->SubNodes[n]);
   }
   this->value.naxis    = 1;
   this->value.nelem    = nelem;
   this->value.naxes[0] = nelem;

   return( vecNode );
}

static int Locate_Col( Node *this )
/*  Locate the TABLE column number of any columns in "this" calculation.  */
/*  Return ZERO if none found, or negative if more than 1 found.          */
{
   Node *that;
   int  i, col=0, newCol, nfound=0;
   
   if( this->nSubNodes==0
       && this->operation<=0 && this->operation!=CONST_OP )
      return gParse.colData[ - this->operation].colnum;

   for( i=0; i<this->nSubNodes; i++ ) {
      that = gParse.Nodes + this->SubNodes[i];
      if( that->operation>0 ) {
	 newCol = Locate_Col( that );
	 if( newCol<=0 ) {
	    nfound += -newCol;
	 } else {
	    if( !nfound ) {
	       col = newCol;
	       nfound++;
	    } else if( col != newCol ) {
	       nfound++;
	    }
	 }
      } else if( that->operation!=CONST_OP ) {
	 /*  Found a Column  */
	 newCol = gParse.colData[- that->operation].colnum;
	 if( !nfound ) {
	    col = newCol;
	    nfound++;
	 } else if( col != newCol ) {
	    nfound++;
	 }
      }
   }
   if( nfound!=1 )
      return( - nfound );
   else
      return( col );
}

static int Test_Dims( int Node1, int Node2 )
{
   Node *that1, *that2;
   int valid, i;

   if( Node1<0 || Node2<0 ) return(0);

   that1 = gParse.Nodes + Node1;
   that2 = gParse.Nodes + Node2;

   if( that1->value.nelem==1 || that2->value.nelem==1 )
      valid = 1;
   else if( that1->type==that2->type
	    && that1->value.nelem==that2->value.nelem
	    && that1->value.naxis==that2->value.naxis ) {
      valid = 1;
      for( i=0; i<that1->value.naxis; i++ ) {
	 if( that1->value.naxes[i]!=that2->value.naxes[i] )
	    valid = 0;
      }
   } else
      valid = 0;
   return( valid );
}   

static void Copy_Dims( int Node1, int Node2 )
{
   Node *that1, *that2;
   int i;

   if( Node1<0 || Node2<0 ) return;

   that1 = gParse.Nodes + Node1;
   that2 = gParse.Nodes + Node2;

   that1->value.nelem = that2->value.nelem;
   that1->value.naxis = that2->value.naxis;
   for( i=0; i<that2->value.naxis; i++ )
      that1->value.naxes[i] = that2->value.naxes[i];
}

/********************************************************************/
/*    Routines for actually evaluating the expression start here    */
/********************************************************************/

void Evaluate_Parser( long firstRow, long nRows )
    /***********************************************************************/
    /*  Reset the parser for processing another batch of data...           */
    /*    firstRow:  Row number of the first element to evaluate           */
    /*    nRows:     Number of rows to be processed                        */
    /*  Initialize each COLUMN node so that its UNDEF and DATA pointers    */
    /*  point to the appropriate column arrays.                            */
    /*  Finally, call Evaluate_Node for final node.                        */
    /***********************************************************************/
{
   int     i, column;
   long    offset, rowOffset;

   gParse.firstRow = firstRow;
   gParse.nRows    = nRows;

   /*  Reset Column Nodes' pointers to point to right data and UNDEF arrays  */

   rowOffset = firstRow - gParse.firstDataRow;
   for( i=0; i<gParse.nNodes; i++ ) {
      if(    gParse.Nodes[i].operation >  0
	  || gParse.Nodes[i].operation == CONST_OP ) continue;

      column = -gParse.Nodes[i].operation;
      offset = gParse.varData[column].nelem * rowOffset;

      gParse.Nodes[i].value.undef = gParse.varData[column].undef + offset;

      switch( gParse.Nodes[i].type ) {
      case BITSTR:
	 gParse.Nodes[i].value.data.strptr =
	    (char**)gParse.varData[column].data + rowOffset;
	 gParse.Nodes[i].value.undef       = NULL;
	 break;
      case STRING:
	 gParse.Nodes[i].value.data.strptr = 
	    (char**)gParse.varData[column].data + rowOffset;
	 gParse.Nodes[i].value.undef = gParse.varData[column].undef + rowOffset;
	 break;
      case BOOLEAN:
	 gParse.Nodes[i].value.data.logptr = 
	    (char*)gParse.varData[column].data + offset;
	 break;
      case LONG:
	 gParse.Nodes[i].value.data.lngptr = 
	    (long*)gParse.varData[column].data + offset;
	 break;
      case DOUBLE:
	 gParse.Nodes[i].value.data.dblptr = 
	    (double*)gParse.varData[column].data + offset;
	 break;
      }
   }

   Evaluate_Node( gParse.resultNode );
}

static void Evaluate_Node( int thisNode )
    /**********************************************************************/
    /*  Recursively evaluate thisNode's subNodes, then call one of the    */
    /*  Do_<Action> functions pointed to by thisNode's DoOp element.      */
    /**********************************************************************/
{
   Node *this;
   int i;
   
   if( gParse.status ) return;

   this = gParse.Nodes + thisNode;
   if( this->operation>0 ) {  /* <=0 indicate constants and columns */
      i = this->nSubNodes;
      while( i-- ) {
	 Evaluate_Node( this->SubNodes[i] );
	 if( gParse.status ) return;
      }
      this->DoOp( this );
   }
}

static void Allocate_Ptrs( Node *this )
{
   long elem, row, size;

   if( this->type==BITSTR || this->type==STRING ) {

      this->value.data.strptr = (char**)malloc( gParse.nRows
						* sizeof(char*) );
      if( this->value.data.strptr ) {
	 this->value.data.strptr[0] = (char*)malloc( gParse.nRows
						     * (this->value.nelem+2)
						     * sizeof(char) );
	 if( this->value.data.strptr[0] ) {
	    row = 0;
	    while( (++row)<gParse.nRows ) {
	       this->value.data.strptr[row] =
		  this->value.data.strptr[row-1] + this->value.nelem+1;
	    }
	    if( this->type==STRING ) {
	       this->value.undef = this->value.data.strptr[row-1]
                                   + this->value.nelem+1;
	    } else {
	       this->value.undef = NULL;  /* BITSTRs don't use undef array */
	    }
	 } else {
	    gParse.status = MEMORY_ALLOCATION;
	    free( this->value.data.strptr );
	 }
      } else {
	 gParse.status = MEMORY_ALLOCATION;
      }

   } else {

      elem = this->value.nelem * gParse.nRows;
      switch( this->type ) {
      case DOUBLE:  size = sizeof( double ); break;
      case LONG:    size = sizeof( long   ); break;
      case BOOLEAN: size = sizeof( char   ); break;
      default:      size = 1;                break;
      }

      this->value.data.ptr = malloc( elem*(size+1) );
      
      if( this->value.data.ptr==NULL ) {
	 gParse.status = MEMORY_ALLOCATION;
      } else {
	 this->value.undef = (char *)this->value.data.ptr + elem*size;
      }
   }
}

static void Do_Unary( Node *this )
{
   Node *that;
   long elem;

   that = gParse.Nodes + this->SubNodes[0];

   if( that->operation==CONST_OP ) {  /* Operating on a constant! */
      switch( this->operation ) {
      case DOUBLE:
      case FLTCAST:
	 if( that->type==LONG )
	    this->value.data.dbl = (double)that->value.data.lng;
	 else if( that->type==BOOLEAN )
	    this->value.data.dbl = ( that->value.data.log ? 1.0 : 0.0 );
	 break;
      case LONG:
      case INTCAST:
	 if( that->type==DOUBLE )
	    this->value.data.lng = (long)that->value.data.dbl;
	 else if( that->type==BOOLEAN )
	    this->value.data.lng = ( that->value.data.log ? 1L : 0L );
	 break;
      case BOOLEAN:
	 if( that->type==DOUBLE )
	    this->value.data.log = ( that->value.data.dbl != 0.0 );
	 else if( that->type==LONG )
	    this->value.data.log = ( that->value.data.lng != 0L );
	 break;
      case UMINUS:
	 if( that->type==DOUBLE )
	    this->value.data.dbl = - that->value.data.dbl;
	 else if( that->type==LONG )
	    this->value.data.lng = - that->value.data.lng;
	 break;
      case NOT:
	 if( that->type==BOOLEAN )
	    this->value.data.log = ( ! that->value.data.log );
	 else if( that->type==BITSTR )
	    bitnot( this->value.data.str, that->value.data.str );
	 break;
      }
      this->operation = CONST_OP;

   } else {

      Allocate_Ptrs( this );

      if( !gParse.status ) {

	 if( this->type!=BITSTR ) {
	    elem = gParse.nRows;
	    if( this->type!=STRING )
	       elem *= this->value.nelem;
	    while( elem-- )
	       this->value.undef[elem] = that->value.undef[elem];
	 }

	 elem = gParse.nRows * this->value.nelem;

	 switch( this->operation ) {

	 case BOOLEAN:
	    if( that->type==DOUBLE )
	       while( elem-- )
		  this->value.data.logptr[elem] =
		     ( that->value.data.dblptr[elem] != 0.0 );
	    else if( that->type==LONG )
	       while( elem-- )
		  this->value.data.logptr[elem] =
		     ( that->value.data.lngptr[elem] != 0L );
	    break;

	 case DOUBLE:
	 case FLTCAST:
	    if( that->type==LONG )
	       while( elem-- )
		  this->value.data.dblptr[elem] =
		     (double)that->value.data.lngptr[elem];
	    else if( that->type==BOOLEAN )
	       while( elem-- )
		  this->value.data.dblptr[elem] =
		     ( that->value.data.logptr[elem] ? 1.0 : 0.0 );
	    break;

	 case LONG:
	 case INTCAST:
	    if( that->type==DOUBLE )
	       while( elem-- )
		  this->value.data.lngptr[elem] =
		     (long)that->value.data.dblptr[elem];
	    else if( that->type==BOOLEAN )
	       while( elem-- )
		  this->value.data.lngptr[elem] =
		     ( that->value.data.logptr[elem] ? 1L : 0L );
	    break;

	 case UMINUS:
	    if( that->type==DOUBLE ) {
	       while( elem-- )
		  this->value.data.dblptr[elem] =
		     - that->value.data.dblptr[elem];
	    } else if( that->type==LONG ) {
	       while( elem-- )
		  this->value.data.lngptr[elem] =
		     - that->value.data.lngptr[elem];
	    }
	    break;

	 case NOT:
	    if( that->type==BOOLEAN ) {
	       while( elem-- )
		  this->value.data.logptr[elem] =
		     ( ! that->value.data.logptr[elem] );
	    } else if( that->type==BITSTR ) {
	       elem = gParse.nRows;
	       while( elem-- )
		  bitnot( this->value.data.strptr[elem],
			  that->value.data.strptr[elem] );
	    }
	    break;
	 }
      }
   }

   if( that->operation>0 ) {
      free( that->value.data.ptr );
   }
}

static void Do_Offset( Node *this )
{
   Node *col;
   long fRow, nRowOverlap, nRowReload, rowOffset;
   long nelem, elem, offset, nRealElem;
   int status;

   col       = gParse.Nodes + this->SubNodes[0];
   rowOffset = gParse.Nodes[  this->SubNodes[1] ].value.data.lng;

   Allocate_Ptrs( this );

   fRow   = gParse.firstRow + rowOffset;
   if( this->type==STRING || this->type==BITSTR )
      nRealElem = 1;
   else
      nRealElem = this->value.nelem;

   nelem = nRealElem;

   if( fRow < gParse.firstDataRow ) {

      /* Must fill in data at start of array */

      nRowReload = gParse.firstDataRow - fRow;
      if( nRowReload > gParse.nRows ) nRowReload = gParse.nRows;
      nRowOverlap = gParse.nRows - nRowReload;

      offset = 0;

      /*  NULLify any values falling out of bounds  */

      while( fRow<1 && nRowReload>0 ) {
	 if( this->type == BITSTR ) {
	    nelem = this->value.nelem;
	    this->value.data.strptr[offset][ nelem ] = '\0';
	    while( nelem-- ) this->value.data.strptr[offset][nelem] = '0';
	    offset++;
	 } else {
	    while( nelem-- )
	       this->value.undef[offset++] = 1;
	 }
	 nelem = nRealElem;
	 fRow++;
	 nRowReload--;
      }

   } else if( fRow + gParse.nRows > gParse.firstDataRow + gParse.nDataRows ) {

      /* Must fill in data at end of array */

      nRowReload = (fRow+gParse.nRows) - (gParse.firstDataRow+gParse.nDataRows);
      if( nRowReload>gParse.nRows ) {
	 nRowReload = gParse.nRows;
      } else {
	 fRow = gParse.firstDataRow + gParse.nDataRows;
      }
      nRowOverlap = gParse.nRows - nRowReload;

      offset = nRowOverlap * nelem;

      /*  NULLify any values falling out of bounds  */

      elem = gParse.nRows * nelem;
      while( fRow+nRowReload>gParse.totalRows && nRowReload>0 ) {
	 if( this->type == BITSTR ) {
	    nelem = this->value.nelem;
	    elem--;
	    this->value.data.strptr[elem][ nelem ] = '\0';
	    while( nelem-- ) this->value.data.strptr[elem][nelem] = '0';
	 } else {
	    while( nelem-- )
	       this->value.undef[--elem] = 1;
	 }
	 nelem = nRealElem;
	 nRowReload--;
      }

   } else {

      nRowReload  = 0;
      nRowOverlap = gParse.nRows;
      offset      = 0;

   }

   if( nRowReload>0 ) {
      switch( this->type ) {
      case BITSTR:
      case STRING:
	 status = (*gParse.loadData)( -col->operation, fRow, nRowReload,
				      this->value.data.strptr+offset,
				      this->value.undef+offset );
	 break;
      case BOOLEAN:
	 status = (*gParse.loadData)( -col->operation, fRow, nRowReload,
				      this->value.data.logptr+offset,
				      this->value.undef+offset );
	 break;
      case LONG:
	 status = (*gParse.loadData)( -col->operation, fRow, nRowReload,
				      this->value.data.lngptr+offset,
				      this->value.undef+offset );
	 break;
      case DOUBLE:
	 status = (*gParse.loadData)( -col->operation, fRow, nRowReload,
				      this->value.data.dblptr+offset,
				      this->value.undef+offset );
	 break;
      }
   }

   /*  Now copy over the overlapping region, if any  */

   if( nRowOverlap <= 0 ) return;

   if( rowOffset>0 )
      elem = nRowOverlap * nelem;
   else
      elem = gParse.nRows * nelem;

   offset = nelem * rowOffset;
   while( nRowOverlap-- && !gParse.status ) {
      while( nelem-- && !gParse.status ) {
	 elem--;
	 if( this->type != BITSTR )
	    this->value.undef[elem] = col->value.undef[elem+offset];
	 switch( this->type ) {
	 case BITSTR:
	    strcpy( this->value.data.strptr[elem       ],
                     col->value.data.strptr[elem+offset] );
	    break;
	 case STRING:
	    strcpy( this->value.data.strptr[elem       ],
                     col->value.data.strptr[elem+offset] );
	    break;
	 case BOOLEAN:
	    this->value.data.logptr[elem] = col->value.data.logptr[elem+offset];
	    break;
	 case LONG:
	    this->value.data.lngptr[elem] = col->value.data.lngptr[elem+offset];
	    break;
	 case DOUBLE:
	    this->value.data.dblptr[elem] = col->value.data.dblptr[elem+offset];
	    break;
	 }
      }
      nelem = nRealElem;
   }
}

static void Do_BinOp_bit( Node *this )
{
   Node *that1, *that2;
   char *sptr1=NULL, *sptr2=NULL;
   int  const1, const2;
   long rows;

   that1 = gParse.Nodes + this->SubNodes[0];
   that2 = gParse.Nodes + this->SubNodes[1];

   const1 = ( that1->operation==CONST_OP );
   const2 = ( that2->operation==CONST_OP );
   sptr1  = ( const1 ? that1->value.data.str : NULL );
   sptr2  = ( const2 ? that2->value.data.str : NULL );

   if( const1 && const2 ) {
      switch( this->operation ) {
      case NE:
	 this->value.data.log = !bitcmp( sptr1, sptr2 );
	 break;
      case EQ:
	 this->value.data.log =  bitcmp( sptr1, sptr2 );
	 break;
      case GT:
      case LT:
      case LTE:
      case GTE:
	 this->value.data.log = bitlgte( sptr1, this->operation, sptr2 );
	 break;
      case '|': 
	 bitor( this->value.data.str, sptr1, sptr2 );
	 break;
      case '&': 
	 bitand( this->value.data.str, sptr1, sptr2 );
	 break;
      case '+':
	 strcpy( this->value.data.str, sptr1 );
	 strcat( this->value.data.str, sptr2 );
	 break;
      }
      this->operation = CONST_OP;

   } else {

      Allocate_Ptrs( this );

      if( !gParse.status ) {
	 rows  = gParse.nRows;
	 switch( this->operation ) {

	    /*  BITSTR comparisons  */

	 case NE:
	 case EQ:
	 case GT:
	 case LT:
	 case LTE:
	 case GTE:
	    while( rows-- ) {
	       if( !const1 )
		  sptr1 = that1->value.data.strptr[rows];
	       if( !const2 )
		  sptr2 = that2->value.data.strptr[rows];
	       switch( this->operation ) {
	       case NE:  this->value.data.logptr[rows] = 
                                                      !bitcmp( sptr1, sptr2 );
                         break;
	       case EQ:  this->value.data.logptr[rows] = 
                                                       bitcmp( sptr1, sptr2 );
                         break;
	       case GT:
	       case LT:
	       case LTE:
	       case GTE: this->value.data.logptr[rows] = 
                                     bitlgte( sptr1, this->operation, sptr2 );
	                 break;
	       }
	       this->value.undef[rows] = 0;
	    }
	    break;
	 
	    /*  BITSTR AND/ORs ...  no UNDEFS in or out */
      
	 case '|': 
	 case '&': 
	 case '+':
	    while( rows-- ) {
	       if( !const1 )
		  sptr1 = that1->value.data.strptr[rows];
	       if( !const2 )
		  sptr2 = that2->value.data.strptr[rows];
	       if( this->operation=='|' )
		  bitor(  this->value.data.strptr[rows], sptr1, sptr2 );
	       else if( this->operation=='&' )
		  bitand( this->value.data.strptr[rows], sptr1, sptr2 );
	       else {
		  strcpy( this->value.data.strptr[rows], sptr1 );
		  strcat( this->value.data.strptr[rows], sptr2 );
	       }
	    }
	    break;
	 }
      }
   }

   if( that1->operation>0 ) {
      free( that1->value.data.strptr[0] );
      free( that1->value.data.strptr    );
   }
   if( that2->operation>0 ) {
      free( that2->value.data.strptr[0] );
      free( that2->value.data.strptr    );
   }
}

static void Do_BinOp_str( Node *this )
{
   Node *that1, *that2;
   char *sptr1, *sptr2, null1=0, null2=0;
   int const1, const2, val;
   long rows;

   that1 = gParse.Nodes + this->SubNodes[0];
   that2 = gParse.Nodes + this->SubNodes[1];

   const1 = ( that1->operation==CONST_OP );
   const2 = ( that2->operation==CONST_OP );
   sptr1  = ( const1 ? that1->value.data.str : NULL );
   sptr2  = ( const2 ? that2->value.data.str : NULL );

   if( const1 && const2 ) {  /*  Result is a constant  */
      switch( this->operation ) {

	 /*  Compare Strings  */

      case NE:
      case EQ:
	 val = ( FSTRCMP( sptr1, sptr2 ) == 0 );
	 this->value.data.log = ( this->operation==EQ ? val : !val );
	 break;
      case GT:
	 this->value.data.log = ( FSTRCMP( sptr1, sptr2 ) > 0 );
	 break;
      case LT:
	 this->value.data.log = ( FSTRCMP( sptr1, sptr2 ) < 0 );
	 break;
      case GTE:
	 this->value.data.log = ( FSTRCMP( sptr1, sptr2 ) >= 0 );
	 break;
      case LTE:
	 this->value.data.log = ( FSTRCMP( sptr1, sptr2 ) <= 0 );
	 break;

	 /*  Concat Strings  */

      case '+':
	 strcpy( this->value.data.str, sptr1 );
	 strcat( this->value.data.str, sptr2 );
	 break;
      }
      this->operation = CONST_OP;

   } else {  /*  Not a constant  */

      Allocate_Ptrs( this );

      if( !gParse.status ) {

	 rows = gParse.nRows;
	 switch( this->operation ) {

	    /*  Compare Strings  */

	 case NE:
	 case EQ:
	    while( rows-- ) {
	       if( !const1 ) null1 = that1->value.undef[rows];
	       if( !const2 ) null2 = that2->value.undef[rows];
	       this->value.undef[rows] = (null1 || null2);
	       if( ! this->value.undef[rows] ) {
		  if( !const1 ) sptr1  = that1->value.data.strptr[rows];
		  if( !const2 ) sptr2  = that2->value.data.strptr[rows];
		  val = ( FSTRCMP( sptr1, sptr2 ) == 0 );
		  this->value.data.logptr[rows] =
		     ( this->operation==EQ ? val : !val );
	       }
	    }
	    break;
	    
	 case GT:
	 case LT:
	    while( rows-- ) {
	       if( !const1 ) null1 = that1->value.undef[rows];
	       if( !const2 ) null2 = that2->value.undef[rows];
	       this->value.undef[rows] = (null1 || null2);
	       if( ! this->value.undef[rows] ) {
		  if( !const1 ) sptr1  = that1->value.data.strptr[rows];
		  if( !const2 ) sptr2  = that2->value.data.strptr[rows];
		  val = ( FSTRCMP( sptr1, sptr2 ) );
		  this->value.data.logptr[rows] =
		     ( this->operation==GT ? val>0 : val<0 );
	       }
	    }
	    break;

	 case GTE:
	 case LTE:
	    while( rows-- ) {
	       if( !const1 ) null1 = that1->value.undef[rows];
	       if( !const2 ) null2 = that2->value.undef[rows];
	       this->value.undef[rows] = (null1 || null2);
	       if( ! this->value.undef[rows] ) {
		  if( !const1 ) sptr1  = that1->value.data.strptr[rows];
		  if( !const2 ) sptr2  = that2->value.data.strptr[rows];
		  val = ( FSTRCMP( sptr1, sptr2 ) );
		  this->value.data.logptr[rows] =
		     ( this->operation==GTE ? val>=0 : val<=0 );
	       }
	    }
	    break;

	    /*  Concat Strings  */
	    
	 case '+':
	    while( rows-- ) {
	       if( !const1 ) null1 = that1->value.undef[rows];
	       if( !const2 ) null2 = that2->value.undef[rows];
	       this->value.undef[rows] = (null1 || null2);
	       if( ! this->value.undef[rows] ) {
		  if( !const1 ) sptr1  = that1->value.data.strptr[rows];
		  if( !const2 ) sptr2  = that2->value.data.strptr[rows];
		  strcpy( this->value.data.strptr[rows], sptr1 );
		  strcat( this->value.data.strptr[rows], sptr2 );
	       }
	    }
	    break;
	 }
      }
   }

   if( that1->operation>0 ) {
      free( that1->value.data.strptr[0] );
      free( that1->value.data.strptr );
   }
   if( that2->operation>0 ) {
      free( that2->value.data.strptr[0] );
      free( that2->value.data.strptr );
   }
}

static void Do_BinOp_log( Node *this )
{
   Node *that1, *that2;
   int vector1, vector2;
   char val1=0, val2=0, null1=0, null2=0;
   long rows, nelem, elem;

   that1 = gParse.Nodes + this->SubNodes[0];
   that2 = gParse.Nodes + this->SubNodes[1];

   vector1 = ( that1->operation!=CONST_OP );
   if( vector1 )
      vector1 = that1->value.nelem;
   else {
      val1  = that1->value.data.log;
   }

   vector2 = ( that2->operation!=CONST_OP );
   if( vector2 )
      vector2 = that2->value.nelem;
   else {
      val2  = that2->value.data.log;
   }

   if( !vector1 && !vector2 ) {  /*  Result is a constant  */
      switch( this->operation ) {
      case OR:
	 this->value.data.log = (val1 || val2);
	 break;
      case AND:
	 this->value.data.log = (val1 && val2);
	 break;
      case EQ:
	 this->value.data.log = ( (val1 && val2) || (!val1 && !val2) );
	 break;
      case NE:
	 this->value.data.log = ( (val1 && !val2) || (!val1 && val2) );
	 break;
      }
      this->operation=CONST_OP;
   } else {
      rows  = gParse.nRows;
      nelem = this->value.nelem;
      elem  = this->value.nelem * rows;

      Allocate_Ptrs( this );

      if( !gParse.status ) {
	 while( rows-- ) {
	    while( nelem-- ) {
	       elem--;

	       if( vector1>1 ) {
		  val1  = that1->value.data.logptr[elem];
		  null1 = that1->value.undef[elem];
	       } else if( vector1 ) {
		  val1  = that1->value.data.logptr[rows];
		  null1 = that1->value.undef[rows];
	       }

	       if( vector2>1 ) {
		  val2  = that2->value.data.logptr[elem];
		  null2 = that2->value.undef[elem];
	       } else if( vector2 ) {
		  val2  = that2->value.data.logptr[rows];
		  null2 = that2->value.undef[rows];
	       }

	       this->value.undef[elem] = (null1 || null2);
	       switch( this->operation ) {

	       case OR:
		  /*  This is more complicated than others to suppress UNDEFs */
		  /*  in those cases where the other argument is DEF && TRUE  */

		  if( !null1 && !null2 ) {
		     this->value.data.logptr[elem] = (val1 || val2);
		  } else if( (null1 && !null2 && val2)
			     || ( !null1 && null2 && val1 ) ) {
		     this->value.data.logptr[elem] = 1;
		     this->value.undef[elem] = 0;
		  }
		  break;

	       case AND:
		  /*  This is more complicated than others to suppress UNDEFs */
		  /*  in those cases where the other argument is DEF && FALSE */

		  if( !null1 && !null2 ) {
		     this->value.data.logptr[elem] = (val1 && val2);
		  } else if( (null1 && !null2 && !val2)
			     || ( !null1 && null2 && !val1 ) ) {
		     this->value.data.logptr[elem] = 0;
		     this->value.undef[elem] = 0;
		  }
		  break;

	       case EQ:
		  this->value.data.logptr[elem] = 
		     ( (val1 && val2) || (!val1 && !val2) );
		  break;

	       case NE:
		  this->value.data.logptr[elem] =
		     ( (val1 && !val2) || (!val1 && val2) );
		  break;
	       }
	    }
	    nelem = this->value.nelem;
	 }
      }
   }

   if( that1->operation>0 ) {
      free( that1->value.data.ptr );
   }
   if( that2->operation>0 ) {
      free( that2->value.data.ptr );
   }
}

static void Do_BinOp_lng( Node *this )
{
   Node *that1, *that2;
   int  vector1, vector2;
   long val1=0, val2=0;
   char null1=0, null2=0;
   long rows, nelem, elem;

   that1 = gParse.Nodes + this->SubNodes[0];
   that2 = gParse.Nodes + this->SubNodes[1];

   vector1 = ( that1->operation!=CONST_OP );
   if( vector1 )
      vector1 = that1->value.nelem;
   else {
      val1  = that1->value.data.lng;
   }

   vector2 = ( that2->operation!=CONST_OP );
   if( vector2 )
      vector2 = that2->value.nelem;
   else {
      val2  = that2->value.data.lng;
   }

   if( !vector1 && !vector2 ) {  /*  Result is a constant  */

      switch( this->operation ) {
      case '~':   /* Treat as == for LONGS */
      case EQ:    this->value.data.log = (val1 == val2);   break;
      case NE:    this->value.data.log = (val1 != val2);   break;
      case GT:    this->value.data.log = (val1 >  val2);   break;
      case LT:    this->value.data.log = (val1 <  val2);   break;
      case LTE:   this->value.data.log = (val1 <= val2);   break;
      case GTE:   this->value.data.log = (val1 >= val2);   break;

      case '+':   this->value.data.lng = (val1  + val2);   break;
      case '-':   this->value.data.lng = (val1  - val2);   break;
      case '*':   this->value.data.lng = (val1  * val2);   break;

      case '%':
	 if( val2 ) this->value.data.lng = (val1 % val2);
	 else       fferror("Divide by Zero");
	 break;
      case '/': 
	 if( val2 ) this->value.data.lng = (val1 / val2); 
	 else       fferror("Divide by Zero");
	 break;
      case POWER:
	 this->value.data.lng = (long)pow((double)val1,(double)val2);
	 break;
      }
      this->operation=CONST_OP;

   } else {

      rows  = gParse.nRows;
      nelem = this->value.nelem;
      elem  = this->value.nelem * rows;

      Allocate_Ptrs( this );

      while( rows-- && !gParse.status ) {
	 while( nelem-- && !gParse.status ) {
	    elem--;

	    if( vector1>1 ) {
	       val1  = that1->value.data.lngptr[elem];
	       null1 = that1->value.undef[elem];
	    } else if( vector1 ) {
	       val1  = that1->value.data.lngptr[rows];
	       null1 = that1->value.undef[rows];
	    }

	    if( vector2>1 ) {
	       val2  = that2->value.data.lngptr[elem];
	       null2 = that2->value.undef[elem];
	    } else if( vector2 ) {
	       val2  = that2->value.data.lngptr[rows];
	       null2 = that2->value.undef[rows];
	    }

	    this->value.undef[elem] = (null1 || null2);
	    switch( this->operation ) {
	    case '~':   /* Treat as == for LONGS */
	    case EQ:   this->value.data.logptr[elem] = (val1 == val2);   break;
	    case NE:   this->value.data.logptr[elem] = (val1 != val2);   break;
	    case GT:   this->value.data.logptr[elem] = (val1 >  val2);   break;
	    case LT:   this->value.data.logptr[elem] = (val1 <  val2);   break;
	    case LTE:  this->value.data.logptr[elem] = (val1 <= val2);   break;
	    case GTE:  this->value.data.logptr[elem] = (val1 >= val2);   break;
	       
	    case '+':  this->value.data.lngptr[elem] = (val1  + val2);   break;
	    case '-':  this->value.data.lngptr[elem] = (val1  - val2);   break;
	    case '*':  this->value.data.lngptr[elem] = (val1  * val2);   break;

	    case '%':   
	       if( val2 ) this->value.data.lngptr[elem] = (val1 % val2);
	       else {
		 this->value.data.lngptr[elem] = 0;
		 this->value.undef[elem] = 1;
	       }
	       break;
	    case '/': 
	       if( val2 ) this->value.data.lngptr[elem] = (val1 / val2); 
	       else {
		 this->value.data.lngptr[elem] = 0;
		 this->value.undef[elem] = 1;
	       }
	       break;
	    case POWER:
	       this->value.data.lngptr[elem] = (long)pow((double)val1,(double)val2);
	       break;
	    }
	 }
	 nelem = this->value.nelem;
      }
   }

   if( that1->operation>0 ) {
      free( that1->value.data.ptr );
   }
   if( that2->operation>0 ) {
      free( that2->value.data.ptr );
   }
}

static void Do_BinOp_dbl( Node *this )
{
   Node   *that1, *that2;
   int    vector1, vector2;
   double val1=0.0, val2=0.0;
   char   null1=0, null2=0;
   long   rows, nelem, elem;

   that1 = gParse.Nodes + this->SubNodes[0];
   that2 = gParse.Nodes + this->SubNodes[1];

   vector1 = ( that1->operation!=CONST_OP );
   if( vector1 )
      vector1 = that1->value.nelem;
   else {
      val1  = that1->value.data.dbl;
   }

   vector2 = ( that2->operation!=CONST_OP );
   if( vector2 )
      vector2 = that2->value.nelem;
   else {
      val2  = that2->value.data.dbl;
   } 

   if( !vector1 && !vector2 ) {  /*  Result is a constant  */

      switch( this->operation ) {
      case '~':   this->value.data.log = ( fabs(val1-val2) < APPROX );   break;
      case EQ:    this->value.data.log = (val1 == val2);   break;
      case NE:    this->value.data.log = (val1 != val2);   break;
      case GT:    this->value.data.log = (val1 >  val2);   break;
      case LT:    this->value.data.log = (val1 <  val2);   break;
      case LTE:   this->value.data.log = (val1 <= val2);   break;
      case GTE:   this->value.data.log = (val1 >= val2);   break;

      case '+':   this->value.data.dbl = (val1  + val2);   break;
      case '-':   this->value.data.dbl = (val1  - val2);   break;
      case '*':   this->value.data.dbl = (val1  * val2);   break;

      case '%':
	 if( val2 ) this->value.data.dbl = val1 - val2*((int)(val1/val2));
	 else       fferror("Divide by Zero");
	 break;
      case '/': 
	 if( val2 ) this->value.data.dbl = (val1 / val2); 
	 else       fferror("Divide by Zero");
	 break;
      case POWER:
	 this->value.data.dbl = (double)pow(val1,val2);
	 break;
      }
      this->operation=CONST_OP;

   } else {

      rows  = gParse.nRows;
      nelem = this->value.nelem;
      elem  = this->value.nelem * rows;

      Allocate_Ptrs( this );

      while( rows-- && !gParse.status ) {
	 while( nelem-- && !gParse.status ) {
	    elem--;

	    if( vector1>1 ) {
	       val1  = that1->value.data.dblptr[elem];
	       null1 = that1->value.undef[elem];
	    } else if( vector1 ) {
	       val1  = that1->value.data.dblptr[rows];
	       null1 = that1->value.undef[rows];
	    }

	    if( vector2>1 ) {
	       val2  = that2->value.data.dblptr[elem];
	       null2 = that2->value.undef[elem];
	    } else if( vector2 ) {
	       val2  = that2->value.data.dblptr[rows];
	       null2 = that2->value.undef[rows];
	    }

	    this->value.undef[elem] = (null1 || null2);
	    switch( this->operation ) {
	    case '~':   this->value.data.logptr[elem] =
                                          ( fabs(val1-val2) < APPROX );   break;
	    case EQ:    this->value.data.logptr[elem] = (val1 == val2);   break;
	    case NE:    this->value.data.logptr[elem] = (val1 != val2);   break;
	    case GT:    this->value.data.logptr[elem] = (val1 >  val2);   break;
	    case LT:    this->value.data.logptr[elem] = (val1 <  val2);   break;
	    case LTE:   this->value.data.logptr[elem] = (val1 <= val2);   break;
	    case GTE:   this->value.data.logptr[elem] = (val1 >= val2);   break;
	       
	    case '+':   this->value.data.dblptr[elem] = (val1  + val2);   break;
	    case '-':   this->value.data.dblptr[elem] = (val1  - val2);   break;
	    case '*':   this->value.data.dblptr[elem] = (val1  * val2);   break;

	    case '%':
	       if( val2 ) this->value.data.dblptr[elem] =
                                val1 - val2*((int)(val1/val2));
	       else {
		 this->value.data.dblptr[elem] = 0.0;
		 this->value.undef[elem] = 1;
	       }
	       break;
	    case '/': 
	       if( val2 ) this->value.data.dblptr[elem] = (val1 / val2); 
	       else {
		 this->value.data.dblptr[elem] = 0.0;
		 this->value.undef[elem] = 1;
	       }
	       break;
	    case POWER:
	       this->value.data.dblptr[elem] = (double)pow(val1,val2);
	       break;
	    }
	 }
	 nelem = this->value.nelem;
      }
   }

   if( that1->operation>0 ) {
      free( that1->value.data.ptr );
   }
   if( that2->operation>0 ) {
      free( that2->value.data.ptr );
   }
}

/*
 *  This Quickselect routine is based on the algorithm described in
 *  "Numerical recipes in C", Second Edition,
 *  Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 *  This code by Nicolas Devillard - 1998. Public domain.
 * http://ndevilla.free.fr/median/median/src/quickselect.c
 */

#define ELEM_SWAP(a,b) { register long t=(a);(a)=(b);(b)=t; }

/* 
 * qselect_median_lng - select the median value of a long array
 *
 * This routine selects the median value of the long integer array
 * arr[].  If there are an even number of elements, the "lower median"
 * is selected.
 *
 * The array arr[] is scrambled, so users must operate on a scratch
 * array if they wish the values to be preserved.
 *
 * long arr[] - array of values
 * int n - number of elements in arr
 *
 * RETURNS: the lower median value of arr[]
 *
 */
long qselect_median_lng(long arr[], int n)
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {

        if (high <= low) { /* One element only */
	  return arr[median];	  
	}

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
	    return arr[median];
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP

#define ELEM_SWAP(a,b) { register double t=(a);(a)=(b);(b)=t; }

/* 
 * qselect_median_dbl - select the median value of a double array
 *
 * This routine selects the median value of the double array
 * arr[].  If there are an even number of elements, the "lower median"
 * is selected.
 *
 * The array arr[] is scrambled, so users must operate on a scratch
 * array if they wish the values to be preserved.
 *
 * double arr[] - array of values
 * int n - number of elements in arr
 *
 * RETURNS: the lower median value of arr[]
 *
 */
double qselect_median_dbl(double arr[], int n)
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) { /* One element only */
            return arr[median] ;
	}

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP

static void Do_Func( Node *this )
{
   Node *theParams[MAXSUBS];
   int  vector[MAXSUBS], allConst;
   lval pVals[MAXSUBS];
   char pNull[MAXSUBS];
   long   ival;
   double dval;
   int  i, valInit;
   long row, elem, nelem;
   double rndVal;

   i = this->nSubNodes;
   allConst = 1;
   while( i-- ) {
      theParams[i] = gParse.Nodes + this->SubNodes[i];
      vector[i]   = ( theParams[i]->operation!=CONST_OP );
      if( vector[i] ) {
	 allConst = 0;
	 vector[i] = theParams[i]->value.nelem;
      } else {
	 if( theParams[i]->type==DOUBLE ) {
	    pVals[i].data.dbl = theParams[i]->value.data.dbl;
	 } else if( theParams[i]->type==LONG ) {
	    pVals[i].data.lng = theParams[i]->value.data.lng;
	 } else if( theParams[i]->type==BOOLEAN ) {
	    pVals[i].data.log = theParams[i]->value.data.log;
	 } else
	    strcpy(pVals[i].data.str, theParams[i]->value.data.str);
	 pNull[i] = 0;
      }
   }

   if( this->nSubNodes==0 ) allConst = 0; /* These do produce scalars */

   if( allConst ) {

      switch( this->operation ) {

	    /* Non-Trig single-argument functions */

	 case sum_fct:
	    if( theParams[0]->type==BOOLEAN )
	       this->value.data.lng = ( pVals[0].data.log ? 1 : 0 );
	    else if( theParams[0]->type==LONG )
	       this->value.data.lng = pVals[0].data.lng;
	    else if( theParams[0]->type==DOUBLE )
	       this->value.data.dbl = pVals[0].data.dbl;
	    else if( theParams[0]->type==BITSTR )
	      strcpy(this->value.data.str, pVals[0].data.str);
	    break;
         case average_fct:
	    if( theParams[0]->type==LONG )
	       this->value.data.dbl = pVals[0].data.lng;
	    else if( theParams[0]->type==DOUBLE )
	       this->value.data.dbl = pVals[0].data.dbl;
	    break;
         case stddev_fct:
	    this->value.data.dbl = 0;  /* Standard deviation of a constant = 0 */
	    break;
	 case median_fct:
	    if( theParams[0]->type==BOOLEAN )
	       this->value.data.lng = ( pVals[0].data.log ? 1 : 0 );
	    else if( theParams[0]->type==LONG )
	       this->value.data.lng = pVals[0].data.lng;
	    else
	       this->value.data.dbl = pVals[0].data.dbl;
	    break;
	 case abs_fct:
	    if( theParams[0]->type==DOUBLE ) {
	       dval = pVals[0].data.dbl;
	       this->value.data.dbl = (dval>0.0 ? dval : -dval);
	    } else {
	       ival = pVals[0].data.lng;
	       this->value.data.lng = (ival> 0  ? ival : -ival);
	    }
	    break;

            /* Special Null-Handling Functions */

         case nonnull_fct:
	    this->value.data.lng = 1; /* Constants are always 1-element and defined */
	    break;
         case isnull_fct:  /* Constants are always defined */
	    this->value.data.log = 0;
	    break;
         case defnull_fct:
	    if( this->type==BOOLEAN )
	       this->value.data.log = pVals[0].data.log;
            else if( this->type==LONG )
	       this->value.data.lng = pVals[0].data.lng;
            else if( this->type==DOUBLE )
	       this->value.data.dbl = pVals[0].data.dbl;
            else if( this->type==STRING )
	       strcpy(this->value.data.str,pVals[0].data.str);
	    break;

	    /* Math functions with 1 double argument */

	 case sin_fct:
	    this->value.data.dbl = sin( pVals[0].data.dbl );
	    break;
	 case cos_fct:
	    this->value.data.dbl = cos( pVals[0].data.dbl );
	    break;
	 case tan_fct:
	    this->value.data.dbl = tan( pVals[0].data.dbl );
	    break;
	 case asin_fct:
	    dval = pVals[0].data.dbl;
	    if( dval<-1.0 || dval>1.0 )
	       fferror("Out of range argument to arcsin");
	    else
	       this->value.data.dbl = asin( dval );
	    break;
	 case acos_fct:
	    dval = pVals[0].data.dbl;
	    if( dval<-1.0 || dval>1.0 )
	       fferror("Out of range argument to arccos");
	    else
	       this->value.data.dbl = acos( dval );
	    break;
	 case atan_fct:
	    this->value.data.dbl = atan( pVals[0].data.dbl );
	    break;
	 case sinh_fct:
	    this->value.data.dbl = sinh( pVals[0].data.dbl );
	    break;
	 case cosh_fct:
	    this->value.data.dbl = cosh( pVals[0].data.dbl );
	    break;
	 case tanh_fct:
	    this->value.data.dbl = tanh( pVals[0].data.dbl );
	    break;
	 case exp_fct:
	    this->value.data.dbl = exp( pVals[0].data.dbl );
	    break;
	 case log_fct:
	    dval = pVals[0].data.dbl;
	    if( dval<=0.0 )
	       fferror("Out of range argument to log");
	    else
	       this->value.data.dbl = log( dval );
	    break;
	 case log10_fct:
	    dval = pVals[0].data.dbl;
	    if( dval<=0.0 )
	       fferror("Out of range argument to log10");
	    else
	       this->value.data.dbl = log10( dval );
	    break;
	 case sqrt_fct:
	    dval = pVals[0].data.dbl;
	    if( dval<0.0 )
	       fferror("Out of range argument to sqrt");
	    else
	       this->value.data.dbl = sqrt( dval );
	    break;
	 case ceil_fct:
	    this->value.data.dbl = ceil( pVals[0].data.dbl );
	    break;
	 case floor_fct:
	    this->value.data.dbl = floor( pVals[0].data.dbl );
	    break;
	 case round_fct:
	    this->value.data.dbl = floor( pVals[0].data.dbl + 0.5 );
	    break;

	    /* Two-argument Trig Functions */

	 case atan2_fct:
	    this->value.data.dbl =
	       atan2( pVals[0].data.dbl, pVals[1].data.dbl );
	    break;

	    /*  Min/Max functions taking 1 or 2 arguments  */

         case min1_fct:
	    /* No constant vectors! */
	    if( this->type == DOUBLE )
	       this->value.data.dbl = pVals[0].data.dbl;
	    else if( this->type == LONG )
	       this->value.data.lng = pVals[0].data.lng;
	    else if( this->type == BITSTR )
	      strcpy(this->value.data.str, pVals[0].data.str);
	    break;
         case min2_fct:
	    if( this->type == DOUBLE )
	       this->value.data.dbl =
		  minvalue( pVals[0].data.dbl, pVals[1].data.dbl );
	    else if( this->type == LONG )
	       this->value.data.lng =
		  minvalue( pVals[0].data.lng, pVals[1].data.lng );
	    break;
         case max1_fct:
	    /* No constant vectors! */
	    if( this->type == DOUBLE )
	       this->value.data.dbl = pVals[0].data.dbl;
	    else if( this->type == LONG )
	       this->value.data.lng = pVals[0].data.lng;
	    else if( this->type == BITSTR )
	      strcpy(this->value.data.str, pVals[0].data.str);
	    break;
         case max2_fct:
	    if( this->type == DOUBLE )
	       this->value.data.dbl =
		  maxvalue( pVals[0].data.dbl, pVals[1].data.dbl );
	    else if( this->type == LONG )
	       this->value.data.lng =
		  maxvalue( pVals[0].data.lng, pVals[1].data.lng );
	    break;

	    /* Boolean SAO region Functions... all arguments scalar dbls */

	 case near_fct:
	    this->value.data.log = bnear( pVals[0].data.dbl, pVals[1].data.dbl,
					  pVals[2].data.dbl );
	    break;
	 case circle_fct:
	    this->value.data.log = circle( pVals[0].data.dbl, pVals[1].data.dbl,
					   pVals[2].data.dbl, pVals[3].data.dbl,
					   pVals[4].data.dbl );
	    break;
	 case box_fct:
	    this->value.data.log = saobox( pVals[0].data.dbl, pVals[1].data.dbl,
					   pVals[2].data.dbl, pVals[3].data.dbl,
					   pVals[4].data.dbl, pVals[5].data.dbl,
					   pVals[6].data.dbl );
	    break;
	 case elps_fct:
	    this->value.data.log =
                               ellipse( pVals[0].data.dbl, pVals[1].data.dbl,
					pVals[2].data.dbl, pVals[3].data.dbl,
					pVals[4].data.dbl, pVals[5].data.dbl,
					pVals[6].data.dbl );
	    break;

            /* C Conditional expression:  bool ? expr : expr */

         case ifthenelse_fct:
            switch( this->type ) {
            case BOOLEAN:
               this->value.data.log = ( pVals[2].data.log ?
                                        pVals[0].data.log : pVals[1].data.log );
               break;
            case LONG:
               this->value.data.lng = ( pVals[2].data.log ?
                                        pVals[0].data.lng : pVals[1].data.lng );
               break;
            case DOUBLE:
               this->value.data.dbl = ( pVals[2].data.log ?
                                        pVals[0].data.dbl : pVals[1].data.dbl );
               break;
            case STRING:
	       strcpy(this->value.data.str, ( pVals[2].data.log ?
                                              pVals[0].data.str :
                                              pVals[1].data.str ) );
               break;
            }
            break;

      }
      this->operation = CONST_OP;

   } else {

      Allocate_Ptrs( this );

      row  = gParse.nRows;
      elem = row * this->value.nelem;

      if( !gParse.status ) {
	 switch( this->operation ) {

	    /* Special functions with no arguments */

	 case row_fct:
	    while( row-- ) {
	       this->value.data.lngptr[row] = gParse.firstRow + row;
	       this->value.undef[row] = 0;
	    }
	    break;
	 case null_fct:
            if( this->type==LONG ) {
               while( row-- ) {
                  this->value.data.lngptr[row] = 0;
                  this->value.undef[row] = 1;
               }
            } else if( this->type==STRING ) {
               while( row-- ) {
                  this->value.data.strptr[row][0] = '\0';
                  this->value.undef[row] = 1;
               }
            }
	    break;
	 case rnd_fct:
	    if( rand()<32768 && rand()<32768 )
	       dval =      32768.0;
	    else
	       dval = 2147483648.0;
	    while( row-- ) {
               rndVal = (double)rand();
               while( rndVal > dval ) dval *= 2.0;
	       this->value.data.dblptr[row] = rndVal/dval;
	       this->value.undef[row] = 0;
	    }
	    break;

	    /* Non-Trig single-argument functions */
	    
	 case sum_fct:
	    elem = row * theParams[0]->value.nelem;
	    if( theParams[0]->type==BOOLEAN ) {
	       while( row-- ) {
		  this->value.data.lngptr[row] = 0;
		  /* Default is UNDEF until a defined value is found */
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( ! theParams[0]->value.undef[elem] ) {
		       this->value.data.lngptr[row] +=
			 ( theParams[0]->value.data.logptr[elem] ? 1 : 0 );
		       this->value.undef[row] = 0;
		     }
		  }
	       }
	    } else if( theParams[0]->type==LONG ) {
	       while( row-- ) {
		  this->value.data.lngptr[row] = 0;
		  /* Default is UNDEF until a defined value is found */
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( ! theParams[0]->value.undef[elem] ) {
		       this->value.data.lngptr[row] +=
			 theParams[0]->value.data.lngptr[elem];
		       this->value.undef[row] = 0;
		     }
		  }
	       }		  
	    } else if( theParams[0]->type==DOUBLE ){
	       while( row-- ) {
		  this->value.data.dblptr[row] = 0.0;
		  /* Default is UNDEF until a defined value is found */
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( ! theParams[0]->value.undef[elem] ) {
		       this->value.data.dblptr[row] +=
			 theParams[0]->value.data.dblptr[elem];
		       this->value.undef[row] = 0;
		     }
		  }
	       }		  
	    } else { /* BITSTR */
	       nelem = theParams[0]->value.nelem;
	       while( row-- ) {
		  char *sptr1 = theParams[0]->value.data.strptr[row];
		  this->value.data.lngptr[row] = 0;
		  this->value.undef[row] = 0;
		  while (*sptr1) {
		    if (*sptr1 == '1') this->value.data.lngptr[row] ++;
		    sptr1++;
		  }
	       }		  
	    }
	    break;

	 case average_fct:
	    elem = row * theParams[0]->value.nelem;
	    if( theParams[0]->type==LONG ) {
	       while( row-- ) {
		  int count = 0;
		  this->value.data.dblptr[row] = 0;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if (theParams[0]->value.undef[elem] == 0) {
		       this->value.data.dblptr[row] +=
			 theParams[0]->value.data.lngptr[elem];
		       count ++;
		     }
		  }
		  if (count == 0) {
		    this->value.undef[row] = 1;
		  } else {
		    this->value.undef[row] = 0;
		    this->value.data.dblptr[row] /= count;
		  }
	       }		  
	    } else if( theParams[0]->type==DOUBLE ){
	       while( row-- ) {
		  int count = 0;
		  this->value.data.dblptr[row] = 0;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if (theParams[0]->value.undef[elem] == 0) {
		       this->value.data.dblptr[row] +=
			 theParams[0]->value.data.dblptr[elem];
		       count ++;
		     }
		  }
		  if (count == 0) {
		    this->value.undef[row] = 1;
		  } else {
		    this->value.undef[row] = 0;
		    this->value.data.dblptr[row] /= count;
		  }
	       }		  
	    }
	    break;
	 case stddev_fct:
	    elem = row * theParams[0]->value.nelem;
	    if( theParams[0]->type==LONG ) {

	       /* Compute the mean value */
	       while( row-- ) {
		  int count = 0;
		  double sum = 0, sum2 = 0;

		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if (theParams[0]->value.undef[elem] == 0) {
		       sum += theParams[0]->value.data.lngptr[elem];
		       count ++;
		     }
		  }
		  if (count > 1) {
		    sum /= count;

		    /* Compute the sum of squared deviations */
		    nelem = theParams[0]->value.nelem;
		    elem += nelem;  /* Reset elem for second pass */
		    while( nelem-- ) {
		      elem--;
		      if (theParams[0]->value.undef[elem] == 0) {
			double dx = (theParams[0]->value.data.lngptr[elem] - sum);
			sum2 += (dx*dx);
		      }
		    }

		    sum2 /= (double)count-1;

		    this->value.undef[row] = 0;
		    this->value.data.dblptr[row] = sqrt(sum2);
		  } else {
		    this->value.undef[row] = 0;       /* STDDEV => 0 */
		    this->value.data.dblptr[row] = 0;
		  }
	       }
	    } else if( theParams[0]->type==DOUBLE ){

	       /* Compute the mean value */
	       while( row-- ) {
		  int count = 0;
		  double sum = 0, sum2 = 0;

		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if (theParams[0]->value.undef[elem] == 0) {
		       sum += theParams[0]->value.data.dblptr[elem];
		       count ++;
		     }
		  }
		  if (count > 1) {
		    sum /= count;

		    /* Compute the sum of squared deviations */
		    nelem = theParams[0]->value.nelem;
		    elem += nelem;  /* Reset elem for second pass */
		    while( nelem-- ) {
		      elem--;
		      if (theParams[0]->value.undef[elem] == 0) {
			double dx = (theParams[0]->value.data.dblptr[elem] - sum);
			sum2 += (dx*dx);
		      }
		    }

		    sum2 /= (double)count-1;

		    this->value.undef[row] = 0;
		    this->value.data.dblptr[row] = sqrt(sum2);
		  } else {
		    this->value.undef[row] = 0;       /* STDDEV => 0 */
		    this->value.data.dblptr[row] = 0;
		  }
	       }
	    }
	    break;

	 case median_fct:
	   elem = row * theParams[0]->value.nelem;
	   nelem = theParams[0]->value.nelem;
	   if( theParams[0]->type==LONG ) {
	       long *dptr = theParams[0]->value.data.lngptr;
	       char *uptr = theParams[0]->value.undef;
	       long *mptr = (long *) malloc(sizeof(long)*nelem);
	       int irow;

	       /* Allocate temporary storage for this row, since the
                  quickselect function will scramble the contents */
	       if (mptr == 0) {
		 fferror("Could not allocate temporary memory in median function");
		 free( this->value.data.ptr );
		 break;
	       }

	       for (irow=0; irow<row; irow++) {
		  long *p = mptr;
		  int nelem1 = nelem;
		  int count = 0;

		  while ( nelem1-- ) { 
		    if (*uptr == 0) {
		      *p++ = *dptr;   /* Only advance the dest pointer if we copied */
		    }
		    dptr ++;  /* Advance the source pointer ... */
		    uptr ++;  /* ... and source "undef" pointer */
		  }
		  
		  nelem1 = (p - mptr);  /* Number of accepted data points */
		  if (nelem1 > 0) {
		    this->value.undef[irow] = 0;
		    this->value.data.lngptr[irow] = qselect_median_lng(mptr, nelem1);
		  } else {
		    this->value.undef[irow] = 1;
		    this->value.data.lngptr[irow] = 0;
		  }
		    
	       }		  

	       free(mptr);
	    } else {
	       double *dptr = theParams[0]->value.data.dblptr;
	       char   *uptr = theParams[0]->value.undef;
	       double *mptr = (double *) malloc(sizeof(double)*nelem);
	       int irow;

	       /* Allocate temporary storage for this row, since the
                  quickselect function will scramble the contents */
	       if (mptr == 0) {
		 fferror("Could not allocate temporary memory in median function");
		 free( this->value.data.ptr );
		 break;
	       }

	       for (irow=0; irow<row; irow++) {
		  double *p = mptr;
		  int nelem1 = nelem;

		  while ( nelem1-- ) { 
		    if (*uptr == 0) {
		      *p++ = *dptr;   /* Only advance the dest pointer if we copied */
		    }
		    dptr ++;  /* Advance the source pointer ... */
		    uptr ++;  /* ... and source "undef" pointer */
		  }

		  nelem1 = (p - mptr);  /* Number of accepted data points */
		  if (nelem1 > 0) {
		    this->value.undef[irow] = 0;
		    this->value.data.dblptr[irow] = qselect_median_dbl(mptr, nelem1);
		  } else {
		    this->value.undef[irow] = 1;
		    this->value.data.dblptr[irow] = 0;
		  }

	       }
	       free(mptr);
	    }
	    break;
	 case abs_fct:
	    if( theParams[0]->type==DOUBLE )
	       while( elem-- ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  this->value.data.dblptr[elem] = (dval>0.0 ? dval : -dval);
		  this->value.undef[elem] = theParams[0]->value.undef[elem];
	       }
	    else
	       while( elem-- ) {
		  ival = theParams[0]->value.data.lngptr[elem];
		  this->value.data.lngptr[elem] = (ival> 0  ? ival : -ival);
		  this->value.undef[elem] = theParams[0]->value.undef[elem];
	       }
	    break;

            /* Special Null-Handling Functions */

	 case nonnull_fct:
	   nelem = theParams[0]->value.nelem;
	   if ( theParams[0]->type==STRING ) nelem = 1;
	   elem = row * nelem;
	   while( row-- ) {
	     int nelem1 = nelem;

	     this->value.undef[row] = 0;        /* Initialize to 0 (defined) */
	     this->value.data.lngptr[row] = 0;
	     while( nelem1-- ) {	
	       elem --;
	       if ( theParams[0]->value.undef[elem] == 0 ) this->value.data.lngptr[row] ++;
	     }
	   }
	   break;
	 case isnull_fct:
	    if( theParams[0]->type==STRING ) elem = row;
	    while( elem-- ) {
	       this->value.data.logptr[elem] = theParams[0]->value.undef[elem];
	       this->value.undef[elem] = 0;
	    }
	    break;
         case defnull_fct:
	    switch( this->type ) {
	    case BOOLEAN:
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pNull[i] = theParams[i]->value.undef[elem];
			   pVals[i].data.log =
			      theParams[i]->value.data.logptr[elem];
			} else if( vector[i] ) {
			   pNull[i] = theParams[i]->value.undef[row];
			   pVals[i].data.log =
			      theParams[i]->value.data.logptr[row];
			}
		     if( pNull[0] ) {
			this->value.undef[elem] = pNull[1];
			this->value.data.logptr[elem] = pVals[1].data.log;
		     } else {
			this->value.undef[elem] = 0;
			this->value.data.logptr[elem] = pVals[0].data.log;
		     }
		  }
	       }
	       break;
	    case LONG:
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pNull[i] = theParams[i]->value.undef[elem];
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[elem];
			} else if( vector[i] ) {
			   pNull[i] = theParams[i]->value.undef[row];
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[row];
			}
		     if( pNull[0] ) {
			this->value.undef[elem] = pNull[1];
			this->value.data.lngptr[elem] = pVals[1].data.lng;
		     } else {
			this->value.undef[elem] = 0;
			this->value.data.lngptr[elem] = pVals[0].data.lng;
		     }
		  }
	       }
	       break;
	    case DOUBLE:
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pNull[i] = theParams[i]->value.undef[elem];
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[elem];
			} else if( vector[i] ) {
			   pNull[i] = theParams[i]->value.undef[row];
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[row];
			}
		     if( pNull[0] ) {
			this->value.undef[elem] = pNull[1];
			this->value.data.dblptr[elem] = pVals[1].data.dbl;
		     } else {
			this->value.undef[elem] = 0;
			this->value.data.dblptr[elem] = pVals[0].data.dbl;
		     }
		  }
	       }
	       break;
	    case STRING:
	       while( row-- ) {
		  i=2; while( i-- )
		     if( vector[i] ) {
			pNull[i] = theParams[i]->value.undef[row];
			strcpy(pVals[i].data.str,
			       theParams[i]->value.data.strptr[row]);
		     }
		  if( pNull[0] ) {
		     this->value.undef[row] = pNull[1];
		     strcpy(this->value.data.strptr[row],pVals[1].data.str);
		  } else {
		     this->value.undef[elem] = 0;
		     strcpy(this->value.data.strptr[row],pVals[0].data.str);
		  }
	       }
	    }
	    break;

	    /* Math functions with 1 double argument */

	 case sin_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     sin( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case cos_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     cos( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case tan_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     tan( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case asin_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  if( dval<-1.0 || dval>1.0 ) {
		     this->value.data.dblptr[elem] = 0.0;
		     this->value.undef[elem] = 1;
		  } else
		     this->value.data.dblptr[elem] = asin( dval );
	       }
	    break;
	 case acos_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  if( dval<-1.0 || dval>1.0 ) {
		     this->value.data.dblptr[elem] = 0.0;
		     this->value.undef[elem] = 1;
		  } else
		     this->value.data.dblptr[elem] = acos( dval );
	       }
	    break;
	 case atan_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  this->value.data.dblptr[elem] = atan( dval );
	       }
	    break;
	 case sinh_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     sinh( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case cosh_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     cosh( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case tanh_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     tanh( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case exp_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  this->value.data.dblptr[elem] = exp( dval );
	       }
	    break;
	 case log_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  if( dval<=0.0 ) {
		     this->value.data.dblptr[elem] = 0.0;
		     this->value.undef[elem] = 1;
		  } else
		     this->value.data.dblptr[elem] = log( dval );
	       }
	    break;
	 case log10_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  if( dval<=0.0 ) {
		     this->value.data.dblptr[elem] = 0.0;
		     this->value.undef[elem] = 1;
		  } else
		     this->value.data.dblptr[elem] = log10( dval );
	       }
	    break;
	 case sqrt_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  dval = theParams[0]->value.data.dblptr[elem];
		  if( dval<0.0 ) {
		     this->value.data.dblptr[elem] = 0.0;
		     this->value.undef[elem] = 1;
		  } else
		     this->value.data.dblptr[elem] = sqrt( dval );
	       }
	    break;
	 case ceil_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     ceil( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case floor_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     floor( theParams[0]->value.data.dblptr[elem] );
	       }
	    break;
	 case round_fct:
	    while( elem-- )
	       if( !(this->value.undef[elem] = theParams[0]->value.undef[elem]) ) {
		  this->value.data.dblptr[elem] = 
		     floor( theParams[0]->value.data.dblptr[elem] + 0.5);
	       }
	    break;

	    /* Two-argument Trig Functions */
	    
	 case atan2_fct:
	    while( row-- ) {
	       nelem = this->value.nelem;
	       while( nelem-- ) {
		  elem--;
		  i=2; while( i-- )
		     if( vector[i]>1 ) {
			pVals[i].data.dbl =
			   theParams[i]->value.data.dblptr[elem];
			pNull[i] = theParams[i]->value.undef[elem];
		     } else if( vector[i] ) {
			pVals[i].data.dbl =
			   theParams[i]->value.data.dblptr[row];
			pNull[i] = theParams[i]->value.undef[row];
		     }
		  if( !(this->value.undef[elem] = (pNull[0] || pNull[1]) ) )
		     this->value.data.dblptr[elem] =
			atan2( pVals[0].data.dbl, pVals[1].data.dbl );
	       }
	    }
	    break;

	    /*  Min/Max functions taking 1 or 2 arguments  */

         case min1_fct:
	    elem = row * theParams[0]->value.nelem;
	    if( this->type==LONG ) {
	       long minVal=0;
	       while( row-- ) {
		  valInit = 1;
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( !theParams[0]->value.undef[elem] ) {
		       if ( valInit ) {
			 valInit = 0;
			 minVal  = theParams[0]->value.data.lngptr[elem];
		       } else {
			 minVal  = minvalue( minVal,
					     theParams[0]->value.data.lngptr[elem] );
		       }
		       this->value.undef[row] = 0;
		     }
		  }  
		  this->value.data.lngptr[row] = minVal;
	       }		  
	    } else if( this->type==DOUBLE ) {
	       double minVal=0.0;
	       while( row-- ) {
		  valInit = 1;
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( !theParams[0]->value.undef[elem] ) {
		       if ( valInit ) {
			 valInit = 0;
			 minVal  = theParams[0]->value.data.dblptr[elem];
		       } else {
			 minVal  = minvalue( minVal,
					     theParams[0]->value.data.dblptr[elem] );
		       }
		       this->value.undef[row] = 0;
		     }
		  }  
		  this->value.data.dblptr[row] = minVal;
	       }		  
	    } else if( this->type==BITSTR ) {
	       char minVal;
	       while( row-- ) {
		  char *sptr1 = theParams[0]->value.data.strptr[row];
		  minVal = '1';
		  while (*sptr1) {
		    if (*sptr1 == '0') minVal = '0';
		    sptr1++;
		  }
		  this->value.data.strptr[row][0] = minVal;
		  this->value.data.strptr[row][1] = 0;     /* Null terminate */
	       }		  
	    }
	    break;
         case min2_fct:
	    if( this->type==LONG ) {
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( pNull[0] && pNull[1] ) {
		       this->value.undef[elem] = 1;
		       this->value.data.lngptr[elem] = 0;
		     } else if (pNull[0]) {
		       this->value.undef[elem] = 0;
		       this->value.data.lngptr[elem] = pVals[1].data.lng;
		     } else if (pNull[1]) {
		       this->value.undef[elem] = 0;
		       this->value.data.lngptr[elem] = pVals[0].data.lng;
		     } else {
		       this->value.undef[elem] = 0;
		       this->value.data.lngptr[elem] =
			 minvalue( pVals[0].data.lng, pVals[1].data.lng );
		     }
		  }
	       }
	    } else if( this->type==DOUBLE ) {
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( pNull[0] && pNull[1] ) {
		       this->value.undef[elem] = 1;
		       this->value.data.dblptr[elem] = 0;
		     } else if (pNull[0]) {
		       this->value.undef[elem] = 0;
		       this->value.data.dblptr[elem] = pVals[1].data.dbl;
		     } else if (pNull[1]) {
		       this->value.undef[elem] = 0;
		       this->value.data.dblptr[elem] = pVals[0].data.dbl;
		     } else {
		       this->value.undef[elem] = 0;
		       this->value.data.dblptr[elem] =
			 minvalue( pVals[0].data.dbl, pVals[1].data.dbl );
		     }
		  }
 	       }
	    }
	    break;

         case max1_fct:
	    elem = row * theParams[0]->value.nelem;
	    if( this->type==LONG ) {
	       long maxVal=0;
	       while( row-- ) {
		  valInit = 1;
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( !theParams[0]->value.undef[elem] ) {
		       if ( valInit ) {
			 valInit = 0;
			 maxVal  = theParams[0]->value.data.lngptr[elem];
		       } else {
			 maxVal  = maxvalue( maxVal,
					     theParams[0]->value.data.lngptr[elem] );
		       }
		       this->value.undef[row] = 0;
		     }
		  }
		  this->value.data.lngptr[row] = maxVal;
	       }		  
	    } else if( this->type==DOUBLE ) {
	       double maxVal=0.0;
	       while( row-- ) {
		  valInit = 1;
		  this->value.undef[row] = 1;
		  nelem = theParams[0]->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     if ( !theParams[0]->value.undef[elem] ) {
		       if ( valInit ) {
			 valInit = 0;
			 maxVal  = theParams[0]->value.data.dblptr[elem];
		       } else {
			 maxVal  = maxvalue( maxVal,
					     theParams[0]->value.data.dblptr[elem] );
		       }
		       this->value.undef[row] = 0;
		     }
		  }
		  this->value.data.dblptr[row] = maxVal;
	       }		  
	    } else if( this->type==BITSTR ) {
	       char maxVal;
	       while( row-- ) {
		  char *sptr1 = theParams[0]->value.data.strptr[row];
		  maxVal = '0';
		  while (*sptr1) {
		    if (*sptr1 == '1') maxVal = '1';
		    sptr1++;
		  }
		  this->value.data.strptr[row][0] = maxVal;
		  this->value.data.strptr[row][1] = 0;     /* Null terminate */
	       }		  
	    }
	    break;
         case max2_fct:
	    if( this->type==LONG ) {
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( pNull[0] && pNull[1] ) {
		       this->value.undef[elem] = 1;
		       this->value.data.lngptr[elem] = 0;
		     } else if (pNull[0]) {
		       this->value.undef[elem] = 0;
		       this->value.data.lngptr[elem] = pVals[1].data.lng;
		     } else if (pNull[1]) {
		       this->value.undef[elem] = 0;
		       this->value.data.lngptr[elem] = pVals[0].data.lng;
		     } else {
		       this->value.undef[elem] = 0;
		       this->value.data.lngptr[elem] =
			 maxvalue( pVals[0].data.lng, pVals[1].data.lng );
		     }
		  }
	       }
	    } else if( this->type==DOUBLE ) {
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( pNull[0] && pNull[1] ) {
		       this->value.undef[elem] = 1;
		       this->value.data.dblptr[elem] = 0;
		     } else if (pNull[0]) {
		       this->value.undef[elem] = 0;
		       this->value.data.dblptr[elem] = pVals[1].data.dbl;
		     } else if (pNull[1]) {
		       this->value.undef[elem] = 0;
		       this->value.data.dblptr[elem] = pVals[0].data.dbl;
		     } else {
		       this->value.undef[elem] = 0;
		       this->value.data.dblptr[elem] =
			 maxvalue( pVals[0].data.dbl, pVals[1].data.dbl );
		     }
		  }
	       }
	    }
	    break;

	    /* Boolean SAO region Functions... all arguments scalar dbls */

	 case near_fct:
	    while( row-- ) {
	       this->value.undef[row] = 0;
	       i=3; while( i-- )
		  if( vector[i] ) {
		     pVals[i].data.dbl = theParams[i]->value.data.dblptr[row];
		     this->value.undef[row] |= theParams[i]->value.undef[row];
		  }
	       if( !(this->value.undef[row]) )
		  this->value.data.logptr[row] =
		     bnear( pVals[0].data.dbl, pVals[1].data.dbl,
			    pVals[2].data.dbl );
	    }
	    break;
	 case circle_fct:
	    while( row-- ) {
	       this->value.undef[row] = 0;
	       i=5; while( i-- )
		  if( vector[i] ) {
		     pVals[i].data.dbl = theParams[i]->value.data.dblptr[row];
		     this->value.undef[row] |= theParams[i]->value.undef[row];
		  }
	       if( !(this->value.undef[row]) )
		  this->value.data.logptr[row] =
		     circle( pVals[0].data.dbl, pVals[1].data.dbl,
			     pVals[2].data.dbl, pVals[3].data.dbl,
			     pVals[4].data.dbl );
	    }
	    break;
	 case box_fct:
	    while( row-- ) {
	       this->value.undef[row] = 0;
	       i=7; while( i-- )
		  if( vector[i] ) {
		     pVals[i].data.dbl = theParams[i]->value.data.dblptr[row];
		     this->value.undef[row] |= theParams[i]->value.undef[row];
		  }
	       if( !(this->value.undef[row]) )
		  this->value.data.logptr[row] =
		     saobox( pVals[0].data.dbl, pVals[1].data.dbl,
			     pVals[2].data.dbl, pVals[3].data.dbl,
			     pVals[4].data.dbl, pVals[5].data.dbl,
			     pVals[6].data.dbl );
	    }
	    break;
	 case elps_fct:
	    while( row-- ) {
	       this->value.undef[row] = 0;
	       i=7; while( i-- )
		  if( vector[i] ) {
		     pVals[i].data.dbl = theParams[i]->value.data.dblptr[row];
		     this->value.undef[row] |= theParams[i]->value.undef[row];
		  }
	       if( !(this->value.undef[row]) )
		  this->value.data.logptr[row] =
		     ellipse( pVals[0].data.dbl, pVals[1].data.dbl,
			      pVals[2].data.dbl, pVals[3].data.dbl,
			      pVals[4].data.dbl, pVals[5].data.dbl,
			      pVals[6].data.dbl );
	    }
	    break;

            /* C Conditional expression:  bool ? expr : expr */

         case ifthenelse_fct:
            switch( this->type ) {
            case BOOLEAN:
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
                     if( vector[2]>1 ) {
                        pVals[2].data.log =
                           theParams[2]->value.data.logptr[elem];
                        pNull[2] = theParams[2]->value.undef[elem];
                     } else if( vector[2] ) {
                        pVals[2].data.log =
                           theParams[2]->value.data.logptr[row];
                        pNull[2] = theParams[2]->value.undef[row];
                     }
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.log =
			      theParams[i]->value.data.logptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.log =
			      theParams[i]->value.data.logptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( !(this->value.undef[elem] = pNull[2]) ) {
                        if( pVals[2].data.log ) {
                           this->value.data.logptr[elem] = pVals[0].data.log;
                           this->value.undef[elem]       = pNull[0];
                        } else {
                           this->value.data.logptr[elem] = pVals[1].data.log;
                           this->value.undef[elem]       = pNull[1];
                        }
                     }
		  }
	       }
               break;
            case LONG:
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
                     if( vector[2]>1 ) {
                        pVals[2].data.log =
                           theParams[2]->value.data.logptr[elem];
                        pNull[2] = theParams[2]->value.undef[elem];
                     } else if( vector[2] ) {
                        pVals[2].data.log =
                           theParams[2]->value.data.logptr[row];
                        pNull[2] = theParams[2]->value.undef[row];
                     }
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.lng =
			      theParams[i]->value.data.lngptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( !(this->value.undef[elem] = pNull[2]) ) {
                        if( pVals[2].data.log ) {
                           this->value.data.lngptr[elem] = pVals[0].data.lng;
                           this->value.undef[elem]       = pNull[0];
                        } else {
                           this->value.data.lngptr[elem] = pVals[1].data.lng;
                           this->value.undef[elem]       = pNull[1];
                        }
                     }
		  }
	       }
               break;
            case DOUBLE:
	       while( row-- ) {
		  nelem = this->value.nelem;
		  while( nelem-- ) {
		     elem--;
                     if( vector[2]>1 ) {
                        pVals[2].data.log =
                           theParams[2]->value.data.logptr[elem];
                        pNull[2] = theParams[2]->value.undef[elem];
                     } else if( vector[2] ) {
                        pVals[2].data.log =
                           theParams[2]->value.data.logptr[row];
                        pNull[2] = theParams[2]->value.undef[row];
                     }
		     i=2; while( i-- )
			if( vector[i]>1 ) {
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[elem];
			   pNull[i] = theParams[i]->value.undef[elem];
			} else if( vector[i] ) {
			   pVals[i].data.dbl =
			      theParams[i]->value.data.dblptr[row];
			   pNull[i] = theParams[i]->value.undef[row];
			}
		     if( !(this->value.undef[elem] = pNull[2]) ) {
                        if( pVals[2].data.log ) {
                           this->value.data.dblptr[elem] = pVals[0].data.dbl;
                           this->value.undef[elem]       = pNull[0];
                        } else {
                           this->value.data.dblptr[elem] = pVals[1].data.dbl;
                           this->value.undef[elem]       = pNull[1];
                        }
                     }
		  }
	       }
               break;
            case STRING:
	       while( row-- ) {
                  if( vector[2] ) {
                     pVals[2].data.log = theParams[2]->value.data.logptr[row];
                     pNull[2] = theParams[2]->value.undef[row];
                  }
                  i=2; while( i-- )
                     if( vector[i] ) {
                        strcpy( pVals[i].data.str,
                                theParams[i]->value.data.strptr[row] );
                        pNull[i] = theParams[i]->value.undef[row];
                     }
                  if( !(this->value.undef[row] = pNull[2]) ) {
                     if( pVals[2].data.log ) {
                        strcpy( this->value.data.strptr[row],
                                pVals[0].data.str );
                        this->value.undef[row]       = pNull[0];
                     } else {
                        strcpy( this->value.data.strptr[row],
                                pVals[1].data.str );
                        this->value.undef[row]       = pNull[1];
                     }
                  } else {
                     this->value.data.strptr[row][0] = '\0';
                  }
	       }
               break;

            }
            break;

	 }
      }
   }

   i = this->nSubNodes;
   while( i-- ) {
      if( theParams[i]->operation>0 ) {
	 /*  Currently only numeric params allowed  */
	 free( theParams[i]->value.data.ptr );
      }
   }
}

static void Do_Deref( Node *this )
{
   Node *theVar, *theDims[MAXDIMS];
   int  isConst[MAXDIMS], allConst;
   long dimVals[MAXDIMS];
   int  i, nDims;
   long row, elem, dsize;

   theVar = gParse.Nodes + this->SubNodes[0];

   i = nDims = this->nSubNodes-1;
   allConst = 1;
   while( i-- ) {
      theDims[i] = gParse.Nodes + this->SubNodes[i+1];
      isConst[i] = ( theDims[i]->operation==CONST_OP );
      if( isConst[i] )
	 dimVals[i] = theDims[i]->value.data.lng;
      else
	 allConst = 0;
   }

   if( this->type==DOUBLE ) {
      dsize = sizeof( double );
   } else if( this->type==LONG ) {
      dsize = sizeof( long );
   } else if( this->type==BOOLEAN ) {
      dsize = sizeof( char );
   } else
      dsize = 0;

   Allocate_Ptrs( this );

   if( !gParse.status ) {

      if( allConst && theVar->value.naxis==nDims ) {

	 /* Dereference completely using constant indices */

	 elem = 0;
	 i    = nDims;
	 while( i-- ) {
	    if( dimVals[i]<1 || dimVals[i]>theVar->value.naxes[i] ) break;
	    elem = theVar->value.naxes[i]*elem + dimVals[i]-1;
	 }
	 if( i<0 ) {
	    for( row=0; row<gParse.nRows; row++ ) {
	       if( this->type==STRING )
		 this->value.undef[row] = theVar->value.undef[row];
	       else if( this->type==BITSTR ) 
		 this->value.undef;  /* Dummy - BITSTRs do not have undefs */
	       else 
		 this->value.undef[row] = theVar->value.undef[elem];

	       if( this->type==DOUBLE )
		  this->value.data.dblptr[row] = 
		     theVar->value.data.dblptr[elem];
	       else if( this->type==LONG )
		  this->value.data.lngptr[row] = 
		     theVar->value.data.lngptr[elem];
	       else if( this->type==BOOLEAN )
		  this->value.data.logptr[row] = 
		     theVar->value.data.logptr[elem];
	       else {
		 /* XXX Note, the below expression uses knowledge of
                    the layout of the string format, namely (nelem+1)
                    characters per string, followed by (nelem+1)
                    "undef" values. */
		  this->value.data.strptr[row][0] = 
		     theVar->value.data.strptr[0][elem+row];
		  this->value.data.strptr[row][1] = 0;  /* Null terminate */
	       }
	       elem += theVar->value.nelem;
	    }
	 } else {
	    fferror("Index out of range");
	    free( this->value.data.ptr );
	 }
	 
      } else if( allConst && nDims==1 ) {
	 
	 /* Reduce dimensions by 1, using a constant index */
	 
	 if( dimVals[0] < 1 ||
	     dimVals[0] > theVar->value.naxes[ theVar->value.naxis-1 ] ) {
	    fferror("Index out of range");
	    free( this->value.data.ptr );
	 } else if ( this->type == BITSTR || this->type == STRING ) {
	    elem = this->value.nelem * (dimVals[0]-1);
	    for( row=0; row<gParse.nRows; row++ ) {
	      if (this->value.undef) 
		this->value.undef[row] = theVar->value.undef[row];
	      memcpy( (char*)this->value.data.strptr[0]
		      + row*sizeof(char)*(this->value.nelem+1),
		      (char*)theVar->value.data.strptr[0] + elem*sizeof(char),
		      this->value.nelem * sizeof(char) );
	      /* Null terminate */
	      this->value.data.strptr[row][this->value.nelem] = 0;
	      elem += theVar->value.nelem+1;
	    }	       
	 } else {
	    elem = this->value.nelem * (dimVals[0]-1);
	    for( row=0; row<gParse.nRows; row++ ) {
	       memcpy( this->value.undef + row*this->value.nelem,
		       theVar->value.undef + elem,
		       this->value.nelem * sizeof(char) );
	       memcpy( (char*)this->value.data.ptr
		       + row*dsize*this->value.nelem,
		       (char*)theVar->value.data.ptr + elem*dsize,
		       this->value.nelem * dsize );
	       elem += theVar->value.nelem;
	    }	       
	 }
      
      } else if( theVar->value.naxis==nDims ) {

	 /* Dereference completely using an expression for the indices */

	 for( row=0; row<gParse.nRows; row++ ) {

	    for( i=0; i<nDims; i++ ) {
	       if( !isConst[i] ) {
		  if( theDims[i]->value.undef[row] ) {
		     fferror("Null encountered as vector index");
		     free( this->value.data.ptr );
		     break;
		  } else
		     dimVals[i] = theDims[i]->value.data.lngptr[row];
	       }
	    }
	    if( gParse.status ) break;

	    elem = 0;
	    i    = nDims;
	    while( i-- ) {
	       if( dimVals[i]<1 || dimVals[i]>theVar->value.naxes[i] ) break;
	       elem = theVar->value.naxes[i]*elem + dimVals[i]-1;
	    }
	    if( i<0 ) {
	       elem += row*theVar->value.nelem;

	       if( this->type==STRING )
		 this->value.undef[row] = theVar->value.undef[row];
	       else if( this->type==BITSTR ) 
		 this->value.undef;  /* Dummy - BITSTRs do not have undefs */
	       else 
		 this->value.undef[row] = theVar->value.undef[elem];

	       if( this->type==DOUBLE )
		  this->value.data.dblptr[row] = 
		     theVar->value.data.dblptr[elem];
	       else if( this->type==LONG )
		  this->value.data.lngptr[row] = 
		     theVar->value.data.lngptr[elem];
	       else if( this->type==BOOLEAN )
		  this->value.data.logptr[row] = 
		     theVar->value.data.logptr[elem];
	       else {
		 /* XXX Note, the below expression uses knowledge of
                    the layout of the string format, namely (nelem+1)
                    characters per string, followed by (nelem+1)
                    "undef" values. */
		  this->value.data.strptr[row][0] = 
		     theVar->value.data.strptr[0][elem+row];
		  this->value.data.strptr[row][1] = 0;  /* Null terminate */
	       }
	    } else {
	       fferror("Index out of range");
	       free( this->value.data.ptr );
	    }
	 }

      } else {

	 /* Reduce dimensions by 1, using a nonconstant expression */

	 for( row=0; row<gParse.nRows; row++ ) {

	    /* Index cannot be a constant */

	    if( theDims[0]->value.undef[row] ) {
	       fferror("Null encountered as vector index");
	       free( this->value.data.ptr );
	       break;
	    } else
	       dimVals[0] = theDims[0]->value.data.lngptr[row];

	    if( dimVals[0] < 1 ||
		dimVals[0] > theVar->value.naxes[ theVar->value.naxis-1 ] ) {
	       fferror("Index out of range");
	       free( this->value.data.ptr );
	    } else if ( this->type == BITSTR || this->type == STRING ) {
	      elem = this->value.nelem * (dimVals[0]-1);
	      elem += row*(theVar->value.nelem+1);
	      if (this->value.undef) 
		this->value.undef[row] = theVar->value.undef[row];
	      memcpy( (char*)this->value.data.strptr[0]
		      + row*sizeof(char)*(this->value.nelem+1),
		      (char*)theVar->value.data.strptr[0] + elem*sizeof(char),
		      this->value.nelem * sizeof(char) );
	      /* Null terminate */
	      this->value.data.strptr[row][this->value.nelem] = 0;
	    } else {
	       elem  = this->value.nelem * (dimVals[0]-1);
	       elem += row*theVar->value.nelem;
	       memcpy( this->value.undef + row*this->value.nelem,
		       theVar->value.undef + elem,
		       this->value.nelem * sizeof(char) );
	       memcpy( (char*)this->value.data.ptr
		       + row*dsize*this->value.nelem,
		       (char*)theVar->value.data.ptr + elem*dsize,
		       this->value.nelem * dsize );
	    }
	 }
      }
   }

   if( theVar->operation>0 ) {
     if (theVar->type == STRING || theVar->type == BITSTR) 
       free(theVar->value.data.strptr[0] );
     else 
       free( theVar->value.data.ptr );
   }
   for( i=0; i<nDims; i++ )
      if( theDims[i]->operation>0 ) {
	 free( theDims[i]->value.data.ptr );
      }
}

static void Do_GTI( Node *this )
{
   Node *theExpr, *theTimes;
   double *start, *stop, *times;
   long elem, nGTI, gti;
   int ordered;

   theTimes = gParse.Nodes + this->SubNodes[0];
   theExpr  = gParse.Nodes + this->SubNodes[1];

   nGTI    = theTimes->value.nelem;
   start   = theTimes->value.data.dblptr;
   stop    = theTimes->value.data.dblptr + nGTI;
   ordered = theTimes->type;

   if( theExpr->operation==CONST_OP ) {

      this->value.data.log = 
	 (Search_GTI( theExpr->value.data.dbl, nGTI, start, stop, ordered )>=0);
      this->operation      = CONST_OP;

   } else {

      Allocate_Ptrs( this );

      times = theExpr->value.data.dblptr;
      if( !gParse.status ) {

	 elem = gParse.nRows * this->value.nelem;
	 if( nGTI ) {
	    gti = -1;
	    while( elem-- ) {
	       if( (this->value.undef[elem] = theExpr->value.undef[elem]) )
		  continue;

            /*  Before searching entire GTI, check the GTI found last time  */
	       if( gti<0 || times[elem]<start[gti] || times[elem]>stop[gti] ) {
		  gti = Search_GTI( times[elem], nGTI, start, stop, ordered );
	       }
	       this->value.data.logptr[elem] = ( gti>=0 );
	    }
	 } else
	    while( elem-- ) {
	       this->value.data.logptr[elem] = 0;
	       this->value.undef[elem]       = 0;
	    }
      }
   }

   if( theExpr->operation>0 )
      free( theExpr->value.data.ptr );
}

static long Search_GTI( double evtTime, long nGTI, double *start,
			double *stop, int ordered )
{
   long gti, step;
                             
   if( ordered && nGTI>15 ) { /*  If time-ordered and lots of GTIs,   */
                              /*  use "FAST" Binary search algorithm  */
      if( evtTime>=start[0] && evtTime<=stop[nGTI-1] ) {
	 gti = step = (nGTI >> 1);
	 while(1) {
	    if( step>1L ) step >>= 1;
	    
	    if( evtTime>stop[gti] ) {
	       if( evtTime>=start[gti+1] )
		  gti += step;
	       else {
		  gti = -1L;
		  break;
	       }
	    } else if( evtTime<start[gti] ) {
	       if( evtTime<=stop[gti-1] )
		  gti -= step;
	       else {
		  gti = -1L;
		  break;
	       }
	    } else {
	       break;
	    }
	 }
      } else
	 gti = -1L;
      
   } else { /*  Use "SLOW" linear search  */
      gti = nGTI;
      while( gti-- )
	 if( evtTime>=start[gti] && evtTime<=stop[gti] )
	    break;
   }
   return( gti );
}

static void Do_REG( Node *this )
{
   Node *theRegion, *theX, *theY;
   double Xval=0.0, Yval=0.0;
   char   Xnull=0, Ynull=0;
   int    Xvector, Yvector;
   long   nelem, elem, rows;

   theRegion = gParse.Nodes + this->SubNodes[0];
   theX      = gParse.Nodes + this->SubNodes[1];
   theY      = gParse.Nodes + this->SubNodes[2];

   Xvector = ( theX->operation!=CONST_OP );
   if( Xvector )
      Xvector = theX->value.nelem;
   else {
      Xval  = theX->value.data.dbl;
   }

   Yvector = ( theY->operation!=CONST_OP );
   if( Yvector )
      Yvector = theY->value.nelem;
   else {
      Yval  = theY->value.data.dbl;
   } 

   if( !Xvector && !Yvector ) {

      this->value.data.log =
	 ( fits_in_region( Xval, Yval, (SAORegion *)theRegion->value.data.ptr )
	   != 0 );
      this->operation      = CONST_OP;

   } else {

      Allocate_Ptrs( this );

      if( !gParse.status ) {

	 rows  = gParse.nRows;
	 nelem = this->value.nelem;
	 elem  = rows*nelem;

	 while( rows-- ) {
	    while( nelem-- ) {
	       elem--;

	       if( Xvector>1 ) {
		  Xval  = theX->value.data.dblptr[elem];
		  Xnull = theX->value.undef[elem];
	       } else if( Xvector ) {
		  Xval  = theX->value.data.dblptr[rows];
		  Xnull = theX->value.undef[rows];
	       }

	       if( Yvector>1 ) {
		  Yval  = theY->value.data.dblptr[elem];
		  Ynull = theY->value.undef[elem];
	       } else if( Yvector ) {
		  Yval  = theY->value.data.dblptr[rows];
		  Ynull = theY->value.undef[rows];
	       }

	       this->value.undef[elem] = ( Xnull || Ynull );
	       if( this->value.undef[elem] )
		  continue;

	       this->value.data.logptr[elem] = 
		  ( fits_in_region( Xval, Yval,
				    (SAORegion *)theRegion->value.data.ptr )
		    != 0 );
	    }
	    nelem = this->value.nelem;
	 }
      }
   }

   if( theX->operation>0 )
      free( theX->value.data.ptr );
   if( theY->operation>0 )
      free( theY->value.data.ptr );
}

static void Do_Vector( Node *this )
{
   Node *that;
   long row, elem, idx, jdx, offset=0;
   int node;

   Allocate_Ptrs( this );

   if( !gParse.status ) {

      for( node=0; node<this->nSubNodes; node++ ) {

	 that = gParse.Nodes + this->SubNodes[node];

	 if( that->operation == CONST_OP ) {

	    idx = gParse.nRows*this->value.nelem + offset;
	    while( (idx-=this->value.nelem)>=0 ) {
	       
	       this->value.undef[idx] = 0;

	       switch( this->type ) {
	       case BOOLEAN:
		  this->value.data.logptr[idx] = that->value.data.log;
		  break;
	       case LONG:
		  this->value.data.lngptr[idx] = that->value.data.lng;
		  break;
	       case DOUBLE:
		  this->value.data.dblptr[idx] = that->value.data.dbl;
		  break;
	       }
	    }
	    
	 } else {
	       
	    row  = gParse.nRows;
	    idx  = row * that->value.nelem;
	    while( row-- ) {
	       elem = that->value.nelem;
	       jdx = row*this->value.nelem + offset;
	       while( elem-- ) {
		  this->value.undef[jdx+elem] =
		     that->value.undef[--idx];

		  switch( this->type ) {
		  case BOOLEAN:
		     this->value.data.logptr[jdx+elem] =
			that->value.data.logptr[idx];
		     break;
		  case LONG:
		     this->value.data.lngptr[jdx+elem] =
			that->value.data.lngptr[idx];
		     break;
		  case DOUBLE:
		     this->value.data.dblptr[jdx+elem] =
			that->value.data.dblptr[idx];
		     break;
		  }
	       }
	    }
	 }
	 offset += that->value.nelem;
      }

   }

   for( node=0; node < this->nSubNodes; node++ )
      if( gParse.Nodes[this->SubNodes[node]].operation>0 )
	 free( gParse.Nodes[this->SubNodes[node]].value.data.ptr );
}

/*****************************************************************************/
/*  Utility routines which perform the calculations on bits and SAO regions  */
/*****************************************************************************/

static char bitlgte(char *bits1, int oper, char *bits2)
{
 int val1, val2, nextbit;
 char result;
 int i, l1, l2, length, ldiff;
 char stream[256];
 char chr1, chr2;

 l1 = strlen(bits1);
 l2 = strlen(bits2);
 if (l1 < l2)
   {
    length = l2;
    ldiff = l2 - l1;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l1--    ) stream[i++] = *(bits1++);
    stream[i] = '\0';
    bits1 = stream;
   }
 else if (l2 < l1)
   {
    length = l1;
    ldiff = l1 - l2;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l2--    ) stream[i++] = *(bits2++);
    stream[i] = '\0';
    bits2 = stream;
   }
 else
    length = l1;

 val1 = val2 = 0;
 nextbit = 1;

 while( length-- )
    {
     chr1 = bits1[length];
     chr2 = bits2[length];
     if ((chr1 != 'x')&&(chr1 != 'X')&&(chr2 != 'x')&&(chr2 != 'X'))
       {
        if (chr1 == '1') val1 += nextbit;
        if (chr2 == '1') val2 += nextbit;
        nextbit *= 2;
       }
    }
 result = 0;
 switch (oper)
       {
        case LT:
             if (val1 < val2) result = 1;
             break;
        case LTE:
             if (val1 <= val2) result = 1;
             break;
        case GT:
             if (val1 > val2) result = 1;
             break;
        case GTE:
             if (val1 >= val2) result = 1;
             break;
       }
 return (result);
}

static void bitand(char *result,char *bitstrm1,char *bitstrm2)
{
 int i, l1, l2, ldiff;
 char stream[256];
 char chr1, chr2;

 l1 = strlen(bitstrm1);
 l2 = strlen(bitstrm2);
 if (l1 < l2)
   {
    ldiff = l2 - l1;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l1--    ) stream[i++] = *(bitstrm1++);
    stream[i] = '\0';
    bitstrm1 = stream;
   }
 else if (l2 < l1)
   {
    ldiff = l1 - l2;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l2--    ) stream[i++] = *(bitstrm2++);
    stream[i] = '\0';
    bitstrm2 = stream;
   }
 while ( (chr1 = *(bitstrm1++)) ) 
    {
       chr2 = *(bitstrm2++);
       if ((chr1 == 'x') || (chr2 == 'x'))
          *result = 'x';
       else if ((chr1 == '1') && (chr2 == '1'))
          *result = '1';
       else
          *result = '0';
       result++;
    }
 *result = '\0';
}

static void bitor(char *result,char *bitstrm1,char *bitstrm2)
{
 int i, l1, l2, ldiff;
 char stream[256];
 char chr1, chr2;

 l1 = strlen(bitstrm1);
 l2 = strlen(bitstrm2);
 if (l1 < l2)
   {
    ldiff = l2 - l1;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l1--    ) stream[i++] = *(bitstrm1++);
    stream[i] = '\0';
    bitstrm1 = stream;
   }
 else if (l2 < l1)
   {
    ldiff = l1 - l2;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l2--    ) stream[i++] = *(bitstrm2++);
    stream[i] = '\0';
    bitstrm2 = stream;
   }
 while ( (chr1 = *(bitstrm1++)) ) 
    {
       chr2 = *(bitstrm2++);
       if ((chr1 == '1') || (chr2 == '1'))
          *result = '1';
       else if ((chr1 == '0') || (chr2 == '0'))
          *result = '0';
       else
          *result = 'x';
       result++;
    }
 *result = '\0';
}

static void bitnot(char *result,char *bits)
{
   int length;
   char chr;

   length = strlen(bits);
   while( length-- ) {
      chr = *(bits++);
      *(result++) = ( chr=='1' ? '0' : ( chr=='0' ? '1' : chr ) );
   }
   *result = '\0';
}

static char bitcmp(char *bitstrm1, char *bitstrm2)
{
 int i, l1, l2, ldiff;
 char stream[256];
 char chr1, chr2;

 l1 = strlen(bitstrm1);
 l2 = strlen(bitstrm2);
 if (l1 < l2)
   {
    ldiff = l2 - l1;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l1--    ) stream[i++] = *(bitstrm1++);
    stream[i] = '\0';
    bitstrm1 = stream;
   }
 else if (l2 < l1)
   {
    ldiff = l1 - l2;
    i=0;
    while( ldiff-- ) stream[i++] = '0';
    while( l2--    ) stream[i++] = *(bitstrm2++);
    stream[i] = '\0';
    bitstrm2 = stream;
   }
 while( (chr1 = *(bitstrm1++)) )
    {
       chr2 = *(bitstrm2++);
       if ( ((chr1 == '0') && (chr2 == '1'))
	    || ((chr1 == '1') && (chr2 == '0')) )
	  return( 0 );
    }
 return( 1 );
}

static char bnear(double x, double y, double tolerance)
{
 if (fabs(x - y) < tolerance)
   return ( 1 );
 else
   return ( 0 );
}

static char saobox(double xcen, double ycen, double xwid, double ywid,
		   double rot,  double xcol, double ycol)
{
 double x,y,xprime,yprime,xmin,xmax,ymin,ymax,theta;

 theta = (rot / 180.0) * myPI;
 xprime = xcol - xcen;
 yprime = ycol - ycen;
 x =  xprime * cos(theta) + yprime * sin(theta);
 y = -xprime * sin(theta) + yprime * cos(theta);
 xmin = - 0.5 * xwid; xmax = 0.5 * xwid;
 ymin = - 0.5 * ywid; ymax = 0.5 * ywid;
 if ((x >= xmin) && (x <= xmax) && (y >= ymin) && (y <= ymax))
   return ( 1 );
 else
   return ( 0 );
}

static char circle(double xcen, double ycen, double rad,
		   double xcol, double ycol)
{
 double r2,dx,dy,dlen;

 dx = xcol - xcen;
 dy = ycol - ycen;
 dx *= dx; dy *= dy;
 dlen = dx + dy;
 r2 = rad * rad;
 if (dlen <= r2)
   return ( 1 );
 else
   return ( 0 );
}

static char ellipse(double xcen, double ycen, double xrad, double yrad,
		    double rot, double xcol, double ycol)
{
 double x,y,xprime,yprime,dx,dy,dlen,theta;

 theta = (rot / 180.0) * myPI;
 xprime = xcol - xcen;
 yprime = ycol - ycen;
 x =  xprime * cos(theta) + yprime * sin(theta);
 y = -xprime * sin(theta) + yprime * cos(theta);
 dx = x / xrad; dy = y / yrad;
 dx *= dx; dy *= dy;
 dlen = dx + dy;
 if (dlen <= 1.0)
   return ( 1 );
 else
   return ( 0 );
}

static void fferror(char *s)
{
    char msg[80];

    if( !gParse.status ) gParse.status = PARSE_SYNTAX_ERR;

    strncpy(msg, s, 80);
    msg[79] = '\0';
    ffpmsg(msg);
}
