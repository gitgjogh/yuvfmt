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

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "sim_opt.h"

static slog_t  g_slog_obj = { 0 };
// static slog_t *g_slog_ptr = &g_slog_obj;

int clip(int v, int minv, int maxv)
{
    v = (v<minv) ? minv : v;
    v = (v>maxv) ? maxv : v;
    return v;
}

#define CLIP(v, minv, maxv)     ((v) = clip((v), (minv), (maxv)))

int slog_set_range(slog_t *sl, int minL, int maxL, void *fp)
{
    minL = max(minL, SLOG_NON);
    maxL = min(maxL, SLOG_ALL);
    if (minL<=maxL) {
        int level;
        for (level = minL; level <= maxL; ++level) {
            sl->fp[level] = fp;
        }
        return (maxL - minL + 1);
    }
    return 0;
}

int slog_init(slog_t *sl, int level)
{
    memset(sl, 0, sizeof(slog_t));
    sl->b_inited = 1;
    if (level != SLOG_NON) { 
        sl->fp[SLOG_NON] = sl->fp[SLOG_ERR] = stderr;
        return slog_set_range(sl, SLOG_ERR+1, level, stdout);
    }
    return 0;
}

void slog_reset(slog_t *sl)
{
    memset(sl, 0, sizeof(slog_t));
}

int xlog_set_range(int minL, int maxL, void *fp)
{
    return slog_set_range(&g_slog_obj, minL, maxL, fp);
}

int xlog_init(int level)
{
    return slog_init(&g_slog_obj, level);
}

void xlog_reset()
{
    slog_reset(&g_slog_obj);
}

int xlevel(int level)
{
    return slog_init(&g_slog_obj, level);
}

int slogv(slog_t *sl, int level, const char *prompt, const char *fmt, va_list ap)
{
    FILE *fp = 0;
    if (sl->b_inited) {
        level = clip(level, SLOG_NON, SLOG_ALL);
        fp = sl->fp[level];
    } else {
        fp = (level <= SLOG_ERR) ? stderr : stdout;
    }
    
    if (fp) {
        if (prompt) {
            fprintf(fp, "@%s>> ", prompt);
        }
        return vfprintf(fp, fmt, ap);
    }

    return 0;
}

int xlogv(int level, const char *fmt, va_list ap)
{
    slogv(&g_slog_obj, level, 0, fmt, ap);
}

int xerrv(const char *fmt, va_list ap)
{
    slogv(&g_slog_obj, SLOG_ERR, "err", fmt, ap);
}

int xdbgv(const char *fmt, va_list ap)
{
    slogv(&g_slog_obj, SLOG_DBG, 0, fmt, ap);
}

int slog (slog_t *sl, int level, const char *prompt, const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = slogv(sl, level, prompt, fmt, ap);
    va_end(ap);
    return r;
}
int xlog (int level, const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = xlogv(level, fmt, ap);
    va_end(ap);
    return r;
}
int xerr (const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = xerrv(fmt, ap);
    va_end(ap);
    return r;
}
int xdbg (const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = xdbgv(fmt, ap);
    va_end(ap);
    return r;
}


void iof_cfg(ios_t *f, const char *path, const char *mode)
{
    f->b_used = (path && mode);
    if (path) {
        strncpy(f->path_buf, path, MAX_PATH);
        f->path = f->path_buf;
    }
    if (mode) {
        strncpy(f->mode, mode, MAX_MODE);
    }
}

void ios_cfg(ios_t *ios, int ch, const char *path, const char *mode)
{
    iof_cfg(&ios[ch], path, mode);
}

int ios_nused(ios_t *ios, int nch)
{
    int ch, j;
    for (ch=j=0; ch<nch; ++ch) {
        j += (!!ios[ch].b_used);
    }
    return j;   
}

/**
 *  @param [i] ios - array of io description
 *  @param [o] nop - num of files truely opened
 *  @return num of files failed to be opened
 */
int ios_open(ios_t ios[], int nch, int *nop)
{
    int ch, j, n_err;
    for (ch=j=n_err=0; ch<nch; ++ch) {
        ios_t *f = &ios[ch];
        if (f->b_used) 
        {
            f->fp = fopen(f->path, f->mode);
            if ( f->fp ) {
                xlog(SLOG_IOS, "@ios>> ch#%d 0x%08x=fopen(%s, %s)\n", 
                                        ch, f->fp, f->path, f->mode);
                ++ j;
            } else {
                xerr("@ios>> error fopen(%s, %s)\n", f->path, f->mode);
                ++ n_err;
            }
        }
    }
    nop ? (*nop = j) : 0;
    return (n_err==0);   
}

int ios_close(ios_t *ios, int nch)
{
    int ch, j;
    for (ch=j=0; ch<nch; ++ch) {
        ios_t *f = &ios[ch];
        if (f->b_used && f->fp) {
            xlog(SLOG_IOS, "@ios>> ch#%d fclose(0x%08x: %s)\n", ch, f->fp, f->path);
            fclose(f->fp);
            f->fp = 0;
            ++ j;
        }
    }
    return j;  
}

int ios_feof(ios_t *p, int ich)
{
    return feof((FILE*)p[ich].fp);
}

char *get_argv(int argc, char *argv[], int i, const char *name)
{
    int s = (argv && i<argc) ? argv[i][0] : 0;
    char *arg = (s != 0 && s != '-') ? argv[i] : 0;
    if (name) {
        xlog(SLOG_CMDL, "@cmdl>> -%s[%d] = `%s'\n", SAFE_STR(name,""), i, SAFE_STR(arg,""));
    }
    return arg;
}

char* get_uint32 (char *str, uint32_t *out)
{
    char  *curr = str;
    
    if ( curr ) {
        int  c, sum;
        for (sum=0, curr=str; (c = *curr) && c >= '0' && c <= '9'; ++ curr) {
            sum = sum * 10 + ( c - '0' );
        }
        if (out) { *out = sum; }
    }

    return curr;
}

int arg_parse_range(int i, int argc, char *argv[], int i_range[2])
{
    char *flag=0;
    char *last=0;
    char *arg = GET_ARGV(++i, "range");
    if (!arg) {
        return -1;
    }
    
    i_range[0] = 0;
    i_range[1] = INT_MAX;
    
    /* parse argv : `$base[~$last]` or `$base[+$count]` */
    //i_range[0] = strtoul (arg, &flag, 10);
    flag = get_uint32 (arg, &i_range[0]);
    if (flag==0 || *flag == 0) {       /* no `~$last` or `+$count` */
        return ++i;
    }

    /* get `~$last` or `+$count` */
    if (*flag != '~' && *flag != '+') {
        xerr("@cmdl>> Err : Invalid flag\n");
        return -1;
    }
    
    //i_range[1] = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, &i_range[1]);
    if (last == 0 || *last != 0 ) {
        xerr("@cmdl>> Err : Invalid count/end\n");
        i_range[1] = INT_MAX;
        return -1;
    }
    
    if (*flag == '+') {
        i_range[1] += i_range[0];
    }
    
    return ++i;
}

int arg_parse_str(int i, int argc, char *argv[], char **p)
{
    char *arg = GET_ARGV(++ i, "string");
    *p = arg ? arg : 0;
    return arg ? ++i : -1;
}

int arg_parse_strcpy(int i, int argc, char *argv[], char *buf, int nsz)
{
    char *arg = GET_ARGV(++ i, "string");
    if (arg) {
        strncpy(buf, arg, nsz);
        buf[nsz-1] = 0;
    } else {
        buf[0] = 0;
    }
    return arg ? ++i : -1;
}

int arg_parse_int(int i, int argc, char *argv[], int *p)
{
    char *arg = GET_ARGV(++ i, "int");
    *p = arg ? atoi(arg) : 0;
    return arg ? ++i : -1;
}

int opt_parse_int(int i, int argc, char *argv[], int *p, int default_val)
{
    char *arg = GET_ARGV(++ i, "int");
    *p = arg ? atoi(arg) : default_val;
    return arg ? ++i : i;
}

int arg_parse_xlevel(int i, int argc, char *argv[])
{
    int j = i;
    while (i>=j && i<argc)
    {
        char *arg = &argv[i][1];
        if (0==strcmp(arg, "xnon")) {
            ++i;    xlevel(SLOG_NON);
        } else
        if (0==strcmp(arg, "xall")) {
            ++i;    xlevel(SLOG_ALL);
        } else
        if (0==strcmp(arg, "xlevel")) {
            int level;
            i = arg_parse_int(i, argc, argv, &level);
            xlevel(level);
        } else
        {
            break;
        }
        j = i;
    }
    
    return i;
}
