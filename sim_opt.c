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
            if (strchr(f->mode, 'a') || strchr(f->mode, 'w') || strchr(f->mode, 'r')) 
            {
                FILE *fp = fopen(f->path, "r");
                if (fp) {
                    char yes_or_no = 0;
                    fclose(fp); fp =0;
                    printf("file `%s' already exist, overwrite? (y/n)\n", f->path);
                    scanf("%c\n", &yes_or_no);
                    if (yes_or_no != 'y') {
                        printf("file `%s' would be skipped\n", f->path);
                        continue;
                    }
                }
            }
            
            f->fp = fopen(f->path, f->mode);
            if ( f->fp ) {
                xlog("@ios>> ch#%d 0x%08x=fopen(%s, %s)\n", ch, f->fp, f->path, f->mode);
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
    int s = (argv && i<argc) ? argv[i][0] : 0;
    char *arg = (s != 0 && s != '-') ? argv[i] : 0;
    if (name) {
        xlog("@cmdl.dbg>> -%s[%d] = `%s'\n", name+1, i, arg?arg:"nil");
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
    return ((type == OPT_T_INTI || type == OPT_T_INTX) ? "d" : 
           ((type == OPT_T_BOOL) ? "b" :
           ((type == OPT_T_STR || type == OPT_T_STRCPY) ? "s" : "?")));
} 

int cmdl_try_default(opt_desc_t *opt, int i_arg) 
{
    void *pval = opt->pval;
    if (!pval) {
        return 0;
    }
    
    switch(opt->type) 
    {
    case OPT_T_BOOL:
        ((int *)pval)[i_arg] = 1;
        return 1;
    case OPT_T_INTI:
    case OPT_T_INTX:
        ((int *)pval)[i_arg] = 0;
        return 1;
    }
    return 0;
}

int cmdl_str2val(opt_desc_t *opt, int i_arg, char *arg) 
{
    void *pval = opt->pval;
    if (!pval) {
        return 0;
    }
    
    switch(opt->type) 
    {
    case OPT_T_BOOL:
    case OPT_T_INTI:
    case OPT_T_INTX:
        ((int *)pval)[i_arg] = atoi(arg);
        break;
    case OPT_T_STR:
        ((char **)pval)[i_arg] = arg;
        break;
    case OPT_T_STRCPY:
        strcpy(((char **)pval)[i_arg], arg);
        break;
    default:
        xerr("not support opt type (%d)\n", opt->type);
        return -1;
    }
    return 0;
}

int cmdl_val2str(opt_desc_t *opt, int i_arg) 
{
    int r = 0;
    char *str = 0;
    void *pval = opt->pval;
    if (!pval) {
        printf("?");
        return 0;
    }
    
    switch(opt->type) 
    {
    case OPT_T_BOOL:
    case OPT_T_INTI:
        r = printf("%d", ((int *)pval)[i_arg] );
        break;
    case OPT_T_INTX:
        r = printf("0x%08x", ((int *)pval)[i_arg] );
        break;
    case OPT_T_STR:
    case OPT_T_STRCPY:
        str = ((char **)pval)[i_arg] ;
        r = printf("%s", str ? str : "?");
        break;
    default:
        xerr("not support opt type (%d)\n", opt->type);
        return -1;
    }
    return r;
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

int cmdl_get_optdesc(int optc, opt_desc_t optv[], const char *name)
{
    int i;
    for (i=0; i<optc; ++i) {
        if (0==strcmp(name, optv[i].name+1)) {
            return i;
        }
    }
    return -1;
}

int cmdl_set_ref(int optc, opt_desc_t optv[], 
                const char *name, int nref, opt_ref_t *refs)
{
    int i_opt = cmdl_get_optdesc(optc, optv, name);
    if (i_opt >= 0) {
        opt_desc_t *opt = &optv[i_opt];
        opt->nref = nref;
        opt->refs = refs;
    }
    return i_opt;
}

int cmdl_set_enum(int optc, opt_desc_t optv[], 
                const char *name, int nenum, opt_enum_t *enums)
{
    int i_opt = cmdl_get_optdesc(optc, optv, name);
    if (i_opt >= 0) {
        opt_desc_t *opt = &optv[i_opt];
        opt->nenum = nenum;
        opt->enums = enums;
    }
    return i_opt;
}


int cmdl_get_optargs(int i, int argc, char *argv[], opt_desc_t *opt)
{
    //xlog("@cmdl>> -%s : cmdl_get_optargs(%d, %d, %s...)\n", 
    //    opt->name, i, argc, argv ? argv[i] : "?");
    int j;
    int    arg_count = argc - i;
    char **arg_array = &argv[i];
    char  *arg = 0;
    
    if (argv == 0) {
        opt->b_default = 1;
        arg = opt->default_val;
        arg_count = arg ? 1 : 0;
        arg_array = &arg;
        xlog("@cmdl.dbg>> use default_val `%s'\n", arg ? arg : "nil");
    } else {
        arg = get_argv(argc, argv, i, opt->name);
    }
    
    char *spl[256] = {0};
    char splbuf[1024] = {0};
    if (arg && arg[0] == '&') 
    {
        if (opt->nref > 0) {
            opt->i_ref = ref_name_2_idx(opt->nref, opt->refs, &arg[1]);
            if (opt->i_ref < 0) {
                xerr("can't find -%s.%s as ref\n", opt->name, arg);
                return -i;
            }
            
            strncpy(splbuf, opt->refs[opt->i_ref].val, 1024);
            arg_count = cmdl_strspl(splbuf, spl, 256);
            if (arg_count>=256) {
                xerr("strspl() overflow\n");
                return -i;
            }
            
            arg_array = spl;
        } else if ( cmdl_enum_usable( opt ) ) {
            opt->i_enum = enum_name_2_idx(opt->nenum, opt->enums, &arg[1]);
            if (opt->i_enum < 0) {
                xerr("can't find -%s.%s as enum\n", opt->name, arg);
                return -i;
            }
            *((int *)opt->pval) = opt->enums[opt->i_enum].val;
            return i + !opt->b_default;
        } else {
            xerr("can't expand -%s.%s neither as ref nor enum\n", opt->name, arg);
            return -i;
        }
    }
    //TODO: bug here
    int i_arg;
    for (i_arg=0; i_arg<opt->narg; ++i_arg) {
        arg = get_argv(arg_count, arg_array, i_arg, opt->name);
        if (arg) {
            cmdl_str2val(opt, i_arg, arg);
        } else if ( !cmdl_try_default(opt, i_arg) ) {
            xerr("%s no more args for %s\n", opt->i_ref >= 0 ? arg : "", opt->name);
            return -(i + opt->b_default ? 0 : (opt->i_ref >= 0 ? 1 : i_arg));
        }
    }
    
    int narg = opt->b_default ? 0 : (opt->i_ref >= 0 ? 1 : i_arg);
    return i + narg;
}

int cmdl_init(int optc, opt_desc_t optv[])
{
    int n_err = 0;
    int i;
    for (i=0; i<optc; ++i) 
    {
        opt_desc_t *opt = &optv[i];
        opt->i_ref = opt->i_enum = -1;
    }
    for (i=0; i<optc; ++i) 
    {
        opt_desc_t *opt = &optv[i];
        if (!opt->name || (opt->name[0] != '*' && opt->name[0] != '+') ) {
            xerr("invalide optiion name\n");
            -- n_err;
        }
        if (opt->narg < 1) {
            xerr("-%s.narg = %d is not allowed\n", opt->name + 1, opt->narg);
            -- n_err;
        }
    }
    return n_err;
}

int cmdl_parse(int i, int argc, char *argv[], int optc, opt_desc_t optv[])
{
    ENTER_FUNC;
    
    if (i>=argc) {
        return -i;
    }
     
    for (i=1; i>=0 && i<argc; )
    {
        char *cmdl_opt = argv[i];
        if (cmdl_opt[0]!='-') {
            xerr("@cmdl>> argv[%d] (%s) is not opt\n", i, cmdl_opt);
            return -i;
        }
        
        int i_opt = cmdl_get_optdesc(optc, optv, cmdl_opt+1);
        if (i_opt < 0) {
            xerr("no option `%s` defined\n", cmdl_opt);
            return -i;
        }
        
        ++ optv[i_opt].n_parse;
        optv[i_opt].argvIdx = i++;
        i = cmdl_get_optargs(i, argc, argv, &optv[i_opt]);
        if (i <= optv[i_opt].argvIdx) {
            xerr("%d = cmdl_get_optargs(%d, %d, %s...)\n", i, optv[i_opt].argvIdx+1, argc, argv[i]);
            return - optv[i_opt].argvIdx;
        }
    }
    
    LEAVE_FUNC;
    
    return i;
}

int cmdl_help(int optc, opt_desc_t optv[])
{
    int i, j;
    
    ENTER_FUNC;
    
    printf("help = {\n");
    
    for (i=0; i<optc; ++i) 
    {
        opt_desc_t *opt = &optv[i];
        printf("    -%s (%c), %%%s{%d}, help='%s', default='%s';\n", 
                opt->name + 1, opt->name[0],
                cmdl_typestr(opt->type), 
                opt->narg, 
                opt->help, 
                opt->default_val ? opt->default_val : "" );

        for (j=0; j<opt->nref; ++j ) {
            opt_ref_t *r = &opt->refs[j];
            printf("\t &%-10s \t `%s'\n", r->name, r->val);
        }
        for (j=0; j<opt->nenum; ++j ) {
            opt_enum_t *e = &opt->enums[j];
            printf("\t &%-10s \t %d\n", e->name, e->val);
        }
    }
    
    printf("};\n");
    
    LEAVE_FUNC;
    
    return 0;
}

int cmdl_check(int optc, opt_desc_t optv[])
{
    int n_err = 0;
    int i_opt, i_arg;
    
    ENTER_FUNC;
    
    for (i_opt=0; i_opt<optc; ++i_opt) 
    {
        opt_desc_t *opt = &optv[i_opt];
        if (opt->n_parse > 0) {
            continue;
        }
        //xlog("@cmdl>> -%-10s hasn't given at cmdl\n", &opt->name[1]);
        if (opt->name[0] == '+') {
            xerr("%s not specified\n", &opt->name[1]);
            -- n_err;
        } else {
            xlog("@cmdl>> try default_val for -%s\n", &opt->name[1]);
            int r = cmdl_get_optargs(0, 0, 0, opt);
            if (r < 0) {
                xerr("%s fail to use default_val\n", &opt->name[1]);
                -- n_err;
            } 
        }
    }
    
    LEAVE_FUNC;
    
    return n_err;
}

int cmdl_result(int optc, opt_desc_t optv[])
{
    int i_opt, i_arg;
    
    ENTER_FUNC;
    
    printf("result = {\n");
    for (i_opt=0; i_opt<optc; ++i_opt) 
    {
        opt_desc_t *opt = &optv[i_opt];
        printf("\t-%s (%c%d) = `", opt->name+1, opt->name[0], opt->n_parse);
        int narg = max(opt->narg, 1);
        for (i_arg=0; i_arg<narg; ++i_arg) {
            cmdl_val2str(opt, i_arg);
            printf("%c", (i_arg==narg-1) ? '\'' : ' ');
        }
        
        //printf("\t ref[#%d]=", opt->i_ref);
        if (opt->i_ref >= 0 && opt->i_ref < opt->nref) {
            printf(" (&%s)", opt->refs[opt->i_ref].name);
        }
        
        //printf("enum[#%d]=", opt->i_enum);
        if (opt->i_enum >= 0 && opt->i_enum < opt->nenum) {
            printf(" (&%s)", opt->enums[opt->i_enum].name);
        }
        printf("; \n");
    }
    printf("}; \n");
    
    LEAVE_FUNC;
    
    return 0;
}