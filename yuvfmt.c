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

/**
 *  @file yuvfmt.c
 *  @brief Covert whatever to YUV420P. For testing cmdl_xxx() funcs Mainly.
 */

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "yuvdef.h"
#include "yuvcvt.h"
#include "yuvfmt.h"

typedef cvt_opt_t fmt_opt_t;
extern const opt_enum_t cmn_fmt[];
extern const int n_cmn_fmt;

const opt_ref_t cmn_wxh[] = 
{
    {"qcif",    "176 , 144 "},
    {"cif",     "352 , 288 "},
    {"360",     "640 , 360 "},
    {"480",     "720 , 480 "},
    {"720",     "1280, 720 "},
    {"1080",    "1920, 1080"},
    {"2k",      "1920, 1080"},
    {"1088",    "1920, 1088"},
    {"2k+",     "1920, 1088"},
    {"2160",    "3840, 2160"},
    {"4k",      "3840, 2160"},
    {"2176",    "3840, 2176"},
    {"4k+",     "3840, 2176"},
};
const int n_cmn_wxh = ARRAY_SIZE(cmn_wxh);

const opt_enum_t cmn_bit[] = 
{
    {"b8",   8},
    {"b10", 10},
    {"b16", 16},
};
const int n_cmn_bit = ARRAY_SIZE(cmn_bit);

int yuv_prop_parser(int i, int argc, char *argv[], yuv_seq_t *yuv, int b_help)
{
    int r = 0, j = 0;
    
    opt_desc_t fmt_opt[] = 
    {
        { "*fmt",    OPT_T_INTI, 1, &yuv->yuvfmt, "%420p", "yuvfmt",  0, n_cmn_fmt, cmn_fmt},
        { "*wxh",    OPT_T_INTI, 2, yuv->wxh,      0,      "w & h",   n_cmn_wxh, 0, cmn_wxh},
        { "*nbit",   OPT_T_INTI, 1, &yuv->nbit,   "8",     "",        0, n_cmn_bit, cmn_bit},
        { "*nlsb",   OPT_T_INTI, 1, &yuv->nlsb,   "0",  ""},
        { "*tile",   OPT_T_BOOL, 1, &yuv->btile,      0, ""},
        { "*stride", OPT_T_INTI, 1, &yuv->y_stride,   0, ""},
        { "*iosize", OPT_T_INTI, 1, &yuv->io_size,    0, ""},
    };
    const int n_fmt_opt = ARRAY_SIZE(fmt_opt);
    
    ENTER_FUNC();
    
    if (b_help) {
        cmdl_help(n_fmt_opt, fmt_opt);
    } else{
        r = cmdl_init(n_fmt_opt, fmt_opt);
        if (r < 0) {
            xerr("invalid cmdl description defined (%d)\n\n", r);
            return -i;
        }
        
        j = cmdl_parse(i, argc, argv, n_fmt_opt, fmt_opt);
        if (j < 0) {
            xerr("fmt_arg_parse() failed (%d)\n\n", r);
            return j;
        }
        //r = cmdl_result(n_fmt_opt, fmt_opt);
        
        r = cmdl_check(n_fmt_opt, fmt_opt);
        if (r < 0) {
            xerr("fmt_arg_check() failed (%d)\n\n", r);
            return -j;
        }
        r = cmdl_result(n_fmt_opt, fmt_opt);
        
        return j;
    }

    return 0;
}

int fmt_arg_init (fmt_opt_t *cfg, int argc, char *argv[])
{
    set_yuv_prop(&cfg->src, 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    set_yuv_prop(&cfg->dst, 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    cfg->frame_range[1] = INT_MAX;
}

int fmt_arg_parse(fmt_opt_t *cfg, int argc, char *argv[])
{
    int i, j;
    yuv_seq_t *seq = &cfg->dst;
    yuv_seq_t *src = &cfg->src;
    yuv_seq_t *dst = &cfg->dst;
    
    ENTER_FUNC();
    
    if (argc<2) {
        return -1;
    }
    if (0 != strcmp(argv[1], "-dst") && 0 != strcmp(argv[1], "-src") &&
        0 != strcmp(argv[1], "-h")   && 0 != strcmp(argv[1], "-help") &&
        0 != strcmp(argv[1], "-x")   && 0 != strcmp(argv[1], "-xall"))
    {
        return -1;
    }

    /**
     *  loop options
     */    
    for (i=1; i>=0 && i<argc; )
    {
        char *arg = argv[i];
        if (arg[0]!='-') {
            xerr("@cmdl>> argv[%d] = `%s` is not option\n", i, arg);
            return -i;
        }
        xlog(SLOG_CMDL, "@cmdl>> argv[%i] = %s\n", i, arg);
        arg += 1;
        
        for (j=0; j<n_cmn_res; ++j) {
            if (0==strcmp(arg, cmn_res[j].name)) {
                src->width  = cmn_res[j].w;
                src->height = cmn_res[j].h;
                break;
            }
        }
        if (j<n_cmn_res) {
            ++i; continue;
        }
        
        for (j=0; j<n_cmn_fmt; ++j) {
            if (0==strcmp(arg, cmn_fmt[j].name)) {
                seq->yuvfmt = cmn_fmt[j].val;
                break;
            }
        }
        if (j<n_cmn_fmt) {
            ++i; continue;
        }
        
        if (0==strcmp(arg, "h") || 0==strcmp(arg, "help")) {
            fmt_arg_help(cfg, argc, argv);
            return 0;
        } else
        if (0==strcmp(arg, "src")) {
            char *path = 0;
            i = arg_parse_str(i, argc, argv, &path);
            if (i>0) {
                ios_cfg(cfg->ios, CVT_IOS_SRC, path, "rb");
                i = yuv_prop_parser(i, argc, argv, &cfg->src, 0);
            }
        } else
        if (0==strcmp(arg, "dst")) {
            char *path = 0;
            i = arg_parse_str(i, argc, argv, &path);
            if (i>0) {
                ios_cfg(cfg->ios, CVT_IOS_DST, path, "wb");
                i = yuv_prop_parser(i, argc, argv, &cfg->dst, 0);
            }
        } else
        if (0==strcmp(arg, "n-frame") || 0==strcmp(arg, "nframe") ||0==strcmp(arg, "n")) {
            int nframe = 0;
            i = arg_parse_int(i, argc, argv, &nframe);
            cfg->frame_range[1] = nframe + cfg->frame_range[0];
        } else
        if (0==strcmp(arg, "f-start")) {
            i = arg_parse_int(i, argc, argv, &cfg->frame_range[0]);
        } else
        if (0==strcmp(arg, "f-range") || 0==strcmp(arg, "f")) {
            i = arg_parse_range(i, argc, argv, cfg->frame_range);
        } else
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
            xerr("@cmdl>> invalid opt `%s`\n", arg);
            return -i;
        }
    }
    
    LEAVE_FUNC();

    return i;
}

int fmt_arg_check(fmt_opt_t *cfg, int argc, char *argv[])
{
    yuv_seq_t *psrc = &cfg->src;
    yuv_seq_t *pdst = &cfg->dst;
    
    ENTER_FUNC();
    
    if (!cfg->ios[CVT_IOS_DST].path || 
        !cfg->ios[CVT_IOS_SRC].path) {
        xerr("@cmdl>> no src or dst\n");
        return -1;
    }
    if (cfg->frame_range[0] >= cfg->frame_range[1]) {
        xerr("@cmdl>> Invalid frame_range %d~%d\n", 
                cfg->frame_range[0], cfg->frame_range[1]);
        return -1;
    }
    if (!psrc->wxh[0] || !psrc->wxh[1]) {
        xerr("@cmdl>> Invalid resolution for src\n");
        return -1;
    }
    psrc->width = psrc->wxh[0];
    psrc->height = psrc->wxh[1];
    if ((psrc->nbit != 8 && psrc->nbit!=10 && psrc->nbit!=16) ||
        (psrc->nbit < psrc->nlsb)) {
        xerr("@cmdl>> Invalid bitdepth (%d/%d) for src\n", 
                psrc->nlsb, psrc->nbit);
        return -1;
    }
    if ((pdst->nbit != 8 && pdst->nbit!=10 && pdst->nbit!=16) ||
        (pdst->nbit < pdst->nlsb)) {
        xerr("@cmdl>> Invalid bitdepth (%d/%d) for dst\n", 
                pdst->nlsb, pdst->nbit);
        return -1;
    }
    
    psrc->nlsb = psrc->nlsb ? psrc->nlsb : psrc->nbit;
    pdst->nlsb = pdst->nlsb ? pdst->nlsb : pdst->nbit;
    
    if (!ios_open(cfg->ios, CVT_IOS_CNT, 0)) {
        ios_close(cfg->ios, CVT_IOS_CNT);
        return -1;
    }
    
    pdst->width  = psrc->width;
    pdst->height = psrc->height;
    set_yuv_prop_by_copy(psrc, 0, psrc);
    set_yuv_prop_by_copy(pdst, 0, pdst);
    show_yuv_prop(psrc, SLOG_CFG, "@cfg>> src: ");
    show_yuv_prop(pdst, SLOG_CFG, "@cfg>> dst: ");
    
    LEAVE_FUNC();
    
    return 0;
}

int fmt_arg_close(fmt_opt_t *cfg)
{
    ios_close(cfg->ios, CVT_IOS_CNT);
}

int fmt_arg_help(fmt_opt_t *cfg, int argc, char *argv[])
{
    printf("yuv format convertor. Options:\n");
    printf("\t -dst name<%%s> {...props...}\n");
    printf("\t -src name<%%s> {...props...}\n");
    printf("\t [%% frame_range %%]\n");
    
    printf("\nset yuv props as follow:\n");
    yuv_prop_parser(0, 0, 0, &cfg->src, 1);
    
    printf("\nset frame_range as follow:\n");
    printf("\t [-f-range|-f <%%d~%%d>]\n");
    printf("\t [-f-start    <%%d>]\n");
    printf("\t [-n-frame|-n <%%d>]\n");
    return 0;
}

int yuv_fmt(int argc, char **argv)
{
    int         r, i;
    fmt_opt_t   cfg;
    yuv_seq_t   seq[2];

    memset(seq, 0, sizeof(seq));
    memset(&cfg, 0, sizeof(cfg));
    fmt_arg_init (&cfg, argc, argv);
    
    r = fmt_arg_parse(&cfg, argc, argv);
    if (r == 0) {
        //help exit
        return 0;
    } else if (r < 0) {
        xerr("fmt_arg_parse() failed\n");
        fmt_arg_help(&cfg, argc, argv);
        return 1;
    }
    r = fmt_arg_check(&cfg, argc, argv);
    if (r < 0) {
        xerr("fmt_arg_check() failed\n");
        fmt_arg_help(&cfg, argc, argv);
        return 1;
    }
    
    int w_align = bit_sat(6, cfg.src.width);
    int h_align = bit_sat(6, cfg.src.height);
    int nbyte   = 1 + (cfg.src.nbit > 8 || cfg.src.nbit > 8);
    for (i=0; i<2; ++i) {
        seq[i].buf_size = w_align * h_align * 3 * nbyte;
        seq[i].pbuf = (uint8_t *)malloc(seq[i].buf_size);
        if(!seq[i].pbuf) {
            xerr("error: malloc seq[%d] fail\n", i);
            return -1;
        }
    }

    /*************************************************************************
     *                          frame loop
     ************************************************************************/
    for (i=cfg.frame_range[0]; i<cfg.frame_range[1]; i++) 
    {
        xlog(SLOG_L1, "@frm> #%d +\n", i);
        r=fseek(cfg.ios[CVT_IOS_SRC].fp, cfg.src.io_size * i, SEEK_SET);
        if (r) {
            xerr("fseek %d error\n", cfg.src.io_size * i);
            return -1;
        }
        
        r = fread(seq[0].pbuf, cfg.src.io_size, 1, cfg.ios[CVT_IOS_SRC].fp);
        if (r<1) {
            if ( ios_feof(cfg.ios, CVT_IOS_SRC) ) {
                xlog(SLOG_INFO, "@seq> reach file end, force stop\n");
            } else {
                xerr("error reading file\n");
            }
            break;
        }

        set_yuv_prop_by_copy(&seq[0], 1, &cfg.src);
        set_yuv_prop_by_copy(&seq[1], 1, &cfg.dst);
        yuv_seq_t *pdst = yuv_cvt_frame(&seq[1], &seq[0]);
        
        r = fwrite(pdst->pbuf, pdst->io_size, 1, cfg.ios[CVT_IOS_DST].fp);
        if (r<1) {
            xerr("error writing file\n");
            break;
        }
        xlog(SLOG_L1, "@frm> #%d -\n", i);
    } // end frame loop
    
    fmt_arg_close(&cfg);
    for (i=0; i<2; ++i) {
        yuv_buf_free(&seq[i]);
    }

    return 0;
}
