/***************************************************************/
/* msg.h                                                       */
/***************************************************************/

#if IS_WINDOWS
    #define GETERRNO    (GetLastError())
#else
    #define GETERRNO    ERRNO
#endif

/* Update get_estat_errmsg() if list is changed */
#define ESTAT_EOF       (-1)
#define ESTAT_EXIT      (-2)
#define ESTAT_CTL_C     (-3)
#define ESTAT_EOL       (-4)
#define ESTAT_IDLEERR   (-5)

#define EDESH_MAX       (-4001)

#define EOPENCMD        (-4001)
#define EREADCMD        (-4002)
#define ETHROW          (-4003)
#define EINVALFLAG      (-4004)
#define EBADOPT         (-4005)
#define EBADVARNAM      (-4006)
#define EUNEXPEOL       (-4007)
#define EBADREDIR       (-4008)
#define EBADFOR         (-4009)
#define EUNEXPCMD       (-4010)
#define EMISSINGEND     (-4011)
#define EUNEXPEXPEOL    (-4012)
#define EEXPEQ          (-4013)
#define EBADNUM         (-4014)
#define EUNEXPTOK       (-4015)
#define EBADID          (-4016)
#define EEXPNUM         (-4017)
#define EEXPBOOL        (-4018)
#define EEXPEOL         (-4019)
#define EEXPTO          (-4020)
#define EMISSINGCLOSE   (-4021)
#define EMANYPARMS      (-4022)
#define EFEWPARMS       (-4023)
#define EEXPCOMMA       (-4024)
#define EMISSINGBRACE   (-4025)
#define EBADFUNCNAM     (-4026)
#define EBADFUNCHDR     (-4027)
#define ENOFUNCHERE     (-4028)
#define ENODUPFUNC      (-4029)
#define EINVALTYP       (-4030)
#define ENORETVAL       (-4031)
#define ENOVAR          (-4032)
#define EDUPREDIR       (-4033)
#define ENOREDIR        (-4034)
#define EOPENREDIR      (-4035)
#define EASSOC          (-4036)
#define ENOCMD          (-4037)
#define EFINDCMD        (-4038)
#define EINVALCMD       (-4039)
#define ENOPATHEXT      (-4040)
#define ENOPATH         (-4041)
#define ERUN            (-4042)
#define EWAIT           (-4043)
#define EFINDFILE       (-4044)
#define EEXPFILE        (-4045)
#define EOPENFILE       (-4046)
#define EUNMATCHEDPAREN (-4047)
#define EUNMATCHEDQUOTE (-4048)
#define EOPENTMPFIL     (-4049)
#define EWRITETMPFIL    (-4050)
#define ERESETTMPFIL    (-4051)
#define EEXPPAREN       (-4052)
#define ETERMINAL       (-4053)
#define EEXPCLPAREN     (-4054)
#define EOPENFORFILE    (-4055)
#define EWRITESTDOUT    (-4056)
#define EWRITESTDERR    (-4057)
#define ESTDOUTPIPED    (-4058)
#define EOPENPIPE       (-4059)
#define ESTDINPIPED     (-4060)
#define EPIPETOINT      (-4061)
#define EPIPETOFUNC     (-4062)
#define EBADPARM        (-4063)
#define ERMDIR          (-4064)
#define EREMOVE         (-4065)
#define EEXPFORMCOMMA   (-4066)
#define EMAXSUBS        (-4067)
#define EMKDIR          (-4068)
#define EDIR            (-4069)
#define EFILSET         (-4070)
#define EUNEXPCOPYEOL   (-4071)
#define ENOCOPYSRC      (-4072)
#define ECOPYSRCDIR     (-4073)
#define ECOPYTGTEXISTS  (-4074)
#define ECOPYERR        (-4075)
#define EDIRREQ         (-4076)
#define ESETATTR        (-4077)
#define EMUSTBEDIR      (-4078)
#define EREMOVEDIR      (-4079)
#define ECHMODBADOPT    (-4080)
#define ECHMODSTAT      (-4081)
#define ECHMOD          (-4082)
#define ECHDIR          (-4083)
#define ERENAMEPARMS    (-4084)
#define ERENAME         (-4085)
#define EDATEFMT        (-4086)
#define EEXITPARM       (-4087)
#define EADDTODATE      (-4088)
#define ESUBDATES       (-4089)
#define EPROGERR        (-4090)
#define ECMDFAILED      (-4091)
#define ENOTINFUNC      (-4092)
#define EDUPPARMNAME    (-4093)
#define EOPENRCFILE     (-4094)
#define ERUNRCFILE      (-4095)
#define ERUNCTLC        (-4096)
#define EOPTCONFLICT    (-4097)
#define ESHIFTPARM      (-4098)
#define EMISSINGPARM    (-4099)
#define EUNDEFFUNC      (-4100)
#define EINITHELP       (-4101)
#define ENOHELP         (-4102)
#define EFINDHELP       (-4103)
#define EWRITEHELP      (-4104)
#define EDIVIDEBYZERO   (-4105)
#define EBADSUBTOKEN    (-4106)
#define ENOFUNCSTRUCT   (-4107)
#define ENORETURNHERE   (-4108)
#define EBADFINFOTYPE   (-4110)
#define EBADFINFOOWNER  (-4111)
#define EMISSINGENDLIST (-4112)
#define EBADBASE        (-4113)
#define EBADBASENUM     (-4114)
#define EFILESTAT       (-4115)
#define EOPENFORDIR     (-4116)
#define EREADFORDIR     (-4117)
#define EBADCSVDELIM    (-4118)
#define ENOCSVDELIM     (-4119)
#define ENOFORCE        (-4120)
#define EEXPDIR         (-4121)
#define ENEWHELP        (-4122)
#define ENOSEARCH       (-4123)
#define EREPTOOBIG      (-4124)
#define ETARGETEXISTS   (-4125)
#define ERMTARGET       (-4126)
#define ECPFILE         (-4127)
#define EBADPID         (-4128)
#define EWAITERR        (-4129)
#define EWAITCMDFAIL    (-4130)
#define ERMREADONLY     (-4131)
#define ECMDARGFAILED   (-4132)

#define EDESH_MIN       (-4299)

/***************************************************************/
void strmcpy(char * tgt, const char * src, size_t tmax);
void strmcat(char * tgt, char * src, size_t tmax);

void clear_error(struct cenv * ce, int inestat);
void show_errmsg(struct cenv * ce, const char * errcmd, int inestat);
void set_errmsg(struct cenv * ce, const char * errcmd, int inestat);
int set_error(struct cenv * ce, int inestat, const char * eparm);
int chain_error(struct cenv * ce, int inestat, int inestat2);
int set_ferror(struct cenv * ce,  int inestat, char *fmt, ...);

int release_estat_error(struct cenv * ce, int inestat);
int get_estat_errnum(struct cenv * ce, int inestat);
char * get_estat_errmsg(struct cenv * ce, int inestat);

void show_warn (struct cenv * ce, const char * fmt, ...);
int printll (struct cenv * ce, const char * fmt, ...);
int printlle (struct cenv * ce, const char * fmt, ...);

/***************************************************************/
