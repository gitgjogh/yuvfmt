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

#include "yuvdef.h"
#include "sim_opt.h"


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

int ios_open(ios_t *ios, int nch, int *nop)
{
    int ch, j, k;
    for (ch=j=k; ch<nch; ++ch) {
        ios_t *f = &ios[ch];
        if (f->b_used) {
            f->fp = fopen(f->path, f->mode);
            if( f->fp ){
                xlog("@ios>> ch#%d 0x%08x=fopen(%s, %s)\n", ch, f->fp, f->path, f->mode);
                ++ j;
            } else {
                xerr("@ios>> error fopen(%s, %s)\n", f->path, f->mode);
                ++ k;
            }
        }
    }
    nop ? (*nop = j) : 0;
    return (k==0);   
}

int ios_close(ios_t *ios, int nch)
{
    int ch, j;
    for (ch=j=0; ch<nch; ++ch) {
        ios_t *f = &ios[ch];
        if (f->b_used && f->fp) {
            xlog("@ios>> ch#%d fclose(0x%08x: %s)\n", ch, f->fp, f->path);
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
    int s = i<argc ? argv[i][0] : 0;
    char *arg = (s != 0 && s != '-') ? argv[i] : 0;
    if (name) {
        xlog("@cmdl>> get_argv[%s]=%s\n", name, arg?arg:"");
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
        xlog("@cmdl>> Err : Invalid flag\n");
        return -1;
    }
    
    //i_range[1] = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, &i_range[1]);
    if (last == 0 || *last != 0 ) {
        xlog("@cmdl>> Err : Invalid count/end\n");
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

int arg_parse_xkey(int i, int argc, char *argv[], slog_t *kl)
{
    const char *arg = GET_ARGV(++ i, "xkey");
    arg ? slog_binds(kl ? kl : xlog_ptr, SLOG_L_ADD, arg) : 0;
    return arg ? ++i : i;
}

char* cmdl_typestr(int type)
{
    return (type == OPT_T_INTI || type == OPT_T_INTX) ? "d" : 
           ((type == OPT_T_STR || type == OPT_T_STRCPY) ? "s" : "?");
} 

int cmdl_str2val(opt_desc_t *opt, int i_arg, const char *arg) 
{
    void *pval = opt->pval + i_arg;
    switch(opt->type) 
    {
    case OPT_T_INTI:
    case OPT_T_INTX:
        *((int *)pval) = atoi(arg);
        break;
    case OPT_T_STR:
        *((const char **)pval) = arg;
        break;
    case OPT_T_STRCPY:
        strcpy(*(char **)pval, arg);
        break;
    default:
        xerr("not support opt type (%d)\n", opt->type);
        return -1;
    }
    return 0;
}

int cmdl_val2str(opt_desc_t *opt, int i_arg) 
{
    void *pval = opt->pval + i_arg;
    switch(opt->type) 
    {
    case OPT_T_INTI:
        printf("%d", *((int *)pval));
        break;
    case OPT_T_INTX:
        printf("0x%08x", *((int *)pval));
        break;
    case OPT_T_STR:
    case OPT_T_STRCPY:
        printf("%s", *((char **)pval));
        break;
    default:
        xerr("not support opt type (%d)\n", opt->type);
        return -1;
    }
    return 0;
}

int ref_name_2_idx(int nref, opt_ref_t *refs, const char *name)
{
    int i;
    for(i=0; i<nref; ++i) {
        if (0==strcmp(refs[i].name, name)) {
            return i;
        }
    }
    return -1;
}

int ref_sval_2_idx(int nref, opt_ref_t *refs, const char* val)
{
    int i;
    for(i=0; i<nref; ++i) {
        if (0==strcmp(refs[i].val, val)) {
            return i;
        }
    }
    return -1;
}

int ref_ival_2_idx(int nref, opt_ref_t *refs, int val)
{
    int i;
    char s[256] = {0};
    snprintf(s, 256, "%s", val);
    return ref_sval_2_idx(nref, refs, s);
}

int enum_val_2_idx(int nenum, opt_enum_t* enums, int val)
{
    int i;
    for(i=0; i<nenum; ++i) {
        if (enums[i].val == val) {
            return i;
        }
    }
    return 0;
}

const char *enum_val_2_name(int nenum, opt_enum_t* enums, int val)
{
    int i = enum_val_2_idx(nenum, enums, val);
    if (i >= 0) {
        return enums[i].name;
    }
    return 0;
}

int enum_name_2_idx(int nenum, opt_enum_t* enums, const char *name)
{
    int i;
    for(i=0; i<nenum; ++i) {
        if (0 == strcmp(enums[i].name, name)) {
            return i;
        }
    }
    return -1;
}

int cmdl_enum_usable(opt_desc_t *opt)
{
    if ((opt->type == OPT_T_INTX || opt->type == OPT_T_INTI) && 
        (opt->narg <= 1 && opt->nenum > 0))
    {
        return 1;
    }
    return 0;
}

int cmdl_help(int optc, opt_desc_t optv[])
{
    int i, j;
    for (i=0; i<optc; ++i) 
    {
        opt_desc_t *opt = &optv[i];
        printf("-%s, <%s>{%d}, ?%s, *%s", 
                opt->name, 
                cmdl_typestr(opt->type), 
                opt->narg,
                opt->help, 
                opt->default_val);

        for (j=0; j<opt->nref; ++j ) {
            opt_ref_t *r = &opt->refs[j];
            printf("\t &%s \t %s\n", r->name, r->val);
        }
        for (j=0; j<opt->nenum; ++j ) {
            opt_enum_t *e = &opt->enums[j];
            printf("\t &%s \t %d\n", e->name, e->val);
        }
        
    }
    return 0;
}

int cmdl_strspl(char *record, char *fieldArr[], int arrSz)
{
    int nkey=0, keylen=0, pos=0;
    keylen = get_token_pos(record, 0, " ", " ,", &pos);
    while (keylen && nkey<arrSz) {
        fieldArr[nkey++] = &record[pos];
        if (record[pos+keylen] == 0) {
            break;
        } else {
            record[pos+keylen] = 0;
            keylen = get_token_pos(record, pos+keylen+1, " ,", " ,", &pos);
        }
    }
    return nkey;
}

int cmdl_parse(int i, int argc, char *argv[], int optc, opt_desc_t optv[])
{
    int i_opt;
    if (i>argc-1) {
        return -i;
    }
    const char *cur_opt = argv[i];
    for (i_opt=0; i_opt<optc; ++i_opt) 
    {
        opt_desc_t *opt = &optv[i_opt];
        cur_opt += 1;
        if (0==strcmp(cur_opt, opt->name)) 
        {
            int   left = argc - (++i);
            char **args = &argv[i];
            char *arg  = get_argv(argc, argv, i, cur_opt);
            int   need = max(opt->narg, 1);
            
            int  i_ref = -1;
            char *spl[256] = {0};
            char splbuf[1024] = {0};
            
            if (arg == 0 && opt->narg == 0) {
                arg  = opt->default_val;
                left = 1;
                args = &arg;
            }
            
            if (arg == 0) {
                xerr("no arg for %s\n", cur_opt);
                return -i;
            }
            
            char *refstr = 0;
            if (arg[0] == '&') 
            {
                if (opt->nref) {
                    i_ref = ref_name_2_idx(opt->nref, opt->refs, &arg[1]);
                    if (i_ref < 0) {
                        xerr("can't find -%s.%s as ref\n", cur_opt, arg);
                        return -i;
                    }
                    
                    strncpy(splbuf, opt->refs[i_ref].val, 1024);
                    left = cmdl_strspl(splbuf, spl, 256);
                    if (left>=256) {
                        xerr("strspl() overflow\n");
                        return -i;
                    }
                    args = spl;
                } else if ( cmdl_enum_usable( opt ) ) {
                    int i_enum = enum_name_2_idx(opt->nenum, opt->enums, &arg[1]);
                    if (i_enum < 0) {
                        xerr("can't find -%s.%s as enum\n", cur_opt, arg);
                        return -i;
                    }
                    *((int *)opt->pval) = opt->enums[i_ref].val;
                    return ++i;
                } else {
                    xerr("can't expand -%s.%s neither as ref nor enum\n", cur_opt, arg);
                    return -i;
                }
            }
            
            int i_arg;
            for (i_arg=0; i_arg<left; ++i_arg) {
                char *arg = get_argv(left, args, i_arg, cur_opt);
                if (!arg) {
                    xerr("%s no more args for %s\n", refstr ? arg : "", cur_opt);
                    return -i - (refstr ? 1 : i_arg);
                } else {
                    cmdl_str2val(opt, i_arg, arg);
                }
            }
            
            return i + (refstr ? 1 : i_arg);
        }
    }
    
    return -i;
}

int cmdl_result(int optc, opt_desc_t optv[])
{
    int i_opt, i_arg;
    for (i_opt=0; i_opt<optc; ++i_opt) 
    {
        opt_desc_t *opt = &optv[i_opt];
        printf("-%s={", opt->name);
        int narg = max(opt->narg, 1);
        for (i_arg=0; i_arg<narg; ++i_arg) {
            cmdl_val2str(opt, i_arg);
            printf(",");
        }
        
        if ( cmdl_enum_usable(opt) ) {
            int val = *((int *)opt->pval);
            const char *name = enum_val_2_name(opt->nenum, opt->enums, val);
            if (name) {
                printf("(&%s)", name);
            }
        }
        printf("}\n");
    }
    return 0;
}