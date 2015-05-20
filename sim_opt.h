/*****************************************************************************
 * Copyright 2014 Jeff <ggjogh@gmail.com>
 *****************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*****************************************************************************/

#ifndef __SIM_OPT_H__
#define __SIM_OPT_H__

#include <stdint.h>
#include <stdarg.h>

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif

typedef enum slog_level {
    SLOG_NON        = 0,
    SLOG_ERR        ,
    SLOG_WARN       ,
    SLOG_ALERT      ,
    SLOG_NOTICE     ,
    SLOG_INFO       ,
    SLOG_DEFULT     ,
    SLOG_CFG        = 10,
    SLOG_L0         = 20,
    SLOG_L1         ,
    SLOG_L2         ,
    SLOG_L3         ,
    SLOG_L4         ,
    SLOG_L5         ,
    SLOG_L6         ,
    SLOG_L7         ,
    SLOG_L8         ,
    SLOG_L9         ,
    SLOG_DBG        = 50,
    SLOG_CMDL       = 55,
    SLOG_MEM        = 60,
    SLOG_IOS        = 70,
    SLOG_FUNC       = 80,
    SLOG_VAR        = 90,
    SLOG_ALL        = 99,
} slog_level_i;


typedef struct simple_level_log
{
    int         b_inited;
    void       *fp[SLOG_ALL+1];
} slog_t;

int slog_set_range(slog_t *sl, int minL, int maxL, void *fp);
int slog_init(slog_t *sl, int level);
void slog_reset(slog_t *sl);
int slogv(slog_t *sl, int level, const char *prompt, const char *fmt, va_list ap);
int slog (slog_t *sl, int level, const char *prompt, const char *fmt, ...);

int xlog_set_range(int minL, int maxL, void *fp);
int xlog_init(int level);
void xlog_reset();
int xlevel(int level);
int xlogv(int level, const char *fmt, va_list ap);
int xlog (int level, const char *fmt, ...);
int xerrv(const char *fmt, va_list ap);
int xerr (const char *fmt, ...);
int xdbgv(const char *fmt, va_list ap);
int xdbg (const char *fmt, ...);

static int fcall_layer = 0;
#define ENTER_FUNC()  xlog(SLOG_FUNC, "@>>>> %-2d: %s(+)\n", fcall_layer++, __FUNCTION__)
#define LEAVE_FUNC()  xlog(SLOG_FUNC, "@<<<< %-2d: %s(-)\n\n", --fcall_layer, __FUNCTION__)
#define EMPTYSTR                ("")
#define SAFE_STR(s, nnstr)      ((s)?(s):(nnstr))


#ifndef MAX_PATH
#define MAX_PATH    256
#endif
#define MAX_MODE    8
typedef struct file_io {
    int     b_used;
    char    path_buf[MAX_PATH];
    char    *path;
    char    mode[MAX_MODE];
    void    *fp;
} ios_t;

void    iof_cfg(ios_t *p, const char *path, const char *mode);
void    ios_cfg(ios_t *p, int ch, const char *path, const char *mode);
int     ios_nused(ios_t *p, int nch);
int     ios_open(ios_t *p, int nch, int *nop);
int     ios_close(ios_t *p, int nch);
int     ios_feof(ios_t *p, int ich);


#define GET_ARGV(idx, name) get_argv(argc, argv, idx, name);
char*   get_argv(int argc, char *argv[], int i, const char *name);
char*   get_uint32 (char *str, uint32_t *out);

int     arg_parse_range(int i, int argc, char *argv[], int i_range[2]);
int     arg_parse_str(int i, int argc, char *argv[], char **p);
int     arg_parse_strcpy(int i, int argc, char *argv[], char *buf, int nsz);
int     arg_parse_int(int i, int argc, char *argv[], int *p);
int     opt_parse_int(int i, int argc, char *argv[], int *p, int default_val);
int     arg_parse_xlevel(int i, int argc, char *argv[]);


enum option_type {
    OPT_T_BOOL      =   'b',
    OPT_T_INTI      =   'd',
    OPT_T_INTX      =   'x',
    OPT_T_STR       =   'sp',
    OPT_T_STRCPY    =   'sa',
};

typedef struct opt_reference {
    char *name;
    char *val;
} opt_ref_t;

typedef struct opt_enum {
    char *name;
    int   val;
} opt_enum_t;

int ref_name_2_idx(int nref, const opt_ref_t *refs, const char *name);
int ref_sval_2_idx(int nref, const opt_ref_t *refs, const char* val);
int ref_ival_2_idx(int nref, const opt_ref_t *refs, int val);
int enum_val_2_idx(int nenum, const opt_enum_t* enums, int val);
int enum_name_2_idx(int nenum, const opt_enum_t* enums, const char *name);
const char *enum_val_2_name(int nenum, const opt_enum_t* enums, int val);

typedef struct option_description {
    char*   name;
    int     type;
    int     narg;
    void*   pval;
    char*   default_val;
    char*   help;
    
    short   nref;
    short   nenum;
    union {
        const opt_ref_t* refs;
        const opt_enum_t* enums;
    };
    
//<! private:
    int     n_parse;
    int     argvIdx;
    int     b_default;
    int     i_ref;
    int     i_enum;
} opt_desc_t;

int cmdl_set_ref(int optc, opt_desc_t optv[], 
                const char *name, int n_ref, const opt_ref_t *refs);
int cmdl_set_enum(int optc, opt_desc_t optv[], 
                const char *name, int n_enum, const opt_enum_t *enums);

int cmdl_getdesc_byref (int optc, opt_desc_t optv[], const char *name);
int cmdl_getdesc_byname(int optc, opt_desc_t optv[], const char *name);
int cmdl_init(int optc, opt_desc_t optv[]);
int cmdl_parse(int i, int argc, char *argv[], int optc, opt_desc_t optv[]);
int cmdl_parse_opt(int i, int argc, char *argv[], opt_desc_t *opt);
int cmdl_help(int optc, opt_desc_t optv[]);
int cmdl_check(int optc, opt_desc_t optv[]);
int cmdl_result(int optc, opt_desc_t optv[]);

#endif  // __SIM_OPT_H__
