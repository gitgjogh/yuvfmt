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

#include "yuvdef.h"
#include "yuvcvt.h"
#include "yuvfmt.h"
#include "sim_opt.h"

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

int yuv_fmt(int argc, char **argv)
{
    int r = 0;
    cvt_opt_t _cfg, *cfg=&_cfg;
    yuv_seq_t *yuv = &cfg->src;
    memset(cfg, 0, sizeof(cvt_opt_t));
    
    opt_desc_t fmt_opt[] = 
    {
        { "+src",    OPT_T_STR,  1, &cfg->ios[0].path, 0, "src yuv file"},
        { "+dst",    OPT_T_STR,  1, &cfg->ios[1].path, 0, "dst yuv file"},
        { "*fmt",    OPT_T_INTI, 1, &yuv->yuvfmt, "&420p", "yuvfmt",  0, n_cmn_fmt, cmn_fmt},
        { "*wxh",    OPT_T_INTI, 2, yuv->wxh,     "&qcif", "w & h", n_cmn_wxh, 0, cmn_wxh},
        { "*nbit",   OPT_T_INTI, 1, &yuv->nbit,   "8",  ""},
        { "*nlsb",   OPT_T_INTI, 1, &yuv->nlsb,   "8",  ""},
        { "*btile",  OPT_T_BOOL, 0, &yuv->btile,  "1",  ""},
        { "*stride", OPT_T_INTI, 1, &yuv->y_stride,   "0", ""},
        { "*iosize", OPT_T_INTI, 1, &yuv->io_size,    "0", ""},
        { "*frange", OPT_T_INTI, 2, cfg->frame_range, "0", ""},
    };
    const int n_fmt_opt = ARRAY_SIZE(fmt_opt);
    
    ENTER_FUNC;
    
    r = cmdl_help(n_fmt_opt, fmt_opt);
    
    cmdl_init(n_fmt_opt, fmt_opt);
    r = cmdl_parse(1, argc, argv, n_fmt_opt, fmt_opt);
    if (r < 0) {
        xerr("fmt_arg_parse() failed (%d)\n\n", r);
        cmdl_help(n_fmt_opt, fmt_opt);
        return 1;
    }
    r = cmdl_result(n_fmt_opt, fmt_opt);
    
    r = cmdl_check(n_fmt_opt, fmt_opt);
    if (r < 0) {
        xerr("fmt_arg_check() failed (%d)\n\n", r);
        cmdl_help(n_fmt_opt, fmt_opt);
        return 1;
    }
    r = cmdl_result(n_fmt_opt, fmt_opt);

    return 0;
}