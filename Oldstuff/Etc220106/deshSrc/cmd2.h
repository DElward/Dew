/***************************************************************/
/* cmd2.h                                                      */
/***************************************************************/

#if IS_WINDOWS
    char * GetWinMessage(long errCode);
    time_t filetime_to_time(const struct _FILETIME * filtim);
#endif

/* int read_directory functions */
int open_directory(void * vrdr, const char * dname);
int read_directory(void * vrdr, char * buf, int bufsize, int * eof);
void close_directory(void * vrdr);
void * new_read_directory(void);
void close_read_directory(void * vrdr);
int get_errstat_directory(void * vrdr);
char * get_errmsg_directory(void * vrdr);

int remove_directory(struct cenv * ce,
                int cmdflags,
                const char * dirname);

void os_makepath(const char * dirname,
		const char * fname,
		char * fullname,
		int fnamemax,
        char ddelim);
int has_drive_name(const char * dirname);

/*  ********  */


void get_cwd(char *cwd, int cwdmax);
void set_time_globals(struct glob * gg);
int patmatches(int casein,
                const char * pat,
                const char * val);

int get_filenames(struct cenv * ce,
                const char * dfname,
                char *** fnamelist);

int chndlr_cd(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_chmod(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_rmdir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);

int chndlr_mkdir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_dir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_copy(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_move(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_remove(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_removedir(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
int chndlr_type(struct cenv * ce,
                struct cmdinfo * ci,
                struct str * cmdstr,
                int * cmdix);
/***************************************************************/
