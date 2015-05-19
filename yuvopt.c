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

#include <limits.h>

#include "yuvdef.h"
#include "yuvopt.h"

const res_t cmn_res[] = {
    {"qcif",    176,    144},
    {"cif",     352,    288},
    {"360",     640,    360},
    {"480",     720,    480},
    {"720",     1280,   720},
    {"1080",    1920,   1080},
    {"2k",      1920,   1080},
    {"1088",    1920,   1088},
    {"2k+",     1920,   1088},
    {"2160",    3840,   2160},
    {"4k",      3840,   2160},
    {"2176",    3840,   2176},
    {"4k+",     3840,   2176},
};

const fmt_t cmn_fmt[] = {
    {"400p",    YUVFMT_400P     },
    {"420p",    YUVFMT_420P     },
    {"420sp",   YUVFMT_420SP    },
    {"420spa",  YUVFMT_420SPA   },
    {"422p",    YUVFMT_422P     },
    {"422sp",   YUVFMT_422SP    },
    {"422spa",  YUVFMT_422SPA   },
    {"uyvy",    YUVFMT_UYVY     },
    {"yuyv",    YUVFMT_YUYV     },
};

const int n_cmn_res = ARRAY_SIZE(cmn_res);
const int n_cmn_fmt = ARRAY_SIZE(cmn_fmt);


char *get_argv(int argc, char *argv[], int i, char *name)
{
    int s = i<argc ? argv[i][0] : 0;
    char *arg = (s != 0 && s != '-') ? argv[i] : 0;
    if (name) {
        printf("@cmdl>> get_argv[%s]=%s\n", name, arg?arg:"");
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

int arg_parse_wxh(int i, int argc, char *argv[], int *pw, int *ph)
{
    int j, w, h;
    char *flag=0;
    char *last=0;
    pw = pw ? pw : &w;
    ph = ph ? ph : &h;
    
    char *arg = GET_ARGV(++ i, "wxh");
    if (!arg) {
        return -1;
    }
    
    for (j=0; j<n_cmn_res; ++j) {
        if (0==strcmp(arg, cmn_res[j].name)) {
            *pw = cmn_res[j].w;  
            *ph = cmn_res[j].h;
            return ++i;
        }
    }
    
    //seq->width = strtoul (arg, &flag, 10);
    flag = get_uint32 (arg, pw);
    if (flag==0 || *flag != 'x') {
        printf("@cmdl>> Err : not (%%d)x(%%d)\n");
        return -1;
    }
    
    //seq->height = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, ph);
    if (last == 0 || *last != 0 ) {
        printf("@cmdl>> Err : not (%%d)x(%%d)\n");
        return -1;
    }

    return -1;
}

int arg_parse_fmt(int i, int argc, char *argv[], int *fmt)
{
    int j;
    
    char *arg = GET_ARGV(++ i, "yuvfmt");
    if (!arg) {
        return -1;
    }
    
    for (j=0; j<n_cmn_fmt; ++j) {
        if (0==strcmp(arg, cmn_fmt[j].name)) {
            *fmt = cmn_fmt[j].ifmt;
            return ++i;
        }
    }
    
    printf("@cmdl>> Err : unrecognized yuvfmt `%s`\n", arg);
    return -1;
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
        printf("@cmdl>> Err : Invalid flag\n");
        return -1;
    }
    
    //i_range[1] = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, &i_range[1]);
    if (last == 0 || *last != 0 ) {
        printf("@cmdl>> Err : Invalid count/end\n");
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

int arg_parse_int(int i, int argc, char *argv[], int *p)
{
    char *arg = GET_ARGV(++ i, "int");
    *p = arg ? atoi(arg) : (-1);
    return arg ? ++i : -1;
}