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


int yuv_prop_parser(cmdl_iter_t *iter, void* dst, cmdl_act_t act, cmdl_opt_t *opt)
{
    ENTER_FUNC();
    
    int ret = 0;
    
    #define YUV_POP_M(member) offsetof(yuv_arg_t, member)
    cmdl_opt_t yuv_opt[] = 
    {
        { 0, "fmt",    1, cmdl_parse_int, YUV_POP_M(seq.yuvfmt), "%420p",  "yuvfmt",  },
        { 1, "wxh",    2, cmdl_parse_ints,YUV_POP_M(seq.wxh),       0,     "w & h",   },
        { 0, "nbit",   1, cmdl_parse_int, YUV_POP_M(seq.nbit),     "8",    "",        },
        { 0, "nlsb",   1, cmdl_parse_int, YUV_POP_M(seq.nlsb),     "0",    ""},
        { 0, "tile",   1, cmdl_parse_int, YUV_POP_M(seq.btile),     0,     ""},
        { 0, "stride", 1, cmdl_parse_int, YUV_POP_M(seq.y_stride),  0,     ""},
        { 0, "iosize", 1, cmdl_parse_int, YUV_POP_M(seq.io_size),   0,     ""},
    };
    const int n_yuv_opt = ARRAY_SIZE(yuv_opt);
    cmdl_set_enum(n_yuv_opt, yuv_opt, "fmt",  n_cmn_fmt, cmn_fmt); 
    cmdl_set_ref (n_yuv_opt, yuv_opt, "wxh",  n_cmn_wxh, cmn_wxh); 
    cmdl_set_enum(n_yuv_opt, yuv_opt, "nbit", n_cmn_bit, cmn_bit); 
    
    if (act == CMDL_ACT_PARSE) 
    {
        char *arg = cmdl_iter_next(iter);
        if (arg && arg[0]!='-') {
            ((yuv_arg_t*)dst)->path = arg;
            return cmdl_parse(iter, dst, n_yuv_opt, yuv_opt);
        } else {
            xerr("no path specified\n");
            return CMDL_RET_ERROR;
        }
    }
    else if (act == CMDL_ACT_HELP) 
    {
        xlprint(iter->layer+1, "usage: -%s path ...prop...\n", opt->names);
        cmdl_help(iter, 0, n_yuv_opt, yuv_opt);
    } 
    else if (act == CMDL_ACT_RESULT)
    {
        xprint("'%s'\n", SAFE_STR(((yuv_arg_t*)dst)->path, ""));
        cmdl_result(iter, dst, n_yuv_opt, yuv_opt);
    }

    return 0;
}

#define FMT_OPT_M(member) offsetof(fmt_opt_t, member)
cmdl_opt_t fmt_opt[] = 
{
    { 0, "h,help",              0, cmdl_parse_help,         0,      0,  },
    { 0, "xl,xlevel,xall,xnon", 0, cmdl_parse_xlevel,       0,      0,  "config log level"},
    { 1, "src",     0, yuv_prop_parser,   FMT_OPT_M(src),           0,  "src path ...prop ..."},
    { 1, "dst",     0, yuv_prop_parser,   FMT_OPT_M(dst),           0,  "dst path ...prop ..."},
    { 0, "f-range", 1, cmdl_parse_range,  FMT_OPT_M(frame_range),   0,  "frame range"},
};
const int n_fmt_opt = ARRAY_SIZE(fmt_opt);

int fmt_arg_check(fmt_opt_t *cfg, int argc, char *argv[])
{
    yuv_seq_t *psrc = &cfg->src.seq;
    yuv_seq_t *pdst = &cfg->dst.seq;
    
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
    if (!psrc->width || !psrc->height) {
        xerr("@cmdl>> Invalid resolution for src\n");
        return -1;
    }
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
    show_yuv_prop(psrc, SLOG_CMDL, "@cfg>> src: ");
    show_yuv_prop(pdst, SLOG_CMDL, "@cfg>> dst: ");
    
    LEAVE_FUNC();
    
    return 0;
}

int fmt_arg_close(fmt_opt_t *cfg)
{
    ios_close(cfg->ios, CVT_IOS_CNT);
}

int fmt_arg_help(fmt_opt_t *cfg, int argc, char *argv[])
{
    cmdl_iter_t iter = {};
    return cmdl_help(&iter, 0, n_fmt_opt, fmt_opt);
}

int yuv_fmt(int argc, char **argv)
{
    int         r, i;
    fmt_opt_t   cfg;
    yuv_seq_t   seq[2];
    yuv_seq_t *psrc = &cfg.src.seq;
    yuv_seq_t *pdst = &cfg.dst.seq;

    memset(seq, 0, sizeof(seq));
    memset(&cfg, 0, sizeof(cfg));
    set_yuv_prop(psrc, 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    set_yuv_prop(pdst, 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    cfg.frame_range[1] = INT_MAX;
    
    cmdl_iter_t iter = cmdl_iter_init(argc, argv, 0);
    r = cmdl_parse(&iter, &cfg, n_fmt_opt, fmt_opt);
    if (r == CMDL_RET_HELP) {
        return cmdl_help(&iter, 0, n_fmt_opt, fmt_opt);
    } else if (r < 0) {
        xerr("cmdl_parse() failed, ret=%d\n", r);
        return 1;
    }

    //cmdl_result(&iter, &cfg, n_fmt_opt, fmt_opt);
    ios_cfg(cfg.ios, CVT_IOS_SRC, cfg.src.path, "rb");
    ios_cfg(cfg.ios, CVT_IOS_DST, cfg.src.path, "rb");
    
    r = fmt_arg_check(&cfg, argc, argv);
    if (r < 0) {
        xerr("fmt_arg_check() failed\n");
        fmt_arg_help(&cfg, argc, argv);
        return 1;
    }
    
    int w_align = bit_sat(6, psrc->width);
    int h_align = bit_sat(6, psrc->height);
    int nbyte   = 1 + (psrc->nbit > 8 || psrc->nbit > 8);
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
        xprint("@frm>> #%d -\n", i);
        r=fseek(cfg.ios[CVT_IOS_SRC].fp, psrc->io_size * i, SEEK_SET);
        if (r) {
            xerr("fseek %d error\n", psrc->io_size * i);
            return -1;
        }
        
        r = fread(seq[0].pbuf, psrc->io_size, 1, cfg.ios[CVT_IOS_SRC].fp);
        if (r<1) {
            if ( ios_feof(cfg.ios, CVT_IOS_SRC) ) {
                xinfo("@seq>> reach file end, force stop\n");
            } else {
                xerr("error reading file\n");
            }
            break;
        }

        set_yuv_prop_by_copy(&seq[0], 1, psrc);
        set_yuv_prop_by_copy(&seq[1], 1, pdst);
        yuv_seq_t *pout = yuv_cvt_frame(&seq[1], &seq[0]);
        
        r = fwrite(pout->pbuf, pout->io_size, 1, cfg.ios[CVT_IOS_DST].fp);
        if (r<1) {
            xerr("error writing file\n");
            break;
        }
         xprint("@frm>> #%d -\n", i);
    } // end frame loop
    
    fmt_arg_close(&cfg);
    for (i=0; i<2; ++i) {
        yuv_buf_free(&seq[i]);
    }

    return 0;
}
