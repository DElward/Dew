/*
**  parsehttp.c  --  Routines for access to HTML files
*/
/***************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
/*#include <ctype.h>*/
/*#include <time.h>*/
/*#include <math.h>*/

#include "dew.h"
#include "utl.h"
#include "parsehttp.h"
#include "dbtreeh.h"
#include "snprfh.h"

/*
** HTML Specifications
**  HTML 4.01   http://www.w3.org/TR/html4/
**  HTML 5.2    http://www.w3.org/TR/html5/
**  HTML 5.3    https://w3c.github.io/html/index.html
**
** Documentation
** W3 Schools   https://www.w3schools.com/html/default.asp
** Wikipedia    https://en.wikipedia.org/wiki/HTML_element
**              https://html.com/
*/
/***************************************************************/
#define CHCASE_NONE     0
#define CHCASE_TO_UPPER 1
#define CHCASE_TO_LOWER 2
/***************************************************************/
/* Forards */
/***************************************************************/
static void htpp_doctypedecl(struct htmls_file * xf,
                          struct html_stream * xs);

static void Html_free_element(void * pdata);
static int htup_handle_auto_end(struct htmls_file * xf,
                           struct html_stream * xs,
                           struct ht_element  * parent_el);
static void Html_free_attlist(void * pdata);
/***************************************************************/
/* Externals */
/***************************************************************/

#define HTCX_RCHR    1
#define HTCX_WHIT    2
#define HTCX_DIG     4
#define HTCX_UPPR    8
#define HTCX_LOWR    16
#define HTCX_NAMS    32      /* Name start */
#define HTCX_NAME    64      /* Name character */
#define HTCX_QUOT    128     /* Name start */
#define HTCX_PBID    256     /* PubidChar */

#define HTCXS_ALPH (HTCX_LOWR | HTCX_UPPR)
#define HTCXS_NAME (HTCX_NAME | HTCX_NAMS | HTCXS_ALPH)
#define HTCXS_NAMS (HTCX_NAMS | HTCXS_ALPH)
#define HTCXS_PBID (HTCX_DIG  | HTCX_PBID | HTCXS_ALPH)

static short int html_ctf[128] = {
/* 00  ^@ */ 0                    , /* 01  ^A */ HTCX_RCHR           ,
/* 02  ^B */ HTCX_RCHR            , /* 03  ^C */ HTCX_RCHR           ,
/* 04  ^D */ HTCX_RCHR            , /* 05  ^E */ HTCX_RCHR           ,
/* 06  ^F */ HTCX_RCHR            , /* 07  ^G */ HTCX_RCHR           ,
/* 08  ^H */ HTCX_RCHR            , /* 09  ^I */ HTCX_WHIT           ,
/* 0A  ^J */ HTCX_WHIT | HTCX_PBID, /* 0B  ^K */ HTCX_RCHR           ,
/* 0C  ^L */ HTCX_RCHR            , /* 0D  ^M */ HTCX_WHIT | HTCX_PBID,
/* 0E  ^N */ HTCX_RCHR            , /* 0F  ^O */ HTCX_RCHR           ,
/* 10  ^P */ HTCX_RCHR            , /* 11  ^Q */ HTCX_RCHR           ,
/* 12  ^R */ HTCX_RCHR            , /* 13  ^S */ HTCX_RCHR           ,
/* 14  ^T */ HTCX_RCHR            , /* 15  ^U */ HTCX_RCHR           ,
/* 16  ^V */ HTCX_RCHR            , /* 17  ^W */ HTCX_RCHR           ,
/* 18  ^X */ HTCX_RCHR            , /* 19  ^Y */ HTCX_RCHR           ,
/* 1A  ^Z */ HTCX_RCHR            , /* 1B     */ HTCX_RCHR           ,
/* 1C     */ HTCX_RCHR            , /* 1D     */ HTCX_RCHR           ,
/* 1E     */ HTCX_RCHR            , /* 1F     */ HTCX_RCHR           ,
/* 20 ' ' */ HTCX_WHIT | HTCX_PBID, /* 21 '!' */ HTCX_PBID,
/* 22 '"' */ HTCX_QUOT,             /* 23 '#' */ HTCX_PBID,
/* 24 '$' */ HTCX_PBID,             /* 25 '%' */ HTCX_PBID,
/* 26 '&' */ 0         ,            /* 27 ''' */ HTCX_QUOT | HTCX_PBID,
/* 28 '(' */ HTCX_PBID,             /* 29 ')' */ HTCX_PBID,
/* 2A '*' */ HTCX_PBID,             /* 2B '+' */ HTCX_PBID,
/* 2C ',' */ HTCX_PBID,             /* 2D '-' */ HTCX_NAME | HTCX_PBID,
/* 2E '.' */ HTCX_NAME | HTCX_PBID, /* 2F '/' */ HTCX_PBID,
/* 30 '0' */ HTCX_DIG  | HTCX_NAME, /* 31 '1' */ HTCX_DIG  | HTCX_NAME,
/* 32 '2' */ HTCX_DIG  | HTCX_NAME, /* 33 '3' */ HTCX_DIG  | HTCX_NAME,
/* 34 '4' */ HTCX_DIG  | HTCX_NAME, /* 35 '5' */ HTCX_DIG  | HTCX_NAME,
/* 36 '6' */ HTCX_DIG  | HTCX_NAME, /* 37 '7' */ HTCX_DIG  | HTCX_NAME,
/* 38 '8' */ HTCX_DIG  | HTCX_NAME, /* 39 '9' */ HTCX_DIG  | HTCX_NAME,
/* 3A ':' */ HTCX_NAMS | HTCX_PBID, /* 3B ';' */ HTCX_PBID,
/* 3C '<' */ 0         ,            /* 3D '=' */ HTCX_PBID,
/* 3E '>' */ 0         ,            /* 3F '?' */ HTCX_PBID,
/* 40 '@' */ HTCX_PBID,             /* 41 'A' */ HTCX_NAMS | HTCX_UPPR,
/* 42 'B' */ HTCX_NAMS | HTCX_UPPR, /* 43 'C' */ HTCX_NAMS | HTCX_UPPR,
/* 44 'D' */ HTCX_NAMS | HTCX_UPPR, /* 45 'E' */ HTCX_NAMS | HTCX_UPPR,
/* 46 'F' */ HTCX_NAMS | HTCX_UPPR, /* 47 'G' */ HTCX_NAMS | HTCX_UPPR,
/* 48 'H' */ HTCX_NAMS | HTCX_UPPR, /* 49 'I' */ HTCX_NAMS | HTCX_UPPR,
/* 4A 'J' */ HTCX_NAMS | HTCX_UPPR, /* 4B 'K' */ HTCX_NAMS | HTCX_UPPR,
/* 4C 'L' */ HTCX_NAMS | HTCX_UPPR, /* 4D 'M' */ HTCX_NAMS | HTCX_UPPR,
/* 4E 'N' */ HTCX_NAMS | HTCX_UPPR, /* 4F 'O' */ HTCX_NAMS | HTCX_UPPR,
/* 50 'P' */ HTCX_NAMS | HTCX_UPPR, /* 51 'Q' */ HTCX_NAMS | HTCX_UPPR,
/* 52 'R' */ HTCX_NAMS | HTCX_UPPR, /* 53 'S' */ HTCX_NAMS | HTCX_UPPR,
/* 54 'T' */ HTCX_NAMS | HTCX_UPPR, /* 55 'U' */ HTCX_NAMS | HTCX_UPPR,
/* 56 'V' */ HTCX_NAMS | HTCX_UPPR, /* 57 'W' */ HTCX_NAMS | HTCX_UPPR,
/* 58 'X' */ HTCX_NAMS | HTCX_UPPR, /* 59 'Y' */ HTCX_NAMS | HTCX_UPPR,
/* 5A 'Z' */ HTCX_NAMS | HTCX_UPPR, /* 5B '[' */ 0       ,
/* 5C '\' */ 0         ,            /* 5D ']' */ 0       ,
/* 5E '^' */ 0         ,            /* 5F '_' */ HTCX_NAMS | HTCX_PBID,
/* 60 '`' */ 0         ,            /* 61 'a' */ HTCX_NAMS | HTCX_LOWR,
/* 62 'b' */ HTCX_NAMS | HTCX_LOWR, /* 63 'c' */ HTCX_NAMS | HTCX_LOWR,
/* 64 'd' */ HTCX_NAMS | HTCX_LOWR, /* 65 'e' */ HTCX_NAMS | HTCX_LOWR,
/* 66 'f' */ HTCX_NAMS | HTCX_LOWR, /* 67 'g' */ HTCX_NAMS | HTCX_LOWR,
/* 68 'h' */ HTCX_NAMS | HTCX_LOWR, /* 69 'i' */ HTCX_NAMS | HTCX_LOWR,
/* 6A 'j' */ HTCX_NAMS | HTCX_LOWR, /* 6B 'k' */ HTCX_NAMS | HTCX_LOWR,
/* 6C 'l' */ HTCX_NAMS | HTCX_LOWR, /* 6D 'm' */ HTCX_NAMS | HTCX_LOWR,
/* 6E 'n' */ HTCX_NAMS | HTCX_LOWR, /* 6F 'o' */ HTCX_NAMS | HTCX_LOWR,
/* 70 'p' */ HTCX_NAMS | HTCX_LOWR, /* 71 'q' */ HTCX_NAMS | HTCX_LOWR,
/* 72 'r' */ HTCX_NAMS | HTCX_LOWR, /* 73 's' */ HTCX_NAMS | HTCX_LOWR,
/* 74 't' */ HTCX_NAMS | HTCX_LOWR, /* 75 'u' */ HTCX_NAMS | HTCX_LOWR,
/* 76 'v' */ HTCX_NAMS | HTCX_LOWR, /* 77 'w' */ HTCX_NAMS | HTCX_LOWR,
/* 78 'x' */ HTCX_NAMS | HTCX_LOWR, /* 79 'y' */ HTCX_NAMS | HTCX_LOWR,
/* 7A 'z' */ HTCX_NAMS | HTCX_LOWR, /* 7B '{' */ 0       ,
/* 7C '|' */ 0         ,            /* 7D '}' */ 0       ,
/* 7E '~' */ 0         ,            /* 7F     */ 0
};

enum eHTML_Tags {
    HTAG_NONE           = 0
  , HTAG_A              = 1
  , HTAG_ABBR           = 2
  , HTAG_ACRONYM        = 3
  , HTAG_ADDRESS        = 4
  , HTAG_APP            = 5
  , HTAG_APPLET         = 6
  , HTAG_AREA           = 7
  , HTAG_ARTICLE        = 8
  , HTAG_ASIDE          = 9
  , HTAG_AUDIO          = 10
  , HTAG_B              = 11
  , HTAG_BASE           = 12
  , HTAG_BASEFONT       = 13
  , HTAG_BDI            = 14
  , HTAG_BDO            = 15
  , HTAG_BGSOUND        = 16
  , HTAG_BIG            = 17
  , HTAG_BLINK          = 18
  , HTAG_BLOCKQUOTE     = 19
  , HTAG_BODY           = 20
  , HTAG_BR             = 21
  , HTAG_BUTTON         = 22
  , HTAG_CANVAS         = 23
  , HTAG_CAPTION        = 24
  , HTAG_CENTER         = 25
  , HTAG_CITE           = 26
  , HTAG_CODE           = 27
  , HTAG_COL            = 28
  , HTAG_COLGROUP       = 29
  , HTAG_COMMAND        = 30
  , HTAG_COMMENT        = 31
  , HTAG_DATALIST       = 32
  , HTAG_DD             = 33
  , HTAG_DEL            = 34
  , HTAG_DETAILS        = 35
  , HTAG_DFN            = 36
  , HTAG_DIALOG         = 37
  , HTAG_DIR            = 38
  , HTAG_DIV            = 39
  , HTAG_DL             = 40
  , HTAG_DT             = 41
  , HTAG_EM             = 42
  , HTAG_EMBED          = 43
  , HTAG_FIELDSET       = 44
  , HTAG_FIGCAPTION     = 45
  , HTAG_FIGURE         = 46
  , HTAG_FONT           = 47
  , HTAG_FOOTER         = 48
  , HTAG_FORM           = 49
  , HTAG_FRAME          = 50
  , HTAG_FRAMESET       = 51
  , HTAG_H1             = 52
  , HTAG_H2             = 53
  , HTAG_H3             = 54
  , HTAG_H4             = 55
  , HTAG_H5             = 56
  , HTAG_H6             = 57
  , HTAG_HEAD           = 58
  , HTAG_HEADER         = 59
  , HTAG_HR             = 60
  , HTAG_HTML           = 61
  , HTAG_HYPE           = 62
  , HTAG_I              = 63
  , HTAG_IFRAME         = 64
  , HTAG_IMAGE          = 65
  , HTAG_IMG            = 66
  , HTAG_INPUT          = 67
  , HTAG_INS            = 68
  , HTAG_ISINDEX        = 69
  , HTAG_KBD            = 70
  , HTAG_KEYGEN         = 71
  , HTAG_LABEL          = 72
  , HTAG_LEGEND         = 73
  , HTAG_LI             = 74
  , HTAG_LINK           = 75
  , HTAG_LISTING        = 76
  , HTAG_MAIN           = 77
  , HTAG_MAP            = 78
  , HTAG_MARK           = 79
  , HTAG_MARQUEE        = 80
  , HTAG_MENU           = 81
  , HTAG_MENUITEM       = 82
  , HTAG_META           = 83
  , HTAG_METER          = 84
  , HTAG_MULTICOL       = 85
  , HTAG_NAV            = 86
  , HTAG_NEXTID         = 87
  , HTAG_NOBR           = 88
  , HTAG_NOEMBED        = 89
  , HTAG_NOFRAMES       = 90
  , HTAG_NOSCRIPT       = 91
  , HTAG_OBJECT         = 92
  , HTAG_OL             = 93
  , HTAG_OPTGROUP       = 94
  , HTAG_OPTION         = 95
  , HTAG_OUTPUT         = 96
  , HTAG_P              = 97
  , HTAG_PARAM          = 98
  , HTAG_PICTURE        = 99
  , HTAG_PLAINTEXT      = 100
  , HTAG_PRE            = 101
  , HTAG_PROGRESS       = 102
  , HTAG_Q              = 103
  , HTAG_RP             = 104
  , HTAG_RT             = 105
  , HTAG_RUBY           = 106
  , HTAG_S              = 107
  , HTAG_SAMP           = 108
  , HTAG_SCRIPT         = 109
  , HTAG_SECTION        = 110
  , HTAG_SELECT         = 111
  , HTAG_SMALL          = 112
  , HTAG_SOUND          = 113
  , HTAG_SOURCE         = 114
  , HTAG_SPACER         = 115
  , HTAG_SPAN           = 116
  , HTAG_STRIKE         = 117
  , HTAG_STRONG         = 118
  , HTAG_STYLE          = 119
  , HTAG_SUB            = 120
  , HTAG_SUMMARY        = 121
  , HTAG_SUP            = 122
  , HTAG_SVG            = 123
  , HTAG_TABLE          = 124
  , HTAG_TBODY          = 125
  , HTAG_TD             = 126
  , HTAG_TEMPLATE       = 127
  , HTAG_TEXTAREA       = 128
  , HTAG_TFOOT          = 129
  , HTAG_TH             = 130
  , HTAG_THEAD          = 131
  , HTAG_TIME           = 132
  , HTAG_TITLE          = 133
  , HTAG_TR             = 134
  , HTAG_TRACK          = 135
  , HTAG_TT             = 136
  , HTAG_U              = 137
  , HTAG_UL             = 138
  , HTAG_VAR            = 139
  , HTAG_VIDEO          = 140
  , HTAG_WBR            = 141
  , HTAG_XMP            = 142
  , HTAG__ROOT          = 143
};

#define HTDF_VOID            1      /* Void element, e.g. <br> */
#define HTDF_NO_HTML5        2      /* Not in HTML 4, but in 5 */
#define HTDF_OBSOLETE        4      /* Not in HTML 4 or 5 */
#define HTDF_PRE_INLINE      8      /* Printing only: No \n after begin tag */
#define HTDF_POST_INLINE     16     /* Printing only: No \n after end tag */
#define HTDF_KEEP_WHITE      32     /* Keep whitespace in content, e.g. <pre> */
#define HTDF_ATTRIBUTE       64     /* Parse as a quoted attribute */
#define HTDF_CONTENT         128    /* Parse as content */
#define HTDF_RAW             256    /* Do not parse content, e.g. <script> */
#define HTDF_NO_NEST         512    /* Ignore same nested element, e.g. <html>*/
/* #define HTDF_CUST_TAG        1024   Tag has custom handling, e.g. <li> */
/* #define HTDF_CUST_END        2048   End tag has custom handling, e.g. <ul> */
#define HTDF_AUTO_END        4096   /* Does not require end tag, e.g. <li> */

#define HTDF_INLINE          (HTDF_PRE_INLINE | HTDF_POST_INLINE)
#define MAX_TAG_LENGTH   14

struct html_tag_def {   /* htd_ */
    char *          htd_name;
    enum eHTML_Tags htd_tag;
    int             htd_flags;
} html_taglist[] = {
    { ""          , HTAG_NONE       , 0                          }
  , { "a"         , HTAG_A          , HTDF_INLINE                }
  , { "abbr"      , HTAG_ABBR       , 0                          }
  , { "acronym"   , HTAG_ACRONYM    , HTDF_NO_HTML5              }
  , { "address"   , HTAG_ADDRESS    , 0                          }
  , { "app"       , HTAG_APP        , HTDF_OBSOLETE              }
  , { "applet"    , HTAG_APPLET     , HTDF_NO_HTML5              }
  , { "area"      , HTAG_AREA       , HTDF_VOID                  }
  , { "article"   , HTAG_ARTICLE    , 0                          }
  , { "aside"     , HTAG_ASIDE      , 0                          }
  , { "audio"     , HTAG_AUDIO      , 0                          }
  , { "b"         , HTAG_B          , HTDF_INLINE                }
  , { "base"      , HTAG_BASE       , HTDF_VOID                  }
  , { "basefont"  , HTAG_BASEFONT   , HTDF_VOID | HTDF_NO_HTML5  }
  , { "bdi"       , HTAG_BDI        , 0                          }
  , { "bdo"       , HTAG_BDO        , 0                          }
  , { "bgsound"   , HTAG_BGSOUND    , HTDF_OBSOLETE | HTDF_VOID  }
  , { "big"       , HTAG_BIG        , HTDF_NO_HTML5              }
  , { "blink"     , HTAG_BLINK      , HTDF_OBSOLETE              }
  , { "blockquote", HTAG_BLOCKQUOTE , 0                          }
  , { "body"      , HTAG_BODY       , 0                          }
  , { "br"        , HTAG_BR         , HTDF_VOID                  }
  , { "button"    , HTAG_BUTTON     , 0                          }
  , { "canvas"    , HTAG_CANVAS     , 0                          }
  , { "caption"   , HTAG_CAPTION    , 0                          }
  , { "center"    , HTAG_CENTER     , HTDF_NO_HTML5              }
  , { "cite"      , HTAG_CITE       , 0                          }
  , { "code"      , HTAG_CODE       , 0                          }
  , { "col"       , HTAG_COL        , HTDF_VOID                  }
  , { "colgroup"  , HTAG_COLGROUP   , 0                          }
  , { "command"   , HTAG_COMMAND    , HTDF_OBSOLETE | HTDF_VOID  }
  , { "comment"   , HTAG_COMMENT    , HTDF_OBSOLETE              }
  , { "datalist"  , HTAG_DATALIST   , 0                          }
  , { "dd"        , HTAG_DD         , HTDF_AUTO_END              }
  , { "del"       , HTAG_DEL        , HTDF_INLINE                }
  , { "details"   , HTAG_DETAILS    , 0                          }
  , { "dfn"       , HTAG_DFN        , 0                          }
  , { "dialog"    , HTAG_DIALOG     , 0                          }
  , { "dir"       , HTAG_DIR        , HTDF_NO_HTML5              }
  , { "div"       , HTAG_DIV        , 0                          }
  , { "dl"        , HTAG_DL         , 0                          }
  , { "dt"        , HTAG_DT         , HTDF_AUTO_END              }
  , { "em"        , HTAG_EM         , 0                          }
  , { "embed"     , HTAG_EMBED      , HTDF_VOID                  }
  , { "fieldset"  , HTAG_FIELDSET   , 0                          }
  , { "figcaption", HTAG_FIGCAPTION , 0                          }
  , { "figure"    , HTAG_FIGURE     , 0                          }
  , { "font"      , HTAG_FONT       , HTDF_NO_HTML5              }
  , { "footer"    , HTAG_FOOTER     , 0                          }
  , { "form"      , HTAG_FORM       , 0                          }
  , { "frame"     , HTAG_FRAME      , HTDF_VOID | HTDF_NO_HTML5  }
  , { "frameset"  , HTAG_FRAMESET   , HTDF_NO_HTML5              }
  , { "h1"        , HTAG_H1         , HTDF_PRE_INLINE            }
  , { "h2"        , HTAG_H2         , HTDF_PRE_INLINE            }
  , { "h3"        , HTAG_H3         , HTDF_PRE_INLINE            }
  , { "h4"        , HTAG_H4         , HTDF_PRE_INLINE            }
  , { "h5"        , HTAG_H5         , HTDF_PRE_INLINE            }
  , { "h6"        , HTAG_H6         , HTDF_PRE_INLINE            }
  , { "head"      , HTAG_HEAD       , 0                          }
  , { "header"    , HTAG_HEADER     , 0                          }
  , { "hr"        , HTAG_HR         , HTDF_VOID                  }
  , { "html"      , HTAG_HTML       , HTDF_NO_NEST               }
  , { "hype"      , HTAG_HYPE       , HTDF_OBSOLETE              }
  , { "i"         , HTAG_I          , HTDF_INLINE                }
  , { "iframe"    , HTAG_IFRAME     , 0                          }
  , { "image"     , HTAG_IMAGE      , HTDF_OBSOLETE | HTDF_VOID  }
  , { "img"       , HTAG_IMG        , HTDF_VOID                  }
  , { "input"     , HTAG_INPUT      , HTDF_VOID                  }
  , { "ins"       , HTAG_INS        , 0                          }
  , { "isindex"   , HTAG_ISINDEX    , HTDF_OBSOLETE | HTDF_VOID  }
  , { "kbd"       , HTAG_KBD        , 0                          }
  , { "keygen"    , HTAG_KEYGEN     , HTDF_OBSOLETE | HTDF_VOID  }
  , { "label"     , HTAG_LABEL      , 0                          }
  , { "legend"    , HTAG_LEGEND     , 0                          }
  , { "li"        , HTAG_LI         , HTDF_AUTO_END              }
  , { "link"      , HTAG_LINK       , HTDF_VOID                  }
  , { "listing"   , HTAG_LISTING    , HTDF_OBSOLETE              }
  , { "main"      , HTAG_MAIN       , 0                          }
  , { "map"       , HTAG_MAP        , 0                          }
  , { "mark"      , HTAG_MARK       , 0                          }
  , { "marquee"   , HTAG_MARQUEE    , HTDF_OBSOLETE              }
  , { "menu"      , HTAG_MENU       , 0                          }
  , { "menuitem"  , HTAG_MENUITEM   , HTDF_VOID                  }
  , { "meta"      , HTAG_META       , HTDF_VOID                  }
  , { "meter"     , HTAG_METER      , 0                          }
  , { "multicol"  , HTAG_MULTICOL   , HTDF_OBSOLETE              }
  , { "nav"       , HTAG_NAV        , 0                          }
  , { "nextid"    , HTAG_NEXTID     , HTDF_OBSOLETE | HTDF_VOID  }
  , { "nobr"      , HTAG_NOBR       , HTDF_OBSOLETE              }
  , { "noembed"   , HTAG_NOEMBED    , HTDF_OBSOLETE              }
  , { "noframes"  , HTAG_NOFRAMES   , HTDF_NO_HTML5              }
  , { "noscript"  , HTAG_NOSCRIPT   , 0                          }
  , { "object"    , HTAG_OBJECT     , 0                          }
  , { "ol"        , HTAG_OL         , 0                          }
  , { "optgroup"  , HTAG_OPTGROUP   , 0                          }
  , { "option"    , HTAG_OPTION     , 0                          }
  , { "output"    , HTAG_OUTPUT     , 0                          }
  , { "p"         , HTAG_P          , 0                          }
  , { "param"     , HTAG_PARAM      , HTDF_VOID                  }
  , { "picture"   , HTAG_PICTURE    , 0                          }
  , { "plaintext" , HTAG_PLAINTEXT  , HTDF_OBSOLETE              }
  , { "pre"       , HTAG_PRE        , HTDF_KEEP_WHITE            }
  , { "progress"  , HTAG_PROGRESS   , 0                          }
  , { "q"         , HTAG_Q          , 0                          }
  , { "rp"        , HTAG_RP         , 0                          }
  , { "rt"        , HTAG_RT         , 0                          }
  , { "ruby"      , HTAG_RUBY       , 0                          }
  , { "s"         , HTAG_S          , 0                          }
  , { "samp"      , HTAG_SAMP       , 0                          }
  , { "script"    , HTAG_SCRIPT     , HTDF_RAW                   }
  , { "section"   , HTAG_SECTION    , 0                          }
  , { "select"    , HTAG_SELECT     , 0                          }
  , { "small"     , HTAG_SMALL      , 0                          }
  , { "sound"     , HTAG_SOUND      , HTDF_OBSOLETE              }
  , { "source"    , HTAG_SOURCE     , HTDF_VOID                  }
  , { "spacer"    , HTAG_SPACER     , HTDF_OBSOLETE              }
  , { "span"      , HTAG_SPAN       , HTDF_INLINE                }
  , { "strike"    , HTAG_STRIKE     , HTDF_NO_HTML5              }
  , { "strong"    , HTAG_STRONG     , 0                          }
  , { "style"     , HTAG_STYLE      , 0                          }
  , { "sub"       , HTAG_SUB        , 0                          }
  , { "summary"   , HTAG_SUMMARY    , 0                          }
  , { "sup"       , HTAG_SUP        , 0                          }
  , { "svg"       , HTAG_SVG        , 0                          }
  , { "table"     , HTAG_TABLE      , 0                          }
  , { "tbody"     , HTAG_TBODY      , 0                          }
  , { "td"        , HTAG_TD         , HTDF_PRE_INLINE            }
  , { "template"  , HTAG_TEMPLATE   , 0                          }
  , { "textarea"  , HTAG_TEXTAREA   , 0                          }
  , { "tfoot"     , HTAG_TFOOT      , 0                          }
  , { "th"        , HTAG_TH         , 0                          }
  , { "thead"     , HTAG_THEAD      , 0                          }
  , { "time"      , HTAG_TIME       , 0                          }
  , { "title"     , HTAG_TITLE      , HTDF_INLINE                }
  , { "tr"        , HTAG_TR         , 0                          }
  , { "track"     , HTAG_TRACK      , HTDF_VOID                  }
  , { "tt"        , HTAG_TT         , HTDF_NO_HTML5              }
  , { "u"         , HTAG_U          , 0                          }
  , { "ul"        , HTAG_UL         , 0                          }
  , { "var"       , HTAG_VAR        , 0                          }
  , { "video"     , HTAG_VIDEO      , 0                          }
  , { "wbr"       , HTAG_WBR        , HTDF_VOID                  }
  , { "xmp"       , HTAG_XMP        , HTDF_OBSOLETE              }
  , { "~ROOT~"    , HTAG__ROOT      , 0                          }
  , { NULL, 0, 0 } };

#define HTML_VOID_ELEMENT(ix) (html_taglist[ix].htd_flags & HTDF_VOID)
#define HTML_PRE_INLINE_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_PRE_INLINE)
#define HTML_POST_INLINE_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_POST_INLINE)
#define HTML_KEEP_WHITE_ELEMENT(ix) \
(html_taglist[ix].htd_flags & (HTDF_KEEP_WHITE | HTDF_RAW))
#define HTML_NO_NEST_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_NO_NEST)
#define HTML_RAW_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_RAW)
/*
#define HTML_CUST_TAG_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_CUST_TAG)
#define HTML_CUST_END_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_CUST_END)
*/
#define HTML_AUTO_END_ELEMENT(ix) \
(html_taglist[ix].htd_flags & HTDF_AUTO_END)

#define HTML_ELEMENT_NAME(ix) (html_taglist[ix].htd_name)

/***************************************************************/

#define MAX_ENTITY_LENGTH   14

struct html_entity_def {   /* hty_ */
    char *          hty_entity;
    int             hty_num;
} html_entlist[] = {
    { ""        , 0     }  , { "AElig"   , 198   }  , { "Aacute"  , 193   }
  , { "Acirc"   , 194   }  , { "Agrave"  , 192   }  , { "Alpha"   , 913   }
  , { "Aring"   , 197   }  , { "Atilde"  , 195   }  , { "Auml"    , 196   }
  , { "Beta"    , 914   }  , { "Ccedil"  , 199   }  , { "Chi"     , 935   }
  , { "Dagger"  , 8225  }  , { "Delta"   , 916   }  , { "ETH"     , 208   }
  , { "Eacute"  , 201   }  , { "Ecirc"   , 202   }  , { "Egrave"  , 200   }
  , { "Epsilon" , 917   }  , { "Eta"     , 919   }  , { "Euml"    , 203   }
  , { "Gamma"   , 915   }  , { "Iacute"  , 205   }  , { "Icirc"   , 206   }
  , { "Igrave"  , 204   }  , { "Iota"    , 921   }  , { "Iuml"    , 207   }
  , { "Kappa"   , 922   }  , { "Lambda"  , 923   }  , { "Mu"      , 924   }
  , { "Ntilde"  , 209   }  , { "Nu"      , 925   }  , { "OElig"   , 338   }
  , { "Oacute"  , 211   }  , { "Ocirc"   , 212   }  , { "Ograve"  , 210   }
  , { "Omega"   , 937   }  , { "Omicron" , 927   }  , { "Oslash"  , 216   }
  , { "Otilde"  , 213   }  , { "Ouml"    , 214   }  , { "Phi"     , 934   }
  , { "Pi"      , 928   }  , { "Prime"   , 8243  }  , { "Psi"     , 936   }
  , { "Rho"     , 929   }  , { "Scaron"  , 352   }  , { "Sigma"   , 931   }
  , { "THORN"   , 222   }  , { "Tau"     , 932   }  , { "Theta"   , 920   }
  , { "Uacute"  , 218   }  , { "Ucirc"   , 219   }  , { "Ugrave"  , 217   }
  , { "Upsilon" , 933   }  , { "Uuml"    , 220   }  , { "Xi"      , 926   }
  , { "Yacute"  , 221   }  , { "Yuml"    , 376   }  , { "Zeta"    , 918   }
  , { "aacute"  , 225   }  , { "acirc"   , 226   }  , { "acute"   , 180   }
  , { "add"     , 43    }  , { "aelig"   , 230   }  , { "agrave"  , 224   }
  , { "alefsym" , 8501  }  , { "alpha"   , 945   }  , { "amp"     , 38    }
  , { "and"     , 8743  }  , { "ang"     , 8736  }  , { "apos"    , 39    }
  , { "aring"   , 229   }  , { "ast"     , 42    }  , { "asymp"   , 8776  }
  , { "atilde"  , 227   }  , { "auml"    , 228   }  , { "bdquo"   , 8222  }
  , { "beta"    , 946   }  , { "brvbar"  , 166   }  , { "bull"    , 8226  }
  , { "cap"     , 8745  }  , { "ccedil"  , 231   }  , { "cedil"   , 184   }
  , { "cent"    , 162   }  , { "chi"     , 967   }  , { "circ"    , 710   }
  , { "clubs"   , 9827  }  , { "cong"    , 8773  }  , { "copy"    , 169   }
  , { "crarr"   , 8629  }  , { "cup"     , 8746  }  , { "curren"  , 164   }
  , { "dagger"  , 8224  }  , { "darr"    , 8595  }  , { "deg"     , 176   }
  , { "delta"   , 948   }  , { "diams"   , 9830  }  , { "divide"  , 247   }
  , { "eacute"  , 233   }  , { "ecirc"   , 234   }  , { "egrave"  , 232   }
  , { "empty"   , 8709  }  , { "emsp"    , 8195  }  , { "ensp"    , 8194  }
  , { "epsilon" , 949   }  , { "equal"   , 61    }  , { "equiv"   , 8801  }
  , { "eta"     , 951   }  , { "eth"     , 240   }  , { "euml"    , 235   }
  , { "euro"    , 8364  }  , { "exclamation", 33 }  , { "exist"   , 8707  }
  , { "fnof"    , 402   }  , { "forall"  , 8704  }  , { "frac12"  , 189   }
  , { "frac14"  , 188   }  , { "frac34"  , 190   }  , { "frasl"   , 8260  }
  , { "gamma"   , 947   }  , { "ge"      , 8805  }  , { "gt"      , 62    }
  , { "harr"    , 8596  }  , { "hearts"  , 9829  }  , { "hellip"  , 8230  }
  , { "horbar"  , 8213  }  , { "iacute"  , 237   }  , { "icirc"   , 238   }
  , { "iexcl"   , 161   }  , { "igrave"  , 236   }  , { "image"   , 8465  }
  , { "infin"   , 8734  }  , { "int"     , 8747  }  , { "iota"    , 953   }
  , { "iquest"  , 191   }  , { "isin"    , 8712  }  , { "iuml"    , 239   }
  , { "kappa"   , 954   }  , { "lambda"  , 955   }  , { "lang"    , 9001  }
  , { "laquo"   , 171   }  , { "larr"    , 8592  }  , { "lceil"   , 8968  }
  , { "ldquo"   , 8220  }  , { "le"      , 8804  }  , { "lfloor"  , 8970  }
  , { "lowast"  , 8727  }  , { "loz"     , 9674  }  , { "lrm"     , 8206  }
  , { "lsaquo"  , 8249  }  , { "lsquo"   , 8216  }  , { "lt"      , 60    }
  , { "macr"    , 175   }  , { "mdash"   , 8212  }  , { "micro"   , 181   }
  , { "middot"  , 183   }  , { "minus"   , 8722  }  , { "mu"      , 956   }
  , { "nabla"   , 8711  }  , { "nbsp"    , 160   }  , { "ndash"   , 8211  }
  , { "ne"      , 8800  }  , { "ni"      , 8715  }  , { "not"     , 172   }
  , { "notin"   , 8713  }  , { "nsub"    , 8836  }  , { "ntilde"  , 241   }
  , { "nu"      , 957   }  , { "oacute"  , 243   }  , { "ocirc"   , 244   }
  , { "oelig"   , 339   }  , { "ograve"  , 242   }  , { "oline"   , 8254  }
  , { "omega"   , 969   }  , { "omicron" , 959   }  , { "oplus"   , 8853  }
  , { "or"      , 8744  }  , { "ordf"    , 170   }  , { "ordm"    , 186   }
  , { "oslash"  , 248   }  , { "otilde"  , 245   }  , { "otimes"  , 8855  }
  , { "ouml"    , 246   }  , { "para"    , 182   }  , { "part"    , 8706  }
  , { "percent" , 37    }  , { "permil"  , 8240  }  , { "perp"    , 8869  }
  , { "phi"     , 966   }  , { "pi"      , 960   }  , { "piv"     , 982   }
  , { "plusmn"  , 177   }  , { "pound"   , 163   }  , { "prime"   , 8242  }
  , { "prod"    , 8719  }  , { "prop"    , 8733  }  , { "psi"     , 968   }
  , { "quot"    , 34    }  , { "radic"   , 8730  }  , { "rang"    , 9002  }
  , { "raquo"   , 187   }  , { "rarr"    , 8594  }  , { "rceil"   , 8969  }
  , { "rdquo"   , 8221  }  , { "real"    , 8476  }  , { "reg"     , 174   }
  , { "rfloor"  , 8971  }  , { "rho"     , 961   }  , { "rlm"     , 8207  }
  , { "rsaquo"  , 8250  }  , { "rsquo"   , 8217  }  , { "sbquo"   , 8218  }
  , { "scaron"  , 353   }  , { "sdot"    , 8901  }  , { "sect"    , 167   }
  , { "shy"     , 173   }  , { "sigma"   , 963   }  , { "sigmaf"  , 962   }
  , { "sim"     , 8764  }  , { "spades"  , 9824  }  , { "sub"     , 8834  }
  , { "sube"    , 8838  }  , { "sum"     , 8721  }  , { "sup"     , 185   }
  , { "sup1"    , 178   }  , { "sup2"    , 179   }  , { "sup3"    , 8835  }
  , { "supe"    , 8839  }  , { "szlig"   , 223   }  , { "tau"     , 964   }
  , { "there4"  , 8756  }  , { "theta"   , 952   }  , { "thetasym", 977   }
  , { "thinsp"  , 8201  }  , { "thorn"   , 254   }  , { "tilde"   , 732   }
  , { "times"   , 215   }  , { "trade"   , 8482  }  , { "uacute"  , 250   }
  , { "uarr"    , 8593  }  , { "ucirc"   , 251   }  , { "ugrave"  , 249   }
  , { "uml"     , 168   }  , { "upsih"   , 978   }  , { "upsilon" , 965   }
  , { "uuml"    , 252   }  , { "weierp"  , 8472  }  , { "xi"      , 958   }
  , { "yacute"  , 253   }  , { "yen"     , 165   }  , { "yuml"    , 255   }
  , { "zeta"    , 950   }  , { "zwj"     , 8205  }  , { "zwnj"    , 8204  }
  , { NULL      , 0     } };


/***************************************************************/

#if HTML_UNIMPLEMENTED
#endif /* HTML_UNIMPLEMENTED */
/***************************************************************/
/***************************************************************/

typedef int (*HTML_FREAD)(void * vp,
    void * buffer,
    int buflen,
    char * errmsg,
    int errmsgsz);
typedef int (*HTML_FOPEN) (void * vp,
            const char * fname,
            const char * fmode,
            char * errmsg,
            int errmsgsz);
typedef int (*HTML_FCLOSE)   (void * vp);
typedef int  (*HTML_ELEMENT) (void  * vp,
                              const char  * elNam,
                              int     emptyFlag,
                              int     nAttrs,
                              char ** attrNames,
                              char ** attrVals);
typedef int  (*HTML_CONTENT) (void * vp, int contLen, const char * contVal);
typedef int  (*HTML_ENDTAG)  (void * vp, const char * elNam);

/***************************************************************/
#define HTML_ATTTYP_CDATA        1
#define HTML_ATTTYP_ENUM         2
#define HTML_ATTTYP_NOTATION     3
#define HTML_ATTTYP_ID           4
#define HTML_ATTTYP_IDREF        5
#define HTML_ATTTYP_IDREFS       6
#define HTML_ATTTYP_ENTITY       7
#define HTML_ATTTYP_ENTITIES     8
#define HTML_ATTTYP_NMTOKEN      9
#define HTML_ATTTYP_NMTOKENS     10

#define HTML_ATTDEFTYP_REQUIRED  1
#define HTML_ATTDEFTYP_IMPLIED   2
#define HTML_ATTDEFTYP_FIXED     3
#define HTML_ATTDEFTYP_VAL       4
/***************************************************************/
#if IS_DEBUG
enum eHTML_rectype {
      HR_HTML_NONE        = 0
    , HR_HTML_STREAM      = 1
    , HR_HTML_S_FILE      = 2
    , HR_HTML_TREE        = 3
    , HR_HTML_P_FILE      = 4
    , HR_HTML_OPEN        = 5
    , HR_HTML_DB          = 6
    , HR_HTML_TBL         = 7
};
#endif
/***************************************************************/
struct ht_eitem {
    HTMLC *  e_name;
};

struct ht_attdef {
    HTMLC *  a_attname;
    int     a_atttyp;
    int     a_defaulttyp;
    int     a_ref_flag;
    HTMLC *  a_defaultval;
    struct dbtree * a_elist;  /* tree struct ht_eitem *'s */
};
/***************************************************************/
struct ht_attlist {

    char *  a_elname;
    struct dbtree * a_attdefs;
};
/***************************************************************/
struct ht_attlists {
    struct dbtree * xa_attlist;
};
/***************************************************************/
#define HTML_NOTTYP_SYSTEM   1
#define HTML_NOTTYP_PUBLIC   2

struct ht_notation {
    HTMLC *  n_notname;
    struct ht_extID * n_extID;
};
/***************************************************************/
struct ht_notations {
    struct dbtree * xn_notations;
};
/***************************************************************/
struct ht_elnode {
    HTMLC                    * xw_elNam;
    int                       xw_nnodes;
    struct ht_elnode       ** xw_nodes;
};
/***************************************************************/
#define XF_ERRMSG_SZ 127
struct html_stream {
#if IS_DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    struct htmls_file *        xf_xf;
    struct ht_extFile *       xf_extFile;

    /* file buffer */
    unsigned char           * xf_fbuf;
    int                       xf_fbuflen;
    int                       xf_fbufsize;
    int                       xf_fbufix;
    int                       xf_prev_fbufix; /* for unget_char */
    int                       xf_iseof;
    int                       xf_tadv;
    int                       xf_encoding;
    /* line buffer */
    HTMLC                    * xf_lbuf;
    int                       xf_lbuflen;
    int                       xf_lbufsize;
    /* token buffer */
    char                    * htss_tbuf;
    int                       htss_tbuflen;
    int                       htss_tbufsize;

    char                    * htss_utf8tbuf;
    int                       htss_utf8tbuflen;
    int                       htss_utf8tbufsize;
    int                       xf_linenum;
    char                      xf_errmsg[XF_ERRMSG_SZ + 1];
};

#define HTDEBUG_SHOW_ELEMENTS       1

struct htmls_file { /* htsf_ */
#ifdef _DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    int                       xv_err;
    char                      xv_errparm[128];
    char                      xv_errline[128];
    int                       xv_errlinenum;
    struct html_stream       * xv_stream;
    int                       xv_vFlags;
    int                       xv_sFlags;
    int                       xv_debug;
    int                       xv_standalone;
    char                      xv_vers[8]; /* either 1.0 or 1.1 */

    /* element content info  */
    HTML_FOPEN                 xq_fopen;
    HTML_FREAD                 xq_fread;
    HTML_FCLOSE                xq_fclose;
    struct ht_extFile *     xv_extFile;

    /* element content info  */
    int                       xc_maxDepth;
    struct ht_ielement     ** xc_ielements;

    struct ht_precontent    * xp_pchead;
    struct ht_precontent    * xp_pctail;

    /****** !DOCTYPE ************************/
    struct ht_entities      * xs_p_entities;
    struct ht_entities      * xs_g_entities;
    struct ht_attlists      * xs_attlists;
    struct ht_elements      * xs_elements;
    struct ht_notations     * xs_notations;

    /****** parsing info *******************/
    HTML_ELEMENT               xi_p_element;
    HTML_CONTENT               xi_p_content;
    HTML_ENDTAG                xi_p_endtag;
    void                    * xi_vp;

    int                       xi_depth;
    int                       xi_rootElement;

    /* Old and deprecated */
#if 0
    HTMLC                    * DEPRECATED_xi_elNam;
    int                        DEPRECATED_xi_elNamSize;
    int                       DEPRECATED_xi_nAttrsSize;
    int                     * DEPRECATED_xi_attrNamesSize;
    int                     * DEPRECATED_xi_attrValsSize;
    int                       DEPRECATED_xi_contentSize;
    int                       DEPRECATED_xi_contentLen;
    int                       DEPRECATED_xi_nAttrs;
    HTMLC                   ** DEPRECATED_xi_attrNames;
    int                     * DEPRECATED_xi_attrLens;
    HTMLC                   ** DEPRECATED_xi_attrVals;
    HTMLC                    * DEPRECATED_xi_content;
    HTMLC                   * DEPRECATED_xf_ebuf;
    int                       DEPRECATED_xf_ebuflen;
    int                       DEPRECATED_xf_ebufsize;
#endif

    /* New and Good */
    char                    * htsf_elNam;
    int                       htsf_elNamSize;
    char                   ** htsf_attrNames;
    int                       htsf_nAttrs;
    int                     * htsf_attrLens;
    char                   ** htsf_attrVals;
    int                       htsf_nAttrsSize;
    int                     * htsf_attrNamesSize;
    int                     * htsf_attrValsSize;
    int                       htsf_contentSize;
    int                       htsf_contentLen;
    char                    * htsf_content;
    /* entity name */
    char                    * htsf_ebuf;
    int                       htsf_ebuflen;
    int                       htsf_ebufsize;
};
/***************************************************************/
struct ht_ielement {
    int                       xc_mxElements;
    int                       xc_ncElements;
    struct ht_element      ** xc_cElements;
    struct ht_element       * xc_element;
    int                       xc_pcdata;
};
/***************************************************************/
#define HTMLSPCTYP_ELEMENT   1
#define HTMLSPCTYP_CONTENT   2
#define HTMLSPCTYP_ENDTAG    3
struct ht_precontent {
    struct ht_precontent    * xp_next;
    unsigned char             xp_typ;
    unsigned char             xp_flag;
    unsigned char             xp_fill0;
    unsigned char             xp_fill1;
    int                       xp_buffer;
};
/***************************************************************/
struct htmltree {   /* htx_ */
#ifdef _DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    char              * htx_elname;     /* UTF-8 */
    int                 htx_tagid;
    int     nattrs;
    char ** attrnam;    /* UTF-8 */
    char ** attrval;    /* UTF-8 */
    int     nsubel;
    struct htmltree ** subel;
    struct htmltree   * htx_parent;
    int                 htx_neldata;     /* UTF-8 */
    char             ** htx_eldata;     /* UTF-8 */
};
struct htmlpfile {  /* hp_ */
#ifdef _DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    struct htmls_file    * xptr;         /* HTML file */
    struct htmltree      * cxtree;
    char               ** xlist;
    int                   xlistix;
    int                   xliststrix;

    /* New and good */
    FILE                  * hp_fref;
    int                     hp_openflags;
    struct htmltree      ** hp_p_htree;
};
/***************************************************************/
#define HTML_ENTYP_SYS       0
#define HTML_ENTYP_GENERAL   1
#define HTML_ENTYP_PARM      2

struct ht_entity {
    HTMLC *  e_name;
    HTMLC *  e_val;
    struct ht_extID * e_extID;
    HTMLC *  e_ndata;
    int     e_typ;  /* 0 = system, 1 = regular, 2 = parameter */
    int     e_len;
    int     e_ix;   /* 1 if e_val is quoted */
};
/***************************************************************/
struct ht_entities {
    struct dbtree * xe_entity;
};
/***************************************************************/
struct ht_eltree {

    HTMLC * lt_op;
    HTMLC lt_repeat;
    int lt_pcdata;
    int lt_nnodes;
    struct ht_eltree ** lt_node;
};
/***************************************************************/
#define HTML_REPEAT_ONE          0
#define HTML_REPEAT_ZERO_OR_ONE  '?'
#define HTML_REPEAT_ONE_OR_MORE  '+'
#define HTML_REPEAT_ZERO_OR_MORE '*'

struct ht_imp_element {
    HTMLC *              m_elName;
    HTMLC                m_repeat;
    int                 m_accessed;
};
/***************************************************************/
struct ht_i_elements {
    int                 i_pcdata;
    struct dbtree *     i_ielements; /* struct ht_imp_element */
};
/***************************************************************/
struct ht_element {
    char *  l_name;
    enum eHTML_Tags     l_tagid;
    int                 l_flags;
    struct ht_attlist * l_attlist;
    struct ht_eltree  * l_tree;
    struct ht_i_elements * l_i_elements;
};
/***************************************************************/
struct ht_elements {
    struct dbtree * xl_elements; /* struct ht_element */
};
/***************************************************************/
#define HTML_EXTYP_SYSTEM    1
#define HTML_EXTYP_PUBLIC    2
struct ht_extID {
    int    x_typ;
    HTMLC * x_extID;
    HTMLC * x_pubID;
};
/***************************************************************/
struct ht_extFile {
    void * fref;
};

#define HTMLVFLAG_DEFAULT       (0)
#define HTMLVFLAG_VALIDATE       1
#define HTMLVFLAGS_NOENCODING    4      /* For files from Internet */
#define HTMLVFLAGS_TOLERANT      8      /* Try to overlook html errors */
#define HTMLVFLAGS_ALLOW_JSP     16     /* Allow JSP in html */

#define HTML_K_DEFAULT_FBUFSIZE  4096    /* Must be at least HTMLCMAXSZ * 2 */
#define HTML_K_DEFAULT_TBUFSIZE  256
#define HTML_K_DEFAULT_LBUFSIZE  256
#define HTML_K_DEFAULT_UBUFSIZE  256
#define HTML_K_DEFAULT_EBUFSIZE  32
#define HTML_K_LBUF_INC          128
#define HTML_K_TBUF_INC          128
#define HTML_K_CBUF_INC          128
#define HTML_K_EBUF_INC          32
#define HTML_K_UBUF_INC          128
#define HTML_MAX_ENTITY_RECUR    100

#define HTMLRESULT_OK    0
#define HTMLRESULT_EOF   (-1)
#define HTMLRESULT_ERR   (-2)
#define HTMLRESULT_INVAL (-3)

#define HTMLE_FOPEN      (-901)
#define HTMLE_FREAD      (-902)
#define HTMLE_FCLOSE     (-903)
#define HTMLE_FNEW       (-904)
#define HTMLE_PROCESS    (-905)

/***************************************************************/
/* Inline Functions                                            */
#define HTML_IS_WHITE(xc) ((xc)<128 ? (html_ctf[xc]&HTCX_WHIT ) : 0)
#define HTML_IS_NSTART(xc)((xc)<128?(html_ctf[xc]&HTCXS_NAMS):HTMLC_NStart(xc))
#define HTML_IS_NAME(xc)  ((xc)<128?(html_ctf[xc]&HTCXS_NAME):HTMLC_Name(xc))
#define HTML_IS_QUOTE(xc) ((xc)<128 ? (html_ctf[xc]&HTCX_QUOT ) : 0)
#define HTML_IS_PUBID(xc) ((xc)<128 ? (html_ctf[xc]&HTCXS_PBID) : 0)
#define HTML_IS_ALPHA(xc) ((xc)<128 ? (html_ctf[xc]&HTCXS_ALPH) : 0)
#define HTML_IS_RCHAR(xc) ((xc)<128?(html_ctf[xc]&HTCX_RCHR) :HTMLC_RChar(xc))
#define HTML_IS_UPPER(xc) ((xc)<128 ? (html_ctf[xc]&HTCX_UPPR) : 0)
#define HTML_IS_LOWER(xc) ((xc)<128 ? (html_ctf[xc]&HTCX_LOWR) : 0)
#define xv_err(xf) ((xf)->xv_err)

/***************************************************************/
static void Html_get_errline(struct htmls_file * xf,
                         struct html_stream * xs,
                         int nextend) {
/*
*/
    HTMLC xc;
    int fbufix;
    int errix;
    int errmax;
    int done;
    unsigned char cb[HTTPCMAXSZ];

    done = 0;
    errix = 0;
    xf->xv_errline[0] = '\0';
    fbufix = xs->xf_fbufix;
    errmax = sizeof(xf->xv_errline) - 5;

    errix = htmlctochar(xf->xv_errline, xs->xf_lbuf, xs->xf_lbuflen, errmax);
    if (errix + nextend < errmax) errmax = errix + nextend;

    if (xs->xf_iseof || fbufix == xs->xf_fbuflen) return;

    if (xs->xf_tadv) {
        fbufix++;
    }

    while (!done) {
        memset(cb, 0, HTTPCMAXSZ);
        cb[0] = xs->xf_fbuf[fbufix];
        if (xs->xf_encoding & 2 || (!xs->xf_encoding && (cb[0] & 0x80))) {
            xs->xf_fbufix++;
            if (fbufix < xs->xf_fbuflen) {
                cb[1] = xs->xf_fbuf[fbufix];
            }

            if (xs->xf_encoding == HTML_ENC_UNICODE_BE) {
                xc = (HTMLC)((cb[0] << 8) | cb[1]);
            } else if (xs->xf_encoding == HTML_ENC_UNICODE_LE) {
                xc = (HTMLC)((cb[1] << 8) | cb[0]);
            } else { /* UTF-8 */
                /* See also get_utf8char() */
                if (cb[0] < 0xE0) {
                    /* 2-byte sequence */
                    xc = (HTMLC)(((cb[0] & 0x1F) << 6) |
                                 (cb[1] & 0x3F));
                } else {
                    fbufix++;
                    if (fbufix < xs->xf_fbuflen) {
                        cb[2] = xs->xf_fbuf[fbufix];
                    }

                    if (cb[0] < 0xF0) {
                        /* 3-byte sequence */
                        xc = (HTMLC)(((cb[0] & 0x0F) << 12) |
                                    ((cb[1] & 0x3F) <<  6) |
                                     (cb[2] & 0x3F));
                    } else {
                        /* Assume 4-byte sequence */
                        fbufix++;
                        if (fbufix < xs->xf_fbuflen) {
                            cb[3] = xs->xf_fbuf[fbufix];
                        }

                        xc = (HTMLC)(((cb[0] & 0x07) << 18) |
                                    ((cb[1] & 0x3F) << 12) |
                                    ((cb[2] & 0x3F) <<  6) |
                                     (cb[3] & 0x3F));
                    }
                }
            }
            errix += htmlctochar(xf->xv_errline + errix, &xc, 1, 5);
        } else {
            xf->xv_errline[errix] = cb[0];
            errix++;
            xf->xv_errline[errix] = '\0';;
        }
        if (errix >= errmax || fbufix == xs->xf_fbuflen) done = 1;
        else fbufix++;
    }
}
/***************************************************************/
static void Html_seterr_S(struct htmls_file * xf,
                         struct html_stream * xs,
                         int errnum,
                         HTMLC * eparm) {

    xf->xv_err = errnum;
    if (eparm) {
        qhtmlctostr(xf->xv_errparm, eparm, sizeof(xf->xv_errparm));
    } else {
        xf->xv_errparm[0] = '\0';
    }

    if (xs) {
        Html_get_errline(xf, xs, 32);
        xf->xv_errlinenum = xs->xf_linenum + 1;
    } else {
        xf->xv_errparm[0] = '\0';
        xf->xv_errlinenum = 0;
    }
}
/***************************************************************/
static void Html_seterr_s(struct htmls_file * xf,
                         struct html_stream * xs,
                         int errnum,
                         char * eparm) {

    xf->xv_err = errnum;
    if (eparm) {
        strmcpy(xf->xv_errparm, eparm, sizeof(xf->xv_errparm));
    } else {
        xf->xv_errparm[0] = '\0';
    }

    if (xs) {
        Html_get_errline(xf, xs, 32);
        xf->xv_errlinenum = xs->xf_linenum + 1;
    } else {
        xf->xv_errparm[0] = '\0';
        xf->xv_errlinenum = 0;
    }
}
/***************************************************************/
static void Html_seterr_C(struct htmls_file * xf,
                         struct html_stream * xs,
                         int errnum,
                         HTMLC eparm) {

    HTMLC ep[2];

    ep[0] = eparm;
    ep[1] = 0;

    Html_seterr_S(xf, xs, errnum, ep);
}
/***************************************************************/
static void Html_seterr(struct htmls_file * xf,
                       struct html_stream * xs,
                       int errnum) {

    Html_seterr_S(xf, xs, errnum, 0);
}
/***************************************************************/
#ifdef UNUSED
static void Html_seterr_i(struct htmls_file * xf,
                         struct html_stream * xs,
                         int errnum,
                         int parm) {

    char nbuf[16];
    HTMLC pbuf[16];

    sprintf(nbuf, "%d", parm);
    strtohtmlc(pbuf, nbuf, 16);

    Html_seterr_S(xf, xs, errnum, pbuf);
}
#endif
/***************************************************************/
static void Html_fread(struct html_stream * xs, int bufix) {

    if (!xs->xf_extFile) {
        xs->xf_iseof = 1;
    } else {
        if (xs->xf_xf->xq_fread) {
            xs->xf_fbuflen = xs->xf_xf->xq_fread(xs->xf_extFile->fref,
                    xs->xf_fbuf + bufix, xs->xf_fbufsize - bufix,
                    xs->xf_errmsg, XF_ERRMSG_SZ);
            if (xs->xf_fbuflen <= 0) {
                xs->xf_iseof = 1;
            }
            xs->xf_fbuflen += bufix;
            xs->xf_fbufix   = bufix;
        } else {
            xs->xf_iseof = 1;
        }
    }
}
/***************************************************************/
#ifdef UNUSED
static int HTMLC_Char(HTMLC xc) {
/*
*/
    return (0);
}
#endif
/***************************************************************/
static int HTMLC_RChar(HTMLC xc) {
/*
[2a]    RestrictedChar    ::=    [#x1-#x8] | [#xB-#xC] | [#xE-#x1F] |
            [#x7F-#x84] | [#x86-#x9F]
*/
    int ok;

    ok = (xc >= 0x0001 && xc <= 0x0008) ||
         (xc >= 0x000B && xc <= 0x000C) ||
         (xc >= 0x000E && xc <= 0x001F) ||
         (xc >= 0x007F && xc <= 0x0084) ||
         (xc >= 0x0086 && xc <= 0x009F);

    return (ok);
}
/***************************************************************/
int HTMLC_NStart(HTMLC xc) {
/*
*/
    int ok;

    ok = (xc == ':' || xc == '_') ||
         (xc >= 'A' && xc <= 'Z') ||
         (xc >= 'a' && xc <= 'a') ||
         (xc >= 0x00D8 && xc <= 0x00F6) ||
         (xc >= 0x00F8 && xc <= 0x02FF) ||
         (xc >= 0x0370 && xc <= 0x00F6) ||
         (xc >= 0x037F && xc <= 0x1FFF) ||
         (xc >= 0x200C && xc <= 0x200D) ||
         (xc >= 0x2070 && xc <= 0x218F) ||
         (xc >= 0x2C00 && xc <= 0x2FEF) ||
         (xc >= 0x3001 && xc <= 0xD7FF) ||
         (xc >= 0xF900 && xc <= 0xFDCF) ||
         (xc >= 0xFDF0 && xc <= 0xFFFD);

    return (ok);
}
/***************************************************************/
static int HTMLC_Name(HTMLC xc) {
/*
[4a]    NameChar    ::=    NameStartChar | "-" | "." | [0-9] | #xB7 |
            [#x0300-#x036F] | [#x203F-#x2040]
*/
    int ok;

    ok = HTMLC_NStart(xc) ||
         (xc == '-' || xc == '.' || xc == 0xB7) ||
         (xc >= '0' && xc <= '9') ||
         (xc >= 0x0300 && xc <= 0x036F) ||
         (xc >= 0x203F && xc <= 0x2040);

    return (ok);
}
/***************************************************************/
static void Html_unget_char(struct html_stream * xs) {

    xs->xf_fbufix = xs->xf_prev_fbufix;
    xs->xf_tadv = 0;
    if (xs->xf_lbuflen) xs->xf_lbuflen--;
}
/***************************************************************/
static HTMLC Html_get_char(struct html_stream * xs) {
/*
    Code range
    hexadecimal      UTF-8
    000000 - 00007F  0xxxxxxx
    000080 - 0007FF  110xxxxx 10xxxxxx
    000800 - 00FFFF  1110xxxx 10xxxxxx 10xxxxxx
    010000 - 10FFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

*/
    HTMLC xc;
    unsigned char cb[HTTPCMAXSZ];

    if (xs->xf_iseof) return (HTMLCEOF);

    if (xs->xf_tadv) {
        xs->xf_fbufix++;
    }

    if (xs->xf_fbufix == xs->xf_fbuflen) {
        Html_fread(xs, 0);
        if (xs->xf_iseof) return (HTMLCEOF);
    }

    xs->xf_prev_fbufix = xs->xf_fbufix;
    cb[0] = xs->xf_fbuf[xs->xf_fbufix];
    xs->xf_tadv = 1;

    if (xs->xf_encoding & 2 || (!xs->xf_encoding && (cb[0] & 0x80))) {
        xs->xf_fbufix++;
        if (xs->xf_fbufix == xs->xf_fbuflen) {
            Html_fread(xs, 1);
            if (xs->xf_iseof) return (HTMLCEOF);
            xs->xf_fbuf[0]      = cb[0];
            xs->xf_prev_fbufix  = 0;
        }
        cb[1] = xs->xf_fbuf[xs->xf_fbufix];

        if (xs->xf_encoding == HTML_ENC_UNICODE_BE) {
            xc = (HTMLC)((cb[0] << 8) | cb[1]);
        } else if (xs->xf_encoding == HTML_ENC_UNICODE_LE) {
            xc = (HTMLC)((cb[1] << 8) | cb[0]);
        } else { /* UTF-8 */
            /* See also get_utf8char() */
            if (cb[0] < 0xE0) {
                /* 2-byte sequence */
                xc = (HTMLC)(((cb[0] & 0x1F) << 6) |
                             (cb[1] & 0x3F));
            } else {
                xs->xf_fbufix++;
                if (xs->xf_fbufix == xs->xf_fbuflen) {
                    Html_fread(xs, 2);
                    if (xs->xf_iseof) return (HTMLCEOF);
                    xs->xf_fbuf[0]      = cb[0];
                    xs->xf_fbuf[1]      = cb[1];
                    xs->xf_prev_fbufix  = 0;
                }
                cb[2] = xs->xf_fbuf[xs->xf_fbufix];

                if (cb[0] < 0xF0) {
                    /* 3-byte sequence */
                    xc = (HTMLC)(((cb[0] & 0x0F) << 12) |
                                ((cb[1] & 0x3F) <<  6) |
                                 (cb[2] & 0x3F));
                } else {
                    /* Assume 4-byte sequence */
                    xs->xf_fbufix++;
                    if (xs->xf_fbufix == xs->xf_fbuflen) {
                        Html_fread(xs, 3);
                        if (xs->xf_iseof) return (HTMLCEOF);
                        xs->xf_fbuf[0]      = cb[0];
                        xs->xf_fbuf[1]      = cb[1];
                        xs->xf_fbuf[2]      = cb[2];
                        xs->xf_prev_fbufix  = 0;
                    }
                    cb[3] = xs->xf_fbuf[xs->xf_fbufix];

                    xc = (HTMLC)(((cb[0] & 0x07) << 18) |
                                ((cb[1] & 0x3F) << 12) |
                                ((cb[2] & 0x3F) <<  6) |
                                 (cb[3] & 0x3F));
                }
            }
        }
    } else {
        xc = (HTMLC)(cb[0]);
    }

    if (xc == '\n' || xc == '\r') {
        xs->xf_lbuflen = 0;
        if (xc == '\n') {
            xs->xf_linenum++;
        }
    } else if (xs->xf_lbuflen || !HTML_IS_WHITE(xc)) {
        if (xs->xf_lbuflen == xs->xf_lbufsize) {
            xs->xf_lbufsize += HTML_K_LBUF_INC;
            xs->xf_lbuf = Realloc(xs->xf_lbuf, HTMLC, xs->xf_lbufsize);
        }
        xs->xf_lbuf[xs->xf_lbuflen] = xc;
        xs->xf_lbuflen++;
    }

    return (xc);
}
/***************************************************************/
static HTMLC Html_peek_char(struct html_stream * xs) {

    HTMLC xc;

    xc = Html_get_char(xs);
    Html_unget_char(xs);

    return (xc);
}
/***************************************************************/
static HTMLC Html_get_nwchar(struct html_stream * xs) {

    HTMLC xc;

    do {
        xc = Html_get_char(xs);
    } while (HTML_IS_WHITE(xc));
    if (xc == HTMLCEOF) return (0);

    return (xc);
}
/***************************************************************/
static HTMLC Html_peek_nwchar(struct html_stream * xs, int * nwhite)
{
    HTMLC xc;

    (*nwhite) = 0;
    xc = Html_get_char(xs);

    while (HTML_IS_WHITE(xc)) {
        (*nwhite)++;
        xc = Html_get_char(xs);
    }
    if (xc == HTMLCEOF) return (0);

    Html_unget_char(xs);

    return (xc);
}
/***************************************************************/
static void HTMLC_token_to_utf8(
    HTMLC * HTMLC_tbuf,
    int     HTMLC_tbuflen,
    char ** char_utf8tbuf,
    int   * char_utf8tbuflen,
    int   * char_utf8tbufsize,
    int    chcase)
{
/*
** See also: put_utf8char()
** chcase:
**      0 - No change
**      1 - Make uppercase
**      2 - Make lowercase
*/
    int ii;

    if ((*char_utf8tbuflen) + (HTMLC_tbuflen+1) * 4 >= (*char_utf8tbufsize)) {
        (*char_utf8tbufsize) =
            (*char_utf8tbuflen) + ((HTMLC_tbuflen + 1) * 4) + HTML_K_UBUF_INC;
        (*char_utf8tbuf) = Realloc((*char_utf8tbuf),char,(*char_utf8tbufsize));
    }

    for (ii = 0; ii < HTMLC_tbuflen; ii++ ) {
        if (HTMLC_tbuf[ii] < 0x80) {
            if ((chcase & CHCASE_TO_UPPER) && HTML_IS_LOWER(HTMLC_tbuf[ii])) {
                (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                    (unsigned char)((HTMLC_tbuf[ii] - 32) & 0xFF);
            } else if ((chcase & CHCASE_TO_LOWER) &&
                       HTML_IS_UPPER(HTMLC_tbuf[ii])) {
                (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                    (unsigned char)((HTMLC_tbuf[ii] + 32) & 0xFF);
            } else {
                (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                    (unsigned char)(HTMLC_tbuf[ii] & 0xFF);
            }
        } else if (HTMLC_tbuf[ii] <= 0x7FF) {
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)((HTMLC_tbuf[ii] >> 6) + 0xC0);
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)((HTMLC_tbuf[ii] & 0x3F) + 0x80);
        } else if (HTMLC_tbuf[ii] <= 0xFFFF) {
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)((HTMLC_tbuf[ii] >> 12) + 0xE0);
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)(((HTMLC_tbuf[ii] >> 6) & 0x3F) + 0x80);
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)((HTMLC_tbuf[ii] & 0x3F) + 0x80);
/* -- Needs 32 bit input character
        } else if (HTMLC_tbuf[ii] <= 0x10FFFF) {
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)((HTMLC_tbuf[ii] >> 18) + 0xF0);
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)(((HTMLC_tbuf[ii] >> 12) & 0x3F) + 0x80);
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)(((HTMLC_tbuf[ii] >> 6) & 0x3F) + 0x80);
            (*char_utf8tbuf)[((*char_utf8tbuflen))++] =
                (unsigned char)((HTMLC_tbuf[ii] & 0x3F) + 0x80);
*/
        } else {
            /* error("invalid code_point"); */
        }
    }
    (*char_utf8tbuf)[(*char_utf8tbuflen)] = '\0';
}
/***************************************************************/
static void Html_put_tbuf_char(HTMLC xc, struct html_stream * xs)
{
    HTMLC_token_to_utf8(
        &xc,
        1,
        &(xs->htss_utf8tbuf),
        &(xs->htss_utf8tbuflen),
        &(xs->htss_utf8tbufsize),
        0);
}
/***************************************************************/
static void Html_set_tbuf_length(struct html_stream * xs, int newtbuflen)
{
    if (newtbuflen <= 0) {
        xs->htss_utf8tbuflen = 0;
    } else if (newtbuflen < xs->htss_utf8tbuflen) {
        xs->htss_utf8tbuflen = newtbuflen;
    }
    xs->htss_utf8tbuf[xs->htss_utf8tbuflen] = '\0';
}
/***************************************************************/
static char * Html_next_token_name(HTMLC xc, struct html_stream * xs) {

    int done;

    done   = 0;
    do {
        Html_put_tbuf_char(xc, xs);

        xc = Html_get_char(xs);
        if (!HTML_IS_NAME(xc)) {
            if (xc != HTMLCEOF) Html_unget_char(xs);
            done = 1;
        }
    } while (!done);

    return (xs->htss_utf8tbuf);
}
/***************************************************************/
static char * Html_next_token_quote_raw(HTMLC xc, struct html_stream * xs) {

    int done;
    HTMLC quot;

    quot = xc;
    xc   = '\"';
    done   = 0;
    do {
        Html_put_tbuf_char(xc, xs);

        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            done = 1;
        } else if (xc == quot) {
            Html_put_tbuf_char('\"', xs);
            done = 1;
        }
    } while (!done);

    return (xs->htss_utf8tbuf);
}
/***************************************************************/
static char * Html_next_token_raw(struct html_stream * xs) {

    int done;
    HTMLC xc;

    done   = 0;
    xs->htss_utf8tbuflen = 0;

    do {
        xc = Html_get_char(xs);
    } while (HTML_IS_WHITE(xc));
    if (xc == HTMLCEOF) return (NULL);

    if (HTML_IS_NSTART(xc)) {
        Html_next_token_name(xc, xs);
    } else if (HTML_IS_QUOTE(xc)) {
        Html_next_token_quote_raw(xc, xs);
    } else {
        Html_put_tbuf_char(xc, xs);
    }

    Html_put_tbuf_char(0, xs);

#if 0
    Html_token_to_utf8(xs, 0);
#endif

    return (xs->htss_utf8tbuf);
}
/***************************************************************/
static char * htpp_next_Utf8Name(struct htmls_file * xf,
                           struct html_stream * xs,
                           int chcase) {

    int ii;
    int len;
    char *tok;

    xs->htss_utf8tbuflen = 0;

    tok = Html_next_token_raw(xs);
    if (!tok) return (0);

    if (!HTML_IS_NSTART(tok[0])) {
        Html_seterr_s(xf, xs, HTMLERR_BADNAME, tok);
        return (0);
    }

    /* validate string for pubid chars */
    len = IStrlen(tok);
    for (ii = 0; ii < len; ii++) {
        if (!HTML_IS_NAME(tok[ii])) {
            Html_seterr_s(xf, xs, HTMLERR_BADNAME, tok);
            return (0);
        } else if ((chcase & CHCASE_TO_UPPER) && HTML_IS_LOWER(tok[ii])) {
            tok[ii] = (unsigned char)((tok[ii] - 32) & 0xFF);
        } else if ((chcase & CHCASE_TO_LOWER) && HTML_IS_UPPER(tok[ii])) {
            tok[ii] = (unsigned char)((tok[ii] + 32) & 0xFF);
        }
    }

    return (tok);
}
/***************************************************************/
#ifdef UNUSED
static char * htpp_next_LowerUtf8Name(struct htmls_file * xf,
                           struct html_stream * xs) {

    char *tok;

    tok = htpp_next_Utf8Name(xf, xs, CHCASE_TO_LOWER);
    if (!tok) return (0);

    /* Html_token_to_utf8(xs, CHCASE_TO_LOWER); */

    return (xs->htss_utf8tbuf);
}
#endif
/***************************************************************/
static void htpp_Comment(struct htmls_file * xf,
                          struct html_stream * xs) {
/*
HTML 5.2 8.1.6. Comments:
Comments must have the following format:
1.The string "<!--"
2.Optionally, text, with the additional restriction that the text must not
    start with the string ">", nor start with the string "->", nor contain the
    strings "<!--", "-->", or "--!>", nor end with the string "<!-".
3.The string "-->"
*/
    int done;
    HTMLC xc;
    int nm;

    done = 0;
    nm   = 0;
    while (!done) {
        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            done = 1;
        } else if (xc == '-') {
            nm++;
        } else if (xc == '>') {
            if (nm >= 2) done = 1;
            else         nm   = 0;
        } else {
            nm = 0;
        }
    }
    Html_unget_char(xs);
}
/***************************************************************/
static void htut_set_Utf8attrNam(struct htmls_file * xf,
                            struct html_stream * xs,
                            char * attrNam) {

    int xLen;
    int nA;
    int ii;

    nA = xf->htsf_nAttrs;

    if (nA == xf->htsf_nAttrsSize) {
        xf->htsf_nAttrsSize += 4;
        xf->htsf_attrNames =
            Realloc(xf->htsf_attrNames, char *, xf->htsf_nAttrsSize);
        xf->htsf_attrVals =
            Realloc(xf->htsf_attrVals , char *, xf->htsf_nAttrsSize);
        xf->htsf_attrLens =
            Realloc(xf->htsf_attrLens , int, xf->htsf_nAttrsSize);
        xf->htsf_attrNamesSize =
            Realloc(xf->htsf_attrNamesSize , int, xf->htsf_nAttrsSize);
        xf->htsf_attrValsSize =
            Realloc(xf->htsf_attrValsSize , int, xf->htsf_nAttrsSize);

        for (ii = nA; ii < xf->htsf_nAttrsSize; ii++) {
            xf->htsf_attrNames[ii]        = 0;
            xf->htsf_attrVals[ii]         = 0;
            xf->htsf_attrLens[ii]         = 0;
            xf->htsf_attrNamesSize[ii]    = 0;
            xf->htsf_attrValsSize[ii]     = 0;
        }
    }

    xLen = IStrlen(attrNam) + 1;
    if (xLen >= xf->htsf_attrNamesSize[nA]) {
        xf->htsf_attrNamesSize[nA] = xLen + 32;
        xf->htsf_attrNames[nA] =
            Realloc(xf->htsf_attrNames[nA], char, xf->htsf_attrNamesSize[nA]);
    }
    memcpy(xf->htsf_attrNames[nA], attrNam, xLen);

    xf->htsf_nAttrs++;
}
/***************************************************************/
static void htup_expand_cbuf(HTMLC ** cbuf,
                               int   * cbufSize) {

    *cbufSize += HTML_K_CBUF_INC;
    *cbuf = Realloc(*cbuf, HTMLC, *cbufSize);
}
/***************************************************************/
#ifdef UNUSED
static void htup_copy_tbuf(struct htmls_file * xf,
                           struct html_stream * xs,
                               HTMLC ** cbuf,
                               int   * cbufSize,
                               int   * cbufLen) {

    HTMLC xc;
    int ii;

    *cbufLen = 0;
    for (ii = 0; ii < xs->DEPRECATED_xf_tbuflen; ii++) {
        xc = xs->DEPRECATED_xf_tbuf[ii];
        if (*cbufLen >= *cbufSize) {
            htup_expand_cbuf(cbuf, cbufSize);
        }
        (*cbuf)[*cbufLen] = xc;
        (*cbufLen)++;
    }

    /* Add zero terminator */
    if (*cbufLen >= *cbufSize) {
        htup_expand_cbuf(cbuf, cbufSize);
    }
    (*cbuf)[*cbufLen] = 0;
}
#endif
/***************************************************************/
static int count_html_entities(void)
{
/*
struct html_entity_def {
    char *          hty_entity;
    int             hty_num;
} html_entlist[] = {
*/
    int tcount = 0;

    while (html_entlist[tcount].hty_entity) {
#if IS_DEBUG
        if (tcount) {
            if (strcmp(html_entlist[tcount - 1].hty_entity,
                       html_entlist[tcount].hty_entity) > 0) {
                printf(
        "**** count_html_entities(): entities not in order at %d: %s\n",
                    tcount, html_entlist[tcount].hty_entity);
            }
        }
#endif
        tcount++;
    }

    return (tcount);
}
/***************************************************************/
static int find_html_entity_ix(const char * hentity)
{
/*
** Returns index within html_entlist[] of hentity.
** Returns 0 if not found. (html_entlist[0] is "")
*/
    static int num_entities = 0;
    int lo;
    int hi;
    int mid;

    if (!num_entities) {
        num_entities = count_html_entities();
    }

    lo = 0;
    hi = num_entities - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(hentity, html_entlist[mid].hty_entity) <= 0)
                                             hi = mid - 1;
        else                                 lo = mid + 1;
    }

    if (lo == num_entities ||
        strcmp(hentity, html_entlist[lo].hty_entity)) lo = 0;

    return (lo);
}
/***************************************************************/
static int find_html_entity(const char * hentity)
{
    int ix;
    int llen;
    int any_upper;
    char lentity[MAX_ENTITY_LENGTH + 2];

    ix = find_html_entity_ix(hentity);
    if (!ix) {
        /* Try only lowercase - Entities are case-sensitive, but we will */
        /* try to be tolerant and look again if their entity has any     */
        /* uppercase characters. e.g. &NBSP; gets found as &nbsp;        */
        any_upper = 0;
        llen = 0;
        while (hentity[llen] && llen < MAX_ENTITY_LENGTH) {
            if (hentity[llen] >= 'A' && hentity[llen] <= 'Z') {
                any_upper = 1;
                lentity[llen] = hentity[llen] + 32;
            } else {
                lentity[llen] = hentity[llen];
            }
            llen++;
        }
        if (any_upper && llen < MAX_ENTITY_LENGTH) {
            lentity[llen] = '\0';
            ix = find_html_entity_ix(lentity);
        }
    }

    return (ix);
}
/***************************************************************/
#ifdef OLD_WAY
static void Html_next_char_ref(struct htmls_file * xf,
                       struct html_stream * xs) {
/*
*/
    HTMLC xc;
    unsigned int num;
    int ix;

    num = 0;
    xc = 0;
    ix = 1; /* skip over # */

    if (ix < xf->DEPRECATED_xf_ebuflen && xf->DEPRECATED_xf_ebuf[ix] == 'x') {
        ix++;
        while (ix < xf->DEPRECATED_xf_ebuflen) {
            xc = xf->DEPRECATED_xf_ebuf[ix++];
            if (xc >= '0' && xc <= '9') {
                num = (num << 4) | (xc - '0');
            } else if (xc >= 'A' && xc <= 'F') {
                num = (num << 4) | (xc - '7');
            } else if (xc >= 'a' && xc <= 'f') {
                num = (num << 4) | (xc - 'W');
            } else {
                Html_seterr(xf, xs, HTMLERR_BADCHREF);
                return;
            }
        }
    } else {
        while (ix < xf->DEPRECATED_xf_ebuflen) {
            xc = xf->DEPRECATED_xf_ebuf[ix++];
            if (xc >= '0' && xc <= '9') {
                num = (num * 10) + (xc - '0');
            } else {
                Html_seterr(xf, xs, HTMLERR_BADCHREF);
                return;
            }
        }
    }

    /* check to make certain at least one character processed */
    if (!xc) {
        Html_seterr(xf, xs, HTMLERR_BADCHREF);
        return;
    }

    xc = (HTMLC)num;
    if (xs->DEPRECATED_xf_tbuflen == xs->DEPRECATED_xf_tbufsize) {
        Html_expand_tbuf(xs);
    }
    xs->DEPRECATED_xf_tbuf[xs->DEPRECATED_xf_tbuflen] = xc;
    xs->DEPRECATED_xf_tbuflen++;
}
#endif
/***************************************************************/
#ifdef OLD_WAY
static void Html_next_token_ref(struct htmls_file * xf,
                               struct html_stream * xs,
                               int depth) {

    struct ht_entity * en;
    struct ht_entity ** pen;
    int ii;
    HTMLC xc;
    int state;
    int last;

    if (depth > HTML_MAX_ENTITY_RECUR) {
        Html_seterr(xf, xs, HTMLERR_RECURENTITY);
        return;
    }

    if (xf->xf_ebuflen && xf->xf_ebuf[0] == '#') {
        Html_next_char_ref(xf, xs);
        return;
    }

    pen = (struct ht_entity **)dbtree_find(xf->xs_g_entities->xe_entity,
                            xf->xf_ebuf, HTMLC_BYTES(xf->xf_ebuflen + 1));
    if (!pen) {
        Html_seterr_S(xf, xs, HTMLERR_NOSUCHENTITY, xf->xf_ebuf);
    } else {
        en = *pen;
        state = 0;
        last = en->e_len - en->e_ix;
        for (ii = en->e_ix; ii < last; ii++) {
            xc = en->e_val[ii];
            if (state) {
                Html_put_entity_char(xc, xf, xs, depth);
                if (xv_err(xf)) return;
                if (xc == ';') state = 2;
/*
**** Removed 1/25/2016: &amp; did not work.
            } else if (xc == '&') {
                xf->xf_ebuflen = 0;
                state = 1;
*/
            }
            if (!state) {
                if (xs->DEPRECATED_xf_tbuflen == xs->DEPRECATED_xf_tbufsize) {
                    Html_expand_tbuf(xs);
                }
                xs->DEPRECATED_xf_tbuf[xs->DEPRECATED_xf_tbuflen] = xc;
                xs->DEPRECATED_xf_tbuflen++;
            } else if (state == 2) {
                state = 0;
            }
        }
    }
}
#endif
/***************************************************************/
static void Html_next_char_ref(struct htmls_file * xf,
                       struct html_stream * xs) {
/*
*/
    HTMLC xc;
    unsigned int num;
    int ix;

    num = 0;
    xc = 0;
    ix = 1; /* skip over # */

    if (ix < xf->htsf_ebuflen && xf->htsf_ebuf[ix] == 'x') {
        ix++;
        while (ix < xf->htsf_ebuflen) {
            xc = xf->htsf_ebuf[ix++];
            if (xc >= '0' && xc <= '9') {
                num = (num << 4) | (xc - '0');
            } else if (xc >= 'A' && xc <= 'F') {
                num = (num << 4) | (xc - '7');
            } else if (xc >= 'a' && xc <= 'f') {
                num = (num << 4) | (xc - 'W');
            } else {
                Html_seterr(xf, xs, HTMLERR_BADCHREF);
                return;
            }
        }
    } else {
        while (ix < xf->htsf_ebuflen) {
            xc = xf->htsf_ebuf[ix++];
            if (xc >= '0' && xc <= '9') {
                num = (num * 10) + (xc - '0');
            } else {
                Html_seterr(xf, xs, HTMLERR_BADCHREF);
                return;
            }
        }
    }

    /* check to make certain at least one character processed */
    if (!xc) {
        Html_seterr(xf, xs, HTMLERR_BADCHREF);
        return;
    }

    xc = (HTMLC)num;
    Html_put_tbuf_char(xc, xs);
}
/***************************************************************/
static void Html_next_token_ref(struct htmls_file * xf,
                               struct html_stream * xs)
{
    int ix;

    if (xf->htsf_ebuflen && xf->htsf_ebuf[0] == '#') {
        Html_next_char_ref(xf, xs);
        return;
    }

    ix = find_html_entity(xf->htsf_ebuf);
    if (!ix) {
        Html_seterr_s(xf, xs, HTMLERR_NOSUCHENTITY, xf->htsf_ebuf);
    } else {
        Html_put_tbuf_char((HTMLC)html_entlist[ix].hty_num, xs);
    }
}
/***************************************************************/
static int Html_put_entity_char(HTMLC xc,
                               struct htmls_file * xf,
                               struct html_stream * xs)
{
    int enterr;

    enterr = 0;

    if (xc == ';') {
        if (!xf->htsf_ebuflen) {
            enterr = 1;
        } else {
            if (!xf->htsf_ebufsize) {
                xf->htsf_ebufsize += HTML_K_EBUF_INC;
                xf->htsf_ebuf = Realloc(xf->htsf_ebuf, char, xf->htsf_ebufsize);
            }
            xf->htsf_ebuf[xf->htsf_ebuflen] = 0;
            Html_next_token_ref(xf, xs);
        }
    } else if (xc < 128) {
        if (!xf->htsf_ebuflen) {
            if (!HTML_IS_NSTART(xc) &&  xc != '#') {
                enterr = 1;
            }
        } else if (!HTML_IS_NAME(xc)) {
            enterr = 1;
        }

        if (!enterr) {
            if (xf->htsf_ebuflen + 1 >= xf->htsf_ebufsize) {
                xf->htsf_ebufsize += HTML_K_EBUF_INC;
                xf->htsf_ebuf = Realloc(xf->htsf_ebuf, char, xf->htsf_ebufsize);
            }
            xf->htsf_ebuf[xf->htsf_ebuflen] = (char)(xc & 0x7F);
            xf->htsf_ebuflen++;
        }
    } else {
        enterr = 1;
    }

    if (enterr) {
        if (xf->xv_vFlags & HTMLVFLAGS_TOLERANT) {
            HTMLC_token_to_utf8(
                &xc,
                1,
                &(xf->htsf_ebuf),
                &(xf->htsf_ebuflen),
                &(xf->htsf_ebufsize),
                0);
        } else {
            Html_seterr_C(xf, xs, HTMLERR_BADGEREF, xc);
        }
    }

    return (enterr);
}
/***************************************************************/
static void Html_move_entity_to_tbuf(struct htmls_file * xf,
                               struct html_stream * xs)
{
    int ii;

    Html_put_tbuf_char('&', xs);
    for (ii = 0; ii < xf->htsf_ebuflen; ii++) {
        Html_put_tbuf_char(xf->htsf_ebuf[ii], xs);
    }
}
/***************************************************************/
static int htup_p_content(struct htmls_file * xf) {

    int result = 0;

    xf->xc_ielements[xf->xi_depth - 1]->xc_pcdata = 1;

    if (xf->xi_p_content) {
        result = xf->xi_p_content(xf->xi_vp,
                xf->htsf_contentLen,
                xf->htsf_content);
    }

    return (result);
}
/***************************************************************/
static int htup_cdata(struct htmls_file * xf,
                       struct html_stream * xs) {

    int done;
    HTMLC xc;
    HTMLC pc;
    int nc;
    int result;

    done = 0;
    nc   = 0;
    xf->htsf_contentLen = 0;

    while (!done) {
        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            done = 1;
        } else if (xc == ']') {
            nc++;
        } else if (xc == '>' && nc >= 2) {
            done = 1;
        } else {
            while (nc) {
                pc = ']';
                HTMLC_token_to_utf8(
                    &pc,
                    1,
                    &(xf->htsf_content),
                    &(xf->htsf_contentLen),
                    &(xf->htsf_contentSize),
                    0);
                nc--;
            }
            HTMLC_token_to_utf8(
                &xc,
                1,
                &(xf->htsf_content),
                &(xf->htsf_contentLen),
                &(xf->htsf_contentSize),
                0);
        }
    }

    result = htup_p_content(xf);

    return (result);
}
/***************************************************************/
static int htup_bang(struct htmls_file * xf,
                       struct html_stream * xs) {

    int result;
    HTMLC xc;
    char * tok;

    result = 0;
    xc = Html_get_char(xs); /* ! */
    if (xc == '-') {
        htpp_Comment(xf, xs);
        Html_get_char(xs); /* > */
    } else if (xc == '[') {
        tok = Html_next_token_raw(xs);
        if (!strcmp(tok, "CDATA")) {
            /* CDATA is not HTML, maybe should check if options allow CDATA */
            xc = Html_get_char(xs); /* ! */
            if (xc == '[') {
                result = htup_cdata(xf, xs);
            } else {
                Html_seterr_C(xf, xs, HTMLERR_EXPCTL, xc);
                result = HTMLRESULT_ERR;
            }
        } else {
            if (xf->xv_vFlags & HTMLVFLAGS_TOLERANT) {
                htpp_Comment(xf, xs);
                Html_get_char(xs); /* > */
            } else {
                /*
                ** Not sure this is correct. Maybe we should always assume
                ** a comment and not need HTMLVFLAGS_TOLERANT.
                */
                Html_seterr_C(xf, xs, HTMLERR_EXPCTL, xc);
                result = HTMLRESULT_ERR;
            }
        }
    } else {
        Html_unget_char(xs);
        tok = htpp_next_Utf8Name(xf, xs, CHCASE_TO_UPPER);
        if (!strcmp(tok, "DOCTYPE")) {
            htpp_doctypedecl(xf, xs);
            if (xv_err(xf)) result = HTMLRESULT_ERR;
        } else {
            Html_seterr_C(xf, xs, HTMLERR_EXPCTL, xc);
            result = HTMLRESULT_ERR;
        }
    }

    return (result);
}
/***************************************************************/
static void htpp_PI(struct htmls_file * xf,
                          struct html_stream * xs) {
/*
*/
    int done;
    HTMLC xc;
    int nm;

    done = 0;
    nm   = 0;
    while (!done) {
        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            done = 1;
        } else if (xc == '?') {
            nm++;
        } else if (xc == '>') {
            if (nm) done = 1;
            else    nm   = 0;
        } else {
            nm = 0;
        }
    }
}
/***************************************************************/
static int htup_PI(struct htmls_file * xf,
                       struct html_stream * xs) {

    int result;

    result = 0;
    htpp_PI(xf, xs);
    if (xv_err(xf)) result = HTMLRESULT_ERR;

    return (result);
}
/***************************************************************/
static struct ht_attlist * htpp_new_attlist(char * iName) {

    struct ht_attlist * att;

    att = New(struct ht_attlist, 1);
    att->a_elname = iName;
    att->a_attdefs = dbtree_new();

    return (att);
}
/***************************************************************/
static struct ht_attlist * htpp_element_attlist(struct htmls_file * xf,
                                      struct ht_element  * el) {

    struct ht_attlist * att;
    char * elNam;

    elNam = Strdup(xf->htsf_elNam);

    att = htpp_new_attlist(elNam);
    el->l_attlist = att;

    return (att);
}
/***************************************************************/
static int count_html_tags(void)
{
/*
**  struct htmhtag_def {
**      char *          htd_name;
**      enum eHTML_Tag  htd_tag;
**      int             htd_flags;
**  } html_taglist[] = {
*/
    int tcount = 0;

    while (HTML_ELEMENT_NAME(tcount)) {
#if IS_DEBUG
        if (html_taglist[tcount].htd_tag != tcount) {
            printf("**** count_html_tags(): tag not equal to count.\n");
        } else if (tcount) {
            if (strcmp(HTML_ELEMENT_NAME(tcount - 1),
                       HTML_ELEMENT_NAME(tcount)) > 0) {
                printf("**** count_html_tags(): tags not in order.\n");
            }
        }
#endif
        tcount++;
    }

    return (tcount);
}
/***************************************************************/
static int find_html_tag_ix(const char * htag)
{
/*
**  struct html_tag_def {
**      char *          htd_name;
**      enum eHTML_Tag  htd_tag;
**      int             htd_flags;
**  } html_taglist[] = {
*/
    static int num_tags = 0;
    int lo;
    int hi;
    int mid;

    if (!num_tags) {
        num_tags = count_html_tags();
    }

    lo = 0;
    hi = num_tags - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (strcmp(htag, html_taglist[mid].htd_name) <= 0)
                                             hi = mid - 1;
        else                                 lo = mid + 1;
    }

    if (lo == num_tags || strcmp(htag, html_taglist[lo].htd_name)) lo = 0;

    return (lo);
}
/***************************************************************/
static int find_html_tag(const char * htag)
{
    int ix;
    int any_upper;
    int tlen;
    char ltag[MAX_TAG_LENGTH + 2];

    ix = find_html_tag_ix(htag);
    if (!ix) {
        /* Try only lowercase - Tags are case-insensitive, so we will */
        /* look again if their tag has any uppercase characters.      */
        /* Though, htag should have been lowered already.             */
        any_upper = 0;
        tlen = 0;
        while (htag[tlen] && tlen < MAX_TAG_LENGTH) {
            if (htag[tlen] >= 'A' && htag[tlen] <= 'Z') {
                any_upper = 1;
                ltag[tlen] = htag[tlen] + 32;
            } else {
                ltag[tlen] = htag[tlen];
            }
            tlen++;
        }
        if (any_upper && tlen < MAX_TAG_LENGTH) {
            ltag[tlen] = '\0';
            ix = find_html_tag_ix(ltag);
        }
    }

    return (ix);
}
/***************************************************************/
static struct ht_element * Html_new_element(
        char * elName,
enum eHTML_Tags tagid) {
/*
*/
    struct ht_element * el;

    el = New(struct ht_element, 1);
    el->l_name       = elName;
    el->l_tagid      = tagid;
    el->l_flags      = html_taglist[tagid].htd_flags;
    el->l_attlist    = NULL;
    el->l_tree       = NULL;
    el->l_i_elements = NULL;

    return (el);
}
/***************************************************************/
static struct ht_element *  htpp_new_element(
        struct htmls_file * xf,
        enum eHTML_Tags tagid) {

    struct ht_element * el;
    char * elNam;

    if (tagid) elNam = NULL;
    else       elNam = Strdup(xf->htsf_elNam);

    el = Html_new_element(elNam, tagid);

    return (el);
}
/***************************************************************/
static void htut_set_Utf8elNam(struct htmls_file * xf, const char * elNam) {

    int xLen;

    xLen = IStrlen(elNam) + 1;
    if (xLen > xf->htsf_elNamSize) {
        xf->htsf_elNamSize = xLen + 32;
        xf->htsf_elNam =
            Realloc(xf->htsf_elNam, char, xf->htsf_elNamSize);
    }

    memcpy(xf->htsf_elNam, elNam, xLen);
}
/***************************************************************/
static struct ht_ielement * htup_new_ielement(void) {

    struct ht_ielement * xie;

    xie = New(struct ht_ielement, 1);

    xie->xc_mxElements      = 0;
    xie->xc_ncElements      = 0;
    xie->xc_element         = 0;
    xie->xc_pcdata          = 0;

    xie->xc_cElements       = 0;

    return (xie);
}
/***************************************************************/
static void htup_init_element(struct htmls_file * xf) {

    if (xf->xi_depth == xf->xc_maxDepth) {
        xf->xc_ielements  =
            Realloc(xf->xc_ielements , struct ht_ielement *, xf->xi_depth + 1);
        xf->xc_ielements[xf->xc_maxDepth] = htup_new_ielement();
        xf->xc_maxDepth = xf->xi_depth + 1;
    }

    xf->xc_ielements[xf->xi_depth]->xc_ncElements  = 0;
    xf->xc_ielements[xf->xi_depth]->xc_element     = 0;
    xf->xc_ielements[xf->xi_depth]->xc_pcdata      = 0;

    xf->htsf_nAttrs = 0;
    xf->htsf_contentLen = 0;
    xf->xi_depth++;
}
/***************************************************************/
static struct ht_element * htup_start_element(struct htmls_file * xf,
                           struct html_stream * xs,
                           const char * elNam,
                           enum eHTML_Tags tagid)
{
/*
*/

    struct ht_attlist * att;
    struct ht_element  * el;

    htup_init_element(xf);

    htut_set_Utf8elNam(xf, elNam);

    el = htpp_new_element(xf, tagid);

    att = htpp_element_attlist(xf, el);

    return (el);
}
/***************************************************************/
static void Html_free_ht_ielement(struct ht_ielement * xie) {

    Html_free_element(xie->xc_element);
}
/***************************************************************/
static void htup_end_element(struct htmls_file * xf) {

    int ii;
    struct ht_ielement * xie;

    xie = xf->xc_ielements[xf->xi_depth - 1];

    if (xf->xv_debug & HTDEBUG_SHOW_ELEMENTS) {
        printf("End element: %S\n", xie->xc_element->l_name);

        for (ii = 0; ii < xie->xc_ncElements; ii++) {
            printf("    :%S\n",
                xie->xc_cElements[ii]->l_name);
        }
    }

    Html_free_ht_ielement(xie);

    xf->xi_depth--;

#if XML_NOT_HTML
    if (xf->xv_sFlags & XMLSFLAG_IMPLYDTD) {
        xup_imply_contents(xf, xf->xc_ielements[xf->xi_depth]);
    }
#endif
}
/***************************************************************/
static void htup_terminate_element(struct htmls_file * xf,
                                    enum eHTML_Tags tagid)
{
    int result;

    result = 0;

    if (!xv_err(xf) && xf->xi_p_endtag) {
        result = xf->xi_p_endtag(xf->xi_vp, HTML_ELEMENT_NAME(tagid));
    }
    htup_end_element(xf);
}
/***************************************************************/
static int htup_handle_custom_end_tag(struct htmls_file * xf,
                           struct html_stream * xs,
                           enum eHTML_Tags tagid,
                           struct ht_element  * parent_el)
{
    int result;

    result = 0;
    switch (tagid) {
        case HTAG_DL:
            break;
        case HTAG_OL:
            if (parent_el->l_tagid == HTAG_LI) {
                htup_terminate_element(xf, tagid);
            }
            break;
        case HTAG_UL:
            if (parent_el->l_tagid == HTAG_LI) {
                htup_terminate_element(xf, tagid);
            }
            break;
        default:
            break;
    }

    return (result);
}
/***************************************************************/
static int htup_ETag_recover(struct htmls_file * xf,
                       struct html_stream * xs,
                           const char * elNam,
                           enum eHTML_Tags eltagid) {
/*
*/

    int result;
    int found;
    int nmissing;
    int maxmissing;
    struct ht_element  * parent_el;

    result = 0;
    nmissing = 0;
    maxmissing = 2;
    if (maxmissing >= xf->xi_depth) maxmissing = xf->xi_depth - 1;

    found = 0;
    while (!found && nmissing <= maxmissing) {
        parent_el = xf->xc_ielements[xf->xi_depth - nmissing - 1]->xc_element;
        if (parent_el->l_tagid == eltagid) {
            found = 1;
        } else {
            nmissing++;
        }
    }


    if (found) {
        while (nmissing >= 0) {
            if (xf->xi_p_endtag) {
                parent_el = xf->xc_ielements[xf->xi_depth - 1]->xc_element;
                result = xf->xi_p_endtag(xf->xi_vp,
                    HTML_ELEMENT_NAME(parent_el->l_tagid));
            }
            htup_end_element(xf);
            nmissing--;
        }
    }

    return (result);
}
/***************************************************************/
static int htup_ETag(struct htmls_file * xf,
                       struct html_stream * xs) {
/*
[42]    ETag    ::=    '</' Name S? '>'
*/

    int result;
    int tagerr;
    int exptag;
    char * elNam;
    enum eHTML_Tags tagid;
    HTMLC xc;
    struct ht_element  * parent_el;
    struct ht_element  * gparent_el;

    result = 0;
    tagerr = 0;
    parent_el = NULL;
    gparent_el = NULL;

    if (!xf->xi_depth) {
        Html_seterr(xf, xs, HTMLERR_UNEXPCLOSE);
        return (HTMLRESULT_ERR);
    }

    elNam = htpp_next_Utf8Name(xf, xs, CHCASE_TO_LOWER);
    if (!elNam) return (HTMLRESULT_ERR);
    tagid = find_html_tag(elNam);

    if (HTML_VOID_ELEMENT(tagid) && (xf->xv_vFlags & HTMLVFLAGS_TOLERANT)) {
        /* Ignore, Looks like </br> */
        return (result);
    }

    if (xf->xi_depth > 0) {
        parent_el = xf->xc_ielements[xf->xi_depth - 1]->xc_element;
    }
    if (xf->xi_depth > 1) {
        gparent_el = xf->xc_ielements[xf->xi_depth - 2]->xc_element;
    }

/*
if (HTML_CUST_END_ELEMENT(tagid) && parent_el) {
        result = htup_handle_custom_end_tag(xf, xs, tagid, parent_el);
        if (xv_err(xf)) return (HTMLRESULT_ERR);
    }
*/

    /* Make certain close matches */
    exptag = xf->xc_ielements[xf->xi_depth - 1]->xc_element->l_tagid;
    if (tagid != exptag) {
        if (HTML_AUTO_END_ELEMENT(exptag)) {
            result = htup_handle_auto_end(xf, xs,
                xf->xc_ielements[xf->xi_depth - 1]->xc_element);
            if (xv_err(xf)) return (HTMLRESULT_ERR);
        } else {
            tagerr = -1;
            if (xf->xv_vFlags & HTMLVFLAGS_TOLERANT) {
#if IS_DEBUG
                if (gparent_el) {
                    printf(
"* Mismatched end tag in line %d. Expecting </%s>, found </%s>, parent <%s>\n",
                        xs->xf_linenum,
                        HTML_ELEMENT_NAME(exptag), HTML_ELEMENT_NAME(tagid),
                        HTML_ELEMENT_NAME(gparent_el->l_tagid));
                } else {
                    printf(
"* Mismatched end tag in line %d. Expecting </%s>, found </%s>\n",
                        xs->xf_linenum,
                        HTML_ELEMENT_NAME(exptag), HTML_ELEMENT_NAME(tagid));
                }
#endif
            } else {
                Html_seterr(xf, xs, HTMLERR_UNEXPCLOSE);
                return (HTMLRESULT_ERR);
            }
        }
    } else if (!tagid) {
        if (strcmp(elNam,
                    xf->xc_ielements[xf->xi_depth-1]->xc_element->l_name)) {
            Html_seterr(xf, xs, HTMLERR_UNEXPCLOSE);
            return (HTMLRESULT_ERR);
        }
    }

    xc = Html_get_nwchar(xs);
    if (xc != '>') {
        if (!tagerr) {
            Html_seterr(xf, xs, HTMLERR_EXPCLOSE);
            return (HTMLRESULT_ERR);
        }
    }

    if (tagerr) {
        result = htup_ETag_recover(xf, xs, elNam, tagid);
    } else {
        if (xf->xi_p_endtag) {
            result = xf->xi_p_endtag(xf->xi_vp, elNam);
        }
        htup_end_element(xf);
    }

    return (result);
}
/***************************************************************/
static int Html_next_token_parsed_raw(struct htmls_file * xf,
                                    struct html_stream * xs,
                                    int leadingspaces,
                                    const char * elNam) {
    int result = 0;
    HTMLC xc;
    int done;
    int elix;

    while (leadingspaces) {
        Html_put_tbuf_char(' ', xs);
        leadingspaces--;
    }

    xs->htss_utf8tbuflen = 0;
    elix = 0;
    done = 0;
    while (!done) {
        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            Html_seterr(xf, xs, HTMLERR_UNEXPEOF);
            return (0);
        } else if (elix == 0) {
            if (xc == '<') elix = 1;
        } else if (elix == 1) {
            if (xc == '/') elix = 2;
            else elix = 0;
        } else if (elNam[elix - 2]) {
           if (HTML_IS_UPPER(xc)) {
               if (xc + 32 == elNam[elix - 2]) elix++;
               else elix = 0;
           } else {
               if (xc == elNam[elix - 2]) elix++;
               else elix = 0;
           }
        } else {    /* elNam[elix - 2] == 0 */
            if (xc == '>') done = 1;
            else elix = 0;
        }

        if (!done) {
            Html_put_tbuf_char(xc, xs);
        }
    }

    Html_set_tbuf_length(xs, xs->htss_utf8tbuflen - elix);

    return (result);
}
/***************************************************************/
static int Html_next_token_parsed_jsp(struct htmls_file * xf,
                                    struct html_stream * xs)
{
    int result = 0;
    HTMLC xc;
    enum e_pjsp_states {
          PJSP_CONTENT
        , PJSP_WHITE
        , PJSP_PERCENT
        , PJSP_ENTITY_STARTED
        , PJSP_DONE
    } state;

    state = PJSP_CONTENT;
    while (state != PJSP_DONE) {
        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            Html_seterr(xf, xs, HTMLERR_UNEXPEOF);
            return (0);
        } else if (HTML_IS_WHITE(xc)) {
            state = PJSP_WHITE;
        } else if (state == PJSP_WHITE) {
            if (xc == '%') state = PJSP_PERCENT;
            else state = PJSP_CONTENT;
        } else if (state == PJSP_PERCENT) {
            if (xc == '>') state = PJSP_DONE;
            else state = PJSP_CONTENT;
        }

        Html_put_tbuf_char(xc, xs);
    }

    return (result);
}
/***************************************************************/
static char * Html_next_token_parsed(struct htmls_file * xf,
                                    struct html_stream * xs,
                                    int leadingspaces,
                                    int tag_flags) {
/*
** partok:
**   0 - literal
**   1 - content
**   2 - content, xml:space="preserve"
*/

    HTMLC xc;
    HTMLC termc;
    int whitecount;
    int enterr;
    enum e_parse_states {
          PSTATE_INIT
        , PSTATE_IN_ENTITY
        , PSTATE_ENTITY_DONE
        , PSTATE_ENTITY_STARTED
        , PSTATE_DONE
    } state;

    state  = PSTATE_INIT;
    whitecount = 0;
    enterr = 0;
    xs->htss_utf8tbuflen = 0;

    if (leadingspaces) {
        Html_put_tbuf_char(' ', xs);
    }


    if (tag_flags & HTDF_ATTRIBUTE) {
        xc = Html_get_nwchar(xs);
        if (xc == '\'' || xc == '"') {
            termc = xc;
        } else {
             termc = 0;
             Html_unget_char(xs);
            /* Allow unquoted attribute values */
            /* Html_seterr(xf, xs, HTMLERR_EXPLITERAL); */
            /* return (0);                              */
        }
    } else {
        termc = '<';
    }

    while (state != PSTATE_DONE) {
        xc = Html_get_char(xs);
        if (xc == HTMLCEOF) {
            Html_seterr(xf, xs, HTMLERR_UNEXPEOF);
            return (0);
        } else if (xc == termc) {
            if (!(tag_flags & HTDF_ATTRIBUTE)) Html_unget_char(xs);
            state = PSTATE_DONE;
        } else if (!termc && (HTML_IS_WHITE(xc) || xc == '>')) {
            /* Unquoted attribute values */
            Html_unget_char(xs);
            state = PSTATE_DONE;
        } else {
            if (state == PSTATE_IN_ENTITY) {
                enterr = Html_put_entity_char(xc, xf, xs);
                if (xv_err(xf)) return (0);
                if (enterr) {
                    Html_move_entity_to_tbuf(xf, xs);
                    state = PSTATE_ENTITY_DONE;
                } else if (xc == ';') {
                    state = PSTATE_ENTITY_DONE;
                }
            } else if (state == PSTATE_ENTITY_STARTED) {
                if (xc == '#' || HTML_IS_ALPHA(xc)) {
                    enterr = Html_put_entity_char(xc, xf, xs);
                    if (xv_err(xf)) return (0);
                    if (enterr) {
                        Html_move_entity_to_tbuf(xf, xs);
                        state = PSTATE_ENTITY_DONE;
                    } else {
                        state = PSTATE_IN_ENTITY;
                    }
                } else {
                    Html_put_tbuf_char('&', xs);
                    Html_put_tbuf_char(xc, xs);
                    state = PSTATE_INIT;
                }
            } else if (xc == '&' && !(tag_flags & HTDF_ATTRIBUTE)) {
                xf->htsf_ebuflen = 0;
                state = PSTATE_ENTITY_STARTED;
            }

            if (state == PSTATE_INIT) {
                if ((tag_flags & HTDF_CONTENT) && HTML_IS_WHITE(xc) &&
                    !(tag_flags & HTDF_KEEP_WHITE)) {
                    if (whitecount) {
                        xc = 0;
                        whitecount++;
                    } else {
                        xc = ' ';
                        whitecount = 1;
                    }
                } else {
                    whitecount = 0;
                }
                if (xc) {
                    Html_put_tbuf_char(xc, xs);
                }
            } else if (state == PSTATE_ENTITY_DONE) {
                state = PSTATE_INIT;
            }
        }
    }

    Html_put_tbuf_char(0, xs);

    return (xs->htss_utf8tbuf);
}
/***************************************************************/
#ifdef UNUSED
static char * Html_next_Utf8token_parsed(struct htmls_file * xf,
                                    struct html_stream * xs,
                                    int leadingspaces,
                                    int tag_flags,
                                    int chcase) {

    char *tok;

    tok = Html_next_token_parsed(xf, xs, leadingspaces, tag_flags);
    if (!tok) return (0);

    /* Html_token_to_utf8(xs, chcase); */

    return (xs->htss_utf8tbuf);
}
/***************************************************************/
static void htup_AttValue(struct htmls_file * xf,
                       struct html_stream * xs) {

    int nA;

    nA = xf->DEPRECATED_xi_nAttrs - 1;

    htup_copy_tbuf(xf, xs,
        &(xf->DEPRECATED_xi_attrVals[nA]),
        &(xf->DEPRECATED_xi_attrValsSize[nA]),
        &(xf->DEPRECATED_xi_attrLens[nA]));
}
#endif
/***************************************************************/
static void htup_expand_utf8buf(char ** cbuf,
                               int   * cbufSize,
                               int new_size) {

    *cbufSize = new_size;
    *cbuf = Realloc(*cbuf, char, *cbufSize);
}
/***************************************************************/
static void htup_append_utf8tbuf(struct htmls_file * xf,
                           struct html_stream * xs,
                               char ** cbuf,
                               int   * cbufSize,
                               int   * cbufLen) {

    char xc;
    int ii;

    if ((*cbufLen) + xs->htss_utf8tbuflen + 1 >= *cbufSize) {
        htup_expand_utf8buf(cbuf, cbufSize,
            (*cbufLen) + xs->htss_utf8tbuflen + HTML_K_CBUF_INC);
    }

    for (ii = 0; ii < xs->htss_utf8tbuflen; ii++) {
        xc = xs->htss_utf8tbuf[ii];
        (*cbuf)[*cbufLen] = xc;
        (*cbufLen)++;
    }

    /* Add zero terminator */
    (*cbuf)[*cbufLen] = 0;
}
/***************************************************************/
static void htup_copy_utf8tbuf(struct htmls_file * xf,
                           struct html_stream * xs,
                               char ** cbuf,
                               int   * cbufSize,
                               int   * cbufLen) {

    char xc;
    int ii;

    if (xs->htss_utf8tbuflen + 1 >= *cbufSize) {
        htup_expand_utf8buf(cbuf, cbufSize,
            xs->htss_utf8tbuflen + HTML_K_CBUF_INC);
    }

    *cbufLen = 0;
    for (ii = 0; ii < xs->htss_utf8tbuflen; ii++) {
        xc = xs->htss_utf8tbuf[ii];
        (*cbuf)[*cbufLen] = xc;
        (*cbufLen)++;
    }

    /* Add zero terminator */
    (*cbuf)[*cbufLen] = 0;
}
/***************************************************************/
static void htup_AttUtf8Value(struct htmls_file * xf,
                       struct html_stream * xs) {

    int nA;

    nA = xf->htsf_nAttrs - 1;

    htup_copy_utf8tbuf(xf, xs,
        &(xf->htsf_attrVals[nA]),
        &(xf->htsf_attrValsSize[nA]),
        &(xf->htsf_attrLens[nA]));
}
/***************************************************************/
static void htup_Attribute(struct htmls_file * xf,
                       struct html_stream * xs,
                       HTMLC pc) {

/*
*/
    char * atNam;
    HTMLC xc;

    if ((xf->xv_vFlags & HTMLVFLAGS_TOLERANT) && !HTML_IS_NSTART(pc)) {
        /*
        ** Handles: <a class="xyz" " href+="www">
        */
        if (pc != '>' && pc != '/') {
            Html_get_char(xs); /* pc */
        }
        return;
    }

    atNam = htpp_next_Utf8Name(xf, xs, CHCASE_TO_LOWER);
    if (!atNam) return;

    htut_set_Utf8attrNam(xf, xs, atNam);
    if (xv_err(xf)) return;

    xc = Html_get_nwchar(xs);
    if (xc == '=') {
        Html_next_token_parsed(xf, xs, 0, HTDF_ATTRIBUTE);
        if (xv_err(xf)) return;

        htup_AttUtf8Value(xf, xs);
        if (xv_err(xf)) return;
    } else if (xc == '>') {
        Html_unget_char(xs);
        Html_set_tbuf_length(xs, 0);
        htup_AttUtf8Value(xf, xs);
        if (xv_err(xf)) return;
    } else if (HTML_IS_NAME(xc)) {
        Html_unget_char(xs);
        Html_set_tbuf_length(xs, 0);
        htup_AttUtf8Value(xf, xs);
        if (xv_err(xf)) return;
    } else if (xf->xv_vFlags & HTMLVFLAGS_TOLERANT) {
        /* Found: <button class="" route:/vacation-rentals at="xxx" */
        while (!HTML_IS_WHITE(xc) && xc != HTMLCEOF && xc != '>') {
            xc = Html_get_char(xs);
        }
        if (xc != HTMLCEOF) Html_unget_char(xs);
        Html_set_tbuf_length(xs, 0);
    } else {
        Html_seterr(xf, xs, HTMLERR_EXPEQ);
        return;
    }
}
/***************************************************************/
static void htup_p_element(struct htmls_file * xf,
                       struct html_stream * xs,
                       struct ht_element  * nel) {

    int ii;
    struct ht_ielement * xie;

    xf->xc_ielements[xf->xi_depth - 1]->xc_element = nel;

    if (xf->xi_depth > 1) {
        xie = xf->xc_ielements[xf->xi_depth - 2];

        ii = xie->xc_ncElements;
        if (ii == xie->xc_mxElements) {
            xie->xc_cElements =
                Realloc(xie->xc_cElements, struct ht_element *, ii + 1);
            xie->xc_mxElements++;
        }
        xie->xc_cElements[ii] = nel;
        xie->xc_ncElements++;
    }

#if XML_NOT_HTML
    if (xf->xv_sFlags & XMLSFLAG_IMPLYDTD) {
        xpp_add_implicit_attlists(xf, el);
    } else {
        xpp_p_attlist(xf, xs, el);
    }
#endif
}
/***************************************************************/
static int htup_handle_custom_tag(struct htmls_file * xf,
                           struct html_stream * xs,
                           enum eHTML_Tags tagid,
                           struct ht_element  * parent_el)
{

    int result;

    result = 0;
    switch (tagid) {
        case HTAG_DD:
            break;
        case HTAG_DT:
            break;
        case HTAG_LI:
            if (parent_el->l_tagid == HTAG_LI) {
                htup_terminate_element(xf, tagid);
            }
            break;
        default:
            break;
    }

    return (result);
}
/***************************************************************/
static int htup_handle_auto_end(struct htmls_file * xf,
                           struct html_stream * xs,
                           struct ht_element  * parent_el)
{

    int result;

    result = 0;

    htup_terminate_element(xf, parent_el->l_tagid);

    return (result);
}
/***************************************************************/
static int htup_insert_default_root_element(struct htmls_file * xf,
                       struct html_stream * xs)
{
    int result;
    struct ht_element  * root_el;

    root_el = htup_start_element(xf, xs,
        HTML_ELEMENT_NAME(HTAG__ROOT), HTAG__ROOT);

    htup_p_element(xf, xs, root_el);

    if (xf->xi_p_element) {
        result = xf->xi_p_element(xf->xi_vp,
                        HTML_ELEMENT_NAME(HTAG__ROOT),
                        0,
                        0,
                        NULL,
                        NULL);
    }

    return (result);
}
/***************************************************************/
static int htup_jsp_content(struct htmls_file * xf,
                       struct html_stream * xs)
{
/*
**
*/

    int result;
    HTMLC pc;

    Html_put_tbuf_char('<', xs);

    pc = Html_get_char(xs); /* % */
    Html_put_tbuf_char(pc, xs);
    pc = Html_peek_char(xs);

    if (!HTML_IS_WHITE(pc) && pc != '=') {
        Html_seterr(xf, xs, HTMLERR_EXP_JSP_OPEN);
        result = HTMLRESULT_ERR;
    } else {
        result = Html_next_token_parsed_jsp(xf, xs);
        if (!result && xv_err(xf)) return HTMLRESULT_ERR;

        htup_append_utf8tbuf(xf, xs,
            &(xf->htsf_content),
            &(xf->htsf_contentSize),
            &(xf->htsf_contentLen));
    }

    if (xf->htsf_contentLen && xf->xi_depth) {
        result = htup_p_content(xf);
    }

    return (result);
}
/***************************************************************/
static int htup_element(struct htmls_file * xf,
                       struct html_stream * xs)
{

/*
<dl>
  <dt>Coffee
  <dd>- black hot drink
  <dt>Milk
  <dd>- white cold drink.
</dl>

<ul>
  <li>Coffee
  <li>Tea
    <ul>
    <li>Black tea
    <li>Green tea
    </ul>
  </li>
  <li>Milk
</ul>
*/
    int result;
    char * elNam;
    int done;
    HTMLC pc;
    int empty;
    int nwhite;
    enum eHTML_Tags tagid;
    struct ht_element  * el;
    struct ht_element  * parent_el;

    pc = Html_peek_char(xs);
    if (pc == '!') {
        Html_get_char(xs); /* ! */
        result = htup_bang(xf, xs);
        return (result);
    } else if (pc == '?') {
        Html_get_char(xs); /* ? */
        result = htup_PI(xf, xs);
        return (result);
    } else if (pc == '/') {
        Html_get_char(xs); /* / */
        result = htup_ETag(xf, xs);
        return (result);
    } else if (pc == '%') {
        if (xf->xv_vFlags & HTMLVFLAGS_ALLOW_JSP) {
            /*
            ** Will treat <% as JSP and treat it as content
            ** for now. Requires HTMLVFLAGS_ALLOW_JSP
            ** WOn website: https://www.usatoday.com/sports/
            ** some JSP leaked into the html, e.g.
            **  <span class="ccc"><%= sectionDisplay %></span>
            */
            xs->htss_utf8tbuflen = 0;
            result = htup_jsp_content(xf, xs);
            return (result);
        }
    }

    result = 0;
    empty = 0;
    parent_el = NULL;

    elNam = htpp_next_Utf8Name(xf, xs, CHCASE_TO_LOWER);
    if (!elNam) return (HTMLRESULT_ERR);
    tagid = find_html_tag(elNam);

    if (!(xf->xi_depth) && tagid != HTAG_HTML) {
        htup_insert_default_root_element(xf, xs);
        if (xv_err(xf)) return (HTMLRESULT_ERR);
    }

    if (xf->xi_depth > 0) {
        parent_el = xf->xc_ielements[xf->xi_depth - 1]->xc_element;
    }

/*
    if (HTML_CUST_TAG_ELEMENT(tagid) && parent_el) {
        result = htup_handle_custom_tag(xf, xs, tagid, parent_el);
        if (xv_err(xf)) return (HTMLRESULT_ERR);
    }
*/

    if (parent_el && HTML_AUTO_END_ELEMENT(tagid) &&
        HTML_AUTO_END_ELEMENT(parent_el->l_tagid)) {
        /* Cannot nest HTDF_AUTO_END elements, e.g. <li> */
        result = htup_handle_auto_end(xf, xs, parent_el);
        if (xv_err(xf)) return (HTMLRESULT_ERR);
    }

    el = htup_start_element(xf, xs, elNam, tagid);
    if (xv_err(xf)) return (HTMLRESULT_ERR);

    if ((el->l_flags & HTDF_NO_NEST) && parent_el &&
                        parent_el->l_tagid == el->l_tagid) {
            empty = 1;
    } else {
        if (HTML_VOID_ELEMENT(el->l_tagid)) {
            empty = 1;
        }
    }

    done = 0;
    while (!done) {
        pc = Html_peek_nwchar(xs, &nwhite);
        if (!pc) {
            Html_seterr(xf, xs, HTMLERR_UNEXPEOF);
            result = HTMLRESULT_ERR;
            done = 1;
        } else if (pc == '/') {
            done = 1;
            empty = 1;
            Html_get_char(xs); /* / */
            pc = Html_get_char(xs); /* > */
            if (pc != '>') {
                Html_seterr(xf, xs, HTMLERR_EXPCLOSE);
                result = HTMLRESULT_ERR;
            }
        } else if (pc == '>') {
            Html_get_char(xs); /* > */
            done = 1;
        } else if (pc == '<' && (xf->xv_vFlags & HTMLVFLAGS_TOLERANT)) {
            /*
            ** Handles: <a class="ccc" href="www" attr="2" </a>
            */
            done = 1;
        } else {
            htup_Attribute(xf, xs, pc);
            if (xv_err(xf)) {
                done = 1;
                result = HTMLRESULT_ERR;
            }
        }
    }

    if (!xv_err(xf)) {
        htup_p_element(xf, xs, el);
    }

    if (!xv_err(xf)) {
        if (xf->xi_p_element) {
            result = xf->xi_p_element(xf->xi_vp,
                            xf->htsf_elNam,
                            empty,
                            xf->htsf_nAttrs,
                            xf->htsf_attrNames,
                            xf->htsf_attrVals);
        }
    }

    if (empty) {
        htup_end_element(xf);
    }

    return (result);
}
/***************************************************************/
static int htup_content(struct htmls_file * xf,
                       struct html_stream * xs,
                       int leadingspaces,
                       struct ht_element * el) {

/*
[43]    content    ::=
*/
    int result = 0;
    char * elNam = NULL;

    if (el->l_flags & HTDF_RAW) {
        elNam = html_taglist[el->l_tagid].htd_name;
        result = Html_next_token_parsed_raw(xf, xs, leadingspaces, elNam);
        if (!result && xv_err(xf)) return HTMLRESULT_ERR;
    } else {
        Html_next_token_parsed(xf, xs, leadingspaces,
            el->l_flags | HTDF_CONTENT);
        if (xv_err(xf)) return HTMLRESULT_ERR;
    }

    htup_copy_utf8tbuf(xf, xs,
        &(xf->htsf_content), &(xf->htsf_contentSize), &(xf->htsf_contentLen));

    if (xf->htsf_contentLen && xf->xi_depth) {
        result = htup_p_content(xf);
    }

    if (el->l_flags & HTDF_RAW) {
        if (xf->xi_p_endtag) {
            result = xf->xi_p_endtag(xf->xi_vp, elNam);
        }
        htup_end_element(xf);
    }
    xf->htsf_contentLen = 0;

    return (result);
}
/***************************************************************/
static int htup_misc(struct htmls_file * xf,
                    struct html_stream * xs) {
/*
[27]    Misc    ::=    Comment | PI | S
[15]    Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
[16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
*/
    HTMLC xc;
    int result;

    result = 0;
    xc = Html_get_char(xs);
    if (xc == '!') {
        xc = Html_get_char(xs); /* ! */
        if (xc == '-') {
            htpp_Comment(xf, xs);
            Html_get_char(xs); /* > */
        } else {
            Html_seterr(xf, xs, HTMLERR_EXPMISC);
            result = HTMLRESULT_ERR;
        }
    } else if (xc == '?') {
        htpp_PI(xf, xs);
        if (xv_err(xf)) result = HTMLRESULT_ERR;

    } else {
        Html_seterr(xf, xs, HTMLERR_EXPMISC);
        result = HTMLRESULT_ERR;
    }

    return (result);
}
/***************************************************************/
static int htpp_process(struct htmls_file * xf,
                       struct html_stream * xs) {

    int result;
    int nwhite;
    HTMLC pc;
    struct ht_element * el;

    nwhite = 0;
    result = 0;
    while (!result) {
        if (xf->xi_depth) {
            el = xf->xc_ielements[xf->xi_depth - 1]->xc_element;
        } else {
            el = 0;
        }

        if (el && HTML_KEEP_WHITE_ELEMENT(el->l_tagid)) {
            pc = Html_peek_char(xs);
        } else {
            pc = Html_peek_nwchar(xs, &nwhite);
        }

        if (!pc) {
            if (el) {
                if (el->l_tagid == HTAG__ROOT) {
                    htup_terminate_element(xf, el->l_tagid);
                    result = HTMLRESULT_EOF;
                } else {
                    Html_seterr(xf, xs, HTMLERR_UNEXPEOF);
                    result = HTMLRESULT_ERR;
                }
            } else {
                result = HTMLRESULT_EOF;
            }
        } else if (el) {
            if (pc == '<') {
                Html_get_char(xs); /* < */
                result = htup_element(xf, xs);
                if (!result && xv_err(xf)) result = HTMLRESULT_ERR;
            } else {
                result = htup_content(xf, xs, nwhite,
                            xf->xc_ielements[xf->xi_depth - 1]->xc_element);
                if (!result && xv_err(xf)) result = HTMLRESULT_ERR;
            }
        } else {
            if (!xf->xi_rootElement) {
                result = htup_element(xf, xs);
                if (!result && xv_err(xf)) result = HTMLRESULT_ERR;
                if (xf->xi_depth) xf->xi_rootElement = 1;
            } else if (pc == '<') {
                Html_get_char(xs); /* < */
                result = htup_misc(xf, xs);
            } else {
                Html_seterr(xf, xs, HTMLERR_UNEXPCHARS);
                result = HTMLRESULT_ERR;
            }
        }
    }

    return (result);
}
/***************************************************************/
static void Html_free_extID(struct ht_extID * extID) {

    Free(extID->x_extID);
    Free(extID->x_pubID);
    Free(extID);
}
/***************************************************************/
#ifdef UNUSED
static HTMLC * htpp_get_WideName(struct htmls_file * xf,
                      struct html_stream * xs) {

    HTMLC *nam;

    nam = htpp_next_Utf8Name(xf, xs);
    if (!nam) return (0);

    return htmlcstrdup(nam);
}
#endif
/***************************************************************/
static char * htpp_get_SystemLiteral(struct htmls_file * xf,
                          struct html_stream * xs) {

    char * tok;

    tok = Html_next_token_raw(xs);
    if (!tok){
        Html_seterr(xf, xs, HTMLERR_EXPLITERAL);
        return (0);
    }

    if (!HTML_IS_QUOTE(tok[0])) {
        Html_seterr(xf, xs, HTMLERR_EXPLITERAL);
        return (0);
    }

    return (Strdup(tok));
}
/***************************************************************/
static char * htpp_get_PubidLiteral(struct htmls_file * xf,
                          struct html_stream * xs) {

    int ii;
    int len;
    char * tok;

    tok = Html_next_token_raw(xs);
    if (!tok){
        Html_seterr(xf, xs, HTMLERR_EXPLITERAL);
        return (0);
    }

    if (!HTML_IS_QUOTE(tok[0])) {
        Html_seterr(xf, xs, HTMLERR_EXPLITERAL);
        return (0);
    }

    /* validate string for pubid chars */
    len = IStrlen(tok) - 1;
    for (ii = 1; ii < len; ii++) {
        if (!HTML_IS_PUBID(tok[ii])) {
            Html_seterr(xf, xs, HTMLERR_BADPUBID);
            return (0);
        }
    }

    return (Strdup(tok));
}
/***************************************************************/
#ifdef UNUSED
static struct ht_extID * htpp_ExternalID(struct htmls_file * xf,
                          struct html_stream * xs,
                          int pubSysReq) {
/*
[75]    ExternalID    ::=    'SYSTEM' S SystemLiteral
                           | 'PUBLIC' S PubidLiteral S SystemLiteral
*/

    HTMLC * tok;
    HTMLC * slit;
    HTMLC * plit;
    struct ht_extID * extID;
    int xtyp;
    int nwhite;
    HTMLC pc;

    slit = 0;
    plit = 0;
    xtyp = 0;

    tok = Html_next_token_raw(xs);
    if (!tok) return (0);

    if (!strcmp(tok, "SYSTEM")) {
        slit = htpp_get_SystemLiteral(xf, xs);
        if (xv_err(xf)) return (0);
        xtyp = HTML_EXTYP_SYSTEM;
    } else if (!strcmp(tok, "PUBLIC")) {
        plit = htpp_get_PubidLiteral(xf, xs);
        if (xv_err(xf)) return (0);
        xtyp = HTML_EXTYP_PUBLIC;

        pc = Html_peek_nwchar(xs, &nwhite);
        if (HTML_IS_QUOTE(pc)) {
            slit = htpp_get_SystemLiteral(xf, xs);
            if (xv_err(xf)) return (0);
        } else if (pubSysReq) {
            Free(plit);
            Html_seterr(xf, xs, HTMLERR_EXPPUBLIT);
            return (0);
        }
    } else {
        Html_seterr(xf, xs, HTMLERR_EXPSYSPUB);
        return (0);
    }

    extID = New(struct ht_extID, 1);
    extID->x_typ   = xtyp;
    extID->x_extID = slit;
    extID->x_pubID = plit;

    return (extID);
}
#endif
/***************************************************************/
static void htpp_doctypedecl(struct htmls_file * xf,
                          struct html_stream * xs) {
/*
HTML 5.2 8.1.1.1. The DOCTYPE:
1.A string that is an ASCII case-insensitive match for the string "&lt;!DOCTYPE"
2.One or more space characters.
3.A string that is an ASCII case-insensitive match for the string "html".
4.Optionally, a DOCTYPE legacy string.
5.Zero or more space characters.
6.A U+003E GREATER-THAN SIGN character (>).
*/
    int done;
    char * tok;

    tok = htpp_next_Utf8Name(xf, xs, CHCASE_TO_LOWER);
    if (!tok) return;
    if (strcmp(tok, "html")) {
        Html_seterr(xf, xs, HTMLERR_NOTHTML);
        return;
    }

    done = 0;
    while (!done) {
        tok = Html_next_token_raw(xs);
        if (!tok) {
            done = 1;
        } else if (tok[0] == '>' && !tok[1]) {
            done = 1;
        }
    }
}
/***************************************************************/
static void htpp_Misc_doctypedecl(struct htmls_file * xf,
                          struct html_stream * xs) {
/*
*/
    char * ctok;
    int done;
    int nwhite;
    HTMLC pc;

    done = 0;
    pc = Html_peek_nwchar(xs, &nwhite);

    while (!done) {
        if (pc == '!') {
            Html_get_char(xs); /* ! */
            pc = Html_peek_char(xs);
            if (pc == '-') {
                htpp_Comment(xf, xs);
                Html_get_char(xs); /* > */
            } else {
                ctok = htpp_next_Utf8Name(xf, xs, CHCASE_TO_UPPER);
                if (!ctok) {
                    done = 1;
                } else {
                    if (!strcmp(ctok, "DOCTYPE")) {
                        htpp_doctypedecl(xf, xs);
                        if (xv_err(xf)) return;
                    } else {
                        Html_seterr(xf, xs, HTMLERR_EXPDOCTYPE);
                        return;
                    }
                }
            }
        } else if (pc == '?') {
            htpp_PI(xf, xs);
            if (xv_err(xf)) return;
        } else {
            done = 1;
        }

        if (!done) {
            pc = Html_peek_nwchar(xs, &nwhite);
            if (pc ==  '<') {
                Html_get_char(xs); /* < */
                pc = Html_peek_nwchar(xs, &nwhite);
            } else {
                done = 1;
            }
        }
    }
}
/***************************************************************/
static void htpp_prolog(struct htmls_file * xf,
                          struct html_stream * xs) {
/*
*/
    char * tok;

    tok = Html_next_token_raw(xs);
    if (strcmp(tok, "<")) {
        Html_seterr(xf, xs, HTMLERR_NOTHTML);
        return;
    }

    htpp_Misc_doctypedecl(xf, xs);
    if (xv_err(xf)) return;
}
/***************************************************************/
static void Html_begin_html(struct htmls_file * xf) {
/*
*/

    htpp_prolog(xf, xf->xv_stream);
    if (xv_err(xf)) return;
}
/***************************************************************/
int Html_process_html(struct htmls_file * xf,
                    void * vp,
                    HTML_ELEMENT p_xf_element,
                    HTML_CONTENT p_xf_content,
                    HTML_ENDTAG  p_xf_endtag) {

    int result;

    xf->xi_p_element = p_xf_element;
    xf->xi_p_content = p_xf_content;
    xf->xi_p_endtag  = p_xf_endtag;
    xf->xi_vp        = vp;

    result = htpp_process(xf, xf->xv_stream);

    return (result);
}
/***************************************************************/
static void Html_autodetect_encoding(struct html_stream * xs) {
/*
** See: http://en.wikipedia.org/wiki/Byte_Order_Mark
**
** Byte order mark:

    Unicode - Big endian
        xs->xf_fbuf[0]  0xfe   -2
        xs->xf_fbuf[1]  0xff   -1
        xs->xf_fbuf[2]  0x00    0
        xs->xf_fbuf[3]  0x3c   60
    Unicode - Little endian
        xs->xf_fbuf[0]  0xff   -1
        xs->xf_fbuf[1]  0xfe   -2
        xs->xf_fbuf[2]  0x3c   60
        xs->xf_fbuf[3]  0x00    0
    UTF-8
        xs->xf_fbuf[0]  0xef  -17
        xs->xf_fbuf[1]  0xbb  -69
        xs->xf_fbuf[2]  0xbf  -65
        xs->xf_fbuf[3]  0x3c   60
*/

    int rem;

    rem = xs->xf_fbuflen - xs->xf_fbufix;

    if (rem < HTTPCMAXSZ) {
        if (rem > xs->xf_fbufix) {
            return;
        }
        if (rem) memcpy(xs->xf_fbuf, xs->xf_fbuf + xs->xf_fbufix, rem);

        Html_fread(xs, rem);
        if (xs->xf_iseof) return;
        xs->xf_fbufix = 0;

        if (xs->xf_fbuflen - xs->xf_fbufix < HTTPCMAXSZ) return;
    }

    if (xs->xf_fbuf[xs->xf_fbufix] == 0xFE) {
        if (xs->xf_fbuf[xs->xf_fbufix + 1] == 0xFF) {
            xs->xf_encoding = HTML_ENC_UNICODE_BE;
            xs->xf_fbufix += 2;
        }
    } else if (xs->xf_fbuf[xs->xf_fbufix] == 0xFF) {
        if (xs->xf_fbuf[xs->xf_fbufix + 1] == 0xFE) {
            xs->xf_encoding = HTML_ENC_UNICODE_LE;
            xs->xf_fbufix += 2;
        }
    } else if (xs->xf_fbuf[xs->xf_fbufix] == 0xEF) {
        if (xs->xf_fbuf[xs->xf_fbufix + 1] == 0xBB) {
            if (xs->xf_fbuf[xs->xf_fbufix + 2] == 0xBF) {
                xs->xf_encoding = HTML_ENC_UTF_8;
                xs->xf_fbufix += 3;
            }
        }
    }
}
/***************************************************************/
static int Html_insert_entity(struct ht_entities * entities,
                  int etyp,
                  HTMLC * wkeyval,
                  HTMLC * wsdata,
                  struct ht_extID * extID,
                  HTMLC * ndata) {

/*
** Assumes wkeyval and wsdata have already been "New'd"
*/
    struct ht_entity * en;
    struct ht_entity ** pen;
    int keylen;

    keylen = htmlcstrbytes(wkeyval);
    pen = (struct ht_entity **)dbtree_find(entities->xe_entity,
                            wkeyval, keylen);
    if (pen) return (0);

    en = New(struct ht_entity, 1);

    en->e_typ   = etyp;
    en->e_name  = wkeyval;
    en->e_val   = wsdata;
    en->e_extID = extID;
    en->e_ndata = ndata;
    en->e_len   = htmlclen(wsdata);
    en->e_ix    = 1;

    return dbtree_insert(entities->xe_entity,
                            wkeyval, keylen, en);
}
/***************************************************************/
static int Html_insert_entity_ss(struct ht_entities * entities,
                  char * keyval,
                  char * sdata) {

    HTMLC wkeyval[128];
    HTMLC wsdata[128];
    int lkeyval;
    int lsdata;

    lkeyval = IStrlen(keyval) + 1;
    if (lkeyval > 128) return (-1);
    strtohtmlc(wkeyval, keyval, 128);

    lsdata = IStrlen(sdata) + 1;
    if (lsdata > 128) return (-1);
    strtohtmlc(wsdata, sdata, 128);

    return Html_insert_entity(entities, HTML_ENTYP_SYS,
                    htmlcstrdup(wkeyval), htmlcstrdup(wsdata), 0, 0);
}
/***************************************************************/
static struct ht_entities *  Html_new_entities(void) {

    struct ht_entities * entities;

    entities = New(struct ht_entities, 1);
    entities->xe_entity = dbtree_new();

    /* predefined entities */
    Html_insert_entity_ss(entities, "lt"  , "\"<\"");
    Html_insert_entity_ss(entities, "gt"  , "\">\"");
    Html_insert_entity_ss(entities, "amp" , "\"&\"");
    Html_insert_entity_ss(entities, "quot", "\"\"\"");
    Html_insert_entity_ss(entities, "apos", "\"\'\"");

    return (entities);
}
/***************************************************************/
static struct ht_elements *  Html_new_elements(void) {

    struct ht_elements * elements;

    elements = New(struct ht_elements, 1);
    elements->xl_elements = dbtree_new();

    return (elements);
}
/***************************************************************/
static struct ht_attlists *  Html_new_attlists(void) {

    struct ht_attlists * attlists;

    attlists = New(struct ht_attlists, 1);
    attlists->xa_attlist = dbtree_new();

    return (attlists);
}
/***************************************************************/
static struct ht_notations *  Html_new_notations(void) {

    struct ht_notations * notations;

    notations = New(struct ht_notations, 1);
    notations->xn_notations = dbtree_new();

    return (notations);
}
/***************************************************************/
static void Html_doc_init(struct htmls_file * xf) {

    xf->xs_g_entities  = Html_new_entities();
    xf->xs_p_entities  = Html_new_entities();
    xf->xs_elements    = Html_new_elements();
    xf->xs_attlists    = Html_new_attlists();
    xf->xs_notations   = Html_new_notations();
}
/***************************************************************/
static struct html_stream * Html_new_stream(struct htmls_file * xf,
                                          struct ht_extFile * extFil,
                                          const char * html_string) {

    struct html_stream * xs;

    xs = New(struct html_stream, 1);
#if IS_DEBUG
    xs->eh_rectype = HR_HTML_STREAM;
#endif

    xs->xf_xf           = xf;
    xs->xf_encoding     = 0;

    if (html_string) {
        xs->xf_fbuflen      = (int)strlen(html_string);
        xs->xf_fbufsize     = xs->xf_fbuflen + 1;
        xs->xf_fbuf         = New(unsigned char, xs->xf_fbufsize);
        memcpy(xs->xf_fbuf, html_string, xs->xf_fbuflen + 1);
    } else {
        xs->xf_fbufsize     = HTML_K_DEFAULT_FBUFSIZE;
        xs->xf_fbuf         = New(unsigned char, xs->xf_fbufsize);
        xs->xf_fbuflen      = 0;
    }
    xs->xf_fbufix       = 0;
    xs->xf_prev_fbufix  = 0;
    xs->xf_iseof        = 0;
    xs->xf_tadv                    = 0;

/*
    xs->DEPRECATED_xf_tbufsize     = HTML_K_DEFAULT_TBUFSIZE;
    xs->DEPRECATED_xf_tbuf         = New(HTMLC, xs->DEPRECATED_xf_tbufsize);
    xs->DEPRECATED_xf_tbuflen      = 0;
*/

    xs->htss_tbufsize     = HTML_K_DEFAULT_TBUFSIZE;
    xs->htss_tbuf         = New(char, xs->htss_tbufsize);
    xs->htss_tbuflen      = 0;

    xs->xf_lbufsize     = HTML_K_DEFAULT_LBUFSIZE;
    xs->xf_lbuf         = New(HTMLC, xs->xf_lbufsize);
    xs->xf_lbuflen      = 0;

    xs->htss_utf8tbufsize     = HTML_K_DEFAULT_UBUFSIZE;
    xs->htss_utf8tbuf         = New(char, xs->htss_utf8tbufsize);
    xs->htss_utf8tbuflen      = 0;

    xs->xf_linenum      = 0;
    xs->xf_extFile      = extFil;

    return (xs);
}
/***************************************************************/
static void Html_init_parsinfo(struct htmls_file * xf) {

    xf->xi_p_element        =    0;
    xf->xi_p_content        = 0;
    xf->xi_p_endtag         = 0;

    xf->xi_depth            = 0;
    xf->xi_rootElement      = 0;

    xf->htsf_elNam          = NULL;
    xf->htsf_attrNames      = NULL;

    xf->htsf_ebufsize         = HTML_K_DEFAULT_EBUFSIZE;
    xf->htsf_ebuf             = New(char, xf->htsf_ebufsize);
    xf->htsf_ebuflen          = 0;
}
/***************************************************************/
struct htmls_file * Html_new(HTML_FOPEN  p_xq_fopen,
                           HTML_FREAD  p_xq_fread,
                           HTML_FCLOSE p_xq_fclose,
                           void * vfp,
                           int vFlags) {

    struct htmls_file * xf;

    xf = New(struct htmls_file, 1);
#if IS_DEBUG
    xf->eh_rectype = HR_HTML_S_FILE;
#endif

    /* init htmls_file */
    xf->xv_err          = 0;
    xf->xv_errparm[0]   = '\0';
    xf->xv_standalone   = 0;
    xf->xv_vers[0]      = '\0';
    xf->xv_sFlags       = 0;
    xf->xv_debug        = 0;

    /* init element content info */
    xf->xc_maxDepth     = 0;
    xf->xc_ielements    = 0;

    xf->xp_pchead       = 0;
    xf->xp_pctail       = 0;

    xf->xv_vFlags       = vFlags;

    Html_init_parsinfo(xf);

    xf->xq_fopen        = p_xq_fopen;
    xf->xq_fread        = p_xq_fread;
    xf->xq_fclose       = p_xq_fclose;

    xf->xv_extFile = New(struct ht_extFile, 1);
    xf->xv_extFile->fref = vfp;
    xf->xv_stream = Html_new_stream(xf, xf->xv_extFile, NULL);

    /* Html_doc_init(xf); */

    if (!(vFlags & HTMLVFLAGS_NOENCODING)) {
        Html_autodetect_encoding(xf->xv_stream);
    }
    if (!xv_err(xf)) Html_begin_html(xf);

    return (xf);
}
/***************************************************************/
static void Html_free_eltree(struct ht_eltree * elt) {

    int ii;

    if (!elt) return;

    Free(elt->lt_op);

    for (ii = 0; ii < elt->lt_nnodes; ii++) {
        Html_free_eltree(elt->lt_node[ii]);
    }
    Free(elt->lt_node);
    Free(elt);
}
/***************************************************************/
static void Html_free_imp_element(void * pdata) {

    struct ht_imp_element * mel = pdata;

    Free(mel->m_elName);

    Free(mel);
}
/***************************************************************/
static void Html_free_i_elements(struct ht_i_elements * ie) {

    dbtree_free(ie->i_ielements, Html_free_imp_element);

    Free(ie);
}
/***************************************************************/
static void Html_free_element(void * pdata) {

    struct ht_element * el = pdata;

    if (!el) return;

    Free(el->l_name);
    Html_free_eltree(el->l_tree);
    if (el->l_i_elements) Html_free_i_elements(el->l_i_elements);
    Html_free_attlist(el->l_attlist);

    Free(el);
}
/***************************************************************/
static void Html_free_elements(struct ht_elements * elements) {

    if (!elements) return;

    dbtree_free(elements->xl_elements, Html_free_element);

    Free(elements);
}
/***************************************************************/
static void Html_free_entity(void * pdata) {

    struct ht_entity * en = pdata;

    if (en->e_extID) Html_free_extID(en->e_extID);
    Free(en->e_ndata);
    Free(en->e_name);
    Free(en->e_val);

    Free(en);
}
/***************************************************************/
static void Html_free_entities(struct ht_entities * entities) {

    if (!entities) return;

    dbtree_free(entities->xe_entity, Html_free_entity);

    Free(entities);
}
/***************************************************************/
static void Html_free_elist(void * pdata) {

    struct ht_eitem * ei = pdata;

    Free(ei->e_name);

    Free(ei);
}
/***************************************************************/
static void Html_free_attdef(void * pdata) {

    struct ht_attdef * ad = pdata;

    Free(ad->a_attname);
    Free(ad->a_defaultval);

    if (ad->a_elist) dbtree_free(ad->a_elist, Html_free_elist);

    Free(ad);
}
/***************************************************************/
static void Html_free_attlist(void * pdata) {

    struct ht_attlist * al = pdata;

    Free(al->a_elname);

    dbtree_free(al->a_attdefs, Html_free_attdef);

    Free(al);
}
/***************************************************************/
static void Html_free_attlists(struct ht_attlists * attlists) {

    if (!attlists) return;

    dbtree_free(attlists->xa_attlist, Html_free_attlist);

    Free(attlists);
}
/***************************************************************/
static void Html_free_notation(void * pdata) {

    struct ht_notation * not = pdata;

    Free(not->n_notname);
    Html_free_extID(not->n_extID);

    Free(not);
}
/***************************************************************/
static void Html_free_notations(struct ht_notations * notations) {

    if (!notations) return;

    dbtree_free(notations->xn_notations, Html_free_notation);

    Free(notations);
}
/***************************************************************/
static void Html_free_stream(struct html_stream * xs) {

    Free(xs->xf_fbuf);
    /* Free(xs->DEPRECATED_xf_tbuf); */
    Free(xs->xf_lbuf);
    Free(xs->htss_utf8tbuf);
    Free(xs->htss_tbuf);

    Free(xs);
}
/***************************************************************/
#ifdef OLD_WAY
static void Html_free_parsinfo(struct htmls_file * xf) {

    int ii;

    for (ii = 0; ii < xf->DEPRECATED_xi_nAttrsSize; ii++) {
        Free(xf->DEPRECATED_xi_attrNames[ii]);
        Free(xf->DEPRECATED_xi_attrVals[ii]);
    }
    Free(xf->DEPRECATED_xi_attrNamesSize);
    Free(xf->DEPRECATED_xi_attrValsSize);
    Free(xf->DEPRECATED_xi_elNam);
    Free(xf->DEPRECATED_xi_attrNames);
    Free(xf->DEPRECATED_xi_attrLens);
    Free(xf->DEPRECATED_xi_attrVals);
    Free(xf->DEPRECATED_xi_content);

    Free(xf->DEPRECATED_xf_ebuf);
}
#endif
/***************************************************************/
static void Html_free_parsinfo(struct htmls_file * xf) {

    int ii;

    for (ii = 0; ii < xf->htsf_nAttrsSize; ii++) {
        Free(xf->htsf_attrNames[ii]);
        Free(xf->htsf_attrVals[ii]);
    }
    Free(xf->htsf_attrNamesSize);
    Free(xf->htsf_attrValsSize);
    Free(xf->htsf_elNam);
    Free(xf->htsf_attrNames);
    Free(xf->htsf_attrLens);
    Free(xf->htsf_attrVals);
    Free(xf->htsf_content);

    Free(xf->htsf_ebuf);
}
/***************************************************************/
static void Html_free_ielement(struct ht_ielement * xie) {

    Free(xie->xc_cElements);

    Free(xie);
}
/***************************************************************/
static void Html_free_content(struct htmls_file * xf) {

    int ii;

    for (ii = 0; ii < xf->xc_maxDepth; ii++) {
        Html_free_ielement(xf->xc_ielements[ii]);
    }
    Free(xf->xc_ielements);
}
/***************************************************************/
static void Html_free_pchead(struct htmls_file * xf) {

    struct ht_precontent * xpc;

    xpc = xf->xp_pchead;
    if (xpc) {
        xf->xp_pchead = xpc->xp_next;
        if (!(xf->xp_pchead)) xf->xp_pctail = 0;
        Free(xpc);
    }
}
/***************************************************************/
static void Html_free(struct htmls_file * xf) {

    Html_free_stream(xf->xv_stream);

    Html_free_entities(xf->xs_g_entities);
    Html_free_entities(xf->xs_p_entities);
    Html_free_attlists(xf->xs_attlists);
    Html_free_elements(xf->xs_elements);
    Html_free_notations(xf->xs_notations);

    Html_free_content(xf);

    Html_free_parsinfo(xf);

    while (xf->xp_pchead) Html_free_pchead(xf);

    Free(xf);
}
/***************************************************************/
void Html_close(struct htmls_file * xf) {

    Free(xf->xv_extFile);

    Html_free(xf);
}
/***************************************************************/
int Html_err(struct htmls_file * xf) {

    return (xf->xv_err);
}
/***************************************************************/
char * Html_err_msg(struct htmls_file * xf) {

    char errmsg[128];
    char * ferrmsg;

    switch (xf->xv_err) {
        case 0   : strcpy(errmsg,
            "No error.");
            break;
        case HTMLERR_NOTHTML   : strcpy(errmsg,
            "File is not a valid HTML file.");
            break;
        case HTMLERR_BADVERS   : strcpy(errmsg,
            "Expecting html version 1.0 or 1.1.");
            break;
        case HTMLERR_UNSUPENCODING   : strcpy(errmsg,
            "Unsupported encoding.");
            break;
        case HTMLERR_BADENCMARK   : strcpy(errmsg,
            "File has conflicting/bad byte order mark.");
            break;
        case HTMLERR_BADSTANDALONE   : strcpy(errmsg,
            "Bad value for standalone");
            break;
        case HTMLERR_EXPLITERAL   : strcpy(errmsg,
            "Expecting quoted literal string");
            break;
        case HTMLERR_BADPUBID   : strcpy(errmsg,
            "Illegal characters in literal string");
            break;
        case HTMLERR_BADNAME   : strcpy(errmsg,
            "Illegal characters in name");
            break;
        case HTMLERR_BADPEREF  : strcpy(errmsg,
            "Expecting semicolon at end of parameter entity reference");
            break;
        case HTMLERR_BADROOT  : strcpy(errmsg,
            "Bad root element.");
            break;
        case HTMLERR_EXPCLOSE: strcpy(errmsg,
            "Expecting > to close construct.");
            break;
        case HTMLERR_EXPDOCTYPE: strcpy(errmsg,
            "Expecting DOCTYPE.");
            break;
        case HTMLERR__UNSUSED001: strcpy(errmsg,
            "Unused001");
            break;
        case HTMLERR_EXPMARKUP: strcpy(errmsg,
            "Expecting markup ENTITY, ELEMENT, ATTLIST, or NOTATION.");
            break;
        case HTMLERR_EXPNDATA: strcpy(errmsg,
            "Expecting NDATA.");
            break;
        case HTMLERR_EXPSYSPUB: strcpy(errmsg,
            "Expecting SYSTEM or PUBLIC.");
            break;
        case HTMLERR_EXPANYEMPTY: strcpy(errmsg,
            "Expecting ANY or EMPTY.");
            break;
        case HTMLERR_EXPPAREN: strcpy(errmsg,
            "Expecting opening parenthesis.");
            break;
        case HTMLERR_EXPSEP: strcpy(errmsg,
            "Expecting separator , or |.");
            break;
        case HTMLERR_REPEAT: strcpy(errmsg,
            "Expecting +, *, or ?.");
            break;
        case HTMLERR_EXPCLOSEPAREN: strcpy(errmsg,
            "Expecting closing parenthesis.");
            break;
        case HTMLERR_EXPBAR: strcpy(errmsg,
            "Expecting vertical bar.");
            break;
        case HTMLERR_EXPSTAR: strcpy(errmsg,
            "Expecting asterisk after #PCDATA list.");
            break;
        case HTMLERR_EXPCOMMA: strcpy(errmsg,
            "Expecting comma.");
            break;
        case HTMLERR_EXPREPEAT: strcpy(errmsg,
            "Expecting *, +, or ?.");
            break;
        case HTMLERR_EXPATTTYP: strcpy(errmsg,
            "Expecting valid ATTLIST type.");
            break;
        case HTMLERR_EXPATTVAL: strcpy(errmsg,
            "Expecting valid ATTLIST value.");
            break;
        case HTMLERR_EXPATTDEF: strcpy(errmsg,
            "Expecting valid ATTLIST default value.");
            break;
        case HTMLERR_EXPPCDATA: strcpy(errmsg,
            "Expecting #PCDATA.");
            break;
        case HTMLERR_NOSUCHPE: strcpy(errmsg,
            "Parameter entity not declared.");
            break;
        case HTMLERR_RECURENTITY: strcpy(errmsg,
            "Infinitely recursive entities.");
            break;
        case HTMLERR_BADEXTDTD: strcpy(errmsg,
            "Bad external DTD file.");
            break;
        case HTMLERR_OPENSYSFILE: strcpy(errmsg,
            "Unable to access SYSTEM file.");
            break;
        case HTMLERR_EXPSYSEOF: strcpy(errmsg,
            "Expecting end of SYSTEM file.");
            break;
        case HTMLERR_EXPIGNINC: strcpy(errmsg,
            "Expecting IGNORE or INCLUDE.");
            break;
        case HTMLERR_EXPOPBRACKET: strcpy(errmsg,
            "Expecting [ to begin IGNORE/INCLUDE.");
            break;
        case HTMLERR_EXPCLBRACKET: strcpy(errmsg,
            "Expecting ] to terminate IGNORE/INCLUDE.");
            break;
        case HTMLERR_EXPPEREF: strcpy(errmsg,
            "Expecting % to begin PE reference.");
            break;
        case HTMLERR_BADIGNINC: strcpy(errmsg,
            "Bad IGNORE or INCLUDE PE reference.");
            break;
        case HTMLERR_EXPPUBLIT: strcpy(errmsg,
            "Expecting system literal for PUBLIC.");
            break;
        case HTMLERR_BADFORM  : strcpy(errmsg,
            "Expecting html to begin with '<'");
            break;
        case HTMLERR_DUPATTR  : strcpy(errmsg,
            "Duplicate attribute specified");
            break;
        case HTMLERR_UNEXPEOF  : strcpy(errmsg,
            "Unexpected end of file encountered");
            break;
        case HTMLERR_INTERNAL1  : strcpy(errmsg,
            "Internal problem #1");
            break;
        case HTMLERR_EXPEQ  : strcpy(errmsg,
            "Expecting = after attribute name.");
            break;
        case HTMLERR_BADGEREF  : strcpy(errmsg,
            "Expecting semicolon at end of entity reference");
            break;
        case HTMLERR_NOSUCHENTITY: strcpy(errmsg,
            "Entity not declared.");
            break;
        case HTMLERR_BADCHREF  : strcpy(errmsg,
            "Bad character reference");
            break;
        case HTMLERR_EXPCTL  : strcpy(errmsg,
            "Expecting <!-- comment, or <!CDATA");
            break;
        case HTMLERR_NOSUCHELEMENT: strcpy(errmsg,
            "Element not declared.");
            break;
        case HTMLERR_NOSUCHATTLIST: strcpy(errmsg,
            "Attribute list not declared.");
            break;
        case HTMLERR_DUPATTLIST: strcpy(errmsg,
            "Attribute list specified more than once.");
            break;
        case HTMLERR_ATTREQ: strcpy(errmsg,
            "Required attribute not specified.");
            break;
        case HTMLERR_UNEXPCHARS: strcpy(errmsg,
            "Unexpected characters in file.");
            break;
        case HTMLERR_EXPMISC: strcpy(errmsg,
            "Expecting only comment or end of file.");
            break;
        case HTMLERR_UNEXPCLOSE: strcpy(errmsg,
            "Mismatched close element.");
            break;
        case HTMLERR_EXP_JSP_OPEN  : strcpy(errmsg,
            "Expecting space or = after <% to begin JSP");
            break;

        default         : sprintf(errmsg,
            "Unrecognized HTML error number %d", xf->xv_err);
            break;
    }

    if (xf->xv_errparm[0]) {
        if (xf->xv_errline[0]) {
            ferrmsg = Smprintf(
                "%s (%d)\nFound \"%s\" in line #%d:\n%s",
                    errmsg, xf->xv_err,
                    xf->xv_errparm, xf->xv_errlinenum, xf->xv_errline);
        } else {
            ferrmsg = Smprintf(
                "%s (%d)\nFound \"%s\"", errmsg, xf->xv_err, xf->xv_errparm);
        }
    } else {
        if (xf->xv_errline[0]) {
            ferrmsg = Smprintf(
                "%s (%d)\nin line #%d:\n%s",
                 errmsg, xf->xv_err, xf->xv_errlinenum, xf->xv_errline);
        } else {
            ferrmsg = Smprintf(
                "%s (%d\n",
                 errmsg, xf->xv_err);
        }
    }

    return (ferrmsg);
}
/***************************************************************/
struct htmltree * htmlp_new_element(char * elnam,
        int eltagid,
        struct htmltree * parent) {

    struct htmltree * htree;

    htree = New(struct htmltree, 1);
#if IS_DEBUG
    htree->eh_rectype = HR_HTML_TREE;
#endif
    htree->htx_elname  = elnam;
    htree->htx_tagid   = eltagid;
    htree->nattrs  = 0;
    htree->attrnam = 0;
    htree->attrval = 0;
    htree->htx_neldata  = 0;
    htree->htx_eldata   = NULL;
    htree->nsubel  = 0;
    htree->subel   = 0;
    htree->htx_parent  = parent;

    if (parent) {
        parent->subel   =
            Realloc(parent->subel, struct htmltree *, parent->nsubel + 1);
        parent->subel[parent->nsubel] = htree;
        parent->nsubel += 1;
    }

    return (htree);
}
/***************************************************************/
#ifdef UNUSED
struct htmltree * htmlp_new_htmltree(char * root_name) {

    struct htmltree * htree;
    int root_tagid;

    root_tagid = find_html_tag(root_name);
    htree = htmlp_new_element(root_name, root_tagid, NULL);

    return (htree);
}
#endif
/***************************************************************/
#ifdef UNUSED
void htmlp_eldata_p(struct htmltree * xtree, char * eldat)
{
/* 02/26/2018 */
    if (xtree->eldata) Free(xtree->eldata);

    xtree->eldata = eldat;
}
/***************************************************************/
void htmlp_eldata(struct htmltree * xtree, const char * eldat)
{
/* 02/26/2018 */

    htmlp_eldata_p(xtree, Strdup(eldat));
}
#endif
/***************************************************************/
int  htmlp_element ( void  * vp,
                     const char  * elNam,
                     int     emptyFlag,
                     int     nAttrs,
                     char ** attrNames,
                     char ** attrVals) {

    struct htmlpfile * htmlp = (struct htmlpfile *)vp;
    int anMax;
    int avLen;
    int avMax;
    int ii;
    int clen;
    int anLen;
    enum eHTML_Tags ceTagid;
    char * celNam;
    struct htmltree * nxtree;

    ceTagid = find_html_tag(elNam);
    if (ceTagid) {
        nxtree = htmlp_new_element(NULL, ceTagid, htmlp->cxtree);
    } else {
        celNam = Strdup(elNam);
        nxtree = htmlp_new_element(celNam, ceTagid, htmlp->cxtree);
    }

    if (!emptyFlag) htmlp->cxtree = nxtree;
    if (!(*(htmlp->hp_p_htree))) *(htmlp->hp_p_htree) = nxtree;

    nxtree->nattrs  = nAttrs;
    if (nAttrs) {
        nxtree->attrnam = New(char *, nAttrs);
        nxtree->attrval = New(char *, nAttrs);

        for (ii = 0; ii < nAttrs; ii++) {
            clen = IStrlen(attrNames[ii]);
            nxtree->attrnam[ii] = NULL;
            anLen = 0;
            anMax = 0;
            append_chars(&(nxtree->attrnam[ii]), &anLen, &anMax,
                attrNames[ii], clen);

            clen = IStrlen(attrVals[ii]);
            nxtree->attrval[ii] = NULL;
            avLen = 0;
            avMax = 0;
            append_chars(&(nxtree->attrval[ii]), &avLen, &avMax,
                attrVals[ii], clen);
        }
    }

    return (1);
}
/***************************************************************/
int htmlp_content (void * vp, int contLen, const char * contVal) {

    struct htmlpfile * htmlp = (struct htmlpfile *)vp;
    struct htmltree * htree = htmlp->cxtree;
    char * xdat;
    int elen;

    if (htree) {
        if (htree->nsubel + 1 == htree->htx_neldata) {
#if 0
    printf("**************** htmlp_content(): reusing htree->htx_eldata[]\n");
#endif
            /* not sure if this can happen */
            xdat = htree->htx_eldata[htree->htx_neldata - 1];
            elen = IStrlen(xdat);
            xdat = Realloc(xdat, char, elen + contLen + 1);
            memcpy(xdat + elen, contVal, contLen);
            xdat[elen + contLen] = '\0';
            htree->htx_eldata[htree->htx_neldata - 1] = xdat;
        } else {
            xdat = New(char, contLen + 1);
            memcpy(xdat, contVal, contLen);
            xdat[contLen] = '\0';
            htree->htx_eldata =
                Realloc(htree->htx_eldata, char*, htree->nsubel + 1);
            while (htree->htx_neldata < htree->nsubel) {
                htree->htx_eldata[htree->htx_neldata] = NULL;
                htree->htx_neldata += 1;
            }
            htree->htx_eldata[htree->htx_neldata] = xdat;
            htree->htx_neldata += 1;
        }
    }

    return (1);
}
/***************************************************************/
int htmlp_endtag  (void * vp, const char * elNam) {

    struct htmlpfile * htmlp = (struct htmlpfile *)vp;

    if (htmlp->cxtree) {
        htmlp->cxtree = htmlp->cxtree->htx_parent;
    }

    return (1);
}
/***************************************************************/
#ifdef WAREHOUSE_ONLY
int htmlp_fopen (void *vfp,
                const char * fname,
                const char * fmode,
                     char * errmsg,
                     int    errmsgsz) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct htmlpfile * hp = vfp;
    FILE * fref;
    int ern;
    char * emsg;

    errmsg[0] = '\0';
    fref = fopen((char*)fname, (char*)fmode);
    if (!fref) {
        ern = ERRNO;
        emsg = os_get_message_str(ern);
        strmcpy(errmsg, emsg, errmsgsz);
        Free(emsg);
        return (HTMLE_FOPEN);
    }

    hp->hp_fref = fref;

    return (0);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
int htmlp_fread (void *vfp,
                    void * rec,
                        int  recsize,
                        char * errmsg,
                        int    errmsgsz) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct htmlpfile * hp = vfp;
    int  len;
    int ern;
    char * emsg;

    errmsg[0] = '\0';

    len = fread (hp->hp_fref, rec, recsize);
    if (!len) {
        if (!feof(hp->hp_fref)) {
            emsg = os_get_message_str(ern);
            strmcpy(errmsg, emsg, errmsgsz);
            Free(emsg);
            len = HTMLE_FREAD;
        }
    }

    return ((int)len);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static int htmlp_fclose(void *vfp) {
/*
**  Read record from text file
**  Returns number of bytes in record including lf if successful
**  Returns 0 if end of file
**  Returns -1 if error
*/
    struct htmlpfile * htmlp = vfp;
    EBLK * err = NULL;

    wh_fclose(htmlp->hp_fref, &err);

    return (0);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef UNUSED
int htmlp_get_htmltree(char * htmlfname,
                     struct htmltree ** pxtree,
                     char * errmsg,
                     int    errmsgsz) {
/*
**** Unused ****
*/

    struct htmlpfile * htmlp;
    int eno;
    int estat;

    errmsg[0] = '\0';

    htmlp = New(struct htmlpfile, 1);
#if IS_DEBUG
    htmlp->eh_rectype = HR_HTML_P_FILE;
#endif
    htmlp->xfref = fopen(htmlfname, "r");
    if (!htmlp->xfref) {
        eno = errno;
        strmcpy(errmsg, strerror(eno), errmsgsz);
        Free(htmlp);
        return (HTMLE_FOPEN);
    }

    htmlp->xptr = Html_new(htmlp_fopen,
                       htmlp_fread,
                       htmlp_fclose,
                       htmlp, HTMLVFLAG_DEFAULT);
    if (Html_err(htmlp->xptr)) {
        strmcpy(errmsg, Html_err_msg(htmlp->xptr), errmsgsz);
        fclose(htmlp->xfref);
        Free(htmlp);
        return (HTMLE_FNEW);
    }

    (*pxtree)    = 0;
    htmlp->xtree = pxtree;

    estat = 0;
    while (estat >= 0) {
        estat = Html_process_html(htmlp->xptr, htmlp,
                    htmlp_element,
                    htmlp_content,
                    htmlp_endtag);
    }

    if (Html_err(htmlp->xptr)) {
        strmcpy(errmsg, Html_err_msg(htmlp->xptr), errmsgsz);
        estat = HTMLE_PROCESS;
    } else {
        estat = 0;
    }

    Html_close(htmlp->xptr);
    fclose(htmlp->xfref);
    Free(htmlp);

    return (estat);
}
#endif
/***************************************************************/
/***************************************************************/
/* HTML File functions                                         */
/***************************************************************/
#define HSERRMSG_SZ                       255
#define HTMLACC_FLAG_LAYOUT               1
#define HTMLACC_FLAG_KEEP_HTMLTREE_IN_DB  2 /* Main htmltree in hdb_htmltree */
#define HTMLACC_FLAG_FILE_NAME            4 /* Name is not "*" */

#define HTML_DEBUG_SHOW_TYPE            1
#define HTML_DEBUG_SHOW_TREES           2
#define HTML_DEBUG_SHOW_URL             4

#define HDB_ACCESS_READ     1
#define HDB_ACCESS_WRITE    2

struct html_open {
#ifdef _DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    struct htmlpfile    * hop_hp;           /* Pointer from htmlp_fopen() */
    char                * hop_f_name;       /* Pointer to name of file */
    char                * hop_mode;         /* Pointer to open mode */
    int                   hop_fflags;       /* 0=beginning, 1=middle */
    struct htmls_file   * hop_ptr;          /* HTML file */
};
struct html_db {
#ifdef _DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    struct html_open    * hdb_htmlfile;      /* Pointer to head open */
    char                * hdb_file_name;
    char                * hdb_file_mode;    /* Pointer to open mode */
    int                   hdb_create;       /* 1 if CREATE, 0 if OPEN */
    int                   hdb_bigendian;    /* 1 if big endian */
    int                   hdb_default_t0;   /* Use t_string or t_nstring */
    int                   hdb_sizeof_default_t0;/* Is sizeof(struct wh_str64) */
    int                   hdb_charset_id;   /* Character set number */
    int                   hdb_debug;        /* 1 if SET DEBUG */
    int                   hdb_access;       /* 1=read,2=write */
    int                   hdb_openflags;
    int                   hdb_access_flags; /*  */
    int                   hdb_debug_flags;

    struct htmltree     * hdb_htmltree;
    struct vtlayouttree * hdb_vtlayouttree;
    struct htmls_file   * hdb_hf;
    struct dbtree       * hdb_htmltables;    /* tree of struct html_tbl */

    int                   hdb_nbreads;    /* Number of reads */
    struct html_read    **hdb_readlist;   /* Read list */
    int                   hdb_nbwrites;   /* Number of writes */
    struct html_wr      **hdb_writelist;  /* Write list */
    struct svarrec     *  hdb_svarrec; /* 02/15/2018 */
};
struct html_tbl {
#ifdef _DEBUG
    enum eHTML_rectype         eh_rectype;
#endif
    struct html_db      * htb_hdb;
    struct vtlayoutset  * htb_vtls;
    struct wh_type      * htb_tbtype;
    int                   htb_layout_rec_type;
    char *                htb_tblname;     /* table name */
    int                   htb_writable;
    char **               htb_asubrecs; /* Subrecord names */
};
typedef int (*HTREE_PRINTER)(void * vp,
    const char * outbuf,
    int export_flags);
struct ht_printinfo { /* hpti_ */
    FILE * hpti_outf;
};
/***************************************************************/
static int ht_printbuf(void * vp,
            const char * outbuf)
{
    int estat = 0;
    struct ht_printinfo * hpti = (struct ht_printinfo*)vp;

    if (!hpti->hpti_outf) {
        printf("%s", outbuf);
    } else {
        fprintf(hpti->hpti_outf, "%s", outbuf);
    }

    return (estat);
}
/***************************************************************/
static int htmltree_printer(void * vp,
            const char * outbuf,
            int export_flags)
{
    int estat = 0;

    estat = ht_printbuf(vp, outbuf);

    return (estat);
}
/***************************************************************/
static int print_htmltree(struct htmltree * xtree,
                HTREE_PRINTER ht_printer,
                void * vp,
                int show_flags,
                int export_flags,
                int depth) {

    int estat = 0;
    char * outbuf;
    int ii;
    int showel;

    showel = 0;
    if (xtree->htx_elname) {
        if (show_flags & SHOWFLAG_ELEMENT) showel = 1;
    } else {
        switch (html_taglist[xtree->htx_tagid].htd_tag) {
            case HTAG_SCRIPT:
                if (show_flags & SHOWFLAG_SCRIPT) showel = 1;
                break;
            default:
                if (show_flags & SHOWFLAG_DEFAULT) showel = 1;
                break;
        }
    }

    if (showel) {
        if (xtree->htx_elname) {
            outbuf = Smprintf("<%s", xtree->htx_elname);
        } else {
            outbuf = Smprintf("<%s",
                html_taglist[xtree->htx_tagid].htd_name);
        }
        estat = ht_printer(vp, outbuf, export_flags);
        Free(outbuf);
    }
    if (estat) showel = 0;

    if (showel) {
        for (ii = 0; !estat && ii < xtree->nattrs; ii++) {
            outbuf = Smprintf(" %s=\"%s\"",
                xtree->attrnam[ii], xtree->attrval[ii]);
            estat = ht_printer(vp, outbuf, export_flags);
            Free(outbuf);
        }
    } else {
        for (ii = 0; !estat && ii < xtree->nsubel; ii++) {
            estat = print_htmltree(xtree->subel[ii], ht_printer, vp,
                            show_flags, export_flags, depth + 1);
        }
    }

    if (showel && (xtree->nsubel || xtree->htx_neldata)) {
        outbuf = Smprintf(">");
        estat = ht_printer(vp, outbuf, export_flags);
        Free(outbuf);

        if (!estat) {
            if (!HTML_PRE_INLINE_ELEMENT(xtree->htx_tagid)) {
                estat = ht_printer(vp, "\n", export_flags);
                if (export_flags & EXFLAG_INDENT) {
                    for (ii = 0; !estat && ii < depth; ii++) {
                        estat = ht_printer(vp, "  ", export_flags);
                    }
                }
            }

            for (ii = 0; !estat && ii < xtree->nsubel + 1; ii++) {
                if (!estat &&
                    ii < xtree->htx_neldata && xtree->htx_eldata[ii]) {
                    outbuf = Smprintf("%s", xtree->htx_eldata[ii]);
                    estat = ht_printer(vp, outbuf, export_flags);
                    Free(outbuf);
                }
                if (ii < xtree->nsubel) {
                    estat = print_htmltree(xtree->subel[ii], ht_printer, vp,
                                    show_flags, export_flags, depth + 1);
                }
            }
        }

        if (!estat) {
            if (xtree->htx_elname) {
                outbuf = Smprintf("</%s>", xtree->htx_elname);
            } else {
                outbuf = Smprintf("</%s>",
                    html_taglist[xtree->htx_tagid].htd_name);
            }
            estat = ht_printer(vp, outbuf, export_flags);
            Free(outbuf);
            if (!HTML_POST_INLINE_ELEMENT(xtree->htx_tagid)) {
                estat = ht_printer(vp, "\n", export_flags);
                if (depth > 1 && (export_flags & EXFLAG_INDENT)) {
                    for (ii = 0; !estat && ii < depth - 1; ii++) {
                        estat = ht_printer(vp, "  ", export_flags);
                    }
                }
            }
        }
    } else if (showel) {
        estat = ht_printer(vp, "/>", export_flags);
        if (!HTML_POST_INLINE_ELEMENT(xtree->htx_tagid)) {
            estat = ht_printer(vp, "\n", export_flags);
            if (depth > 1 && (export_flags & EXFLAG_INDENT)) {
                for (ii = 0; !estat && ii < depth - 1; ii++) {
                    estat = ht_printer(vp, "  ", export_flags);
                }
            }
        }
    }

    return (estat);
}
/***************************************************************/
int htmltree_print(struct htmltree * xtree,
                HTREE_PRINTER ht_printer,
                void * vp,
                int show_flags,
                int export_flags) {

    int estat = 0;
    char * outbuf;

    if (!xtree) {
        outbuf = Smprintf("null html\n");
        estat = ht_printer(vp, outbuf, export_flags);
        Free(outbuf);
        return (estat);
    }

    if (!(export_flags & EXFLAG_NO_DOCTYPE)) {
        outbuf = Smprintf("<!DOCTYPE html>\n");
        estat = ht_printer(vp, outbuf, export_flags);
        Free(outbuf);
    }

    estat = print_htmltree(xtree, ht_printer, vp,
                show_flags, export_flags, 0);

    return (estat);
}
/***************************************************************/
void show_htmltree(struct htmltree * xtree, int show_flags, int export_flags)
{   /* 04/25/2018 */

    int estat = 0;
    struct ht_printinfo hpti;

    hpti.hpti_outf  = NULL;

    estat = htmltree_print(xtree, htmltree_printer, &hpti, show_flags, export_flags);
}
/***************************************************************/
struct htmls_file * Html_new_from_string(const char * html_string,
                           int vFlags)
{

    struct htmls_file * xf;

    xf = New(struct htmls_file, 1);
#if IS_DEBUG
    xf->eh_rectype = HR_HTML_S_FILE;
#endif

    /* init htmls_file */
    xf->xv_err          = 0;
    xf->xv_errparm[0]   = '\0';
    xf->xv_standalone   = 0;
    xf->xv_vers[0]      = '\0';
    xf->xv_sFlags       = 0;
    xf->xv_debug        = 0;

    /* init element content info */
    xf->xc_maxDepth     = 0;
    xf->xc_ielements    = 0;

    xf->xp_pchead       = 0;
    xf->xp_pctail       = 0;

    xf->xv_vFlags       = vFlags;

    Html_init_parsinfo(xf);

    xf->xq_fopen        = NULL;
    xf->xq_fread        = NULL;
    xf->xq_fclose       = NULL;

    xf->xv_extFile = New(struct ht_extFile, 1);
    xf->xv_extFile->fref = NULL;
    xf->xv_stream = Html_new_stream(xf, xf->xv_extFile, html_string);

    /* Html_doc_init(xf); */

    if (vFlags & HTMLVFLAGS_NOENCODING) {
        xf->xv_stream->xf_encoding = HTML_ENC_UTF_8;
    } else {
        Html_autodetect_encoding(xf->xv_stream);
    }
    if (!xv_err(xf)) Html_begin_html(xf);

    return (xf);
}
/***************************************************************/
int htmlstr_flags_to_vFlags(int htmlstr_flags)
{
/*
**
*/
    int vFlags;

    vFlags = HTMLVFLAGS_TOLERANT | HTMLVFLAGS_ALLOW_JSP;

    if (htmlstr_flags & HTMLSTR_FLAG_ENC_UTF_8) {
        vFlags |= HTMLVFLAGS_NOENCODING;
    }

    return (vFlags);
}
/***************************************************************/
struct htmlpfile * htmlp_new_htmlpfile(void) {

    struct htmlpfile * hp;

    hp = New(struct htmlpfile, 1);
#if IS_DEBUG
    hp->eh_rectype = HR_HTML_P_FILE;
#endif
    hp->hp_openflags = 0;

    return (hp);
}
/***************************************************************/
void free_htmlpfile(struct htmlpfile * hp) {

    Free(hp);
}
/***************************************************************/
int html_get_htmltree_from_string(const char * html_string,
                     struct htmltree ** phtree,
                     int htmlstr_flags,
                     char * errmsg,
                     int    errmsgsz)
{
    int estat;
    int vFlags;
    char * emsg;
    struct htmls_file * hf;
    struct htmlpfile * htmlp;
    int xverr;

    estat = 0;
    (*phtree) = NULL;
    errmsg[0] = '\0';

    vFlags = htmlstr_flags_to_vFlags(htmlstr_flags);

    hf = Html_new_from_string(html_string, vFlags);

    htmlp = htmlp_new_htmlpfile();
    htmlp->hp_p_htree = phtree;

    xverr = 0;
    while (xverr >= 0) {
        xverr = Html_process_html(hf,
                        htmlp,
                        htmlp_element,
                        htmlp_content,
                        htmlp_endtag);
    }

    if (Html_err(hf)) {
        emsg = Html_err_msg(hf);
        strmcpy(errmsg, emsg, errmsgsz);
        Free(emsg);
    }

    Html_close(hf);
    free_htmlpfile(htmlp);

    return (estat);
}
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void wh_html_read_htmltree( struct html_db *hdb, EBLK ** err) {

    int estat;
    int vFlags;

    vFlags = HTMLVFLAGS_TOLERANT | HTMLVFLAGS_ALLOW_JSP;

    hdb->hdb_hf = Html_new(htmlp_fopen,
                       htmlp_fread,
                       htmlp_fclose,
                       hdb->hdb_htmlfile->hop_hp,
                       vFlags);

    if (Html_err(hdb->hdb_hf)) {
        *err = set_error(19502);
        set_error_text(*err, Html_err_msg(hdb->hdb_hf));
        return;
    }

    hdb->hdb_htmltree = NULL;
    hdb->hdb_htmlfile->hop_hp->hp_p_htree = &(hdb->hdb_htmltree);

    estat = 0;
    while (estat >= 0) {
        estat = Html_process_html(hdb->hdb_hf,
                        hdb->hdb_htmlfile->hop_hp,
                        htmlp_element,
                        htmlp_content,
                        htmlp_endtag);
    }

    if (!Html_err(hdb->hdb_hf)) {
        show_htmltree(hdb->hdb_htmltree, EXFLAG_INDENT);
    } else {
        *err = set_error(19501);
        set_error_text(*err,Html_err_msg(hdb->hdb_hf));
        set_error_long(*err, "%d", Html_err(hdb->hdb_hf));
    }

    Html_close(hdb->hdb_hf);
    hdb->hdb_hf = NULL;
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static struct html_db * new_html_db (void) {

    struct html_db  *hdb;

    hdb = New(struct html_db, 1);
#if IS_DEBUG
    hdb->eh_rectype = HR_HTML_DB;
#endif
    hdb->hdb_htmlfile        = NULL;
    hdb->hdb_file_name       = NULL;
    hdb->hdb_file_mode       = NULL;
    hdb->hdb_create          = 0;
    hdb->hdb_debug           = 0;
    hdb->hdb_charset_id      = 0;
    hdb->hdb_bigendian       = 0; /* NSTRINGs are always small endian */
    hdb->hdb_default_t0      = 0;
    hdb->hdb_sizeof_default_t0 = 0;
    hdb->hdb_access_flags    = 0;
    hdb->hdb_debug_flags     = 0;

    hdb->hdb_htmltree        = NULL;
    hdb->hdb_hf              = NULL;

    hdb->hdb_vtlayouttree     = NULL;
    hdb->hdb_htmltables       = dbtree_new();

    hdb->hdb_svarrec         = svar_new();

    return (hdb);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void parse_html_options (STMT * stmt,
                                    struct html_db *hdb,
                                    char ** outfmode,
                                    int  * outflags,
                                    int    default_mode,
                                    EBLK   ** err) {
/*
    OPEN db-tag HTML html-file-name [MODE=mode]

    html-file-name   is the name of JSON file to open.

    mode            is the open mode of the file.  Same as FIXED,
                    TEXT and CSV files.
*/
    int done;
    struct token toke;
    struct token toke2;
    int openflags;
    int modeflags;
    int mode_flag;
    int stat;
    int def_t0;    /* default t0 */

    modeflags = 0;
    openflags = 0;
    stat = 0;
    def_t0 = 0;
    (*outfmode) = 0;
    (*outflags) = 0;

    done = 0;
    while (!done) {
        get_token (stmt, &toke, err);
        if (*err) return;

        if (!toke.text[0]) {
            done = 1;

        } else if (!strcmp(toke.text, "MODE")) {
            get_token (stmt, &toke, err);
            if (*err) return;

            if (strcmp(toke.text, "=")) {
                *err = set_error(19102);
                set_error_text(*err, toke.text);
                return;
            }
            get_token_white (stmt, &toke, err); /* raw=no upshift */
            if (*err) return;
            if (toke.text[0]) {
                hdb->hdb_file_mode = Strdup(toke.text);
            }

        } else {
            mode_flag = parse_file_open_parm(toke.text, &stat);
            if (mode_flag) {
                openflags |= mode_flag;
                /* stat |= 1; -- removed 08/19/2015 */
            } else {
                /* Added 03/28/2016 */
                get_token (stmt, &toke2, err);
                if (*err) return;

                if (strcmp(toke2.text, "=")) {
                    *err = set_error(19102);
                    set_error_text(*err, toke.text);
                    return;
                }
                get_token (stmt, &toke2, err);
                if (*err) return;
                if (toke2.text[0]) {
                    svar_set(hdb->hdb_svarrec, toke.text, toke2.text,
                        SVAR_FLAG_REMOVE_OPTIONAL_DOUBLE_QUOTES |
                        SVAR_FLAG_NO_DUPLICATE_VAR);
                } else {
                    *err = set_error(19003);
                    set_error_text(*err, toke.text);
                    return;
                }
            }
         }
    }

    check_svarrec(hdb->hdb_svarrec, err);
    if (*err) return;

    /* Set STRING or NSTRING */
    if (def_t0 == 0) {
        hdb->hdb_default_t0 = t_str;
        hdb->hdb_sizeof_default_t0 = sizeof(struct wh_str64);
    } else {
        hdb->hdb_default_t0        = def_t0;
        hdb->hdb_sizeof_default_t0 = sizeof(struct wh_str64);
    }

    fixup_open_flags(openflags, stat, modeflags, default_mode,
                    outfmode, outflags, err);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static int calc_html_access_from_file_openflags(int openflags) {

    int faccess;

    faccess = 0;

    if (openflags & WH_OPEN_WRITE) {
        faccess |= HDB_ACCESS_WRITE;
    }

    if (openflags & WH_OPEN_READ) {
        faccess = HDB_ACCESS_READ;
    }

    return (faccess);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void open_html_file (STMT * stmt,
                           struct html_db *hdb,
                           int    create, /* 0 = read, 1 = write */
                           EBLK   ** err) {

    struct token     tok1;
    int     default_mode;
/*
** Read file name from command line
*/
    get_file_name (stmt, &tok1, err);
    if (*err) return;
    if (!strlen(tok1.text)) {
        *err = set_error(19103);
        return;
    }
    hdb->hdb_file_name = Strdup(tok1.text);
    hdb->hdb_create    = create;

    if (create) default_mode = WH_OPEN_WRITE | WH_OPEN_ERASE | WH_OPEN_TEXT;
    else        default_mode = WH_OPEN_READ | WH_OPEN_TEXT;
    default_mode |= WH_OPEN_USE_FLAGS | WH_ALLOW_URL;

    parse_html_options(stmt, hdb,
        &(hdb->hdb_file_mode), &(hdb->hdb_openflags), default_mode, err);
    if (*err) return;

    hdb->hdb_access = calc_html_access_from_file_openflags(hdb->hdb_openflags);

    if (!strcmp(hdb->hdb_file_name, WHOPEN_NO_FILE)) {
        hdb->hdb_htmlfile = NULL;
    } else {
        if (!hdb->hdb_file_mode) {
            if (create) hdb->hdb_file_mode = Strdup("wb");
            else        hdb->hdb_file_mode = Strdup("rb");
        }

        hdb->hdb_access_flags |= HTMLACC_FLAG_LAYOUT; /* default */
        hdb->hdb_access_flags |= HTMLACC_FLAG_FILE_NAME;
    }
    hdb->hdb_access_flags |= HTMLACC_FLAG_KEEP_HTMLTREE_IN_DB; /* default */
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static struct html_open * wh_html_open_file (char * htmlfname,
                            char * htmlfmode,
                            int openflags, /* 02/15/2018 */
                            EBLK ** err) {

    struct html_open *hop;
    int estat;
    char herrmsg[HSERRMSG_SZ + 1];

    hop = New(struct html_open, 1);
#if IS_DEBUG
    hop->eh_rectype = HR_HTML_OPEN;
#endif

    hop->hop_hp = htmlp_new_htmlpfile(openflags);

    estat = htmlp_fopen(hop->hop_hp, htmlfname, htmlfmode,
                    herrmsg, HSERRMSG_SZ);
    if (estat) {
        *err = set_error(19402);
        set_error_text(*err, htmlfname);
        set_error_text(*err, herrmsg);
        Free(hop->hop_hp);
        return (NULL);
    }

    hop->hop_f_name    = Strdup(htmlfname);
    hop->hop_mode      = Strdup(htmlfmode);
    hop->hop_fflags    = 0;

    return (hop);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void wh_html_close_html_open (struct html_open *hop) {

    if (hop->hop_hp) {
        htmlp_fclose(hop->hop_hp);
        free_htmlpfile(hop->hop_hp);
    }

    Free(hop->hop_f_name);
    Free(hop->hop_mode);
    Free(hop);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void html_close_open(struct html_db *hdb) {

    if (hdb->hdb_htmlfile) {
        wh_html_close_html_open(hdb->hdb_htmlfile);
        hdb->hdb_htmlfile = NULL;
    }
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void wh_html_close_file( struct html_db *hdb, EBLK ** err) {

    html_close_open(hdb);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void html_read_html_file(
    struct html_db  *hdb,
    char * file_name,
    EBLK ** err) {
/*
**  Perform OPEN statement
*/
    hdb->hdb_htmlfile = wh_html_open_file(
        file_name, hdb->hdb_file_mode,
        hdb->hdb_openflags, err);
    if (*err) return;

    wh_html_read_htmltree(hdb, err);

    wh_html_close_file(hdb, err);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void * html_open (STMT *stmt, EBLK ** err) {
/*
**  Perform OPEN statement
*/
    struct html_db  *hdb;

    hdb = new_html_db();

    open_html_file(stmt, hdb, 0, err);

    if (!(*err) &&
        (hdb->hdb_access_flags & HTMLACC_FLAG_FILE_NAME) &&
        (hdb->hdb_openflags & WH_OPEN_READ)) {
        html_read_html_file(hdb, hdb->hdb_file_name, err);
    }

    if (*err) {
        Free (hdb);
        return (0);
    }

    return (hdb);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
void free_htmltree_data(struct htmltree * htree) {

    int ii;

    if (!htree) return;

    if (htree->nsubel) {
        for (ii = 0; ii < htree->nsubel; ii++) {
            free_htmltree(htree->subel[ii]);
        }
        Free(htree->subel);
    }

    if (htree->nattrs) {
        for (ii = 0; ii < htree->nattrs; ii++) {
            Free(htree->attrnam[ii]);
            Free(htree->attrval[ii]);
        }
        Free(htree->attrnam);
        Free(htree->attrval);
    }

    if (htree->htx_neldata) {
        for (ii = 0; ii < htree->htx_neldata; ii++) {
            Free(htree->htx_eldata[ii]);
        }
        Free(htree->htx_eldata);
    }

    Free(htree->htx_elname);
}
/***************************************************************/
void free_htmltree(struct htmltree * htree) {

    if (htree) {
        free_htmltree_data(htree);
        Free(htree);
    }
}
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void html_free_table (void * vpxtbl) {

    struct html_tbl * xtbl = vpxtbl;

    Free(xtbl->htb_tblname);

    Free(xtbl);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
static void html_close (struct html_db *hdb, EBLK ** err) {

    dbtree_free(hdb->hdb_htmltables, html_free_table);
    free_vtlayouttree(hdb->hdb_vtlayouttree);

    svar_free(hdb->hdb_svarrec);

    Free(hdb->hdb_file_name);
    Free(hdb->hdb_file_mode);
    if (hdb->hdb_htmltree) free_htmltree(hdb->hdb_htmltree);

    Free(hdb);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
#ifdef WAREHOUSE_ONLY
struct nlocal_funcs * html_lvectors(void) {

    struct nlocal_funcs * lv;

    lv = New(struct nlocal_funcs, 1);

    lv->nl_open         = (NL_OPEN)html_open;
    lv->nl_close        = (NL_CLOSE)html_close;

/*
** 04/24/2018
*/

    /*
    lv->nl_close        = (NL_CLOSE)html_close;
    lv->nl_layout       = (NL_LAYOUT)html_layout;
    lv->nl_export       = (NL_EXPORT)html_export;
    lv->nl_reveal       = (NL_REVEAL)html_reveal;
    lv->nl_list         = (NL_LIST)html_list;
    lv->nl_info         = (NL_INFO)html_info;
    lv->nl_show         = (NL_SHOW)html_show;
    lv->nl_set          = (NL_SET)html_set;
    lv->nl_db_dot       = (NL_DB_DOT)html_db_dot;
    lv->nl_prep_read    = (NL_PREP_READ)html_prep_read;
    lv->nl_describe     = (NL_DESCRIBE)html_describe;
    lv->nl_begin_read   = (NL_BEGIN_READ)html_begin_read;
    lv->nl_read_next    = (NL_READ_NEXT)html_read_next;
    lv->nl_end_read     = (NL_END_READ)html_end_read;
    lv->nl_table_info   = (NL_TABLE_INFO)html_table_info;
    lv->nl_create       = (NL_CREATE)html_create;
    lv->nl_prep_write   = (NL_PREP_WRITE)html_prep_write;
    lv->nl_write        = (NL_WRITE)html_write;
    lv->nl_commit       = (NL_COMMIT)html_commit;
    lv->nl_format       = (NL_FORMAT)html_table_format;
    */

    lv->nl_ptr          = 0;

    return (lv);
}
#endif /* WAREHOUSE_ONLY */
/***************************************************************/
