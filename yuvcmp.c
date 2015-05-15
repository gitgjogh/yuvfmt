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
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>

#include "yuvdef.h"
#include "yuvcvt.h"
#include "yuvcmp.h"


double get_stat_psnr(dstat_t *s)
{
    if (s->ssd) {
        return 10.0 * log10( (double)65025 * s->cnt / s->ssd );
    } else {
        return 0;
    }
}


dstat_t b8_rect_diff(int w, int h, uint8_t *base[3], 
                     int stride[3], dstat_t *stat)
{
    int i, j, k, d;
    dstat_t st = {w*h, 0, 0};
    for (j=0; j<h; ++j) {
        uint8_t *base0 = base[0] + j * stride[0];
        uint8_t *base1 = base[1] + j * stride[1];
        uint8_t *base2 = base[2] + j * stride[2];
        for (i=0; i<w; ++i) {
            d = (int)(*base0++) - (int)(*base1++);
            d = d>0 ? d : -d;
            (*base2++) = (uint8_t)d;
            st.sad += d;
            st.ssd += d*d;
        }
    }
    
    if (stat) {
        stat->cnt += st.cnt;
        stat->sad += st.sad;
        stat->ssd += st.ssd;
    }
    return st;
}

dstat_t b16_rect_diff(int w, int h, uint8_t *base[3], 
                      int stride[3], dstat_t *stat)
{
    int i, j, k, d;
    dstat_t st = {w*h, 0, 0};
    for (j=0; j<h; ++j) {
        uint16_t *base0 = (uint16_t *)(base[0] + j * stride[0]);
        uint16_t *base1 = (uint16_t *)(base[1] + j * stride[1]);
        uint16_t *base2 = (uint16_t *)(base[2] + j * stride[2]);
        for (i=0; i<w; ++i) {
            d = (int)(*base0++) - (int)(*base1++);
            d = d>0 ? d : -d;
            (*base2++) = (uint16_t)d;
            st.sad += d;
            st.ssd += d*d;
        }
    }
    
    if (stat) {
        stat->cnt += st.cnt;
        stat->sad += st.sad;
        stat->ssd += st.ssd;
    }
    
    return st;
}

dstat_t yuv_diff(yuv_seq_t *seq1, yuv_seq_t *seq2, 
                 yuv_seq_t *diff, dstat_t *stat)
{
    yuv_seq_t*  seq[3] = {seq1, seq2, diff};
    uint8_t*    base[3] = {0};
    int         stride[3] = {0};
    
    int fmt = seq1->yuvfmt;
    int w   = seq1->width; 
    int h   = seq1->height;
    int i;
    dstat_t (*rect_diff)(int w, int h, uint8_t *base[3], 
                         int stride[3], dstat_t *stat);
    dstat_t st = {0};
    
    ENTER_FUNC();
    
    rect_diff = (seq1->nbit == 8) ? b8_rect_diff : b16_rect_diff;
    
    for (i=0; i<3; ++i) {
        base[i]   = seq[i]->pbuf;
        stride[i] = seq[i]->y_stride;
    }

    if      (fmt == YUVFMT_400P)
    {
        rect_diff(w, h, base, stride, &st);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        rect_diff(w, h, base, stride, &st);
        
        for (i=0; i<3; ++i) {
            base[i]  += seq[i]->y_size;
            stride[i] = seq[i]->uv_stride;
        }
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        rect_diff(w, h, base, stride, &st);
        
        for (i=0; i<3; ++i) {
            base[i]  += seq[i]->uv_size;
        }
        
        rect_diff(w, h, base, stride, &st);
    }
    else if (is_semi_planar(fmt))
    {
        rect_diff(w, h, base, stride, &st);
        
        for (i=0; i<3; ++i) {
            base[i]  += seq[i]->y_size;
            stride[i] = seq[i]->uv_stride;
        }        
        h   = is_mch_422(fmt) ? h : h/2;
        
        rect_diff(w, h, base, stride, &st);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        rect_diff(w, h, base, stride, &st);
    }
    
    LEAVE_FUNC();
    
    if (stat) {
        stat->cnt += st.cnt;
        stat->sad += st.sad;
        stat->ssd += st.ssd;
    }
    
    return st;
}

int cmp_arg_init (cmp_opt_t *cfg, int argc, char *argv[])
{
    set_yuv_prop(&cfg->seq[0], 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    set_yuv_prop(&cfg->seq[1], 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    set_yuv_prop(&cfg->seq[2], 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    cfg->frame_range[1] = INT_MAX;
}

int cmp_arg_parse(cmp_opt_t *cfg, int argc, char *argv[])
{
    int i, j;
    yuv_seq_t *yuv = &cfg->seq[0];
    yuv_seq_t *seq = &cfg->seq[0];
    
    ENTER_FUNC();
    
    if (argc<2) {
        return -1;
    }
    if (0 != strcmp(argv[1], "-i0") && 0 != strcmp(argv[1], "-i1") &&
        0 != strcmp(argv[1], "-h")  && 0 != strcmp(argv[1], "-help") &&
        0 != strcmp(argv[1], "-x")  && 0 != strcmp(argv[1], "-xall"))
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
            xerr("@cmdl>> unrecognized arg `%s`\n", arg);
            return -i;
        }
        arg += 1;
        
        for (j=0; j<n_cmn_res; ++j) {
            if (0==strcmp(arg, cmn_res[j].name)) {
                yuv->width  = cmn_res[j].w;
                yuv->height = cmn_res[j].h;
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
            cmp_arg_help();
            return 0;
        } else
        if (0==strcmp(arg, "i0")) {
            char *path;
            seq = &cfg->seq[0];
            i = arg_parse_str(i, argc, argv, &path);
            ios_cfg(cfg->ios, 0, path, "rb");
        } else
        if (0==strcmp(arg, "i1")) {
            char *path;
            seq = &cfg->seq[1];
            i = arg_parse_str(i, argc, argv, &path);
            ios_cfg(cfg->ios, 1, path, "rb");
        } else
        if (0==strcmp(arg, "o") || 0==strcmp(arg, "diff")) {
            char *path;
            seq = &cfg->seq[2];
            i = arg_parse_str(i, argc, argv, &path);
            ios_cfg(cfg->ios, 2, path, "wb");
        } else
        if (0==strcmp(arg, "wxh")) {
            i = arg_parse_wxh(i, argc, argv, &yuv->width, &yuv->height);
        } else
        if (0==strcmp(arg, "fmt")) {
            i = arg_parse_fmt(i, argc, argv, &seq->yuvfmt);
        } else
        if (0==strcmp(arg, "b10")) {
            ++i;    seq->nbit = 10;     seq->nlsb = 10;
        } else
        if (0==strcmp(arg, "nbit") || 0==strcmp(arg, "b")) {
            i = arg_parse_int(i, argc, argv, &seq->nbit);
        } else
        if (0==strcmp(arg, "nlsb")) {
            i = arg_parse_int(i, argc, argv, &seq->nlsb);
        } else
        if (0==strcmp(arg, "btile") || 0==strcmp(arg, "tile") || 0==strcmp(arg, "t")) {
            ++i;    seq->btile = 1;     seq->yuvfmt = YUVFMT_420SP;
        } else
        if (0==strcmp(arg, "stride")) {
            i = arg_parse_int(i, argc, argv, &seq->y_stride);
        } else
        if (0==strcmp(arg, "iosize")) {
            i = arg_parse_int(i, argc, argv, &seq->io_size);
        } else
        if (0==strcmp(arg, "n-frame") || 0==strcmp(arg, "n")) {
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
        if (0==strcmp(arg, "blksz")) {
            i = arg_parse_range(i, argc, argv, &cfg->blksz);
        } else
        if (0==strcmp(arg, "xnon")) {
            ++i;    xlevel(SLOG_NON);
        } else
        if (0==strcmp(arg, "xall")) {
            ++i;    xlevel(SLOG_ALL);
        } else
        if (0==strcmp(arg, "x") || 0==strcmp(arg, "xlevel")) {
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

int cmp_arg_check(cmp_opt_t *cfg, int argc, char *argv[])
{
    int i = 0;
    yuv_seq_t* yuv = &cfg->seq[0];
    
    ENTER_FUNC();
    
    for (i=0; i<2; ++i) 
    {
        yuv_seq_t* psrc = &cfg->seq[i];
        if (!cfg->ios[i].path) {
            xerr("@cmdl>> no input %d\n", i);
            return -1;
        }
        
        if ((psrc->nbit!= 8 && psrc->nbit!=10 && psrc->nbit!=16) || 
            (psrc->nbit < psrc->nlsb)) {
            xerr("@cmdl>> invalid bitdepth (%d/%d) for input %d\n", 
                    psrc->nlsb, psrc->nbit, i);
            return -1;
        }
        psrc->nlsb = psrc->nlsb ? psrc->nlsb : psrc->nbit;
    }
    
    if (!yuv->width || !yuv->height) {
        xerr("@cmdl>> invalid resolution(%dx%d)\n",
                yuv->width, yuv->height);
        return -1;
    }
        
    if (cfg->frame_range[0] >= cfg->frame_range[1]) {
        xerr("@cmdl>> Invalid frame_range %d~%d\n", 
                cfg->frame_range[0], cfg->frame_range[1]);
        return -1;
    }
    
    if (!ios_open(cfg->ios, CMP_IOS_CNT, 0)) {
        ios_close(cfg->ios, CMP_IOS_CNT);
        return -1;
    }
    
    cfg->seq[2].width  = cfg->seq[1].width  = cfg->seq[0].width;
    cfg->seq[2].height = cfg->seq[1].height = cfg->seq[0].height;
    for (i=0; i<3; ++i) {
        set_yuv_prop_by_copy(&cfg->seq[i], 0, &cfg->seq[i]);
        xlog(SLOG_CFG, "@cfg> yuv#%d: ", i);  
        show_yuv_prop(&cfg->seq[i], SLOG_CFG, 0);
    }
    
    LEAVE_FUNC();
    
    return 0;
}

int cmp_arg_close(cmp_opt_t *cfg)
{
    ios_close(cfg->ios, CMP_IOS_CNT);
}

int cmp_arg_help()
{
    printf("yuv sequences comparation. Options\n");
    printf("-wxh <%%dx%%d>\n");
    printf("-i0 name<%%s> {...yuv props...} \n");
    printf("-i1 name<%%s> {...yuv props...} \n");
    printf("-o  name<%%s> {...yuv props...} \n");
    printf("\n...yuv props...\n");
    printf("\t [-fmt <420p,420sp,uyvy,422p>]\n");
    printf("\t [-stride <%%d>]\n");
    printf("\t [-fsize <%%d>]\n");
    printf("\t [-b10]\n");
    printf("\t [-btile]\n");
    return 0;
}

int yuv_cmp(int argc, char **argv)
{
    int         r, i, j;
    cmp_opt_t   cfg;
    yuv_seq_t   seq[4];
    yuv_seq_t*  spl[2];
    yuv_seq_t*  ptmp;
    dstat_t     stat[2] = {{0}, {0}};
    double      psnr = 0;
    
    memset(seq, 0, sizeof(seq));
    memset(&cfg, 0, sizeof(cfg));
    cmp_arg_init (&cfg, argc, argv);
    
    r = cmp_arg_parse(&cfg, argc, argv);
    if (r == 0) {
        //help exit
        return 0;
    } else if (r < 0) {
        xerr("cmp_arg_parse() failed\n");
        cmp_arg_help();
        return 1;
    }
    r = cmp_arg_check(&cfg, argc, argv);
    if (r < 0) {
        xerr("cmp_arg_check() failed\n");
        return 1;
    }

    set_yuv_prop(&seq[3], 0, cfg.seq[0].width, cfg.seq[0].height, 
            get_spl_fmt(cfg.seq[0].yuvfmt), 
            cfg.seq[0].nbit>8 ? BIT_16 : BIT_8, 
            cfg.seq[0].nbit>8 ? BIT_16 : BIT_8, 
            TILE_0, 0, 0);
    show_yuv_prop(&seq[3], SLOG_DBG, "@cfg>> mid type: ");

    /*************************************************************************
     *                          frame loop
     ************************************************************************/
    for (j=cfg.frame_range[0]; j<cfg.frame_range[1]; j++) 
    {
        xlog(SLOG_DBG, "@frm> **** %d ****\n", j);

        for (i=0; i<2; ++i) 
        {
            set_yuv_prop_by_copy(&seq[i], 1, &cfg.seq[i]);
            r = fseek(cfg.ios[i].fp, seq[i].io_size * j, SEEK_SET);
            if (r) {
                xerr("%d: fseek %d error\n", i, seq[i].io_size * j);
                return -1;
            }
            r = fread(seq[i].pbuf, seq[i].io_size, 1, cfg.ios[i].fp);
            if (r<1) {
                if ( ios_feof(cfg.ios, i) ) {
                    xlog(SLOG_L1, "@seq>> $%d: reach file end, force stop\n", i);
                } else {
                    xerr("@seq>> $%d: error reading file\n", i);
                }
                break;
            }
            
            // get one buffer unused
            ptmp = (i==0) ? &seq[2] : 
                            (spl[0] == &seq[0] ? &seq[2] : &seq[0]);
            set_yuv_prop_by_copy(ptmp, 1, &seq[3]);
            spl[i] = yuv_cvt_frame(ptmp, &seq[i]);
        }
        if ( ios_feof(cfg.ios, 0) || ios_feof(cfg.ios, 1) ) {
            break;
        }
        
        for (i=0; i<3; ++i) {
            if (&seq[i] != spl[0] && &seq[i] != spl[1]) {
                ptmp = &seq[i];
                break;
            }
        }
        set_yuv_prop_by_copy(ptmp, 1, &seq[3]);
        stat[0] = yuv_diff(spl[0], spl[1], ptmp, &stat[1]);
        
        psnr = get_stat_psnr(&stat[0]);
        xlog(SLOG_L1, "@frm>> #%d: PSNR = %.2llf\n", j, psnr);
        
        if (cfg.ios[2].fp) {
            set_yuv_prop_by_copy(spl[0], 1, &cfg.seq[2]);
            yuv_seq_t* diff = yuv_cvt_frame(spl[0], ptmp);
            r = fwrite(diff->pbuf, diff->io_size, 1, cfg.ios[2].fp);
            if (r<1) {
                xerr("error writing file\n");
                break;
            }
        }
    }
    
    psnr = get_stat_psnr(&stat[1]);
    xlog(SLOG_L0, "@seq>> PSNR = %.2llf\n", psnr);
    
    cmp_arg_close(&cfg);
    for (i=0; i<3; ++i) {
        yuv_buf_free(&seq[i]);
    }
    
    return !!stat[1].ssd;
}