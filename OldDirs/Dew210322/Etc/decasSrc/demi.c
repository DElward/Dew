/**************************************************************/
/* demi.c                                                     */
/**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#include "types.h"
#include "decas.h"
#include "demi.h"
#include "dbtreeh.h"
#include "util.h"
#include "demicp.h"

/**** Forward declarations ***/
static int sdemi_eval_expression(struct demirec * demi, const char  * expbuf, struct demival * demv);
static int sdemi_eval_exp(struct demirec * demi,
    const char  * expbuf,
    int * eix,
    char * toke,
    int tokesz,
    struct demival * demv);
static void sdemi_demityp_free(struct demityp * demt);
static int sdemi_calc_demityp_alignment(struct demityp * demt, int align);
static int sdemi_parse_type(struct demirec * demi,
    const char  * typdesc,
    struct demityp ** pdemt,
    int typsize);
static int sdemi_parse_typespec(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt);
static struct demityp * sdemi_copy_demityp(struct demityp * demtsrc);
static int sdemi_calc_demityp_struct_alignment(struct demityp_struct * demts, int align);
static int demi_register_function(void * vdemi,
        const char * funcname,
        DEMI_CALLABLE_FUNCTION func);

/**** External declarations ***/
int demi_register_callable_functions(void * vdemi,
    int (*register_func)(void * vd, const char * funcnam, DEMI_CALLABLE_FUNCTION func),
    const char * funccfg);

/***************************************************************/
#ifdef UNUSED
/*
** From: https://stackoverflow.com/questions/15169387/definitive-crc-for-c
*/
static unsigned char const crc8_table[] = {
    0xea, 0xd4, 0x96, 0xa8, 0x12, 0x2c, 0x6e, 0x50, 0x7f, 0x41, 0x03, 0x3d,
    0x87, 0xb9, 0xfb, 0xc5, 0xa5, 0x9b, 0xd9, 0xe7, 0x5d, 0x63, 0x21, 0x1f,
    0x30, 0x0e, 0x4c, 0x72, 0xc8, 0xf6, 0xb4, 0x8a, 0x74, 0x4a, 0x08, 0x36,
    0x8c, 0xb2, 0xf0, 0xce, 0xe1, 0xdf, 0x9d, 0xa3, 0x19, 0x27, 0x65, 0x5b,
    0x3b, 0x05, 0x47, 0x79, 0xc3, 0xfd, 0xbf, 0x81, 0xae, 0x90, 0xd2, 0xec,
    0x56, 0x68, 0x2a, 0x14, 0xb3, 0x8d, 0xcf, 0xf1, 0x4b, 0x75, 0x37, 0x09,
    0x26, 0x18, 0x5a, 0x64, 0xde, 0xe0, 0xa2, 0x9c, 0xfc, 0xc2, 0x80, 0xbe,
    0x04, 0x3a, 0x78, 0x46, 0x69, 0x57, 0x15, 0x2b, 0x91, 0xaf, 0xed, 0xd3,
    0x2d, 0x13, 0x51, 0x6f, 0xd5, 0xeb, 0xa9, 0x97, 0xb8, 0x86, 0xc4, 0xfa,
    0x40, 0x7e, 0x3c, 0x02, 0x62, 0x5c, 0x1e, 0x20, 0x9a, 0xa4, 0xe6, 0xd8,
    0xf7, 0xc9, 0x8b, 0xb5, 0x0f, 0x31, 0x73, 0x4d, 0x58, 0x66, 0x24, 0x1a,
    0xa0, 0x9e, 0xdc, 0xe2, 0xcd, 0xf3, 0xb1, 0x8f, 0x35, 0x0b, 0x49, 0x77,
    0x17, 0x29, 0x6b, 0x55, 0xef, 0xd1, 0x93, 0xad, 0x82, 0xbc, 0xfe, 0xc0,
    0x7a, 0x44, 0x06, 0x38, 0xc6, 0xf8, 0xba, 0x84, 0x3e, 0x00, 0x42, 0x7c,
    0x53, 0x6d, 0x2f, 0x11, 0xab, 0x95, 0xd7, 0xe9, 0x89, 0xb7, 0xf5, 0xcb,
    0x71, 0x4f, 0x0d, 0x33, 0x1c, 0x22, 0x60, 0x5e, 0xe4, 0xda, 0x98, 0xa6,
    0x01, 0x3f, 0x7d, 0x43, 0xf9, 0xc7, 0x85, 0xbb, 0x94, 0xaa, 0xe8, 0xd6,
    0x6c, 0x52, 0x10, 0x2e, 0x4e, 0x70, 0x32, 0x0c, 0xb6, 0x88, 0xca, 0xf4,
    0xdb, 0xe5, 0xa7, 0x99, 0x23, 0x1d, 0x5f, 0x61, 0x9f, 0xa1, 0xe3, 0xdd,
    0x67, 0x59, 0x1b, 0x25, 0x0a, 0x34, 0x76, 0x48, 0xf2, 0xcc, 0x8e, 0xb0,
    0xd0, 0xee, 0xac, 0x92, 0x28, 0x16, 0x54, 0x6a, 0x45, 0x7b, 0x39, 0x07,
    0xbd, 0x83, 0xc1, 0xff};

unsigned crc8(unsigned crc, unsigned char const *data, size_t len)
{
    if (data == NULL)
        return 0;
    crc &= 0xff;
    unsigned char const *end = data + len;
    while (data < end)
        crc = crc8_table[crc ^ *data++];
    return crc;
}
/***************************************************************/
uintr crc32tab[256] =
{
    0x00000000L, 0x04C11DB7L, 0x09823B6EL, 0x0D4326D9L,
    0x130476DCL, 0x17C56B6BL, 0x1A864DB2L, 0x1E475005L,
    0x2608EDB8L, 0x22C9F00FL, 0x2F8AD6D6L, 0x2B4BCB61L,
    0x350C9B64L, 0x31CD86D3L, 0x3C8EA00AL, 0x384FBDBDL,
    0x4C11DB70L, 0x48D0C6C7L, 0x4593E01EL, 0x4152FDA9L,
    0x5F15ADACL, 0x5BD4B01BL, 0x569796C2L, 0x52568B75L,
    0x6A1936C8L, 0x6ED82B7FL, 0x639B0DA6L, 0x675A1011L,
    0x791D4014L, 0x7DDC5DA3L, 0x709F7B7AL, 0x745E66CDL,
    0x9823B6E0L, 0x9CE2AB57L, 0x91A18D8EL, 0x95609039L,
    0x8B27C03CL, 0x8FE6DD8BL, 0x82A5FB52L, 0x8664E6E5L,
    0xBE2B5B58L, 0xBAEA46EFL, 0xB7A96036L, 0xB3687D81L,
    0xAD2F2D84L, 0xA9EE3033L, 0xA4AD16EAL, 0xA06C0B5DL,
    0xD4326D90L, 0xD0F37027L, 0xDDB056FEL, 0xD9714B49L,
    0xC7361B4CL, 0xC3F706FBL, 0xCEB42022L, 0xCA753D95L,
    0xF23A8028L, 0xF6FB9D9FL, 0xFBB8BB46L, 0xFF79A6F1L,
    0xE13EF6F4L, 0xE5FFEB43L, 0xE8BCCD9AL, 0xEC7DD02DL,
    0x34867077L, 0x30476DC0L, 0x3D044B19L, 0x39C556AEL,
    0x278206ABL, 0x23431B1CL, 0x2E003DC5L, 0x2AC12072L,
    0x128E9DCFL, 0x164F8078L, 0x1B0CA6A1L, 0x1FCDBB16L,
    0x018AEB13L, 0x054BF6A4L, 0x0808D07DL, 0x0CC9CDCAL,
    0x7897AB07L, 0x7C56B6B0L, 0x71159069L, 0x75D48DDEL,
    0x6B93DDDBL, 0x6F52C06CL, 0x6211E6B5L, 0x66D0FB02L,
    0x5E9F46BFL, 0x5A5E5B08L, 0x571D7DD1L, 0x53DC6066L,
    0x4D9B3063L, 0x495A2DD4L, 0x44190B0DL, 0x40D816BAL,
    0xACA5C697L, 0xA864DB20L, 0xA527FDF9L, 0xA1E6E04EL,
    0xBFA1B04BL, 0xBB60ADFCL, 0xB6238B25L, 0xB2E29692L,
    0x8AAD2B2FL, 0x8E6C3698L, 0x832F1041L, 0x87EE0DF6L,
    0x99A95DF3L, 0x9D684044L, 0x902B669DL, 0x94EA7B2AL,
    0xE0B41DE7L, 0xE4750050L, 0xE9362689L, 0xEDF73B3EL,
    0xF3B06B3BL, 0xF771768CL, 0xFA325055L, 0xFEF34DE2L,
    0xC6BCF05FL, 0xC27DEDE8L, 0xCF3ECB31L, 0xCBFFD686L,
    0xD5B88683L, 0xD1799B34L, 0xDC3ABDEDL, 0xD8FBA05AL,
    0x690CE0EEL, 0x6DCDFD59L, 0x608EDB80L, 0x644FC637L,
    0x7A089632L, 0x7EC98B85L, 0x738AAD5CL, 0x774BB0EBL,
    0x4F040D56L, 0x4BC510E1L, 0x46863638L, 0x42472B8FL,
    0x5C007B8AL, 0x58C1663DL, 0x558240E4L, 0x51435D53L,
    0x251D3B9EL, 0x21DC2629L, 0x2C9F00F0L, 0x285E1D47L,
    0x36194D42L, 0x32D850F5L, 0x3F9B762CL, 0x3B5A6B9BL,
    0x0315D626L, 0x07D4CB91L, 0x0A97ED48L, 0x0E56F0FFL,
    0x1011A0FAL, 0x14D0BD4DL, 0x19939B94L, 0x1D528623L,
    0xF12F560EL, 0xF5EE4BB9L, 0xF8AD6D60L, 0xFC6C70D7L,
    0xE22B20D2L, 0xE6EA3D65L, 0xEBA91BBCL, 0xEF68060BL,
    0xD727BBB6L, 0xD3E6A601L, 0xDEA580D8L, 0xDA649D6FL,
    0xC423CD6AL, 0xC0E2D0DDL, 0xCDA1F604L, 0xC960EBB3L,
    0xBD3E8D7EL, 0xB9FF90C9L, 0xB4BCB610L, 0xB07DABA7L,
    0xAE3AFBA2L, 0xAAFBE615L, 0xA7B8C0CCL, 0xA379DD7BL,
    0x9B3660C6L, 0x9FF77D71L, 0x92B45BA8L, 0x9675461FL,
    0x8832161AL, 0x8CF30BADL, 0x81B02D74L, 0x857130C3L,
    0x5D8A9099L, 0x594B8D2EL, 0x5408ABF7L, 0x50C9B640L,
    0x4E8EE645L, 0x4A4FFBF2L, 0x470CDD2BL, 0x43CDC09CL,
    0x7B827D21L, 0x7F436096L, 0x7200464FL, 0x76C15BF8L,
    0x68860BFDL, 0x6C47164AL, 0x61043093L, 0x65C52D24L,
    0x119B4BE9L, 0x155A565EL, 0x18197087L, 0x1CD86D30L,
    0x029F3D35L, 0x065E2082L, 0x0B1D065BL, 0x0FDC1BECL,
    0x3793A651L, 0x3352BBE6L, 0x3E119D3FL, 0x3AD08088L,
    0x2497D08DL, 0x2056CD3AL, 0x2D15EBE3L, 0x29D4F654L,
    0xC5A92679L, 0xC1683BCEL, 0xCC2B1D17L, 0xC8EA00A0L,
    0xD6AD50A5L, 0xD26C4D12L, 0xDF2F6BCBL, 0xDBEE767CL,
    0xE3A1CBC1L, 0xE760D676L, 0xEA23F0AFL, 0xEEE2ED18L,
    0xF0A5BD1DL, 0xF464A0AAL, 0xF9278673L, 0xFDE69BC4L,
    0x89B8FD09L, 0x8D79E0BEL, 0x803AC667L, 0x84FBDBD0L,
    0x9ABC8BD5L, 0x9E7D9662L, 0x933EB0BBL, 0x97FFAD0CL,
    0xAFB010B1L, 0xAB710D06L, 0xA6322BDFL, 0xA2F33668L,
    0xBCB4666DL, 0xB8757BDAL, 0xB5365D03L, 0xB1F740B4L
};
/***************************************************************/
uint32 calc_crc32(uint32 crc32, const char *buf, int buflen) {

    register int ii;
    register int jj;

    for ( jj = 0;  jj < buflen;  jj++ ) {
        ii = ( (int) ( crc32 >> 24) ^ *buf++ ) & 0xFF;
        crc32 = ( crc32 << 8 ) ^ crc32tab[ii];
    }

   return (crc32);
}
/***************************************************************/
#endif
/***************************************************************/
int demi_set_error(struct demirec * demi, int dstat, const char * fmt, ...) {

	va_list args;
    int oldlen;
    int msgbuflen;
    char msgbuf[DEMI_ERRMSG_SIZE];

	va_start(args, fmt);
	vsprintf(msgbuf, fmt, args);
	va_end(args);

    fprintf(stderr, "%s (DEMIERR %d)\n", msgbuf, dstat);
    msgbuflen = (int)strlen(msgbuf);
    if (!(demi->demi_errmsg)) {
        demi->demi_errmsg = New(char, msgbuflen + 1);
        oldlen = 0;
    } else {
        oldlen = (int)strlen(demi->demi_errmsg);
        demi->demi_errmsg = Realloc(demi->demi_errmsg, char, oldlen + msgbuflen + 2);
        demi->demi_errmsg[oldlen] = '\n';
        oldlen++;
    }
    strcpy(demi->demi_errmsg + oldlen, msgbuf);

	return (dstat);
}
/***************************************************************/
static void sdemi_get_cfg(unsigned char * cfg)
{
/*
** 12/08/2019
*/
    int32 endian = 1;

    cfg[DEMI_CFGIX_ENDIAN           ] = ((unsigned char*)&endian)[3];
    cfg[DEMI_CFGIX_ALIGN            ] = (unsigned char)(sizeof(void*) & 0xFF);
    cfg[DEMI_CFGIX_CHAR_BYTES       ] = (unsigned char)(sizeof(char) & 0xFF);
    cfg[DEMI_CFGIX_SHORT_BYTES      ] = (unsigned char)(sizeof(short) & 0xFF);
    cfg[DEMI_CFGIX_INT_BYTES        ] = (unsigned char)(sizeof(int) & 0xFF);
    cfg[DEMI_CFGIX_LONG_BYTES       ] = (unsigned char)(sizeof(long) & 0xFF);
    cfg[DEMI_CFGIX_WCHAR_BYTES      ] = (unsigned char)(sizeof(wchar_t) & 0xFF);
    cfg[DEMI_CFGIX_PTR_BYTES        ] = (unsigned char)(sizeof(void*) & 0xFF);
    cfg[DEMI_CFGIX_FLOAT_BYTES      ] = (unsigned char)(sizeof(float) & 0xFF);
    cfg[DEMI_CFGIX_DOUBLE_BYTES     ] = (unsigned char)(sizeof(double) & 0xFF);
    cfg[DEMI_CFGIX_LONG_LONG_BYTES  ] = (unsigned char)(sizeof(long long) & 0xFF);
    cfg[DEMI_CFGIX_AVAIL_1          ] = 0;
}
/***************************************************************/
static struct demimemory * demc_new_demmem(int memsize)
{
/*
** 12/15/2019
*/
    struct demimemory * demmem;

    demmem = New(struct demimemory, 1);
    demmem->demmem_size   = memsize;
    demmem->demmem_length = 0;
    demmem->demmem_ptr    = New(uchar, memsize);

    return (demmem);
}
/***************************************************************/
struct demirec * demi_new_demirec(void)
{
/*
** 12/07/2019
*/
    struct demirec * demi;

    demi = New(struct demirec, 1);
    demi->demi_dstat = 0;
    demi->demi_flags = 0;
    demi->demi_itemtree = dbtree_new();
    demi->demi_errmsg = NULL;

    demi->demi_nfuncs = 0;
    demi->demi_xfuncs = 0;
    demi->demi_funcs  = NULL;

    sdemi_get_cfg(demi->demi_cfg);

    return (demi);
}
/***************************************************************/
struct demi_boss * demb_new_demi_boss(void)
{
/*
** 12/14/2019
*/
    struct demi_boss * demb;

    demb = New(struct demi_boss, 1);
    demb->demb_flags = 0;
    demb->demb_nfuncs = 0;
    demb->demb_channel = 0;
    demb->demb_channel_type = DEMI_DEFAULT_CHANNEL_TYPE;
    demb->demb_max_channels  = DEMI_MAX_CHANNELS;
    demb->demb_nchannels  = 0;
    demb->demb_channel = NULL;
    demb->demb_demi = demi_new_demirec();
    demb->demb_send_buffer = demc_new_demmem(DEMIMEMORY_DEFAULT_SIZE);

    return (demb);
}
/***************************************************************/
void demi_get_errmsg(struct demirec * demi, char * errmsg, int errmsgsz)
{
/*
** 12/07/2019
*/
    int oldlen;

    if (!(demi->demi_errmsg)) {
        if (errmsgsz > 0) errmsg[0] = '\0';
    } else {
        oldlen = (int)strlen(demi->demi_errmsg);
        if (oldlen < errmsgsz) {
            strcpy(errmsg, demi->demi_errmsg);
        } else if (errmsgsz > 0) {
            memcpy(errmsg, demi->demi_errmsg, errmsgsz - 1);
            errmsg[errmsgsz - 1] = '\0';
        }
    }
}
/***************************************************************/
char * demi_get_errmsg_ptr(struct demirec * demi)
{
/*
** 12/19/2019
*/
    return (demi->demi_errmsg);
}
/***************************************************************/
char * demb_get_errmsg_ptr(struct demi_boss * demb)
{
/*
** 12/07/2019
*/
    return (demb->demb_demi->demi_errmsg);
}
/***************************************************************/
static void demc_free_demmem(struct demimemory * demmem)
{
/*
** 12/15/2019
*/
    Free(demmem->demmem_ptr);
    Free(demmem);
}
/***************************************************************/
static void sdemc_free_demichannel(struct demichannel * demc)
{
/*
** 12/21/2019
*/
    demc_free_demmem(demc->demc_recv_demmem);
    demc_free_demmem(demc->demc_send_demmem);
}
/***************************************************************/
static void sdemi_demival_release(struct demival * demv)
{
/*
** 12/14/2019
*/
    switch (demv->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            break;
        case DEMIVAL_TYP_INT:
            break;
        case DEMIVAL_TYP_STR:
            Free(demv->demv_u.demv_str);
            break;
        default:
            break;
    }
    memset(demv, 0, sizeof(struct demival));
}
/***************************************************************/
static void sdemi_demival_free(struct demival * demv)
{
/*
** 12/14/2019
*/
    sdemi_demival_release(demv);
    Free(demv);
}
/***************************************************************/
static void sdemi_demiitem_release(struct demiitem * demm)
{
/*
** 12/14/2019
*/

    switch (demm->demm_item_typ) {
        case DEMI_ITMTYP_CONST:
            sdemi_demival_free(demm->demm_u.demm_demv);
            break;
        case DEMI_ITMTYP_NAMED_TYPE:
            break;
        case DEMI_ITMTYP_FUNC:
            break;
        default:
            break;
    }
    Free(demm->demm_item_nam);
    memset(demm, 0, sizeof(struct demiitem));
}
/***************************************************************/
static void sdemi_demiitem_free(struct demiitem * demm)
{
/*
** 12/07/2019
*/
    sdemi_demiitem_release(demm);
    Free(demm);
}
/***************************************************************/
static void demi_free_vdemiitem(void * vdemm)
{
/*
** 12/07/2019
*/
    sdemi_demiitem_free((struct demiitem *)vdemm);
}
/***************************************************************/
void demi_free_demirec(struct demirec * demi)
{
/*
** 12/07/2019
*/
    Free(demi->demi_errmsg);

    dbtree_free(demi->demi_itemtree, demi_free_vdemiitem);
    Free(demi->demi_funcs);

    Free(demi);
}
/***************************************************************/
void demb_free_demi_boss(struct demi_boss * demb)
{
/*
** 12/14/2019
*/
    int chix;

    demi_free_demirec(demb->demb_demi);
    demc_free_demmem(demb->demb_send_buffer);


    for (chix = 0; chix < demb->demb_nchannels; chix++) {
        sdemc_free_demichannel(demb->demb_channel[chix]);
    }
    Free(demb->demb_channel);

    Free(demb);
}
/***************************************************************/
static void sdemc_free_peer_demichannel(struct demichannel * demc)
{
/*
** 01/12/2020
*/
    sdemc_free_demichannel(demc);
}
/***************************************************************/
static void sdemi_free_demi_worker(struct demi_worker * demw)
{
/*
** 01/12/2020
*/
    sdemc_free_peer_demichannel(demw->demw_channel);
    demi_free_demirec(demw->demw_demi);
    Free(demw);
}
/***************************************************************/
void demb_shut(struct demi_boss * demb)
{
/*
** 12/14/2019
*/
    demb_free_demi_boss(demb);
}
/***************************************************************/
static void sdemi_demifield_free(struct demifield * demf)
{
/*
** 12/23/2019
*/
    Free(demf->demf_name);
    sdemi_demityp_free(demf->demf_dmityp);
    Free(demf);
}
/***************************************************************/
static void sdemi_demityp_struct_free(struct demityp_struct * demts)
{
/*
** 12/23/2019
*/
    int ii;

    for (ii = 0; ii < demts->demts_num_fields; ii++) {
        sdemi_demifield_free(demts->demts_fields[ii]);
    }
    Free(demts->demts_fields);
    Free(demts);
}
/***************************************************************/
static void sdemi_demityp_array_free(struct demityp_array * demta)
{
/*
** 12/23/2019
*/
    sdemi_demityp_free(demta->demta_dmityp);
    Free(demta);
}
/***************************************************************/
static void sdemi_demityp_ptr_free(struct demityp_ptr * demtp)
{
/*
** 12/23/2019
*/
    sdemi_demityp_free(demtp->demtp_dmityp);
    Free(demtp);
}
/***************************************************************/
static struct demival * sdemi_demival_new(void)
{
/*
** 12/14/2019
*/
    struct demival * demv;

    demv = New(struct demival, 1);
    demv->demt_val_typ = 0;
    demv->demv_u.demv_ptr = NULL;

    return (demv);
}
/***************************************************************/
static struct demityp_array * sdemi_demityp_array_new(void)
{
/*
** 12/22/2019
*/
    struct demityp_array * demta;

    demta = New(struct demityp_array, 1);
    demta->demta_array_type     = 0;
    demta->demta_array_stride   = 0;
    demta->demta_num_elements   = 0;
    demta->demta_total_size     = 0;
    demta->demta_dmityp         = NULL;

    return (demta);
}
/***************************************************************/
static struct demityp_ptr * sdemi_demityp_ptr_new(void)
{
/*
** 12/24/2019
*/
    struct demityp_ptr * demtp;

    demtp = New(struct demityp_ptr, 1);
    demtp->demtp_ptr_type       = 0;
    demtp->demtp_ptrdat         = '\0';
    demtp->demtp_array_stride   = 0;
    demtp->demtp_dmityp         = NULL;

    return (demtp);
}
/***************************************************************/
static struct demifield * sdemi_demifield_new(void)
{
/*
** 12/24/2019
*/
    struct demifield * demf;

    demf = New(struct demifield, 1);
    demf->demf_name         = NULL;
    demf->demf_offset       = 0;
    demf->demf_aligned_size = 0;
    demf->demf_dmityp       = NULL;

    return (demf);
}
/***************************************************************/
static struct demityp_struct * sdemi_demityp_struct_new(void)
{
/*
** 12/14/2019
*/
    struct demityp_struct * demts;

    demts = New(struct demityp_struct, 1);
    demts->demts_num_fields     = 0;
    demts->demts_max_fields     = 0;
    demts->demts_total_size     = 0;
    demts->demts_fields         = NULL;

    return (demts);
}
/***************************************************************/
static struct demityp * sdemi_demityp_new(int simp_typ)
{
/*
** 12/22/2019
*/
    struct demityp * demt;

    demt = New(struct demityp, 1);
    demt->demt_simp_typ     = simp_typ;
    demt->demt_typ_flags    = 0;
    demt->demt_typ_size     = 0;
    demt->demt_u.demt_ptr   = NULL;

    return (demt);
}
/***************************************************************/
static struct deminamedtyp * sdemi_deminamedtyp_new(struct demityp * demt)
{
/*
** 01/22/2019
*/
    struct deminamedtyp * demnt;

    demnt = New(struct deminamedtyp, 1);
    demnt->demnt_demt         = demt;
    demnt->demnt_name         = NULL;
    demnt->demnt_named_demiitem = NULL;
    demnt->demnt_demp_pack    = NULL;
    demnt->demnt_demp_unpack  = NULL;
    demnt->demnt_pack_tgtdemmem    = demc_new_demmem(DEMICPU_MEMORY_DEFAULT_SIZE);
    demnt->demnt_unpack_tgtdemmem  = demc_new_demmem(DEMICPU_MEMORY_DEFAULT_SIZE);

    return (demnt);
}
/***************************************************************/
static struct demiitem * sdemi_demiitem_new(int item_typ)
{
/*
** 12/14/2019
*/
    struct demiitem * demm;

    demm = New(struct demiitem, 1);
    demm->demm_item_nam = NULL;
    demm->demm_item_typ = item_typ;
    demm->demm_u.demm_ptr = NULL;

    return (demm);
}
/***************************************************************/
static struct demifunc * sdemi_demifunc_new(const char * funcname)
{
/*
** 01/05/2020
*/
    struct demifunc * demu;

    demu = New(struct demifunc, 1);
    demu->demu_name = Strdup(funcname);
    demu->demu_func_index = 0;
    demu->demu_func = NULL;
    demu->demu_in_demnt = NULL;
    demu->demu_out_demnt = NULL;

    return (demu);
}
/***************************************************************/
static uchar * demmem_resize(struct demimemory * demmem, int memsize)
{
/*
** 12/16/2019
*/
    if (memsize > demmem->demmem_size) {
        demmem->demmem_size = memsize + 64;
        demmem->demmem_ptr =
            Realloc(demmem->demmem_ptr, uchar, demmem->demmem_size);
    }

    return (demmem->demmem_ptr);
}
/***************************************************************/
static void demmem_copy(struct demimemory * demmem, uchar * src, int srclen)
{
/*
** 12/19/2019
*/
    uchar * ptr;

    ptr = demmem_resize(demmem, srclen);
    memcpy(ptr, src, srclen);
    demmem->demmem_length = srclen;
}
/***************************************************************/
static struct demichannel * sdemc_new_demichannel(int channel_type)
{
/*
** 12/10/2019
*/
    struct demichannel * demc;

    demc = New(struct demichannel, 1);
    demc->demc_channel_type = channel_type;
    demc->demc_pkt_recv_sn  = 0;
    demc->demc_pkt_send_sn  = 0;
    demc->demc_recv_demmem  = NULL;
    demc->demc_send_demmem  = NULL;
    demc->demc_u.demc_ptr   = NULL;

    return (demc);
}
/***************************************************************/
static struct demichannel * sdemc_new_peer_demichannel(struct demichannel * demc_src)
{
/*
** 12/10/2019
*/
    struct demichannel * demc_peer;

    demc_peer = sdemc_new_demichannel(demc_src->demc_channel_type);
    demc_peer->demc_recv_demmem = demc_src->demc_send_demmem;
    demc_peer->demc_send_demmem = demc_src->demc_recv_demmem;

    return (demc_peer);
}
/***************************************************************/
static struct demi_worker * sdemi_new_demi_worker(struct demichannel * demc)
{
/*
** 12/19/2019
*/
    struct demi_worker * demw;

    demw = New(struct demi_worker, 1);
    memset(demw->demw_boss_cfg, 0, DEMI_CFG__SIZE);
    demw->demw_channel_type = demc->demc_channel_type;
    demw->demw_channel = sdemc_new_peer_demichannel(demc);
    demw->demw_demi = demi_new_demirec();

    return (demw);
}
/***************************************************************/
static int demi_ensure_demifunc(struct demirec * demi,
        const char * funcname,
        struct demifunc ** pdemu)
{
/*
** 01/05/2020
*/
    int dstat;
    int dup;
    void ** tnode;
    struct demiitem * demm;
    struct demifunc * demu;

    dstat = 0;
    (*pdemu) = NULL;
    demu = NULL;

    tnode = dbtree_find(demi->demi_itemtree, funcname, STRBYTES(funcname));
    if (!tnode) {
        demu = sdemi_demifunc_new(funcname);
        demm = sdemi_demiitem_new(DEMI_ITMTYP_FUNC);
        demm->demm_item_nam = Strdup(funcname);
        demm->demm_u.demm_demu = demu;
        dup = dbtree_insert(demi->demi_itemtree, funcname, STRBYTES(funcname), demm);
    } else {
        demm = *(struct demiitem **)tnode;
        if (demm->demm_item_typ != DEMI_ITMTYP_FUNC) {
            dstat = demi_set_error(demi, DEMIERR_FUNC_NOT_FUNC,
                "Function already declared as another item. Found: '%s'", funcname);
        } else {
            demu = demm->demm_u.demm_demu;
        }
    }

    if (!dstat) {
        (*pdemu) = demu;
    }

    return (dstat);
}
/***************************************************************/
static int demi_register_function(void * vdemi,
        const char * funcname,
        DEMI_CALLABLE_FUNCTION func)
{
/*
** 01/05/2020
*/
    int dstat;
    struct demirec * demi;
    struct demifunc * demu;

    demi = (struct demirec *)vdemi;

    dstat = demi_ensure_demifunc(demi, funcname, &demu);

    if (!dstat) {
        if (demu->demu_func) {
            dstat = demi_set_error(demi, DEMIERR_DUPLICATE_FUNC_REGISTRATION,
                "Function already registered. Found: '%s'", funcname);
        } else {
            demu->demu_func = func;
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_init_demi_worker(struct demirec * demi, const char * funccfg)
{
/*
** 01/05/2020
*/
    int dstat;

    dstat = demi_register_callable_functions(demi, demi_register_function, funccfg);

    return (dstat);
}
/***************************************************************/
static struct demichan_direct * sdemi_new_demichan_direct(struct demichannel * demc)
{
/*
** 12/10/2019
*/
    struct demichan_direct * demcd;

    demcd = New(struct demichan_direct, 1);
    demcd->demcd_demw     = sdemi_new_demi_worker(demc);

    return (demcd);
}
/***************************************************************/
static int sdemi_channel_startup(struct demirec * demi,
    struct demichannel * demc,
    const char * funccfg)
{
/*
** 12/10/2019
*/
    int dstat;

    dstat = 0;

    switch (demc->demc_channel_type) {
        case DEMI_CHANNEL_DIRECT:
            demc->demc_u.demc_demcd = sdemi_new_demichan_direct(demc);
            dstat = sdemi_init_demi_worker(demc->demc_u.demc_demcd->demcd_demw->demw_demi, funccfg);
            break;

        default:
            dstat = demi_set_error(demi, DEMIERR_CHANNEL_UNSUPPORTED,
                "Channel %d is not supported.", demc->demc_channel_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_start_channel(struct demi_boss * demb,
    struct demichannel ** pdemc,
    const char * funccfg)
{
/*
** 12/10/2019
*/
    int dstat;
    struct demichannel * demc;

    dstat = 0;
    (*pdemc) = NULL;

    demc = sdemc_new_demichannel(demb->demb_channel_type);
    demc->demc_recv_demmem = demc_new_demmem(DEMIMEMORY_DEFAULT_SIZE);
    demc->demc_send_demmem = demc_new_demmem(DEMIMEMORY_DEFAULT_SIZE);

    dstat = sdemi_channel_startup(demb->demb_demi, demc, funccfg);

    if (!dstat) {
        (*pdemc) = demc;
        demb->demb_nchannels += 1;
    }

    return (dstat);
}
/***************************************************************/
static int sdemc_send_pkt(
    struct demirec * demi,
    struct demichannel * demc,
    void * sendpkt,
    int sendpktlen)
{
/*
** 12/15/2019
*/
    int dstat;

    dstat = 0;
    switch (demc->demc_channel_type) {
        case DEMI_CHANNEL_DIRECT:
            break;
        default:
            dstat = demi_set_error(demi, DEMCHAN_UNSUPPORTED,
                "Channel %d is not supported.", demc->demc_channel_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
static int sdemc_prepare_send_pkt(
    struct demichannel * demc,
    uchar mop,
    void * msgdata,
    int msgdatalen,
    void ** sendpkt,
    int * sendpktlen)
{
/*
** 12/15/2019
*/
    int dstat;
    int pktlen;
    char * pktbuf;

    dstat = 0;
    pktlen = DEMI_PKT_HEAD_LEN + msgdatalen + DEMI_PKT_TAIL_LEN;
    pktbuf = demmem_resize(demc->demc_send_demmem, pktlen);
    pktbuf[0] = mop;
    pktbuf[1] = (pktlen >> 8) & 0xFF;
    pktbuf[2] = pktlen        & 0xFF;
    memcpy(pktbuf + 3, msgdata, msgdatalen);
    (*sendpktlen) = pktlen;

    return (dstat);
}
/***************************************************************/
static int sdemc_sendmsg(
    struct demirec * demi,
    struct demichannel * demc,
    uchar mop,
    void * msgdata,
    int msgdatalen)
{
/*
** 12/15/2019
*/
    int dstat;
    int sendpktlen;
    void * sendpkt;

    dstat = sdemc_prepare_send_pkt(demc, mop, msgdata, msgdatalen,
                    &sendpkt, &sendpktlen);
    if (!dstat) {
        switch (demc->demc_channel_type) {
            case DEMI_CHANNEL_DIRECT:
                dstat = sdemc_send_pkt(demi, demc, sendpkt, sendpktlen);
                break;
           default:
                dstat = demi_set_error(demi, DEMCHAN_UNSUPPORTED,
                    "Channel %d is not supported.", demc->demc_channel_type);
                break;
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemc_recvmsg(struct demirec * demi, struct demichannel * demc)
{
/*
** 12/16/2019
*/
    int dstat;
    int pktsentlen;
    uchar * pktsent;

    dstat = 0;
    switch (demc->demc_channel_type) {
        case DEMI_CHANNEL_DIRECT:
            pktsent = demc->demc_recv_demmem->demmem_ptr;
            /* mop = pktsent[0]; */
            pktsentlen = (pktsent[1] << 8) | pktsent[2];
            demmem_copy(demc->demc_recv_demmem, pktsent, pktsentlen);
            break;

        default:
            dstat = demi_set_error(demi, DEMCHAN_UNSUPPORTED,
                "Channel %d is not supported.", demc->demc_channel_type);
            break;
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_msghndlr_ok(struct demi_boss * demb, uchar * msg, int msglen)
{
/*
** 12/21/2019 - Boss process DMTB_OK message
*/
    int dstat;

    dstat = 0;

    return (dstat);
}
/***************************************************************/
static int sdemb_msghndlr_err(struct demi_boss * demb, uchar * msg, int msglen)
{
/*
** 01/13/2020 - Boss process DMTB_OK message
*/
    int dstat;
    int in_dstat;
    int errmsglen;
    char * errmsg;

    dstat = 0;
    if (msglen < DEMC_ERR_MIN_LEN) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_ERR_MSGLEN,
            "ERROR type message too short. Length=%d Minimum=%d", msglen , DEMC_ERR_MIN_LEN);
    } else {
        in_dstat  = (msg[0] << 8) | msg[1];
        errmsglen = (msg[2] << 8) | msg[3];
        if (errmsglen + DEMC_ERR_MIN_LEN != msglen) {
            dstat = demi_set_error(demb->demb_demi, DEMIERR_ERR_MSGLEN,
                "ERROR type message length mismatch. Length=%d Expected=%d",
                    msglen, errmsglen + DEMC_ERR_MIN_LEN);
        } else {
            errmsg = New(char, errmsglen + 1);
            memcpy(errmsg, msg + DEMC_ERR_MIN_LEN, errmsglen);
            errmsg[errmsglen] = '\0';
            dstat = demi_set_error(demb->demb_demi, DEMIERR_WORKER_ERROR,
                "Worker error %d: %s",
                    in_dstat, in_dstat);
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_msghndlr_result(struct demi_boss * demb, uchar * msg, int msglen)
{
/*
** 12/21/2019 - Boss process DMTB_OK message
*/
    int dstat;

    dstat = 0;

    return (dstat);
}
/***************************************************************/
static int sdemb_process_msg(struct demi_boss * demb, uchar * msg, int msglen)
{
/*
** 12/19/2019 - Boss process received message
*/
    int dstat;

    dstat = 0;
    if (msglen < DEMC_MIN_MESSAGE_LENGTH) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_MSG_LENGTH_SHORT,
            "Boss message length is %d, but the minimum is %d.", msglen, DEMC_MIN_MESSAGE_LENGTH);
    } else {
        switch (msg[0]) {
            case DMTB_OK:
                dstat = sdemb_msghndlr_ok(demb, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTB_ERR:
                dstat = sdemb_msghndlr_err(demb, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTB_FUNC_RESULT:
                dstat = sdemb_msghndlr_result(demb, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;

            default:
                dstat = demi_set_error(demb->demb_demi, DEMIERR_UNRECOGNIZED_BOSS_MSG,
                    "Unrecognized boss msg: %d", (int)(msg[0]));
                break;
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemw_msghndlr_cfg(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 12/19/2019 - Worker process DMTW_CFG message
*/
    int dstat;

    dstat = 0;
    if (msglen != DEMI_CFG__SIZE) {
        dstat = demi_set_error(demw->demw_demi, DEMIERR_MSG_LENGTH_IMPROPER,
            "Worker cfg message length is %d, but the expected is %d.", msglen, DEMI_CFG__SIZE);
    } else {
        memcpy(demw->demw_boss_cfg, msg, msglen);
        if (memcmp(demw->demw_demi->demi_cfg, demw->demw_boss_cfg, DEMI_CFG__SIZE)) {
            dstat = demi_set_error(demw->demw_demi, DEMIERR_CONFIG_MISMATCH,
                "Worker/boss configuration mismatch.");
        }
    }

    return (dstat);
}
/***************************************************************/
static int recvbuf_int_data(uchar * msg, int msglen, int * msgix, int * val)
{
/*
** 12/21/2019
*/
    int derrn;
    unsigned int uval;

    derrn = 0;
    uval = 0;
    while ((*msgix) < msglen && msg[*msgix] & 0x80) {
        uval |= msg[*msgix] & 0x7F;
        (*msgix) += 1;
        uval <<= 7;
    }
    if ((*msgix) < msglen) {
        uval |= msg[*msgix];
        (*msgix) += 1;
    } else {
        derrn = DEMIERR_RECV_INT_OVFL;
    }
    (*val) = (int)uval;

    return (derrn);
}
/***************************************************************/
static int recvbuf_cstr_data(uchar * msg, int msglen, int * msgix, char ** cstr)
{
/*
** 12/21/2019
*/
    int derrn;
    int slen;

    derrn = recvbuf_int_data(msg, msglen, msgix, &slen);
    if ((*msgix) + slen <= msglen) {
        (*cstr) = New(char, slen + 1);
        memcpy((*cstr), msg + (*msgix), slen);
        (*cstr)[slen] = '\0';
        (*msgix) += slen;
    } else {
        derrn = DEMIERR_RECV_CSTR_OVFL;
    }

    return (derrn);
}
/***************************************************************/
#ifdef UNUSED
static int recvbuf_demival_data(uchar * msg, int msglen, int * msgix, struct demival ** demv)
{
/*
** 12/21/2019
*/
    int derrn;
    int val_typ;

    if ((*msgix) < msglen) {
        val_typ = msg[*msgix];
        (*msgix) += 1;
        (*demv) = sdemi_demival_new();
        (*demv)->demt_val_typ = val_typ;
        switch (val_typ) {
            case DEMIVAL_TYP_BOOL:
                (*demv)->demv_u.demv_bool = msg[*msgix]?1:0;
                (*msgix) += 1;
                break;
            case DEMIVAL_TYP_INT:
                derrn = recvbuf_int_data(msg, msglen, msgix, &((*demv)->demv_u.demv_int));
                break;
            case DEMIVAL_TYP_STR:
                derrn = recvbuf_cstr_data(msg, msglen, msgix, &((*demv)->demv_u.demv_str));
                break;
            case 0:
                break;

            default:
                break;
        }
    } else {
        derrn = DEMIERR_RECV_VAL_OVFL;
    }

    return (derrn);
}
#endif
/***************************************************************/
static int sdemw_msghndlr_const(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 12/19/2019 - Worker process DMTW_CONST message
*/
    int dstat;
    int derrn;
    int msgix;
    int dup;
    char * constname;
    char * constbuf;
    struct demival * demv;
    struct demiitem * demm;

    dstat = 0;
    if (msglen < DEMI_CONST_MIN) {
        dstat = demi_set_error(demw->demw_demi, DEMIERR_MSG_LENGTH_IMPROPER,
            "Worker const message length is %d, but the minimum is %d.", msglen, DEMI_CONST_MIN);
    } else {
        msgix = 0;
        derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(constname));
        if (!derrn) {
            derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(constbuf));
        }
        if (derrn) {
            dstat = demi_set_error(demw->demw_demi, derrn,
                "Worker const message error.");
        }
        if (!dstat) {
            demv = sdemi_demival_new();

            dstat = sdemi_eval_expression(demw->demw_demi, constbuf, demv);
            Free(constbuf);
        }
        if (!dstat) {
            if (msgix != msglen) {
                dstat = demi_set_error(demw->demw_demi, DEMIERR_RECV_LENGTH_MISMATCH,
                    "Worker receive const message length mismatch: len=%d, ix=%d",
                        msglen, msgix);
            } else {
                demm = sdemi_demiitem_new(DEMI_ITMTYP_CONST);
                demm->demm_item_nam = constname;
                demm->demm_u.demm_demv = demv;
                dup = dbtree_insert(demw->demw_demi->demi_itemtree, constname, STRBYTES(constname), demm);
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemw_msghndlr_type(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 01/12/2020 - Worker process DMTW_TYPE message
*/
    int dstat;
    int derrn;
    int msgix;
    int dup;
    int typsize;
    char * typname;
    char * typdesc;
    struct deminamedtyp * demnt;
    struct demityp * demt;
    struct demiitem * demm;

    dstat = 0;
    if (msglen < DEMI_CONST_MIN) {
        dstat = demi_set_error(demw->demw_demi, DEMIERR_MSG_LENGTH_IMPROPER,
            "Worker type message length is %d, but the minimum is %d.", msglen, DEMI_CONST_MIN);
    } else {
        msgix = 0;
        derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(typname));
        if (!derrn) {
            derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(typdesc));
        }
        if (!derrn) {
            derrn = recvbuf_int_data(msg, msglen, &msgix, &typsize);
        }
        if (derrn) {
            dstat = demi_set_error(demw->demw_demi, derrn,
                "Worker type message error on type: %s", typname);
        }
 
        if (!dstat) {
            demt = NULL;
            dstat = sdemi_parse_type(demw->demw_demi, typdesc, &demt, typsize);
        }

        if (!dstat) {
            if (msgix != msglen) {
                dstat = demi_set_error(demw->demw_demi, DEMIERR_RECV_LENGTH_MISMATCH,
                    "Worker receive type message length mismatch: len=%d, ix=%d",
                        msglen, msgix);
            } else {
                demnt = sdemi_deminamedtyp_new(demt);
                demnt->demnt_name = Strdup(typname);
                demm = sdemi_demiitem_new(DEMI_ITMTYP_NAMED_TYPE);
                demm->demm_item_nam = typname;
                demm->demm_u.demm_demnt = demnt;
                dup = dbtree_insert(demw->demw_demi->demi_itemtree, typname, STRBYTES(typname), demm);
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static void sdemi_add_callable_func(struct demirec * demi,
    struct demifunc * demu,
    int func_ref)
{
/*
** 01/12/2020
*/
    int func_index;

    func_index = FUNC_REF_TO_INDEX(func_ref);
    demu->demu_func_index = func_index;

    if (func_index >= demi->demi_xfuncs) {
        if (!demi->demi_xfuncs) demi->demi_xfuncs = 4;
        else demi->demi_xfuncs *= 2;
        if (func_index >= demi->demi_xfuncs) demi->demi_xfuncs = func_index + 1;
        demi->demi_funcs  = Realloc(demi->demi_funcs, struct demifunc *, demi->demi_xfuncs);
    }
    demi->demi_funcs[func_index] = demu;
}
/***************************************************************/
static int sdemw_msghndlr_func_define(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 01/12/2020 - Worker process DMTW_FUNC_DEFINE message
*/
    int dstat;
    int derrn;
    int func_ref;
    int msgix;
    char * funcname;
    char * funcinargs;
    char * funcoutargs;
    struct demifunc * demu;

    dstat = 0;
    if (msglen < DEMI_CONST_MIN) {
        dstat = demi_set_error(demw->demw_demi, DEMIERR_MSG_LENGTH_IMPROPER,
            "Worker func message length is %d, but the minimum is %d.", msglen, DEMI_CONST_MIN);
    } else {
        msgix = 0;
        derrn = recvbuf_int_data(msg, msglen, &msgix, &func_ref);
        if (!derrn) {
            derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(funcname));
        }
        if (!derrn) {
            derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(funcinargs));
        }
        if (!derrn) {
            derrn = recvbuf_cstr_data(msg, msglen, &msgix, &(funcoutargs));
        }
        if (derrn) {
            dstat = demi_set_error(demw->demw_demi, derrn,
                "Worker func message error.");
        }
 
        if (!dstat) {
            dstat = demi_ensure_demifunc(demw->demw_demi, funcname, &demu);
        }
 
        if (!dstat) {
            sdemi_add_callable_func(demw->demw_demi, demu, func_ref);
        }

        if (!dstat) {
            if (msgix != msglen) {
                dstat = demi_set_error(demw->demw_demi, DEMIERR_RECV_LENGTH_MISMATCH,
                    "Worker receive type message length mismatch: len=%d, ix=%d",
                        msglen, msgix);
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemw_msghndlr_func_call(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 12/19/2019 - Worker process DMTW_FUNC_CALL message
*/
    int dstat;

    dstat = 0;

    return (dstat);
}
/***************************************************************/
static int sdemw_msghndlr_shutdown(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 12/19/2019 - Worker process DMTW_SHUTDOWN message
*/
    int dstat;

    dstat = DSTAT_WORKER_SHUTDOWN;

    return (dstat);
}
/***************************************************************/
static int sdemw_send_response(struct demi_worker * demw, int in_dstat)
{
/*
** 12/19/2019 - Worker send response message
*/
    int dstat;
    char * msg;
    char * errmsg;
    int msglen;
    int errmsglen;

    if (!in_dstat) {
        dstat = sdemc_sendmsg(demw->demw_demi, demw->demw_channel, DMTB_OK, NULL, 0);
    } else if (in_dstat == DSTAT_WORKER_SHUTDOWN) {
        dstat = sdemc_sendmsg(demw->demw_demi, demw->demw_channel, DMTB_SHUTTING_DOWN, NULL, 0);
    } else {
        errmsg = demi_get_errmsg_ptr(demw->demw_demi);
        if (errmsg) errmsglen = (int)strlen(errmsg);
        else errmsglen = 0;
        msglen = errmsglen + 4;
        msg = New(uchar, msglen);
        msg[0] = (uchar)((in_dstat >> 8) & 0xFF);
        msg[1] = (uchar)(in_dstat        & 0xFF);
        msg[2] = (uchar)((errmsglen >> 8) & 0xFF);
        msg[3] = (uchar)(errmsglen        & 0xFF);
        dstat = sdemc_sendmsg(demw->demw_demi, demw->demw_channel, DMTB_ERR, msg, msglen);
        Free(msg);
    }

    return (dstat);
}
/***************************************************************/
static int sdemw_process_msg(struct demi_worker * demw, uchar * msg, int msglen)
{
/*
** 12/19/2019 - Worker process received message
*/
    int dstat;

    dstat = 0;
    if (msglen < DEMC_MIN_MESSAGE_LENGTH) {
        dstat = demi_set_error(demw->demw_demi, DEMIERR_MSG_LENGTH_SHORT,
            "Worker message length is %d, but the minimum is %d.", msglen, DEMC_MIN_MESSAGE_LENGTH);
    } else {
        switch (msg[0]) {
            case DMTW_CFG:
                dstat = sdemw_msghndlr_cfg(demw, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTW_CONST:
                dstat = sdemw_msghndlr_const(demw, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTW_TYPE:
                dstat = sdemw_msghndlr_type(demw, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTW_FUNC_DEFINE:
                dstat = sdemw_msghndlr_func_define(demw, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTW_FUNC_CALL:
                dstat = sdemw_msghndlr_func_call(demw, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;
            case DMTW_SHUTDOWN:
                dstat = sdemw_msghndlr_shutdown(demw, msg + DEMC_MSG_HEADER_LEN, msglen - DEMC_MSG_HEADER_LEN);
                break;

            default:
                dstat = demi_set_error(demw->demw_demi, DEMIERR_UNRECOGNIZED_WORKER_MSG,
                    "Unrecognized worker msg: %d", (int)(msg[0]));
                break;
        }
    }

    dstat = sdemw_send_response(demw, dstat);

    return (dstat);
}
/***************************************************************/
static int sdemw_recv_and_process_response(struct demi_worker * demw)
{
/*
** 12/19/2019 - Worker receive and process message
*/
    int dstat;
    struct demimemory * recv_demmem;

    dstat = sdemc_recvmsg(demw->demw_demi, demw->demw_channel);
    if (!dstat) {
        recv_demmem = demw->demw_channel->demc_recv_demmem;
        dstat = sdemw_process_msg(demw, recv_demmem->demmem_ptr, recv_demmem->demmem_length);
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_recv_and_process_response(struct demi_boss * demb, struct demichannel * demc)
{
/*
** 12/16/2019 - Boss receive and process message
*/
    int dstat;

    dstat = 0;
    switch (demc->demc_channel_type) {
        case DEMI_CHANNEL_DIRECT:
            dstat = sdemw_recv_and_process_response(demc->demc_u.demc_demcd->demcd_demw);
            if (!dstat) {
                dstat = sdemc_recvmsg(demb->demb_demi, demc);
            }
            break;

        default:
            dstat = demi_set_error(demb->demb_demi, DEMCHAN_UNSUPPORTED,
                "Channel %d is not supported.", demc->demc_channel_type);
            break;
    }

    if (!dstat) {
        dstat = sdemb_process_msg(demb, demc->demc_recv_demmem->demmem_ptr, demc->demc_recv_demmem->demmem_length);
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_init_channel(struct demi_boss * demb, struct demichannel * demc)
{
/*
** 12/15/2019
Send config message, recv OK
Send init message, recv OK
*/
    int dstat;

    dstat = sdemc_sendmsg(demb->demb_demi, demc,
                DMTW_CFG, demb->demb_demi->demi_cfg, DEMI_CFG__SIZE);
    if (!dstat) {
        dstat = sdemb_recv_and_process_response(demb, demc);
    }

    return (dstat);
}
/***************************************************************/
int demb_start(struct demi_boss * demb, const char * funccfg)
{
/*
** 12/10/2019
*/
    int dstat;
    int chix;
    struct demichannel * demc;

    dstat = 0;
    demb->demb_channel = New(struct demichannel *, demb->demb_max_channels);

    chix = 0;
    while (!dstat && chix < demb->demb_max_channels) {
        dstat = sdemb_start_channel(demb, &demc, funccfg);
        if (!dstat) {
            demb->demb_channel[chix] = demc;
            chix++;
        }
    }

    if (!dstat) {
        demb->demb_flags |= DEMS_FLAG_STARTED;
    }

    if (!dstat) {
        for (chix = 0; !dstat && chix < demb->demb_nchannels; chix++) {
            dstat = sdemb_init_channel(demb, demb->demb_channel[chix]);
        }
    }

    return (dstat);
}
/***************************************************************/
int demb_call_func(
    struct demi_boss * demb,
    const char * funcname,
    void * funcinargs,
    void ** funcoutargs)
{
/*
** 12/10/2019
*/
    int dstat;

    dstat = 0;
    if (!(demb->demb_flags & DEMS_FLAG_STARTED)) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_NOT_STARTED, "%s", "Not started.");
        return (dstat);
    }

    return (dstat);
}
/***************************************************************/
int demb_wait_all_funcs(struct demi_boss * demb, int wait_flags)
{
/*
** 12/10/2019
*/
    int dstat;

    dstat = 0;
    if (!(demb->demb_flags & DEMS_FLAG_STARTED)) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_NOT_STARTED, "%s", "Not started.");
        return (dstat);
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_get_toke_quote(struct demirec * demi, const char * expbuf, int * eix, char * toke, int tokesz)
{
/*
** 12/11/2019
*/
    int dstat;
    int done;
    char qch;
    char ch;
    int tix;

    dstat = 0;
    done = 0;
    tix = 0;
    qch = expbuf[*eix];
    (*eix)++;

    while (!done && !dstat) {
        ch = expbuf[*eix];
        if (!ch) {
            dstat = demi_set_error(demi, DEMIERR_MISSING_CLOSE_QUOTE, "%s",
                "Missing closing quotation mark.");
        } else if (tix + 1 >= tokesz) {
            dstat = demi_set_error(demi, DEMIERR_TOKEN_OVERFLOW, "%s",
                "Quoted token overflow.");
        } else if (ch == qch) {
            toke[tix++] = ch;
            (*eix)++;
            done = 1;
        } else if (ch == '\\') {
            (*eix)++;
            ch = expbuf[*eix];
            if (!ch) {
                dstat = demi_set_error(demi, DEMIERR_MISSING_CLOSE_QUOTE, "%s",
                    "Missing closing quotation mark.");
            } else if (isalnum(ch)) {
                switch (ch) {
                    case '0': toke[tix++] =  0; break;
                    case 'a': toke[tix++] =  7; break;
                    case 'f': toke[tix++] = 12; break;
                    case 'n': toke[tix++] = 10; break;
                    case 'r': toke[tix++] = 13; break;
                    default:
                        dstat = demi_set_error(demi, DEMIERR_INVALID_BACKSLASH_ESCAPE,
                            "Invalid backslash escape: '%c'", ch);
                     break;
                }
            } else if (ch < ' ' || ch > '~') {
                dstat = demi_set_error(demi, DEMIERR_INVALID_BACKSLASH_ESCAPE,
                    "Non-printing backslash escape: 0x%x", ch);
            } else {
                toke[tix++] = ch;
                (*eix)++;
            }
        } else {
            toke[tix++] = ch;
            (*eix)++;
        }
    }
    toke[tix] = '\0';

    return (dstat);
}
/***************************************************************/
static char sdemi_next_char(struct demirec * demi, const char * expbuf, int * eix)
{
/*
** 01/29/2020
*/
    char ch;

    while (isspace(expbuf[*eix])) (*eix)++;
    ch = expbuf[*eix];

    return (ch);
}
/***************************************************************/
static int sdemi_get_toke(struct demirec * demi, const char * expbuf, int * eix, char * toke, int tokesz)
{
/*
** 12/11/2019
*/
    int dstat;
    int tix;
    int done;
    char ch;

    dstat = 0;
    done = 0;
    tix = 0;
    while (isspace(expbuf[*eix])) (*eix)++;

    while (!done) {
        ch = expbuf[*eix];
        if (!ch) {
            done = 1;
        } else if (isalnum(ch) || ch == '_') {
            if (tix + 1 < tokesz) {
                toke[tix++] = ch;
            } else {
                dstat = demi_set_error(demi, DEMIERR_TOKEN_OVERFLOW, "%s",
                    "Token overflow.");
                done = 1;
            }
            (*eix)++;
        } else if (!tix && ch == '\"') {
            dstat = sdemi_get_toke_quote(demi, expbuf, eix, toke, tokesz);
            done = 1;
        } else if (isspace(ch)) {
            done = 1;
        } else {
            if (!tix) {
                toke[tix++] = ch;
                (*eix)++;
            }
            done = 1;
        }
    }
    toke[tix] = '\0';

    return (dstat);
}
/***************************************************************/
static int sdemi_toke_to_number(struct demirec * demi,
    const char * numbuf,
    struct demival * demv)
{
/*
** 12/13/2019 - Max value is 999999999
*/
    int dstat;
    int ix;
    int numval;

    dstat = 0;
    memset(demv, 0, sizeof(struct demival));
    numval = 0;
    ix = 0;
    while (!dstat && isdigit(numbuf[ix])) {
        if (ix >= 9) {
            dstat = demi_set_error(demi, DEMIERR_EXPECTING_NUMBER,
                "Integer overflow. Found: '%s'", numbuf);
        } else {
            numval = (numval * 10) + (numbuf[ix] - '0');
            ix++;
        }
    }

    if (!dstat && (!ix || numbuf[ix])) {
        dstat = demi_set_error(demi, DEMIERR_EXPECTING_NUMBER,
            "Expecting valid number. Found: '%s'", numbuf);
    }

    if (!dstat) {
        demv->demt_val_typ = DEMIVAL_TYP_INT;
        demv->demv_u.demv_int = numval;
    }

    return (dstat);
}
/***************************************************************/
static void sdemi_demityp_release(struct demityp * demt)
{
/*
** 12/22/2019
*/
    switch (demt->demt_simp_typ) {
        case DEMITYP_CHAR:
            break;
        case DEMITYP_INT8:
            break;
        case DEMITYP_WCHAR:
            break;
        case DEMITYP_INT16:
            break;
        case DEMITYP_INT32:
            break;
        case DEMITYP_INT64:
            break;
        case DEMITYP_FLOAT32:
            break;
        case DEMITYP_FLOAT64:
            break;
        case DEMITYP_STRUCT:
            sdemi_demityp_struct_free(demt->demt_u.demt_demts);
            break;
        case DEMITYP_ARRAY:
            sdemi_demityp_array_free(demt->demt_u.demt_demta);
            break;
        case DEMITYP_POINTER:
            sdemi_demityp_ptr_free(demt->demt_u.demt_demtp);
            break;
        default:
            break;
    }
    memset(demt, 0, sizeof(struct demityp));
}
/***************************************************************/
static void sdemi_demityp_free(struct demityp * demt)
{
/*
** 12/22/2019
*/
    if (demt) sdemi_demityp_release(demt);
    Free(demt);
}
/***************************************************************/
static void sdemi_deminamedtyp_free(struct deminamedtyp * demnt)
{
/*
** 01/22/2020
*/
    if (!demnt) return;

    sdemi_demityp_free(demnt->demnt_demt);
    Free(demnt->demnt_name);
    sdemi_free_demiprog(demnt->demnt_demp_pack);
    sdemi_free_demiprog(demnt->demnt_demp_unpack);
    demc_free_demmem(demnt->demnt_pack_tgtdemmem);
    demc_free_demmem(demnt->demnt_unpack_tgtdemmem);
}
/***************************************************************/
static char * sdemi_demival_display(struct demival * demv)
{
/*
** 12/14/2019
*/
    static char valbuf[128];

    switch (demv->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            if (demv->demv_u.demv_bool == 0) strcpy(valbuf, "false");
            else                             strcpy(valbuf, "true");
            break;
        case DEMIVAL_TYP_INT:
            sprintf(valbuf, "%d", demv->demv_u.demv_int);
            break;
        case DEMIVAL_TYP_STR:
            if (strlen(demv->demv_u.demv_str) + 4 < sizeof(valbuf)) {
                sprintf(valbuf, "\"%s\"", demv->demv_u.demv_str);
            } else {
                valbuf[0] = '\"';
                memcpy(valbuf + 1, demv->demv_u.demv_str, sizeof(valbuf) - 2);
                valbuf[ sizeof(valbuf) - 1] = '\0';
            }
            break;
        case 0:
            strcpy(valbuf, "** Null value type **");
            break;
        default:
            strcpy(valbuf, "** Bad value type **");
            break;
    }

    return (valbuf);
}
/***************************************************************/
static int sdemi_op_add(struct demirec * demi,
    struct demival * demvtgt,
    struct demival * demvsrc)
{
/*
** 12/14/2019
*/
    int dstat;
    int tlen;
    int nlen;

    dstat = 0;
    switch (demvsrc->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Add not available for boolean.");
            break;
        case DEMIVAL_TYP_INT:
            if (demvtgt->demt_val_typ != demvsrc->demt_val_typ) {
                dstat = demi_set_error(demi, DEMIERR_DTYPE_MISMATCH,
                    "%s", "Add operand not both integers.");
            } else {
                demvtgt->demv_u.demv_int += demvsrc->demv_u.demv_int;
            }
            break;
        case DEMIVAL_TYP_STR:
            if (demvtgt->demt_val_typ != demvsrc->demt_val_typ) {
                dstat = demi_set_error(demi, DEMIERR_DTYPE_MISMATCH,
                    "%s", "Add operand not both strings.");
            } else {
                tlen = (int)strlen(demvtgt->demv_u.demv_str);
                nlen = (int)strlen(demvsrc->demv_u.demv_str) + tlen + 1;
                demvtgt->demv_u.demv_str = Realloc(demvtgt->demv_u.demv_str, char, nlen);
                strcpy(demvtgt->demv_u.demv_str + tlen, demvsrc->demv_u.demv_str);
            }
            sdemi_demival_release(demvsrc);
            break;
        case 0:
            dstat = demi_set_error(demi, DEMIERR_NULL_VALUE_TYPE,
                "%s", "Null value type on add.");
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VALUE_TYPE,
                "Undefined value type on add. Found: '%d'", demvsrc->demt_val_typ);
            break;

    }

    return (dstat);
}
/***************************************************************/
static int sdemi_op_subtract(struct demirec * demi,
    struct demival * demvtgt,
    struct demival * demvsrc)
{
/*
** 12/14/2019
*/
    int dstat;

    dstat = 0;
    switch (demvsrc->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Subtract not available for boolean.");
            break;
        case DEMIVAL_TYP_INT:
            if (demvtgt->demt_val_typ != demvsrc->demt_val_typ) {
                dstat = demi_set_error(demi, DEMIERR_DTYPE_MISMATCH,
                    "%s", "Subtract operands not both integers.");
            } else {
                demvtgt->demv_u.demv_int -= demvsrc->demv_u.demv_int;
            }
            break;
        case DEMIVAL_TYP_STR:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Subtract not available for strings.");
            break;
        case 0:
            dstat = demi_set_error(demi, DEMIERR_NULL_VALUE_TYPE,
                "%s", "Null value type on subtract.");
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VALUE_TYPE,
                "Undefined value type on subtract. Found: '%d'", demvsrc->demt_val_typ);
            break;

    }

    return (dstat);
}
/***************************************************************/
static int sdemi_op_multiply(struct demirec * demi,
    struct demival * demvtgt,
    struct demival * demvsrc)
{
/*
** 12/14/2019
*/
    int dstat;

    dstat = 0;
    switch (demvsrc->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Multiply not available for boolean.");
            break;
        case DEMIVAL_TYP_INT:
            if (demvtgt->demt_val_typ != demvsrc->demt_val_typ) {
                dstat = demi_set_error(demi, DEMIERR_DTYPE_MISMATCH,
                    "%s", "Multiply operands not both integers.");
            } else {
                demvtgt->demv_u.demv_int *= demvsrc->demv_u.demv_int;
            }
            break;
        case DEMIVAL_TYP_STR:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Multiply not available for strings.");
            break;
        case 0:
            dstat = demi_set_error(demi, DEMIERR_NULL_VALUE_TYPE,
                "%s", "Null value type on multiply.");
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VALUE_TYPE,
                "Undefined value type on multiply. Found: '%d'", demvsrc->demt_val_typ);
            break;

    }

    return (dstat);
}
/***************************************************************/
static int sdemi_op_divide(struct demirec * demi,
    struct demival * demvtgt,
    struct demival * demvsrc)
{
/*
** 12/14/2019
*/
    int dstat;

    dstat = 0;
    switch (demvsrc->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Divide not available for boolean.");
            break;
        case DEMIVAL_TYP_INT:
            if (demvtgt->demt_val_typ != demvsrc->demt_val_typ) {
                dstat = demi_set_error(demi, DEMIERR_DTYPE_MISMATCH,
                    "%s", "Divide operands not both integers.");
            } else {
                demvtgt->demv_u.demv_int *= demvsrc->demv_u.demv_int;
            }
            break;
        case DEMIVAL_TYP_STR:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Divide not available for strings.");
            break;
        case 0:
            dstat = demi_set_error(demi, DEMIERR_NULL_VALUE_TYPE,
                "%s", "Null value type on divide.");
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VALUE_TYPE,
                "Undefined value type on divide. Found: '%d'", demvsrc->demt_val_typ);
            break;

    }

    return (dstat);
}
/***************************************************************/
static int sdemi_op_unary(struct demirec * demi,
    struct demival * demvsrc,
    char unaryop)
{
/*
** 12/14/2019
*/
    int dstat;

    dstat = 0;
    switch (demvsrc->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Unary operation not available for boolean.");
            break;
        case DEMIVAL_TYP_INT:
            if (unaryop == '-') demvsrc->demv_u.demv_int = -(demvsrc->demv_u.demv_int);
            break;
        case DEMIVAL_TYP_STR:
            dstat = demi_set_error(demi, DEMIERR_BAD_OP_DTYPE,
                "%s", "Unary operation not available for strings.");
            break;
        case 0:
            dstat = demi_set_error(demi, DEMIERR_NULL_VALUE_TYPE,
                "%s", "Null value type on unary operation.");
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VALUE_TYPE,
                "Undefined value type on unary operation. Found: '%d'", demvsrc->demt_val_typ);
            break;

    }

    return (dstat);
}
/***************************************************************/
static int sdemi_copy_demival(struct demirec * demi,
    struct demival * demvtgt,
    struct demival * demvsrc)
{
/*
** 12/14/2019
*/
    int dstat;

    dstat = 0;
    demvtgt->demt_val_typ = demvsrc->demt_val_typ;
    switch (demvsrc->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            demvtgt->demv_u.demv_bool = demvsrc->demv_u.demv_bool;
            break;
        case DEMIVAL_TYP_INT:
            demvtgt->demv_u.demv_int = demvsrc->demv_u.demv_int;
            break;
        case DEMIVAL_TYP_STR:
            Free(demvtgt->demv_u.demv_str);
            demvtgt->demv_u.demv_str = Strdup(demvsrc->demv_u.demv_str);
            break;
        case 0:
            dstat = demi_set_error(demi, DEMIERR_NULL_VALUE_TYPE,
                "%s", "Null value type.");
            break;
        default:
            dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VALUE_TYPE,
                "Undefined value type. Found: '%d'", demvsrc->demt_val_typ);
            break;

    }

    return (dstat);
}
/***************************************************************/
static int sdemi_get_value(struct demirec * demi,
    const char * varname,
    struct demival * demv)
{
/*
** 12/14/2019
*/
    int dstat;
    void ** tnode;
    struct demiitem * demm;

    dstat = 0;
    tnode = dbtree_find(demi->demi_itemtree, varname, STRBYTES(varname));
    if (!tnode) {
        dstat = demi_set_error(demi, DEMIERR_UNDEFINED_VARIABLE, 
            "Undefined variable. Found: '%s'", varname);
    } else {
        demm = *(struct demiitem **)tnode;
        if (demm->demm_item_typ != DEMI_ITMTYP_CONST) {
            dstat = demi_set_error(demi, DEMIERR_EXP_CONST_VARIABLE, "%s",
                "Expecting const variable.");
        } else {
            dstat = sdemi_copy_demival(demi, demv, demm->demm_u.demm_demv);
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_eval_primary(struct demirec * demi,
    const char  * expbuf,
    int * eix,
    char * toke,
    int tokesz,
    struct demival * demv)
{
/*
** 12/13/2019
*/
    int dstat;

    dstat = 0;
    if (isdigit(toke[0])) {
        dstat = sdemi_toke_to_number(demi, toke, demv);
        if (!dstat) {
            dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        }
    } else if (isalpha(toke[0]) || toke[0] == '_') {
        dstat = sdemi_get_value(demi, toke, demv);
        if (!dstat) {
            dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        }
    } else if (toke[0] == '(') {
        dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        if (!dstat) {
            dstat = sdemi_eval_exp(demi, expbuf, eix, toke, tokesz, demv);
        }
        if (!dstat && toke[0] != ')') {
            dstat = demi_set_error(demi, DEMIERR_EXP_CLOSE_PAREN, 
                "Expecting closing parenthesis. Found: '%s'", toke);
        }
        if (!dstat) {
            dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        }
    } else {
        dstat = demi_set_error(demi, DEMIERR_UNEXP_CHAR, 
            "Unexpected character. Found: '%s'", toke);
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_eval_unop(struct demirec * demi,
    const char  * expbuf,
    int * eix,
    char * toke,
    int tokesz,
    struct demival * demv)
{
/*
** 12/14/2019
*/
    int dstat;
    char op[4];

    if (!strcmp(toke, "-") || !strcmp(toke, "+")) {
        strcpy(op, toke);
        dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        if (!dstat) {
            dstat = sdemi_eval_unop(demi, expbuf, eix, toke, tokesz, demv);
        }
        if (!dstat) {
            dstat = sdemi_op_unary(demi, demv, op[0]);
        }
    } else {
        dstat = sdemi_eval_primary(demi, expbuf, eix, toke, tokesz, demv);
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_eval_term(struct demirec * demi,
    const char  * expbuf,
    int * eix,
    char * toke,
    int tokesz,
    struct demival * demv)
{
/*
** 12/14/2019
*/
    int dstat;
    char op[4];
    struct demival demv2;

    dstat = sdemi_eval_unop(demi, expbuf, eix, toke, tokesz, demv);
    while (!dstat && (!strcmp(toke, "*") || !strcmp(toke, "/"))) {
        strcpy(op, toke);
        dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        if (!dstat) {
            memset(&demv2, 0, sizeof(struct demival));
            dstat = sdemi_eval_unop(demi, expbuf, eix, toke, tokesz, &demv2);
        }
        if (!dstat) {
            if (op[0] == '/') {
                dstat = sdemi_op_divide(demi, demv, &demv2);
            } else if (op[0] == '*') {
                dstat = sdemi_op_multiply(demi, demv, &demv2);
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_eval_sum(struct demirec * demi,
    const char * expbuf,
    int * eix,
    char * toke,
    int tokesz,
    struct demival * demv)
{
/*
** 12/13/2019
*/
    int dstat;
    char op[4];
    struct demival demv2;

    dstat = sdemi_eval_term(demi, expbuf, eix, toke, tokesz, demv);
    while (!dstat && (!strcmp(toke, "-") || !strcmp(toke, "+"))) {
        strcpy(op, toke);
        dstat = sdemi_get_toke(demi, expbuf, eix, toke, tokesz);
        if (!dstat) {
            memset(&demv2, 0, sizeof(struct demival));
            dstat = sdemi_eval_term(demi, expbuf, eix, toke, tokesz, &demv2);
        }
        if (!dstat) {
            if (op[0] == '-') {
                dstat = sdemi_op_subtract(demi, demv, &demv2);
            } else if (op[0] == '+') {
                dstat = sdemi_op_add(demi, demv, &demv2);
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_eval_exp(struct demirec * demi,
    const char  * expbuf,
    int * eix,
    char * toke,
    int tokesz,
    struct demival * demv)
{
/*
** 12/11/2019
*/
    int dstat;

    dstat = sdemi_eval_sum(demi, expbuf, eix, toke, tokesz, demv);

    return (dstat);
}
/***************************************************************/
static int sdemi_eval_expression(struct demirec * demi, const char  * expbuf, struct demival * demv)
{
/*
** 12/13/2019
*/
    int dstat;
    int eix;
    char toke[DEMI_MAX_TOKE_SIZE + 1];

    eix = 0;
    memset(demv, 0, sizeof(struct demival));

    dstat = sdemi_get_toke(demi, expbuf, &eix, toke, sizeof(toke));
    if (!dstat) {
        dstat = sdemi_eval_exp(demi, expbuf, &eix, toke, sizeof(toke), demv);
    }
    if (!dstat && toke[0]) {
        dstat = demi_set_error(demi, DEMIERR_EXP_END_OF_LINE,
            "Expecting end of line. Found: '%s'", toke);
    }

    return (dstat);
}
/***************************************************************/
#ifdef OLD_WAY
static void sendbuf_int_data(int val, uchar * msg, int * msgix)
{
/*
** 12/21/2019
*/
    unsigned int uval;

    uval = (unsigned int)val;
    while (uval >= 0x80) {
        msg[*msgix] = (uchar)((uval & 0x7F) | 0x80);
        (*msgix) += 1;
        uval >>= 7;
    }
    msg[*msgix] = (uchar)(uval & 0x7F);
    (*msgix) += 1;
}
#endif
static void sendbuf_int_data(int val, uchar * msg, int * msgix)
{
/*
** 01/13/2020
*/
    unsigned int uval;

    uval = (unsigned int)val;
    if (uval < 0x80) {
        msg[*msgix] = (uchar)(uval);
        (*msgix) += 1;
    } else if (uval < 0x4000) {
        msg[*msgix]     = (uchar)((uval >>  7) & 0x7F) | 0x80;
        msg[*msgix + 1] = (uchar)((uval      ) & 0x7F);
        (*msgix) += 2;
    } else if (uval < 0x200000) {
        msg[*msgix]     = (uchar)((uval >> 14) & 0x7F) | 0x80;
        msg[*msgix + 1] = (uchar)((uval >>  7) & 0x7F) | 0x80;
        msg[*msgix + 2] = (uchar)((uval      ) & 0x7F);
        (*msgix) += 3;
    } else if (uval < 0x10000000) {
        msg[*msgix]     = (uchar)((uval >> 21) & 0x7F) | 0x80;
        msg[*msgix + 1] = (uchar)((uval >> 14) & 0x7F) | 0x80;
        msg[*msgix + 2] = (uchar)((uval >>  7) & 0x7F) | 0x80;
        msg[*msgix + 3] = (uchar)((uval      ) & 0x7F);
        (*msgix) += 4;
    } else {
        msg[*msgix]     = (uchar)((uval >> 28) & 0x7F) | 0x80;
        msg[*msgix + 1] = (uchar)((uval >> 21) & 0x7F) | 0x80;
        msg[*msgix + 2] = (uchar)((uval >> 14) & 0x7F) | 0x80;
        msg[*msgix + 3] = (uchar)((uval >>  7) & 0x7F) | 0x80;
        msg[*msgix + 4] = (uchar)((uval      ) & 0x7F);
        (*msgix) += 5;
    }
}
/***************************************************************/
static void sendbuf_cstr_data(const char * cstr, uchar * msg, int * msgix)
{
/*
** 12/21/2019
*/
    int slen;

    slen = (int)strlen(cstr);
    sendbuf_int_data(slen, msg, msgix);
    memcpy(msg + (*msgix), cstr, slen);
    (*msgix) += slen;
}
/***************************************************************/
#ifdef UNUSED
static void sendbuf_demival_data(struct demival * demv, uchar * msg, int * msgix)
{
/*
** 12/21/2019
*/

    msg[*msgix] = (uchar)(demv->demt_val_typ & 0xFF);
    (*msgix) += 1;
    switch (demv->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            msg[*msgix] = demv->demv_u.demv_bool?1:0;
            (*msgix) += 1;
            break;
        case DEMIVAL_TYP_INT:
            sendbuf_int_data(demv->demv_u.demv_int, msg, msgix);
            break;
        case DEMIVAL_TYP_STR:
            sendbuf_cstr_data(demv->demv_u.demv_str, msg, msgix);
            break;
        case 0:
            break;

        default:
            break;
    }
}
#endif
/***************************************************************/
static int sendbuf_int_size(int val)
{
/*
** 12/21/2019
*/
    int vallen;
    unsigned int uval;

    vallen = 1;
    uval = (unsigned int)val;
    while (uval >= 0x80) {
        vallen++;
        uval >>= 7;
    }

    return (vallen);
}
/***************************************************************/
static int sendbuf_cstr_size(const char * cstr)
{
/*
** 12/21/2019
*/
    int vallen;
    int slen;

    slen = (int)strlen(cstr);
    vallen  = sendbuf_int_size(slen);
    vallen += slen;

    return (vallen);
}
/***************************************************************/
#ifdef UNUSED
static int sendbuf_demival_size(struct demival * demv)
{
/*
** 12/21/2019
*/
    int vallen;

    vallen = 1; /* demt_val_typ */
    switch (demv->demt_val_typ) {
        case DEMIVAL_TYP_BOOL:
            vallen += 1; /* Bool */
            break;
        case DEMIVAL_TYP_INT:
            vallen += sendbuf_int_size(demv->demv_u.demv_int);
            break;
        case DEMIVAL_TYP_STR:
            vallen += sendbuf_cstr_size(demv->demv_u.demv_str);
            break;
        case 0:
            break;

        default:
            vallen = -1;
            break;
    }

    return (vallen);
}
#endif
/***************************************************************/
static int sdemb_send_const(struct demi_boss * demb,
    struct demichannel * demc,
    const char * constname,
    const char * constbuf)
{
/*
** 12/21/2019
*/
    int dstat;
    int msglen;
    int msgix;
    uchar * msg;

    dstat = 0;
    msglen = 0;

    msglen += sendbuf_cstr_size(constname);
    msglen += sendbuf_cstr_size(constbuf);

    msg = demmem_resize(demb->demb_send_buffer, msglen);

    msgix = 0;
    sendbuf_cstr_data(constname, msg, &msgix);
    sendbuf_cstr_data(constbuf, msg, &msgix);
    if (msgix != msglen) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_SEND_LENGTH_MISMATCH,
            "Boss send const message length mismatch: len=%d, ix=%d",
                msglen, msgix);
    } else {
        dstat = sdemc_sendmsg(demb->demb_demi, demc, DMTW_CONST, msg, msglen);
        if (!dstat) {
            dstat = sdemb_recv_and_process_response(demb, demc);
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_send_type(struct demi_boss * demb,
    struct demichannel * demc,
    const char * typname,
    const char * typdesc,
    size_t typsize)
{
/*
** 01/12/2020
*/
    int dstat;
    int msglen;
    int msgix;
    uchar * msg;

    dstat = 0;
    msglen = 0;

    msglen += sendbuf_cstr_size(typname);
    msglen += sendbuf_cstr_size(typdesc);
    msglen += sendbuf_int_size(typsize);

    msg = demmem_resize(demb->demb_send_buffer, msglen);

    msgix = 0;
    sendbuf_cstr_data(typname, msg, &msgix);
    sendbuf_cstr_data(typdesc, msg, &msgix);
    sendbuf_int_data(typsize, msg, &msgix);
    if (msgix != msglen) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_SEND_LENGTH_MISMATCH,
            "Boss send type message length mismatch: len=%d, ix=%d",
                msglen, msgix);
    } else {
        dstat = sdemc_sendmsg(demb->demb_demi, demc, DMTW_TYPE, msg, msglen);
        if (!dstat) {
            dstat = sdemb_recv_and_process_response(demb, demc);
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemb_send_func(struct demi_boss * demb,
    struct demichannel * demc,
    int func_index,
    const char * funcname,
    const char * funcinargs,
    const char * funcoutargs)
{
/*
** 01/12/2020
*/
    int dstat;
    int msglen;
    int msgix;
    int func_ref;
    uchar * msg;

    dstat = 0;
    msglen = 0;
    func_ref = FUNC_INDEX_TO_REF(func_index);

    msglen += sendbuf_int_size(func_ref);
    msglen += sendbuf_cstr_size(funcname);
    msglen += sendbuf_cstr_size(funcinargs);
    msglen += sendbuf_cstr_size(funcoutargs);

    msg = demmem_resize(demb->demb_send_buffer, msglen);

    msgix = 0;
    sendbuf_int_data(func_ref, msg, &msgix);
    sendbuf_cstr_data(funcname, msg, &msgix);
    sendbuf_cstr_data(funcinargs, msg, &msgix);
    sendbuf_cstr_data(funcoutargs, msg, &msgix);
    if (msgix != msglen) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_SEND_LENGTH_MISMATCH,
            "Boss send func message length mismatch: len=%d, ix=%d",
                msglen, msgix);
    } else {
        dstat = sdemc_sendmsg(demb->demb_demi, demc, DMTW_FUNC_DEFINE, msg, msglen);
        if (!dstat) {
            dstat = sdemb_recv_and_process_response(demb, demc);
        }
    }

    return (dstat);
}
/***************************************************************/
int demb_define_const(
    struct demi_boss * demb,
    const char * constname,
    const char * fmt, ...)
{
/*
** 12/08/2019
*/
	va_list args;
    int dstat;
    int dup;
    int chix;
    struct demival * demv;
    char constbuf[DEMI_MAX_CONST_SIZE + 1];
    void ** tnode;
    struct demiitem * demm;

    dstat = 0;

	va_start(args, fmt);
	vsprintf(constbuf, fmt, args);
	va_end(args);

    if (!(demb->demb_flags & DEMS_FLAG_STARTED)) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_NOT_STARTED, "%s", "Not started.");
    } else {
        tnode = dbtree_find(demb->demb_demi->demi_itemtree, constname, STRBYTES(constname));
        if (tnode) {
                dstat = demi_set_error(demb->demb_demi, DEMIERR_DUPLICATE_CONST,
                    "Constant already declared. Found: '%s'", constname);
        }
    }
 
    if (!dstat) {
        demv = sdemi_demival_new();

        dstat = sdemi_eval_expression(demb->demb_demi, constbuf, demv);
        if (dstat) {
            sdemi_demival_free(demv);
        } else {
            demm = sdemi_demiitem_new(DEMI_ITMTYP_CONST);
            demm->demm_item_nam = Strdup(constname);
            demm->demm_u.demm_demv = demv;
            printf("demi_define_const(\"%s\") val=%s\n",
                constname, sdemi_demival_display(demv));
            dup = dbtree_insert(demb->demb_demi->demi_itemtree, constname, STRBYTES(constname), demm);
        }
    }

    if (!dstat) {
        for (chix = 0; !dstat && chix < demb->demb_nchannels; chix++) {
            dstat = sdemb_send_const(demb, demb->demb_channel[chix],
                constname, constbuf);
        }
    }

    return (dstat);
}
/***************************************************************/
static void demityp_display(struct demityp * demt, int dflags, int tab)
{
/*
** 12/22/2019
*/
    int ii;
    int jj;
    struct demifield * demf;
    struct demityp * tdemt;

    if (demt->demt_typ_flags & DEMT_TYP_FLAG_UNSIGNED) printf("unsigned ");

    switch (demt->demt_simp_typ) {
        case DEMITYP_CHAR:
            printf("char");
            break;
        case DEMITYP_INT8:
            printf("int8");
            break;
        case DEMITYP_WCHAR:
            printf("wchar");
            break;
        case DEMITYP_INT16:
            printf("int16");
            break;
        case DEMITYP_INT32:
            printf("int32");
            break;
        case DEMITYP_INT64:
            printf("int64");
            break;
        case DEMITYP_FLOAT32:
            printf("float32");
            break;
        case DEMITYP_FLOAT64:
            printf("float64");
            break;
        case DEMITYP_VOID:
            printf("void");
            break;
        case DEMITYP_STRUCT:
            printf("{\n");
            for (ii = 0; ii < demt->demt_u.demt_demts->demts_num_fields; ii++) {
                demf = demt->demt_u.demt_demts->demts_fields[ii];
                for (jj = 0; jj < tab; jj++) printf(" ");
                printf("%-8s : ", demf->demf_name);
                demityp_display(demf->demf_dmityp, dflags | DEMT_DISP_FLAG_NO_NL, tab + 4);
                printf(" /* %d */;\n", demf->demf_offset);
            }
            for (jj = 0; jj < tab - 4; jj++) printf(" ");
            printf("}");
            break;
        case DEMITYP_ARRAY:
            /* go to bottom and display type */
            tdemt = demt;
            while (tdemt->demt_simp_typ == DEMITYP_ARRAY) {
                tdemt = tdemt->demt_u.demt_demta->demta_dmityp;
            }
            demityp_display(tdemt, dflags | DEMT_DISP_FLAG_NO_NL, tab);
            /* start at top, display dimension, then descend one level */
            tdemt = demt;
            while (tdemt->demt_simp_typ == DEMITYP_ARRAY) {
                if (tdemt->demt_u.demt_demta->demta_array_type == DEMTA_ARRAY_TYPE_FIXED_SIZE) {
                    printf("[%d]", tdemt->demt_u.demt_demta->demta_num_elements);
                } else {
                    printf("[??]");
                }
                tdemt = tdemt->demt_u.demt_demta->demta_dmityp;
            }
            break;
        case DEMITYP_POINTER:
            printf("%c", demt->demt_u.demt_demtp->demtp_ptrdat);
            if ((demt->demt_u.demt_demtp->demtp_dmityp->demt_simp_typ & DEMITYP_SIMPLE_MASK) != 0) {
                printf("(");
                demityp_display(demt->demt_u.demt_demtp->demtp_dmityp, dflags, tab);
                printf(")");
            } else {
                demityp_display(demt->demt_u.demt_demtp->demtp_dmityp, dflags, tab);
            }
            break;

        default:
            printf("?? type=%d ??", demt->demt_simp_typ);
            break;
    }
    if (!(dflags & DEMT_DISP_FLAG_NO_NL)) printf("\n");
}
/***************************************************************/
static void sdemi_demityp_display(const char  * typname, struct demityp * demt, int dflags)
{
/*
** 12/22/2019
*/
    int tab;

    tab = 4;

    printf("%s : ", typname);
    demityp_display(demt, dflags, tab);
    printf("\n");
}
/***************************************************************/
static int sdemi_parse_pointer_type(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt)
{
/*
** 01/04/2020
**
**  <pointer type>      ::= * <pointer reftype> | * void |  ^ <pointer reftype> | ^ void
**  <pointer reftype>   ::= <typespec> | <typespec> <ptr array bounds>
**  <ptr array bounds>  ::= '[]' | '[' <array bounds> ']'
*/
    int dstat;
    char ptrdat;
    struct demityp * demt;
    struct demityp_ptr * demtp;

    ptrdat = toke[0];

    dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
    if (!dstat) {
        demtp = sdemi_demityp_ptr_new();
        demtp->demtp_ptrdat = ptrdat;

        (*pdemt) = sdemi_demityp_new(DEMITYP_POINTER);
        (*pdemt)->demt_typ_flags = 0;
        (*pdemt)->demt_typ_size  = sizeof(void*);
        (*pdemt)->demt_u.demt_demtp = demtp;
        if (!strcmp(toke, "void")) {
            demt = sdemi_demityp_new(DEMITYP_VOID);
            demtp->demtp_dmityp = demt;
            demtp->demtp_ptr_type = DEMTP_PTR_TYPE_VOID_PTR;
        } else {
            demt = NULL;
            dstat = sdemi_parse_typespec(demi, typdesc, tix, toke, tokesz, &demt);
            if (!dstat) {
                if (toke[0] != '[') {
                    demtp->demtp_dmityp = demt;
                    demtp->demtp_ptr_type = DEMTP_PTR_TYPE_SINGLE_ITEM;
                } else {
                    dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
                    if (!dstat) {
                        if (toke[0] != ']') {
                            dstat = demi_set_error(demi, DEMIERR_TYPE_UNSUPPORTED_ARRAY,
                                "Expecting closing bracket in array type. Found: '%s'", toke);
                        } else if (demt->demt_simp_typ == DEMITYP_STRUCT) {
                            dstat = demi_set_error(demi, DEMIERR_PTR_ARRAY_STRUCT,
                                "Pointer to an array of struct not supported");
                        } else if (demt->demt_simp_typ == DEMITYP_ARRAY) {
                            dstat = demi_set_error(demi, DEMIERR_PTR_ARRAY_ARRAY,
                                "Pointer to an array of array not supported");
                        } else {
                            demtp->demtp_dmityp = demt;
                            demtp->demtp_array_stride = demt->demt_typ_size;
                            demtp->demtp_ptr_type = DEMTP_PTR_TYPE_NULL_TERM;
                            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
                       }
                    }
                }
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static void sdemi_demityp_struct_add_field(struct demityp_struct * demts, struct demifield * demf)
{
/*
** 12/24/2019
*/
    int tot_size;

    if (demts->demts_num_fields == demts->demts_max_fields) {
        if (!demts->demts_max_fields) demts->demts_max_fields = 4;
        else demts->demts_max_fields *= 2;
        demts->demts_fields = Realloc(demts->demts_fields, struct demifield *, demts->demts_max_fields);
    }
    demts->demts_fields[demts->demts_num_fields] = demf;
    demts->demts_num_fields += 1;

    tot_size = demf->demf_offset + demf->demf_dmityp->demt_typ_size;
    if (demts->demts_total_size < tot_size) {
        demts->demts_total_size = tot_size;
    }
}
/***************************************************************/
static int sdemi_parse_compound_type(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt)
{
/*
** 12/22/2019
*/
    int dstat;
    int nnames;
    int mxnames;
    int done;
    int ii;
    int align_mask;
    int field_offset;
    int tsize;
    char ** namelist;
    struct demityp_struct * demts;
    struct demifield * demf;
    struct demityp * demt;

    dstat = 0;
    namelist = NULL;
    mxnames = 0;
    field_offset = 0;
    demts = sdemi_demityp_struct_new();

    dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);

    while (!dstat && toke[0] != '}') {
        nnames = 0;
        done = 0;
        while (!dstat && !done) {
            if (isalpha(toke[0]) || toke[0] == '_') {
                if (nnames == mxnames) {
                    if (!mxnames) mxnames = 2;
                    else mxnames *= 2;
                    namelist = Realloc(namelist, char*, mxnames);
                }
                namelist[nnames++] = Strdup(toke);
                dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
                if (!dstat) {
                    if (toke[0] == ':') {
                        done = 1;
                        dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
                    } else if (toke[0] == ',') {
                        dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
                    } else {
                        dstat = demi_set_error(demi, DEMIERR_BAD_FIELD_SEPARATOR, 
                            "Invalid field name separator. Found: '%s'", toke);
                    }
                }
            } else {
                dstat = demi_set_error(demi, DEMIERR_BAD_FIELD_NAME, 
                    "Invalid field name. Found: '%s'", toke);
            }
        }

        if (!dstat) {
            demt = NULL;
            dstat = sdemi_parse_typespec(demi, typdesc, tix, toke, tokesz, &demt);
        }

        if (!dstat && toke[0] != ';') {
            dstat = demi_set_error(demi, DEMIERR_BAD_FIELD_SEPARATOR, 
                "Expecting ';' as field separator. Found: '%s'", toke);
        }

        if (!dstat) {
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        }

        if (!dstat) {
            align_mask = sdemi_calc_demityp_alignment(demt, demi->demi_cfg[DEMI_CFGIX_ALIGN]) - 1;
            for (ii = 0; !dstat && ii < nnames; ii++) {
                demf = sdemi_demifield_new();
                demf->demf_name = namelist[ii];
                namelist[ii] = NULL;
                while (field_offset & align_mask) field_offset++;
                demf->demf_offset = field_offset;
                demf->demf_dmityp = demt;
                sdemi_demityp_struct_add_field(demts, demf);
                field_offset += demt->demt_typ_size;
            }
        }
    }
    Free(namelist);

    if (!dstat && (field_offset == 0 || demts->demts_num_fields == 0)) {
        dstat = demi_set_error(demi, DEMIERR_STRUCT_EMPTY, "%s",
            "Structure has no fields.");
    }

    if (dstat) {
        sdemi_demityp_struct_free(demts);
    } else {
        tsize = demts->demts_total_size;
        align_mask = sdemi_calc_demityp_struct_alignment(demts, demi->demi_cfg[DEMI_CFGIX_ALIGN]) - 1;
        while (tsize & align_mask) tsize++;

        (*pdemt) = sdemi_demityp_new(DEMITYP_STRUCT);
        (*pdemt)->demt_typ_size = tsize;
        (*pdemt)->demt_u.demt_demts = demts;

        dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
    }

    return (dstat);
}
/***************************************************************/
static struct demifield * sdemi_copy_demifield(struct demifield * demfsrc)
{
/*
** 12/24/2019
*/
    struct demifield * demftgt;

    demftgt = sdemi_demifield_new();
    demftgt->demf_name         = Strdup(demfsrc->demf_name);
    demftgt->demf_offset       = demfsrc->demf_offset;
    demftgt->demf_aligned_size = demfsrc->demf_aligned_size;
    demftgt->demf_dmityp       = sdemi_copy_demityp(demfsrc->demf_dmityp);

    return (demftgt);
}
/***************************************************************/
static struct demityp_struct * sdemi_copy_demityp_struct(struct demityp_struct * demtssrc)
{
/*
** 12/24/2019
*/
    struct demityp_struct * demtstgt;
    int ii;

    demtstgt = sdemi_demityp_struct_new();
    demtstgt->demts_num_fields     = demtssrc->demts_num_fields;
    demtstgt->demts_max_fields     = demtssrc->demts_num_fields; /* sic */
    demtstgt->demts_total_size     = demtssrc->demts_total_size;
    demtstgt->demts_fields         = New(struct demifield *, demtstgt->demts_max_fields);

    for (ii = 0; ii < demtssrc->demts_num_fields; ii++) {
        demtstgt->demts_fields[ii] = sdemi_copy_demifield(demtssrc->demts_fields[ii]);
    }

    return (demtstgt);
}
/***************************************************************/
static struct demityp_array * sdemi_copy_demityp_array(struct demityp_array * demtasrc)
{
/*
** 12/24/2019
*/
    struct demityp_array * demtatgt;

    demtatgt = sdemi_demityp_array_new();
    demtatgt->demta_array_type     = demtasrc->demta_array_type;
    demtatgt->demta_array_stride   = demtasrc->demta_array_stride;
    demtatgt->demta_num_elements   = demtasrc->demta_num_elements;
    demtatgt->demta_total_size     = demtasrc->demta_total_size;
    demtatgt->demta_dmityp         = sdemi_copy_demityp(demtasrc->demta_dmityp);

    return (demtatgt);
}
/***************************************************************/
static struct demityp_ptr * sdemi_copy_demityp_ptr(struct demityp_ptr * demtpsrc)
{
/*
** 12/24/2019
*/
    struct demityp_ptr * demtptgt;

    demtptgt = sdemi_demityp_ptr_new();
    demtptgt->demtp_ptr_type       = demtpsrc->demtp_ptr_type;
    demtptgt->demtp_array_stride   = demtpsrc->demtp_array_stride;
    demtptgt->demtp_ptrdat         = demtpsrc->demtp_ptrdat;
    demtptgt->demtp_dmityp         = sdemi_copy_demityp(demtpsrc->demtp_dmityp);

    return (demtptgt);
}
/***************************************************************/
static struct demityp * sdemi_copy_demityp(struct demityp * demtsrc)
{
/*
** 12/22/2019
*/
    struct demityp * demttgt;

    demttgt = sdemi_demityp_new(demtsrc->demt_simp_typ);

    demttgt->demt_typ_flags     = demtsrc->demt_typ_flags;
    demttgt->demt_typ_size      = demtsrc->demt_typ_size;

    switch (demtsrc->demt_simp_typ) {
        case 0:
            break;
        case DEMITYP_CHAR:
            break;
        case DEMITYP_INT8:
            break;
        case DEMITYP_WCHAR:
            break;
        case DEMITYP_INT16:
            break;
        case DEMITYP_INT32:
            break;
        case DEMITYP_INT64:
            break;
        case DEMITYP_FLOAT32:
            break;
        case DEMITYP_FLOAT64:
            break;
        case DEMITYP_STRUCT:
            demttgt->demt_u.demt_demts = sdemi_copy_demityp_struct(demtsrc->demt_u.demt_demts);
            break;
        case DEMITYP_ARRAY:
            demttgt->demt_u.demt_demta = sdemi_copy_demityp_array(demtsrc->demt_u.demt_demta);
            break;
        case DEMITYP_POINTER:
            demttgt->demt_u.demt_demtp = sdemi_copy_demityp_ptr(demtsrc->demt_u.demt_demtp);
            break;

        default:
            /* error */
            break;
    }

    return (demttgt);
}
/***************************************************************/
static int sdemi_parse_defined_type(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt)
{
/*
** 12/22/2019
*/
    int dstat;
    void ** tnode;
    struct demiitem * demm;

    dstat = 0;
    tnode = dbtree_find(demi->demi_itemtree, toke, STRBYTES(toke));
    if (!tnode) {
        dstat = demi_set_error(demi, DEMIERR_UNDEFINED_TYPE, 
            "Undefined type. Found: '%s'", toke);
    } else {
        demm = *(struct demiitem **)tnode;
        if (demm->demm_item_typ != DEMI_ITMTYP_NAMED_TYPE) {
            dstat = demi_set_error(demi, DEMIERR_EXP_CONST_VARIABLE, "%s",
                "Expecting type name.");
        } else {
            (*pdemt) = sdemi_copy_demityp(demm->demm_u.demm_demnt->demnt_demt);
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_parse_simple_type(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt)
{
/*
** 12/22/2019
**  <simple type>       ::= <signed type> | <unsigned type> | <floating type> | <defined type>
**  <signed type>       ::= char | short | int | long | int8 | int16 | int32 | int64
**  <unsigned type>     ::= unsigned <signed type> | uint8 | uint16 | uint32 | uint64
**  <floating type>     ::= float | double
*/
    int dstat;
    int simp_typ;
    int typ_size;
    int typ_flags;

    dstat = 0;
    simp_typ = 0;
    typ_flags = 0;
    typ_size = 0;
    if (!strcmp(toke, "unsigned")) {
        typ_flags |= DEMT_TYP_FLAG_UNSIGNED;
        dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
    }

    if (!dstat) {
        if (!strcmp(toke, "char")) {
            simp_typ = DEMITYP_CHAR;
            typ_size = sizeof(int8);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "int8")) {
            simp_typ = DEMITYP_INT8;
            typ_size = sizeof(int8);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "wchar")) {
            simp_typ = DEMITYP_WCHAR;
            typ_size = sizeof(tWCHAR);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "short") || !strcmp(toke, "int16")) {
            simp_typ = DEMITYP_INT16;
            typ_size = sizeof(int16);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "int") || !strcmp(toke, "int32")) {
            simp_typ = DEMITYP_INT32;
            typ_size = sizeof(int32);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "long") || !strcmp(toke, "int64")) {
            simp_typ = DEMITYP_INT64;
            typ_size = sizeof(int64);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "uint8")) {
            simp_typ = DEMITYP_INT8;
            typ_flags |= DEMT_TYP_FLAG_UNSIGNED;
            typ_size = sizeof(char);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "uint16")) {
            simp_typ = DEMITYP_INT16;
            typ_flags |= DEMT_TYP_FLAG_UNSIGNED;
            typ_size = sizeof(int16);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "uint32")) {
            simp_typ = DEMITYP_INT32;
            typ_flags |= DEMT_TYP_FLAG_UNSIGNED;
            typ_size = sizeof(int32);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "uint64")) {
            simp_typ = DEMITYP_INT64;
            typ_flags |= DEMT_TYP_FLAG_UNSIGNED;
            typ_size = sizeof(int64);
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "float") || !strcmp(toke, "float32")) {
            simp_typ = DEMITYP_FLOAT32;
            typ_size = sizeof(float);
            if (typ_flags & DEMT_TYP_FLAG_UNSIGNED) {
                dstat = demi_set_error(demi, DEMIERR_TYPE_NOT_UNSIGNED,
                    "Type not appropriate for unsigned. Found: '%s'", toke);
            }
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        } else if (!strcmp(toke, "double") || !strcmp(toke, "float64")) {
            simp_typ = DEMITYP_FLOAT64;
            typ_size = sizeof(double);
            if (typ_flags & DEMT_TYP_FLAG_UNSIGNED) {
                dstat = demi_set_error(demi, DEMIERR_TYPE_NOT_UNSIGNED,
                    "Type not appropriate for unsigned. Found: '%s'", toke);
            }
            dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        }
    }

    if (!dstat) {
        if (simp_typ) {
            (*pdemt) = sdemi_demityp_new(simp_typ);

            (*pdemt)->demt_typ_flags = typ_flags;
            (*pdemt)->demt_typ_size  = typ_size;
        } else {
            dstat = sdemi_parse_defined_type(demi, typdesc, tix, toke, tokesz, pdemt);
            if (!dstat) {
                dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
            }
        }
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_calc_aligned_size(int elsize, int align)
{
/*
** 12/23/2019
**
** Examples:
**  elsize  align   stride
**  1       4       1
**  2       4       2
**  3       4       4
**  4       4       4
**  5       4       8
**  11      4       12
**  1       8       1
**  2       8       2
**  3       8       4
**  4       8       4
**  5       8       8
**  11      8       16
**  1       16      1
**  2       16      2
**  3       16      4
**  4       16      4
**  5       16      8
**  11      16      16
*/
    int stride;
    int mask;
    int p2size;

    stride = 0;
    if (elsize < align) {
        /* Find smallest power of 2 that is larger or equal to elsize */
        p2size = 1;
        while (p2size < elsize) p2size <<= 1;
        if (p2size < align) {
            stride = p2size;
        }
    }

    if (!stride) {
        mask = align - 1;
        stride = elsize;
        while (stride & mask) stride++;
    }

    return (stride);
}
/***************************************************************/
static int sdemi_calc_demityp_struct_alignment(struct demityp_struct * demts, int align)
{
/*
** 12/24/2019
*/
    int alignment;
    int ii;
    int falign;

    alignment = 0;
    for (ii = 0; ii < demts->demts_num_fields; ii++) {
        falign = sdemi_calc_demityp_alignment(demts->demts_fields[ii]->demf_dmityp, align);
        if (falign > alignment) alignment = falign;
    }

    return (alignment);
}
/***************************************************************/
static int sdemi_calc_demityp_alignment(struct demityp * demt, int align)
{
/*
** 12/24/2019
*/
    int alignment;

    switch (demt->demt_simp_typ) {
        case DEMITYP_CHAR:
            alignment = sizeof(char);
            break;
        case DEMITYP_INT8:
            alignment = sizeof(char);
            break;
        case DEMITYP_WCHAR:
            alignment = sizeof(tWCHAR);
            break;
        case DEMITYP_INT16:
            alignment = sizeof(int16);
            break;
        case DEMITYP_INT32:
            alignment = sizeof(int32);
            break;
        case DEMITYP_INT64:
            alignment = sizeof(int64);
            break;
        case DEMITYP_FLOAT32:
            alignment = sizeof(float);
            break;
        case DEMITYP_FLOAT64:
            alignment = sizeof(double);
            break;
        case DEMITYP_STRUCT:
            alignment = sdemi_calc_demityp_struct_alignment(demt->demt_u.demt_demts, align);
            break;
        case DEMITYP_ARRAY:
            alignment = sdemi_calc_demityp_alignment(demt->demt_u.demt_demta->demta_dmityp, align);
            break;
        case DEMITYP_POINTER:
            alignment = sizeof(void*);
            break;
        case DEMITYP_VOID:
            alignment = sizeof(char);
            break;
        default:
            break;
    }
    if (alignment > align) alignment = align;

    return (alignment);
}
/***************************************************************/
static int sdemi_parse_array_bounds(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp_array ** pdemta)
{
/*
** 12/22/2019
**  <array type>        ::= <typespec> '[' <array bounds> ']'
**  <array bounds>      ::= <const expression>
*/
    int dstat;
    struct demityp_array * demta;
    struct demival * demv;

    (*pdemta) = NULL;
    demta = sdemi_demityp_array_new();

    dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
    if (!dstat) {
        demv = sdemi_demival_new();
        dstat = sdemi_eval_exp(demi, typdesc, tix, toke, tokesz, demv);
        if (!dstat) {
            if (demv->demt_val_typ != DEMIVAL_TYP_INT) {
                dstat = demi_set_error(demi, DEMIERR_EXP_INT_ARRAY_BOUNDS,
                    "Expecting an integer for array bounds");
            } else if (demv->demv_u.demv_int <= 0) {
                dstat = demi_set_error(demi, DEMIERR_EXP_INT_ARRAY_BOUNDS,
                    "Array bounds must be a positive integer");
            } else {
                demta->demta_array_type     = DEMTA_ARRAY_TYPE_FIXED_SIZE;
                demta->demta_num_elements   = demv->demv_u.demv_int;
                dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
            }
        }
    }
    if (!dstat) (*pdemta) = demta;

    return (dstat);
}
/***************************************************************/
static int sdemi_parse_array_bounds_list(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt)
{
/*
** 01/13/2020
*/
    int dstat;
    int ndims;
    struct demityp * demt;
    struct demityp_array * ademta[MAX_ARRAY_DIMENSIONS];

    dstat = 0;
    ndims = 0;
    while (!dstat && toke[0] == '[') {
        if (ndims == MAX_ARRAY_DIMENSIONS) {
                dstat = demi_set_error(demi, DEMIERR_MAX_ARRAY_DIMENSIONS,
                    "Arrays may have a miximum of %d dimensions.", MAX_ARRAY_DIMENSIONS);
        } else {
            dstat = sdemi_parse_array_bounds(demi, typdesc, tix, toke, tokesz, &(ademta[ndims]));
            ndims++;
        }
    }

    while (!dstat && ndims) {
        ndims--;
        ademta[ndims]->demta_array_stride   = 
            sdemi_calc_aligned_size((*pdemt)->demt_typ_size, demi->demi_cfg[DEMI_CFGIX_ALIGN]);
        ademta[ndims]->demta_total_size     = ademta[ndims]->demta_num_elements * ademta[ndims]->demta_array_stride;
        ademta[ndims]->demta_dmityp         = (*pdemt);

        demt = sdemi_demityp_new(DEMITYP_ARRAY);
        demt->demt_typ_size = ademta[ndims]->demta_total_size;
        demt->demt_u.demt_demta = ademta[ndims];
        (*pdemt) = demt;
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_parse_typespec(struct demirec * demi,
    const char  * typdesc,
    int * tix,
    char * toke,
    int tokesz,
    struct demityp ** pdemt)
{
/*
** 12/22/2019
**
**  <typespec>          ::= <complex type> | '(' <typespec> ')'
**  <complex type>      ::= <simple type> | <pointer type> | <array type> | <compound type>
**  <array type>        ::= <typespec> '[' <array bounds> ']'
**  <pointer type>      ::= * <pointer reftype> | * void |  ^ <pointer reftype> | ^ void
**  <pointer reftype>   ::= <typespec> | <typespec> <ptr array bounds>
**  <ptr array bounds>  ::= '[]' | '[' <array bounds> ']'
*/
    int dstat;

    if (toke[0] == '{') {
        dstat = sdemi_parse_compound_type(demi, typdesc, tix, toke, tokesz, pdemt);
    } else if (toke[0] == '*' || toke[0] == '^') {
        dstat = sdemi_parse_pointer_type(demi, typdesc, tix, toke, tokesz, pdemt);
    } else if (toke[0] == '(') {
        dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
        if (!dstat) {
            dstat = sdemi_parse_typespec(demi, typdesc, tix, toke, tokesz, pdemt);
        }
        if (!dstat) {
            if (toke[0] != ')') {
                dstat = demi_set_error(demi, DEMIERR_TYPE_EXP_CLOSE_PAREN,
                    "Expecting closing parenthesis in type. Found: '%s'", toke);
            } else {
                dstat = sdemi_get_toke(demi, typdesc, tix, toke, tokesz);
            }
        }
    } else {
        dstat = sdemi_parse_simple_type(demi, typdesc, tix, toke, tokesz, pdemt);
    }

    if (!dstat && toke[0] == '[' && sdemi_next_char(demi, typdesc, tix) != ']') {
        dstat = sdemi_parse_array_bounds_list(demi, typdesc, tix, toke, tokesz, pdemt);
    }

    return (dstat);
}
/***************************************************************/
static int sdemi_parse_type(struct demirec * demi,
    const char  * typdesc,
    struct demityp ** pdemt,
    int typsize)
{
/*
** 12/22/2019
**
** Type specification:
**  <typespec>          ::= <complex type> | '(' <typespec> ')'
**  <complex type>      ::= <simple type> | <pointer type> | <array type> | <compound type>
**  <simple type>       ::= <signed type> | <unsigned type> | <floating type> | <defined type>
**  <signed type>       ::= char | short | int | long | int8 | int16 | int32 | int64
**  <unsigned type>     ::= unsigned <signed type> | uint8 | uint16 | uint32 | uint64
**  <floating type>     ::= float | double
**  <pointer type>      ::= * <pointer reftype> | * void |  ^ <pointer reftype> | ^ void
**  <pointer reftype>   ::= <typespec> | <typespec> <ptr array bounds>
**  <ptr array bounds>  ::= '[]' | '[' <array bounds> ']'
**  <array type>        ::= <typespec> '[' <array bounds> ']'
**  <array bounds>      ::= <const expression>
**  <compound type>     ::= '{' <field list> '}'
**  <field list>        ::= <field name> [,<field name> [,...] : <typespec> ;
**
** Future enhanced:
**    <array bounds> ::= <const expression> | <const expression> @ <record expression>
**    <ptr array bounds>  ::= '[]' | '[' <array bounds> ']' | | '[' @ <record expression> ']'
** Examples:
** Desc                                                     | c             | Demi     
** A single character                                       | char          | char
** Pointer to a single character                            | char *        | * char
** Pointer to a zero terminated char string                 | char *        | * char[]
** Pointer to a pointer to a single character               | char **       | ** char
** Pointer to a pointer to a zero terminated char string    | char **       | ** char[]
** Pointer to a NULL terminated list of char strings        | char **       | *(* char[])[]
*/
    int dstat;
    int tix;
    char toke[DEMI_MAX_TOKE_SIZE + 1];

    tix = 0;
    (*pdemt) = NULL;

    dstat = sdemi_get_toke(demi, typdesc, &tix, toke, sizeof(toke));
    if (!dstat) {
        dstat = sdemi_parse_typespec(demi, typdesc, &tix, toke, sizeof(toke), pdemt);
    }

    if (!dstat && toke[0]) {
        dstat = demi_set_error(demi, DEMIERR_EXP_END_OF_LINE,
            "Expecting end of line. Found: '%s'", toke);
    }

    if (!dstat) {
        if ((*pdemt)->demt_typ_size != typsize) {
            dstat = demi_set_error(demi, DEMIERR_TYPE_SIZE_MISMATCH,
                "Type size mismatch. Calculated size=%d, but actual size=%d",
                (*pdemt)->demt_typ_size, typsize);
        }
    }

    return (dstat);
}
/***************************************************************/
int demb_define_type(
    struct demi_boss * demb,
    const char * typname,
    const char * typdesc,
    size_t typsize)
{
/*
** 12/22/2019
*/
    int dstat;
    int dup;
    int chix;
    void ** tnode;
    struct deminamedtyp * demnt;
    struct demityp * demt;
    struct demiitem * demm;

    dstat = 0;
    if (!(demb->demb_flags & DEMS_FLAG_STARTED)) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_NOT_STARTED, "%s", "Not started.");
        return (dstat);
    } else {
        tnode = dbtree_find(demb->demb_demi->demi_itemtree, typname, STRBYTES(typname));
        if (tnode) {
            dstat = demi_set_error(demb->demb_demi, DEMIERR_DUPLICATE_TYPE,
                "Type already declared. Found: '%s'", typname);
        }
    }
 
    if (!dstat) {
        dstat = sdemi_parse_type(demb->demb_demi, typdesc, &demt, typsize);
    }
 
    if (!dstat) {
        demnt = sdemi_deminamedtyp_new(demt);
        demnt->demnt_name = Strdup(typname);
        demnt->demnt_demp_unpack  = demi_new_demiprog();
        demnt->demnt_demp_pack    = demi_new_demiprog();
        dstat = demi_gen_pack_prog(demb->demb_demi, demnt);
    }
 
    if (!dstat) {
        dstat = demi_gen_unpack_prog(demb->demb_demi, demnt);
    }

    if (dstat) {
        sdemi_demityp_free(demt);
    } else {
        demm = sdemi_demiitem_new(DEMI_ITMTYP_NAMED_TYPE);
        demm->demm_item_nam = Strdup(typname);
        demm->demm_u.demm_demnt = demnt;
        sdemi_demityp_display(typname, demnt->demnt_demt, 0);
        dup = dbtree_insert(demb->demb_demi->demi_itemtree, typname, STRBYTES(typname), demm);
    }

    if (!dstat) {
        for (chix = 0; !dstat && chix < demb->demb_nchannels; chix++) {
            dstat = sdemb_send_type(demb, demb->demb_channel[chix],
                typname, typdesc, typsize);
        }
    }

    return (dstat);
}
/***************************************************************/
int demb_define_func(
    struct demi_boss * demb,
    const char * funcname,
    const char * funcinargs,
    const char * funcoutargs,
    void ** vpdemu)
{
/*
** 01/05/2020
*/
    int dstat;
    int chix;
    int func_ref;
    void ** tnode;
    struct deminamedtyp * demnt_in;
    struct deminamedtyp * demnt_out;
    struct demiitem * demm;
    struct demifunc * demu;

    dstat = 0;
    (*vpdemu) = NULL;
    demu = NULL;
    demnt_in = NULL;
    demnt_out = NULL;

    if (!(demb->demb_flags & DEMS_FLAG_STARTED)) {
        dstat = demi_set_error(demb->demb_demi, DEMIERR_NOT_STARTED, "%s", "Not started.");
        return (dstat);
    }
 
    if (!dstat) {
        tnode = dbtree_find(demb->demb_demi->demi_itemtree, funcinargs, STRBYTES(funcinargs));
        if (!tnode) {
            dstat = demi_set_error(demb->demb_demi, DEMIERR_FUNC_IN_TYPE_UNRECOGNIZED,
                "Function input type not declared. Found: '%s'", funcinargs);
        } else {
            demm = *(struct demiitem **)tnode;
            if (demm->demm_item_typ != DEMI_ITMTYP_NAMED_TYPE) {
                dstat = demi_set_error(demb->demb_demi, DEMIERR_FUNC_IN_TYPE_NOT_TYPE,
                    "Function input type not a type. Found: '%s'", funcinargs);
            } else {
                demnt_in = demm->demm_u.demm_demnt;
            }
        }
    }
 
    if (!dstat) {
        tnode = dbtree_find(demb->demb_demi->demi_itemtree, funcoutargs, STRBYTES(funcoutargs));
        if (!tnode) {
            dstat = demi_set_error(demb->demb_demi, DEMIERR_FUNC_OUT_TYPE_UNRECOGNIZED,
                "Function output type not declared. Found: '%s'", funcoutargs);
        } else {
            demm = *(struct demiitem **)tnode;
            if (demm->demm_item_typ != DEMI_ITMTYP_NAMED_TYPE) {
                dstat = demi_set_error(demb->demb_demi, DEMIERR_FUNC_OUT_TYPE_NOT_TYPE,
                    "Function output type not a type. Found: '%s'", funcoutargs);
            } else {
                demnt_out = demm->demm_u.demm_demnt;
            }
        }
    }
 
    if (!dstat) {
        dstat = demi_ensure_demifunc(demb->demb_demi, funcname, &demu);
    }

    if (!dstat) {
        if (demu->demu_in_demnt || demu->demu_out_demnt) {
            dstat = demi_set_error(demb->demb_demi, DEMIERR_DUPLICATE_FUNC,
                "Function already declared as another item. Found: '%s'", funcname);
        }
    }

    if (!dstat) {
        demu->demu_func_index = demb->demb_nfuncs;
        func_ref = FUNC_INDEX_TO_REF(demu->demu_func_index);
        sdemi_add_callable_func(demb->demb_demi, demu, func_ref);
        demb->demb_nfuncs += 1;
        demu->demu_in_demnt  = demnt_in;
        demu->demu_out_demnt = demnt_out;
        (*vpdemu) = demu;
    }

    if (!dstat) {
        for (chix = 0; !dstat && chix < demb->demb_nchannels; chix++) {
            dstat = sdemb_send_func(demb, demb->demb_channel[chix], demu->demu_func_index,
                funcname, funcinargs, funcoutargs);
        }
    }

    return (dstat);
}
/***************************************************************/
int demb_call_function(
    struct demi_boss * demb,
    void * vdemu,
    void * funcinargs,
    void * funcoutrtn)
{
/*
** 01/05/2020
void hlog(FILE * outf,
             int logflags,
             const void * vldata,
             int ldatalen,
             const char * fmt, ...);
*/
    int dstat;
    struct demifunc * demu = (struct demifunc *)vdemu;

    dstat = 0;

#if DEMI_TESTING
    if (!dstat) {
        printf("Original data:\n");
        dstat = demi_print_data(demb->demb_demi, funcinargs, demu->demu_in_demnt->demnt_demt, 0);
    }
    if (!dstat) {
        //hlog(0, 0, funcinargs, demu->demu_in_demnt->demnt_demt->demt_typ_size,
        //    "Function %s() args size=%d:\n",
        //    demu->demu_name,
        //    demu->demu_in_demnt->demnt_demt->demt_typ_size);
        dstat = demi_run_demiprog(demb->demb_demi,
            demu->demu_in_demnt->demnt_demp_pack,
            demu->demu_in_demnt->demnt_pack_tgtdemmem,
            funcinargs,
            demu->demu_in_demnt->demnt_demt->demt_typ_size, 0);
        if (!dstat) {
            dstat = demi_run_demiprog(demb->demb_demi,
                demu->demu_in_demnt->demnt_demp_unpack,
                demu->demu_in_demnt->demnt_unpack_tgtdemmem,
                demu->demu_in_demnt->demnt_pack_tgtdemmem->demmem_ptr,
                demu->demu_in_demnt->demnt_pack_tgtdemmem->demmem_length, 0);
        }
        if (!dstat) {
            printf("Unpacked data:\n");
            dstat = demi_print_data(demb->demb_demi,
                demu->demu_in_demnt->demnt_unpack_tgtdemmem->demmem_ptr,
                demu->demu_in_demnt->demnt_demt, 0);
        }
    }
#endif

    return (dstat);
}
/***************************************************************/
