#ifndef PARSEHTTPH_INCLUDED
#define PARSEHTTPH_INCLUDED
/***************************************************************/
/*
**  parsehttp.h  --  Header for access to HTML files
*/
/***************************************************************/

/* Error numbers                                               */
#define HTMLERR_NOERR            0
#define HTMLERR_NOTHTML          3801
#define HTMLERR_BADVERS          3802
#define HTMLERR_UNSUPENCODING    3803
#define HTMLERR_BADENCMARK       3804
#define HTMLERR_BADSTANDALONE    3805
#define HTMLERR_EXPLITERAL       3806
#define HTMLERR_BADPUBID         3807
#define HTMLERR_BADNAME          3808
#define HTMLERR_BADPEREF         3809
#define HTMLERR_BADROOT          3810
#define HTMLERR_EXPCLOSE         3811
#define HTMLERR_EXPDOCTYPE       3812
#define HTMLERR__UNSUSED001      3813
#define HTMLERR_EXPMARKUP        3814
#define HTMLERR_EXPNDATA         3815
#define HTMLERR_EXPSYSPUB        3816
#define HTMLERR_EXPANYEMPTY      3817
#define HTMLERR_EXPPAREN         3818
#define HTMLERR_EXPSEP           3819
#define HTMLERR_REPEAT           3820
#define HTMLERR_EXPCLOSEPAREN    3821
#define HTMLERR_EXPBAR           3822
#define HTMLERR_EXPSTAR          3823
#define HTMLERR_EXPCOMMA         3824
#define HTMLERR_EXPREPEAT        3825
#define HTMLERR_EXPATTTYP        3826
#define HTMLERR_EXPATTVAL        3827
#define HTMLERR_EXPATTDEF        3828
#define HTMLERR_EXPPCDATA        3829
#define HTMLERR_NOSUCHPE         3830
#define HTMLERR_RECURENTITY      3831
#define HTMLERR_BADEXTDTD        3832
#define HTMLERR_OPENSYSFILE      3833
#define HTMLERR_EXPSYSEOF        3834
#define HTMLERR_EXPIGNINC        3835
#define HTMLERR_EXPOPBRACKET     3836
#define HTMLERR_EXPCLBRACKET     3837
#define HTMLERR_EXPPEREF         3838  /* should never happen */
#define HTMLERR_BADIGNINC        3839
#define HTMLERR_EXPPUBLIT        3840
#define HTMLERR_BADFORM          3841
#define HTMLERR_DUPATTR          3842
#define HTMLERR_UNEXPEOF         3843
#define HTMLERR_INTERNAL1        3844  /* should never happen */
#define HTMLERR_EXPEQ            3845
#define HTMLERR_BADGEREF         3846
#define HTMLERR_NOSUCHENTITY     3847
#define HTMLERR_BADCHREF         3848
#define HTMLERR_EXPCTL           3849
#define HTMLERR_NOSUCHELEMENT    3850
#define HTMLERR_NOSUCHATTLIST    3851
#define HTMLERR_DUPATTLIST       3852
#define HTMLERR_ATTREQ           3853
#define HTMLERR_UNEXPCHARS       3854
#define HTMLERR_EXPMISC          3855
#define HTMLERR_EXP_JSP_OPEN     3856
#define HTMLERR_UNEXPCLOSE       3857
/***************************************************************/

#define EXFLAG_EOL_CR               1
#define EXFLAG_EOL_LF               2   /* CRLF = 3 */
#define EXFLAG_UTF8                 4
#define EXFLAG_WIDE_LITTLE_ENDIAN   8
#define EXFLAG_WIDE_BIG_ENDIAN      16
#define EXFLAG_ALLQUOTED            32
#define EXFLAG_XML                  64
#define EXFLAG_INDENT               128
#define EXFLAG_NO_DOCTYPE           256
#define EXFLAG_TRIM_LAST            512 /* 03/11/2019 */
#define EXFLAG_STATS                1024    /* 07/11/2019 */

#define EXPORT_DEFAULT_DELIM        ','
#define EXPORT_DEFAULT_QUOTE        '\"'

/***************************************************************/

#define SHOWFLAG_DEFAULT            1
#define SHOWFLAG_ELEMENT            2
#define SHOWFLAG_SCRIPT             4
#define SHOWFLAG_ALL                65535

/***************************************************************/

#define HTML_ENC_UTF_8           0  /* default */
#define HTML_ENC_1_BYTE          1  /* Single byte encoding */
#define HTML_ENC_UNICODE_BE      2  /* Unicode big endian */
#define HTML_ENC_UNICODE_LE      3  /* Unicode little endian */
/***************************************************************/
#define HTMLSTR_FLAG_ENC_UTF_8     1 /* Encoding is UTF-8, do not check */
/***************************************************************/

void free_htmltree(struct htmltree * htree);
int html_get_htmltree_from_string(const char * html_string,
                     struct htmltree ** phtree,
                     int htmlstr_flags,
                     char * errmsg,
                     int    errmsgsz);
void show_htmltree(struct htmltree * xtree, int show_flags, int export_flags);

/***************************************************************/
#endif /* PARSEHTTPH_INCLUDED */
