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
#include <malloc.h>
#include <stdio.h>

#include "yuvdef.h"
#include "yuvcvt.h"


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

const opt_enum_t cmn_fmt[] = {
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

const char* show_fmt(int ifmt)
{
    return enum_val_2_name(n_cmn_fmt, cmn_fmt, ifmt);
}

int arg_parse_wxh(int i, int argc, char *argv[], int *pw, int *ph)
{
    int j, w, h;
    char *flag=0;
    char *last=0;
    pw = pw ? pw : &w;
    ph = ph ? ph : &h;
    
    char *arg = GET_ARGV(i, "wxh");
    if (!arg) {
        return -1;
    }
   
    if (arg[0] == '%') { 
        for (j=0; j<n_cmn_res; ++j) {
            if (0==strcmp(&arg[1], cmn_res[j].name)) {
                *pw = cmn_res[j].w;  
                *ph = cmn_res[j].h;
                return ++i;
            }
        }
        if (j>=n_cmn_res) {
            xerr("unknown enum or ref %s used in %s\n", arg, __FUNCTION__);
            return -1;
        }
    }
    
    //seq->width = strtoul (arg, &flag, 10);
    flag = get_uint32 (arg, pw);
    if (flag==0 || *flag != 'x') {
        xerr("@cmdl>> not (%%d)x(%%d)\n");
        return -1;
    }
    
    //seq->height = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, ph);
    if (last == 0 || *last != 0 ) {
        xerr("@cmdl>> not (%%d)x(%%d)\n");
        return -1;
    }

    return ++i;
}

int arg_parse_fmt(int i, int argc, char *argv[], int *fmt)
{
    int j;
    
    char *arg = GET_ARGV(i, "yuvfmt");
    if (!arg) {
        return -1;
    }
    
    if (arg[0] == '%') { 
        for (j=0; j<n_cmn_fmt; ++j) {
            if (0==strcmp(arg, cmn_fmt[j].name)) {
                *fmt = cmn_fmt[j].val;
                return ++i;
            }
        }
        if (j>=n_cmn_fmt) {
            xerr("unknown enum or ref %s used in %s\n", arg, __FUNCTION__);
            return -1;
        }
    }
    
    xerr("@cmdl>> unrecognized yuvfmt `%s`\n", arg);
    return -1;
}

/** 
 *  422p <-> 420p uv down/up sampling
 */
int b8_mch_p2p(yuv_seq_t *pdst, yuv_seq_t *psrc)
{
    int src_fmt = psrc->yuvfmt;
    int dst_fmt = pdst->yuvfmt;

    uint8_t* src_y_base = psrc->pbuf;                   uint8_t* src_y = src_y_base;
    uint8_t* src_u_base = src_y_base + psrc->y_size;    uint8_t* src_u = src_u_base;
    uint8_t* src_v_base = src_u_base + psrc->uv_size;   uint8_t* src_v = src_v_base;
    
    uint8_t* dst_y_base = pdst->pbuf;                   uint8_t* dst_y = dst_y_base;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;    uint8_t* dst_u = dst_u_base;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;   uint8_t* dst_v = dst_v_base;

    int src_uv_shift = is_mch_420(src_fmt) ? 0 : psrc->uv_stride;
    int dst_uv_shift = is_mch_420(dst_fmt) ? 0 : pdst->uv_stride;
    
    int linesize = sat_div(psrc->width * psrc->nbit, 8); 
    int h   = psrc->height; 
    int y;

    ENTER_FUNC();
    show_yuv_prop(psrc, SLOG_DBG, "src ");
    show_yuv_prop(pdst, SLOG_DBG, "dst ");
    
    assert((psrc->nbit==8 && pdst->nbit==8) || (psrc->nbit==16 && pdst->nbit==16));
    assert(is_mch_planar(src_fmt));
    assert(is_mch_planar(dst_fmt));
    
    for (y=0; y<h; ++y) {
        memcpy(dst_y, src_y, linesize);
        dst_y += pdst->y_stride;
        src_y += psrc->y_stride;
    }
    
    if (src_fmt==YUVFMT_400P || dst_fmt==YUVFMT_400P)
        return 0;
    
    linesize /= 2;
    h   = h/2;
    
    for (y=0; y<h; ++y) 
    {
        memcpy(dst_u, src_u, linesize);
        memcpy(dst_v, src_v, linesize);
        
        dst_u += pdst->uv_stride + dst_uv_shift;
        src_u += psrc->uv_stride + src_uv_shift;
        dst_v += pdst->uv_stride + dst_uv_shift;
        src_v += psrc->uv_stride + src_uv_shift;
        
        if (dst_uv_shift>0) 
        {
            memcpy(dst_u + dst_uv_shift, src_u + src_uv_shift, linesize);
            memcpy(dst_v + dst_uv_shift, src_v + src_uv_shift, linesize);
        }
    }
    
    LEAVE_FUNC();
    
    return 0;
}

/**
 *  @itl : uv is interlaced (here 420sp)
 *  @spl : uv is splitted
 */
int b8_mch_sp2p(yuv_seq_t *itl, yuv_seq_t *spl, int b_interlacing)
{
    int fmt = itl->yuvfmt;
    
    uint8_t* itl_y_base = itl->pbuf;
    uint8_t* itl_u_base = itl_y_base + itl->y_size;
    
    uint8_t* spl_y_base = spl->pbuf;
    uint8_t* spl_u_base = spl_y_base + spl->y_size;
    uint8_t* spl_v_base = spl_u_base + spl->uv_size;
    
    int w   = itl->width; 
    int h   = itl->height; 
    int x, y;

    ENTER_FUNC();
    
    assert(itl->nbit==8 && spl->nbit==8);
    assert(is_semi_planar(itl->yuvfmt));
    assert(is_mch_planar(spl->yuvfmt));
    
    for (y=0; y<h; ++y) {
        uint8_t* itl_y = itl_y_base + y * itl->y_stride;
        uint8_t* spl_y = spl_y_base + y * spl->y_stride;
        memcpy(spl_y, itl_y, w);
    }

    w   = w/2;
    h   = is_mch_422(fmt) ? h : h/2;
    
    for (y=0; y<h; ++y) {
        uint8_t* itl_u = itl_u_base + y * itl->uv_stride;
        uint8_t* spl_u = spl_u_base + y * spl->uv_stride;
        uint8_t* spl_v = spl_v_base + y * spl->uv_stride;

        if (b_interlacing == INTERLACING) {
            for (x=0; x<w; ++x) {
                *(itl_u++) = *(spl_u++);
                *(itl_u++) = *(spl_v++);
            }
        } else {
            for (x=0; x<w; ++x) {
                *(spl_u++) = *(itl_u++);
                *(spl_v++) = *(itl_u++);
            }
        }
    }
    
    LEAVE_FUNC();
    
    return 0;
}

/**
 *  @itl : luma & chroma is interlaced (here uyvy or yuyv)
 *  @spl : luma & chroma is splitted
 */
int b8_mch_yuyv2p(yuv_seq_t *itl, yuv_seq_t *spl, int b_interlacing)
{
    uint8_t* itl_y_base = itl->pbuf;
    
    uint8_t* spl_y_base = spl->pbuf;
    uint8_t* spl_u_base = spl_y_base + spl->y_size;
    uint8_t* spl_v_base = spl_u_base + spl->uv_size;

    int w   = itl->width; 
    int h   = itl->height; 
    int x, y;

    ENTER_FUNC();
    show_yuv_prop(itl, SLOG_DBG, "itl ");
    show_yuv_prop(spl, SLOG_DBG, "spl ");
    
    assert(itl->nbit==8 && spl->nbit==8);
    assert(is_mch_mixed(itl->yuvfmt));
    assert(is_mch_planar(spl->yuvfmt));

    w   = w/2;

    for (y=0; y<h; ++y) 
    {
        uint8_t* itl_y  = itl_y_base + y * itl->y_stride;
        uint8_t* spl_y  = spl_y_base + y * spl->y_stride;
        uint8_t* spl_u  = spl_u_base + y * spl->uv_stride;
        uint8_t* spl_v  = spl_v_base + y * spl->uv_stride;

        if (b_interlacing == INTERLACING) {
            if (itl->yuvfmt == YUVFMT_YUYV) {
                for (x=0; x<w; ++x) {
                    *(itl_y++) = *(spl_y++);
                    *(itl_y++) = *(spl_u++);
                    *(itl_y++) = *(spl_y++);
                    *(itl_y++) = *(spl_v++);
                }
            } else {
                for (x=0; x<w; ++x) {
                    *(itl_y++) = *(spl_u++);
                    *(itl_y++) = *(spl_y++);
                    *(itl_y++) = *(spl_v++);
                    *(itl_y++) = *(spl_y++);
                }
            }
        } else {
            if (itl->yuvfmt == YUVFMT_YUYV) {
                for (x=0; x<w; ++x) {
                    *(spl_y++) = *(itl_y++);
                    *(spl_u++) = *(itl_y++);
                    *(spl_y++) = *(itl_y++);
                    *(spl_v++) = *(itl_y++);
                }
            } else {
                for (x=0; x<w; ++x) {
                    *(spl_u++) = *(itl_y++);
                    *(spl_y++) = *(itl_y++);
                    *(spl_v++) = *(itl_y++);
                    *(spl_y++) = *(itl_y++);
                }
            }
        }
    }   /* end for y*/

    LEAVE_FUNC();
    
    return 0;
}

int b16_mch_p2p(yuv_seq_t *pdst, yuv_seq_t *psrc)
{
    return b8_mch_p2p(pdst, psrc);
}

/**
 *  @itl : uv is interlaced (here 420sp)
 *  @spl : uv is splitted
 */
int b16_mch_sp2p(yuv_seq_t *itl, yuv_seq_t *spl, int b_interlacing)
{
    int fmt = itl->yuvfmt;
    
    uint8_t* itl_y_base = itl->pbuf;
    uint8_t* itl_u_base = itl_y_base + itl->y_size;
    
    uint8_t* spl_y_base = spl->pbuf;
    uint8_t* spl_u_base = spl_y_base + spl->y_size;
    uint8_t* spl_v_base = spl_u_base + spl->uv_size;
    
    int w   = itl->width; 
    int h   = itl->height; 
    int x, y;

    ENTER_FUNC();
    
    assert(itl->nbit==16 && spl->nbit==16);
    assert(is_semi_planar(itl->yuvfmt));
    assert(is_mch_planar(spl->yuvfmt));
    
    for (y=0; y<h; ++y) {
        uint16_t* itl_y = (uint16_t*)(itl_y_base + y * itl->y_stride);
        uint16_t* spl_y = (uint16_t*)(spl_y_base + y * spl->y_stride);
        memcpy(spl_y, itl_y, w*sizeof(uint16_t));
    }

    w   = w/2;
    h   = is_mch_422(fmt) ? h : h/2;
    
    for (y=0; y<h; ++y) {
        uint16_t* itl_u = (uint16_t*)(itl_u_base + y * itl->uv_stride);
        uint16_t* spl_u = (uint16_t*)(spl_u_base + y * spl->uv_stride);
        uint16_t* spl_v = (uint16_t*)(spl_v_base + y * spl->uv_stride);

        if (b_interlacing == INTERLACING) {
            for (x=0; x<w; ++x) {
                *(itl_u++) = *(spl_u++);
                *(itl_u++) = *(spl_v++);
            }
        } else {
            for (x=0; x<w; ++x) {
                *(spl_u++) = *(itl_u++);
                *(spl_v++) = *(itl_u++);
            }
        }
    }
    
    LEAVE_FUNC();
    
    return 0;
}

/**
 *  @itl : luma & chroma is interlaced (here uyvy or yuyv)
 *  @spl : luma & chroma is splitted
 */
int b16_mch_yuyv2p(yuv_seq_t *itl, yuv_seq_t *spl, int b_interlacing)
{
    uint8_t* itl_y_base = itl->pbuf;
    
    uint8_t* spl_y_base = spl->pbuf;
    uint8_t* spl_u_base = spl_y_base + spl->y_size;
    uint8_t* spl_v_base = spl_u_base + spl->uv_size;

    int w   = itl->width; 
    int h   = itl->height; 
    int x, y;

    ENTER_FUNC();
    
    assert(itl->nbit==16 && spl->nbit==16);
    assert(is_mch_mixed(itl->yuvfmt));
    assert(is_mch_planar(spl->yuvfmt));

    w   = w/2;

    for (y=0; y<h; ++y) 
    {
        uint16_t* itl_y = (uint16_t*)(itl_y_base + y * itl->y_stride );
        uint16_t* spl_y = (uint16_t*)(spl_y_base + y * spl->y_stride );
        uint16_t* spl_u = (uint16_t*)(spl_u_base + y * spl->uv_stride);
        uint16_t* spl_v = (uint16_t*)(spl_v_base + y * spl->uv_stride);

        if (b_interlacing == INTERLACING) {
            if (itl->yuvfmt == YUVFMT_YUYV) {
                for (x=0; x<w; ++x) {
                    *(itl_y++) = *(spl_y++);
                    *(itl_y++) = *(spl_u++);
                    *(itl_y++) = *(spl_y++);
                    *(itl_y++) = *(spl_v++);
                }
            } else {
                for (x=0; x<w; ++x) {
                    *(itl_y++) = *(spl_u++);
                    *(itl_y++) = *(spl_y++);
                    *(itl_y++) = *(spl_v++);
                    *(itl_y++) = *(spl_y++);
                }
            }
        } else {
            if (itl->yuvfmt == YUVFMT_YUYV) {
                for (x=0; x<w; ++x) {
                    *(spl_y++) = *(itl_y++);
                    *(spl_u++) = *(itl_y++);
                    *(spl_y++) = *(itl_y++);
                    *(spl_v++) = *(itl_y++);
                }
            } else {
                for (x=0; x<w; ++x) {
                    *(spl_u++) = *(itl_y++);
                    *(spl_y++) = *(itl_y++);
                    *(spl_v++) = *(itl_y++);
                    *(spl_y++) = *(itl_y++);
                }
            }
        }
    }   /* end for y*/

    LEAVE_FUNC();
    
    return 0;
}

int b16_rect_scale
(
    int w, int h, int lshift,
    void* src_base, int src_stride,
    void* dst_base, int dst_stride
)
{
    int x, y;
    int rshift = -lshift;
    
    for (y=0; y<h; ++y) 
    {
        uint16_t* src = (uint16_t*)((uint8_t*)src_base + y * src_stride);
        uint16_t* dst = (uint16_t*)((uint8_t*)dst_base + y * dst_stride);
        
        if (lshift > 0) {
            for (x=0; x<w; ++x) {
                *dst++ = *src++ << lshift;
            }
        } else {
            for (x=0; x<w; ++x) {
                *dst++ = *src++ >> rshift;
            }
        }
    }
    return 0;
}

int b16_mch_scale(yuv_seq_t *pdst, yuv_seq_t *psrc)
{
    uint8_t* src_base = psrc->pbuf;
    uint8_t* dst_base = pdst->pbuf;
    int src_stride = psrc->y_stride;
    int dst_stride = pdst->y_stride;
    
    int fmt = psrc->yuvfmt;
    int w   = psrc->width; 
    int h   = psrc->height; 
    int x, y;
    
    int lshift = (psrc->nlsb > 0) ? (16 - psrc->nlsb) : 0;
    int rshift = (pdst->nlsb > 0) ? (16 - pdst->nlsb) : 0;
    lshift -= rshift;
    
    ENTER_FUNC();
    
    assert(psrc->nbit==16 && pdst->nbit==16);
    assert(psrc->nlsb!=0  && pdst->nlsb!=0 );
    assert(psrc->yuvfmt   == pdst->yuvfmt  );
    assert(psrc->width    == pdst->width   );
    assert(psrc->height   == pdst->height  );

    if      (fmt == YUVFMT_400P)
    {
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
        
        src_base   += psrc->y_size;
        dst_base   += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
        
        src_base   += psrc->uv_size;
        dst_base   += pdst->uv_size;
        
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
    }
    else if (is_semi_planar(fmt))
    {
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
        
        src_base   += psrc->y_size;
        dst_base   += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b16_rect_scale(w, h, lshift, src_base, src_stride, dst_base, dst_stride);
    }
    
    LEAVE_FUNC();
    
    return 0;
}

int b16_rect_transpose(uint8_t* rect_base, int dstw, int dsth)
{
    #define TR_BUF_SIZE 4094
    static uint8_t tr_buf[TR_BUF_SIZE];
    uint16_t *tr_base = (uint16_t*)tr_buf;
    int size_needed = dstw * dsth * sizeof(uint16_t);
    if (size_needed>TR_BUF_SIZE)
    {
        tr_base = (uint16_t*) malloc ( size_needed );
        if (tr_base==0) {
            xerr("%s : malloc fail!\n", __FUNCTION__);
            return 0;
        }
    }
    memcpy(tr_base, rect_base, size_needed);

    int x, y;
    uint16_t* p16_base = (uint16_t*)rect_base;
    static uint16_t m;
    for(y=0; y<dsth; ++y)
    {
        for(x=0; x<dstw; ++x)
        {
            p16_base[y*dstw+x] = tr_base[x*dsth+y];
        }
    }
    
    if (size_needed>TR_BUF_SIZE)
    {
        free(tr_base);
    }
}

/** bitdepth conversion
 *  @param [in] b08_stride byte stride
 *  @param [in] b16_stride byte stride
 *  @param [in] nlsb       valid bit is at MSB if nlsb < 0
 */
int b16_n_b8_cvt
(
    void* b16_base, int b16_stride, int nlsb,
    void* b08_base, int b08_stride, int b_clip8,
    int w,  int h
)
{
    int x, y;
    int nshift = (nlsb > 0) ? (nlsb - 8) : 8;

    for (y=0; y<h; ++y) 
    {
        uint16_t* p16 = (uint16_t*)((uint8_t*)b16_base + y * b16_stride);
        uint8_t*  p08 = (uint8_t* )((uint8_t*)b08_base + y * b08_stride);
        
        if (b_clip8 == B16_2_B8) {
            for (x=0; x<w; ++x) {
                *(p08 ++) = (uint8_t )(*(p16++) >> nshift);
            }
        } else {
            for (x=0; x<w; ++x) {
                *(p16++) = (uint16_t)(*(p08 ++) << nshift);
            }
        }
    }
    
    return w*h;
}

int b16_n_b8_cvt_mch(yuv_seq_t *rect16, yuv_seq_t *rect08, int b_clip8)
{
    int fmt = rect16->yuvfmt;
    
    uint8_t* b16_base   = rect16->pbuf; 
    uint8_t* b08_base   = rect08->pbuf; 
    int nlsb        = rect16->nlsb;
    int b16_stride  = rect16->y_stride; 
    int b08_stride  = rect08->y_stride;
    int w   = rect16->width; 
    int h   = rect16->height; 

    ENTER_FUNC();
    
    assert (rect16->nbit  == 16);
    assert (rect16->nlsb  >= 8);
    assert (rect08->nbit  == 8);
    assert (rect16->yuvfmt  == rect08->yuvfmt);
    assert (rect16->width   == rect08->width);
    assert (rect16->height  == rect08->height);
    
    if      (fmt == YUVFMT_400P)
    {
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
        
        b16_base   += rect16->y_size;
        b08_base   += rect08->y_size; 
        b16_stride  = rect16->uv_stride;
        b08_stride  = rect08->uv_stride;
        
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
        
        b16_base   += rect16->uv_size;
        b08_base   += rect08->uv_size;
        
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
    }
    else if (is_semi_planar(fmt))
    {
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
        
        b16_base   += rect16->y_size;
        b08_base   += rect08->y_size; 
        b16_stride  = rect16->uv_stride;
        b08_stride  = rect08->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b16_n_b8_cvt(b16_base, b16_stride, nlsb, b08_base, b08_stride, b_clip8, w, h);
    }
    
    LEAVE_FUNC();

    return 0;
}

int yuv_copy_rect(int w, int h, uint8_t *dst, int dst_stride, uint8_t *src, int src_stride)
{
    int i = 0;
    int stride = MIN(dst_stride, src_stride);
    w = w ? w : stride;
    w = MIN(w, stride);
    for (i=0; i<h; ++i) {
        memcpy(dst, src, stride);
        dst += dst_stride;
        src += src_stride;
    }
    return 0;   
}

int yuv_copy_frame(yuv_seq_t *pdst, yuv_seq_t *psrc)
{
    ENTER_FUNC();
    show_yuv_prop(psrc, SLOG_DBG, "src ");
    show_yuv_prop(pdst, SLOG_DBG, "dst ");

    #define CMP(prop) (psrc->prop != pdst->prop)
    if (CMP(yuvfmt) || CMP(width) || CMP(height) || CMP(nbit) || CMP(btile)) 
    {
        xerr("%s(): diff in basic info\n", __FUNCTION__);
        return -1;
    }
    if (pdst->btile && (CMP(tile.tw) || CMP(tile.th) || CMP(tile.tsz))) 
    {
        xerr("%s(): diff in tile mode\n", __FUNCTION__);
        return -1;
    }

    uint8_t* src_base = psrc->pbuf;
    uint8_t* dst_base = pdst->pbuf;
    int fmt = psrc->yuvfmt;
    int h   = psrc->height; 
    
    yuv_copy_rect(0, h, 
            dst_base, pdst->y_stride, 
            src_base, psrc->y_stride);

    if (is_mch_420(fmt) || is_mch_422(fmt))
    {
        h /= get_uv_ds_ratio_h(fmt); 
        src_base   += psrc->y_size;
        dst_base   += pdst->y_size; 
        yuv_copy_rect(0, h, 
                dst_base, pdst->uv_stride, 
                src_base, psrc->uv_stride);
        
        if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
        {
            src_base   += psrc->uv_size;
            dst_base   += pdst->uv_size;
            yuv_copy_rect(0, h, 
                    dst_base, pdst->uv_stride, 
                    src_base, psrc->uv_stride);
        }
    }
    
    LEAVE_FUNC();
    
    return 0;
}

int get_roi_shift_y(yuv_seq_t *yuv)
{
    int nbyte = yuv->nbit / 8;
    return yuv->y_stride * yuv->roi.y 
                 + nbyte * yuv->roi.x;
}

int get_roi_shift_uv(yuv_seq_t *yuv)
{
    int nbyte = yuv->nbit / 8;
    int ds_w = get_uv_ds_ratio_w(yuv->yuvfmt);
    int ds_h = get_uv_ds_ratio_h(yuv->yuvfmt);
    ds_w = ds_w ? ds_w : 1;
    ds_h = ds_h ? ds_h : 1;
    
    return yuv->uv_stride * yuv->roi.y / ds_w 
                  + nbyte * yuv->roi.x / ds_h;
}

uint8_t *get_roi_base_y(yuv_seq_t *yuv)
{
    return (uint8_t *)yuv->pbuf + get_roi_shift_y(yuv);
}

uint8_t *get_roi_base_uv(yuv_seq_t *yuv)
{
    return (uint8_t *)yuv->pbuf + get_roi_shift_uv(yuv) + yuv->y_size;
}

/**
 *  @brief copy psrc->roi to pdst->roi
 */
int yuv_copy_roi(yuv_seq_t *pdst, yuv_seq_t *psrc)
{
    ENTER_FUNC();
    show_yuv_prop(psrc, SLOG_DBG, "src ");
    show_yuv_prop(pdst, SLOG_DBG, "dst ");

    #define CMP(prop) (psrc->prop != pdst->prop)
    if (CMP(yuvfmt) || CMP(width) || CMP(height) || CMP(nbit) || CMP(btile)) 
    {
        xerr("%s(): diff in basic info\n", __FUNCTION__);
        return -1;
    }
    if (pdst->btile) 
    {
        xerr("%s(): not support tile fmt\n", __FUNCTION__);
        return -1;
    }
    if (pdst->nbit != 16 && pdst->nbit != 8)
    {
        xerr("%s(): nbit=%d, not in {16,8}\n", __FUNCTION__, pdst->nbit);
        return -1; 
    }

    int nbyte = pdst->nbit / 8;
    uint8_t* src_base = get_roi_base_y(psrc);
    uint8_t* dst_base = get_roi_base_y(pdst);
    int fmt = psrc->yuvfmt;
    
    pdst->roi.w = psrc->roi.w;
    pdst->roi.h = psrc->roi.h;
    assert(is_valid_roi(psrc->width, psrc->height, &psrc->roi));
    assert(is_valid_roi(pdst->width, pdst->height, &pdst->roi));
    
    int roi_w = nbyte * psrc->roi.w;
    int roi_h = psrc->roi.h;
    
    yuv_copy_rect(roi_w, roi_h,
            dst_base, pdst->y_stride, 
            src_base, psrc->y_stride);

    if (is_mch_420(fmt) || is_mch_422(fmt))
    {
        roi_w /= get_uv_ds_ratio_w(fmt);
        roi_h /= get_uv_ds_ratio_h(fmt);
        
        src_base = get_roi_base_uv(psrc);
        dst_base = get_roi_base_uv(pdst);
        
        yuv_copy_rect(roi_w, roi_h,
                dst_base, pdst->y_stride, 
                src_base, psrc->y_stride);
                
        if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
        {
            src_base += psrc->uv_size;
            dst_base += pdst->uv_size;
            
            yuv_copy_rect(roi_w, roi_h,
                    dst_base, pdst->y_stride, 
                    src_base, psrc->y_stride);
        }
    }
    
    LEAVE_FUNC();
    
    return 0;
}

int not_null_roi(rect_t *roi)
{
    return (roi && (roi->x||roi->y||roi->w||roi->h));
}

int is_valid_roi(int w, int h, rect_t *roi)
{
    if (w && h && not_null_roi(roi) &&
            is_in_range(0, w, roi->x) &&
            is_in_range(0, w, roi->x + roi->w) &&
            is_in_range(0, h, roi->y) &&
            is_in_range(0, h, roi->y + roi->h)) 
    {
        return 1;
    }
    return 0;
}

/**
 *  @param [in] pdst description for target yuv format
 *      The buffer @pdst bound is just for median used. "pdst->pbuf"
 *      is not guaranteed to hold the target yuv data at any point.
 *  @param [in] psrc hold yuv buffer compliant to source yuv format (@psrc itself)
 *  @return either @pdst or @psrc which hold yuv buffer compliant to @pdst
 */
yuv_seq_t *yuv_cvt_frame(yuv_seq_t *pdst, yuv_seq_t *psrc)
{
    yuv_seq_t cfg_src, cfg_dst;
    
    ENTER_FUNC();
    show_yuv_prop(pdst, SLOG_DBG, "dst ");
    show_yuv_prop(psrc, SLOG_DBG, "src ");

    memcpy(&cfg_src, psrc, sizeof(yuv_seq_t));
    memcpy(&cfg_dst, pdst, sizeof(yuv_seq_t));
    
    #define SWAP_SRC_DST()  do { \
        yuv_seq_t *ptmp=psrc; psrc=pdst; pdst=ptmp; \
    } while(0)
    
    SWAP_SRC_DST();
    
    /**
     *  b10-untile/unpack, b8-untile
     */
    if (cfg_src.nbit==10) 
    {
        SWAP_SRC_DST();
        set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                cfg_src.yuvfmt, BIT_16, BIT_10, TILE_0, 0, 0);
        if (cfg_src.btile) {
            b10_tile_unpack_mch(psrc, pdst, B10_2_B16);
        } else {
            b10_rect_unpack_mch(psrc, pdst, B10_2_B16);
        }
    } 
    else if (cfg_src.nbit==8)
    {
        if (cfg_src.btile) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_8, BIT_8, TILE_0, 0, 0);
            b8_tile_2_mch(psrc, pdst, TILE2RECT);
        }
    }

    /**
     *  bit-shift
     */
    if (pdst->nbit != cfg_dst.nbit) {
        if (pdst->nbit==16 && cfg_dst.nbit==8) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_8, BIT_8, TILE_0, 0, 0);
            b16_n_b8_cvt_mch(psrc, pdst, B16_2_B8);
        } 
        else if (pdst->nbit==8 && cfg_dst.nbit>8) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_16, cfg_dst.nbit, TILE_0, 0, 0);
            pdst->nlsb = cfg_dst.nlsb;
            b16_n_b8_cvt_mch(pdst, psrc, B8_2_B16);
        }
    } else if (cfg_dst.nbit == 16) {
        if (pdst->nlsb != cfg_dst.nlsb) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_16, BIT_16, TILE_0, 0, 0);
            b16_mch_scale(pdst, psrc);
        } 
    }

    /**
     * fmt convertion.
     */        
    if (cfg_src.yuvfmt  != cfg_dst.yuvfmt)
    {
        int nbit = pdst->nbit;
        int nlsb = pdst->nlsb;
        assert(nbit == 8 || nbit == 16);
        int (*mch_p2p   )(yuv_seq_t*, yuv_seq_t*);
        int (*mch_sp2p  )(yuv_seq_t*, yuv_seq_t*, int);
        int (*mch_yuyv2p)(yuv_seq_t*, yuv_seq_t*, int);
        mch_p2p    = (nbit==8) ? b8_mch_p2p    : b16_mch_p2p    ;
        mch_sp2p   = (nbit==8) ? b8_mch_sp2p   : b16_mch_sp2p   ;
        mch_yuyv2p = (nbit==8) ? b8_mch_yuyv2p : b16_mch_yuyv2p ;

        // uv de-interlace
        if (cfg_src.yuvfmt != get_spl_fmt(cfg_src.yuvfmt)) { 
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    get_spl_fmt(cfg_src.yuvfmt), nbit, nlsb, TILE_0, 0, 0);
            if (is_semi_planar(cfg_src.yuvfmt)) {
                mch_sp2p(psrc, pdst, SPLITTING);
            } else if (cfg_src.yuvfmt == YUVFMT_UYVY || cfg_src.yuvfmt == YUVFMT_YUYV) {
                mch_yuyv2p(psrc, pdst, SPLITTING);
            }
        }
        
        // uv re-sample
        if (pdst->yuvfmt != get_spl_fmt(cfg_dst.yuvfmt) )
        {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    get_spl_fmt(cfg_dst.yuvfmt), nbit, nlsb, TILE_0, 0, 0);
            mch_p2p(pdst, psrc); 
        }

        // uv interlace
        if (pdst->yuvfmt != cfg_dst.yuvfmt)
        {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, nbit, nlsb, TILE_0, 0, 0);
            if (is_semi_planar(cfg_dst.yuvfmt)) {
                mch_sp2p(pdst, psrc, INTERLACING);
            } else if (cfg_dst.yuvfmt == YUVFMT_UYVY  || cfg_dst.yuvfmt == YUVFMT_YUYV ) {
                mch_yuyv2p(pdst, psrc, INTERLACING);
            }
        }
    }

    /**
     *  b10-tile/pack, b8-tile
     */
    if (cfg_dst.nbit==10) 
    {
        SWAP_SRC_DST();
        if (cfg_dst.btile) {
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, BIT_10, BIT_10, TILE_1, 0, 0);
            b10_tile_unpack_mch(pdst, psrc, B16_2_B10);
        } else {
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, BIT_10, BIT_10, TILE_0, 
                    cfg_dst.y_stride, cfg_dst.io_size);
            b10_rect_unpack_mch(pdst, psrc, B16_2_B10);
        }
    }
    else if (cfg_dst.nbit==8)
    {
        if (cfg_dst.btile) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, 1, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, BIT_8, BIT_8, TILE_1, 0, 0);
            b8_tile_2_mch(pdst, psrc, RECT2TILE);
        }
    }
    else if (cfg_dst.nbit==16)
    {
        //TODO: 
    }

    // buf re-placement
    if (pdst->y_stride != cfg_dst.y_stride || 
        pdst->io_size  != cfg_dst.io_size  )
    {
        SWAP_SRC_DST();
        set_yuv_prop_by_copy(pdst, 1, &cfg_dst);
        yuv_copy_frame(pdst, psrc); 
    }
    
    LEAVE_FUNC();
    
    return pdst;
}

int cvt_arg_init (cvt_opt_t *cfg, int argc, char *argv[])
{
    set_yuv_prop(&cfg->src, 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    set_yuv_prop(&cfg->dst, 0, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    cfg->frame_range[1] = INT_MAX;
}

int cvt_arg_parse(cvt_opt_t *cfg, int argc, char *argv[])
{
    int i, j;
    yuv_seq_t *seq = &cfg->dst;
    yuv_seq_t *src = &cfg->src;
    yuv_seq_t *dst = &cfg->dst;
    
    ENTER_FUNC();
    
    if (argc<2) {
        xerr("No arg specified.\n");
        return -1;
    }
    
    const char* start_opts = "h, help, i, src, o, dst, x, xl, xlevel, xall, xnon";
    if (argv[1][0]!='-' || 0 > field_in_record(argv[1], start_opts))
    {
        xerr("1st opt not in `%s`\n", start_opts);
        return -1;
    }

    /**
     *  loop options
     */    
    for (i=1; i>=0 && i<argc; )
    {
        xdbg("@cmdl>> argv[%d]=%s\n", i, argv[i]);

        char *arg = argv[i];
        if (arg[0]!='-') {
            xerr("`%s` is not an option\n", arg);
            return -i;
        }
        
        /** search for enum or ref first
         */
        if (arg[1] == '%') 
        {
            for (j=0; j<n_cmn_res; ++j) {
                if (0==strcmp(&arg[2], cmn_res[j].name)) {
                    src->width  = cmn_res[j].w;
                    src->height = cmn_res[j].h;
                    break;
                }
            }
            if (j<n_cmn_res) {
                ++i; continue;
            }
            
            for (j=0; j<n_cmn_fmt; ++j) {
                if (0==strcmp(&arg[2], cmn_fmt[j].name)) {
                    seq->yuvfmt = cmn_fmt[j].val;
                    break;
                }
            }
            if (j<n_cmn_fmt) {
                ++i; continue;
            }

            xerr("Unknown enum or ref %s used in %s\n", arg, __FUNCTION__);
            return -i;
        }
        
        arg += 1;
        ++i;

        if (0==strcmp(arg, "h") || 0==strcmp(arg, "help")) {
            cvt_arg_help();
            return 0;
        } else
        if (0==strcmp(arg, "i") || 0==strcmp(arg, "src")) {
            char *path = 0;
            seq = &cfg->src;
            i = arg_parse_str(i, argc, argv, &path);
            ios_cfg(cfg->ios, CVT_IOS_SRC, path, "rb");
        } else
        if (0==strcmp(arg, "o") || 0==strcmp(arg, "dst")) {
            char *path = 0;
            seq = &cfg->dst;
            i = arg_parse_str(i, argc, argv, &path);
            ios_cfg(cfg->ios, CVT_IOS_DST, path, "wb");
        } else
        if (0==strcmp(arg, "wxh")) {
            i = arg_parse_wxh(i, argc, argv, &src->width, &src->height);
        } else
        if (0==strcmp(arg, "fmt")) {
            i = arg_parse_fmt(i, argc, argv, &seq->yuvfmt);
        } else
        if (0==strcmp(arg, "b10")) {
            seq->nbit = 10;     seq->nlsb = 10;
        } else
        if (0==strcmp(arg, "nbit") || 0==strcmp(arg, "b")) {
            i = arg_parse_int(i, argc, argv, &seq->nbit);
        } else
        if (0==strcmp(arg, "nlsb")) {
            i = arg_parse_int(i, argc, argv, &seq->nlsb);
        } else
        if (0==strcmp(arg, "btile") || 0==strcmp(arg, "tile") || 0==strcmp(arg, "t")) {
            i = opt_parse_int(i, argc, argv, &seq->btile, 1);
            seq->btile ? (seq->yuvfmt = YUVFMT_420SP) : 0;
        } else  
        if (0==strcmp(arg, "n-frame") || 0==strcmp(arg, "nframe") ||
            0==strcmp(arg, "f")       || 0==strcmp(arg, "frame")) {
            int nframe = 0;
            i = arg_parse_int(i, argc, argv, &nframe);
            cfg->frame_range[1] = nframe + cfg->frame_range[0];
        } else
        if (0==strcmp(arg, "f-start")) {
            i = arg_parse_int(i, argc, argv, &cfg->frame_range[0]);
        } else
        if (0==strcmp(arg, "f-range")) {
            i = arg_parse_range(i, argc, argv, cfg->frame_range);
        } else
        if (0==strcmp(arg, "stride")) {
            i = arg_parse_int(i, argc, argv, &seq->y_stride);
        } else
        if (0==strcmp(arg, "iosize")) {
            i = arg_parse_int(i, argc, argv, &seq->io_size);
        } else
        if (0==strcmp(arg, "xnon")) {
            xlevel(SLOG_NON);
        } else
        if (0==strcmp(arg, "xall")) {
            xlevel(SLOG_ALL);
        } else
        if (0==strcmp(arg, "xlevel")) {
            int level;
            i = arg_parse_int(i, argc, argv, &level);
            xlevel(level);
        } else
        {
            xerr("Unrecognized opt `%s`\n", arg);
            return 1-i;
        }
    }
    
    LEAVE_FUNC();

    return i;
}

int cvt_arg_check(cvt_opt_t *cfg, int argc, char *argv[])
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
    show_yuv_prop(psrc, SLOG_CFG, "@cfg>> src: ");
    show_yuv_prop(pdst, SLOG_CFG, "@cfg>> dst: ");
    
    LEAVE_FUNC();
    
    return 0;
}

int cvt_arg_close(cvt_opt_t *cfg)
{
    ios_close(cfg->ios, CVT_IOS_CNT);
}

int cvt_arg_help()
{
    printf("yuv format convertor. Options:\n");
    printf("\t -i|-dst name<%%s> {...props...}\n");
    printf("\t -o|-src name<%%s> {...props...}\n");
    printf("\t ...frame range...   <%%d~%%d>\n");

    printf("\nset frame range as follow:\n");
    printf("\t [-f-range    <%%d~%%d>]\n");
    printf("\t [-f-start    <%%d>]\n");
    printf("\t [-frame|-f   <%%d>]\n");
    
    printf("\nset yuv props as follow:\n");
    printf("\t [-wxh <%%dx%%d>]\n");
    printf("\t [-fmt <%%420p,%%420sp,%%uyvy,%%422p>]\n");
    printf("\t [-stride <%%d>]\n");
    printf("\t [-iosize <%%d>]  //frame buf size\n");
    printf("\t [-b10]\n");
    printf("\t [-btile|-tile|-t]\n");
    
    int j;
    printf("\n-wxh option can be short as follow:\n");
    for (j=0; j<n_cmn_res; ++j) {
        printf("\t -%%%-4s = \"-wxh %4dx%-4d\"\n", cmn_res[j].name, cmn_res[j].w, cmn_res[j].h);
    }
    printf("\n-fmt option can be short as follow:\n");
    for (j=0; j<n_cmn_fmt; ++j) {
        printf("\t -%%%-7s = \"-fmt %d (%%%-7s)\"\n", cmn_fmt[j].name, cmn_fmt[j].val, cmn_fmt[j].name);
    }
    return 0;
}

int yuv_cvt(int argc, char **argv)
{
    int         r, i;
    cvt_opt_t   cfg;
    yuv_seq_t   seq[2];

    memset(seq, 0, sizeof(seq));
    memset(&cfg, 0, sizeof(cfg));
    cvt_arg_init (&cfg, argc, argv);
    
    r = cvt_arg_parse(&cfg, argc, argv);
    if (r == 0) {
        //help exit
        return 0;
    } else if (r < 0) {
        // xerr("cvt_arg_parse() failed\n");
        return 1;
    }
    r = cvt_arg_check(&cfg, argc, argv);
    if (r < 0) {
        // xerr("cvt_arg_check() failed\n");
        return 1;
    }
    
    int w_align = bit_sat(6, cfg.src.width);
    int h_align = bit_sat(6, cfg.src.height);
    int nbyte   = 1 + (cfg.src.nbit > 8 || cfg.src.nbit > 8);
    for (i=0; i<2; ++i) {
        seq[i].buf_size = w_align * h_align * 3 * nbyte;
        seq[i].pbuf = (uint8_t *)malloc(seq[i].buf_size);
        if(!seq[i].pbuf) {
            xerr("malloc for seq[%d] failed\n", i);
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
        
        set_yuv_prop_by_copy(&seq[0], 1, &cfg.src);
        r = fread(seq[0].pbuf, cfg.src.io_size, 1, cfg.ios[CVT_IOS_SRC].fp);
        if (r<1) {
            if ( ios_feof(cfg.ios, CVT_IOS_SRC) ) {
                xlog(SLOG_INFO, "@seq> reach file end, force stop\n");
            } else {
                xerr("error reading file\n");
            }
            break;
        }

        set_yuv_prop_by_copy(&seq[1], 1, &cfg.dst);
        yuv_seq_t *pdst = yuv_cvt_frame(&seq[1], &seq[0]);
        
        r = fwrite(pdst->pbuf, pdst->io_size, 1, cfg.ios[CVT_IOS_DST].fp);
        if (r<1) {
            xerr("error writing file\n");
            break;
        }
        xlog(SLOG_L1, "@frm> #%d -\n", i);
    } // end frame loop
    
    cvt_arg_close(&cfg);
    for (i=0; i<2; ++i) {
        yuv_buf_free(&seq[i]);
    }

    return 0;
}
