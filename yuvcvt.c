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

#include "yuvdef.h"
#include "yuvcvt.h"
#include "sim_opt.h"


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

    return ++i;
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

/** 
 *  422p <-> 420p uv down/up sampling
 */
int b8_mch_p2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
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

    ENTER_FUNC;
    
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
        dst_u += pdst->uv_stride + dst_uv_shift;
        src_u += psrc->uv_stride + src_uv_shift;
        dst_v += pdst->uv_stride + dst_uv_shift;
        src_v += psrc->uv_stride + src_uv_shift;
        
        memcpy(dst_u, src_u, linesize);
        memcpy(dst_v, src_v, linesize);
        
        if (dst_uv_shift>0) 
        {
            memcpy(dst_u + dst_uv_shift, src_u + src_uv_shift, linesize);
            memcpy(dst_v + dst_uv_shift, src_v + src_uv_shift, linesize);
        }
    }
    
    LEAVE_FUNC;
    
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

    ENTER_FUNC;
    
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
    
    LEAVE_FUNC;
    
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

    ENTER_FUNC;
    
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

    LEAVE_FUNC;
    
    return 0;
}

int b16_mch_p2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    return b8_mch_p2p(psrc, pdst);
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

    ENTER_FUNC;
    
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
    
    LEAVE_FUNC;
    
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

    ENTER_FUNC;
    
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

    LEAVE_FUNC;
    
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

int b16_mch_scale(yuv_seq_t *psrc, yuv_seq_t *pdst)
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
    
    ENTER_FUNC;
    
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
    
    LEAVE_FUNC;
    
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
            printf("%s : malloc fail!\n", __FUNCTION__);
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

    ENTER_FUNC;
    
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
    
    LEAVE_FUNC;

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
    
    ENTER_FUNC;

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
        set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
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
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
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
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_8, BIT_8, TILE_0, 0, 0);
            b16_n_b8_cvt_mch(psrc, pdst, B16_2_B8);
        } 
        else if (pdst->nbit==8 && cfg_dst.nbit>8) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_16, cfg_dst.nbit, TILE_0, 0, 0);
            pdst->nlsb = cfg_dst.nlsb;
            b16_n_b8_cvt_mch(pdst, psrc, B8_2_B16);
        }
    } else if (cfg_dst.nbit == 16) {
        if (pdst->nlsb != cfg_dst.nlsb) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    cfg_src.yuvfmt, BIT_16, BIT_16, TILE_0, 0, 0);
            b16_mch_scale(psrc, pdst);
        } 
    }

    /**
     * fmt convertion.
     */        
    if (cfg_src.yuvfmt != cfg_dst.yuvfmt) 
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
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    get_spl_fmt(cfg_src.yuvfmt), nbit, nlsb, TILE_0, 0, 0);
            if (is_semi_planar(cfg_src.yuvfmt)) {
                mch_sp2p(psrc, pdst, SPLITTING);
            } else if (cfg_src.yuvfmt == YUVFMT_UYVY || cfg_src.yuvfmt == YUVFMT_YUYV) {
                mch_yuyv2p(psrc, pdst, SPLITTING);
            }
        }
        
        // uv re-sample
        if (get_spl_fmt(cfg_src.yuvfmt) != get_spl_fmt(cfg_dst.yuvfmt))
        {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    get_spl_fmt(cfg_dst.yuvfmt), nbit, nlsb, TILE_0, 0, 0);
            mch_p2p(psrc, pdst); 
        }

        // uv interlace
        if (cfg_dst.yuvfmt != get_spl_fmt(cfg_dst.yuvfmt)) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
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
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, BIT_10, BIT_10, TILE_1, 0, 0);
            b10_tile_unpack_mch(pdst, psrc, B16_2_B10);
        } else {
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, BIT_10, BIT_10, TILE_0, 0, 0);
            b10_rect_unpack_mch(pdst, psrc, B16_2_B10);
        }
    }
    else if (cfg_dst.nbit==8)
    {
        if (cfg_dst.btile) {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, cfg_src.width, cfg_src.height, 
                    cfg_dst.yuvfmt, BIT_8, BIT_8, TILE_1,
                    cfg_dst.y_stride, cfg_dst.io_size);
            b8_tile_2_mch(pdst, psrc, RECT2TILE);
        }
    }
    else if (cfg_dst.nbit==16)
    {
        //TODO: 
    }
    
    LEAVE_FUNC;
    
    return pdst;
}

int cvt_arg_init (cvt_opt_t *cfg, int argc, char *argv[])
{
}

int cvt_arg_parse(cvt_opt_t *cfg, int argc, char *argv[])
{
    int i, j;
    yuv_seq_t *seq = &cfg->dst;
    
    ENTER_FUNC;
    
    if (argc<2) {
        return -1;
    }
    if (0 != strcmp(argv[1], "-dst") && 0 != strcmp(argv[1], "-src") &&
        0 != strcmp(argv[1], "-h")   && 0 != strcmp(argv[1], "-help") )
    {
        return -1;
    }
    
    /**
     *  init options
     */
    set_yuv_prop(&cfg->src, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    set_yuv_prop(&cfg->dst, 0, 0, YUVFMT_420P, BIT_8, BIT_8, TILE_0, 0, 0);
    cfg->frame_range[1] = INT_MAX;

    /**
     *  loop options
     */    
    for (i=1; i>=0 && i<argc; )
    {
        char *arg = argv[i];
        if (arg[0]!='-') {
            printf("@cmdl>> Err : unrecognized arg `%s`\n", arg);
            return -i;
        }
        arg += 1;
        
        for (j=0; j<n_cmn_res; ++j) {
            if (0==strcmp(arg, cmn_res[j].name)) {
                seq->width  = cmn_res[j].w;
                seq->height = cmn_res[j].h;
                break;
            }
        }
        if (j<n_cmn_res) {
            ++i; continue;
        }
        
        for (j=0; j<n_cmn_fmt; ++j) {
            if (0==strcmp(arg, cmn_fmt[j].name)) {
                seq->yuvfmt = cmn_fmt[j].ifmt;
                break;
            }
        }
        if (j<n_cmn_fmt) {
            ++i; continue;
        }
        
        if (0==strcmp(arg, "h") || 0==strcmp(arg, "help")) {
            cvt_arg_help();
            return -1;
        } else
        if (0==strcmp(arg, "src")) {
            seq = &cfg->src;
            i = arg_parse_str(i, argc, argv, &cfg->src.path);
        } else
        if (0==strcmp(arg, "dst")) {
            seq = &cfg->dst;
            i = arg_parse_str(i, argc, argv, &cfg->dst.path);
        } else
        if (0==strcmp(arg, "wxh")) {
            i = arg_parse_wxh(i, argc, argv, &seq->width, &seq->height);
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
            i = opt_parse_int(i, argc, argv, &seq->btile, 1);
            seq->btile ? (seq->yuvfmt = YUVFMT_420SP) : 0;
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
        if (0==strcmp(arg, "stride")) {
            i = arg_parse_int(i, argc, argv, &seq->y_stride);
        } else
        if (0==strcmp(arg, "iosize")) {
            i = arg_parse_int(i, argc, argv, &seq->io_size);
        } else
        {
            printf("@cmdl>> Err : invalid opt `%s`\n", arg);
            return -i;
        }
    }
    
    LEAVE_FUNC;

    return i;
}

int cvt_arg_check(cvt_opt_t *cfg, int argc, char *argv[])
{
    yuv_seq_t *psrc = &cfg->src;
    yuv_seq_t *pdst = &cfg->dst;
    
    ENTER_FUNC;
    
    if (!cfg->src.path || !cfg->dst.path) {
        printf("@cmdl>> Err : no src or dst\n");
        return -1;
    }
    if (cfg->frame_range[0] >= cfg->frame_range[1]) {
        printf("@cmdl>> Err : Invalid frame_range %d~%d\n", 
                cfg->frame_range[0], cfg->frame_range[1]);
        return -1;
    }
    if (!psrc->width || !psrc->height) {
        printf("@cmdl>> Err : invalid resolution for src\n");
        return -1;
    }
    if ((psrc->nbit != 8 && psrc->nbit!=10 && psrc->nbit!=16) ||
        (psrc->nbit < psrc->nlsb)) {
        printf("@cmdl>> Err : invalid bitdepth (%d/%d) for src\n", 
                psrc->nlsb, psrc->nbit);
        return -1;
    }
    if ((pdst->nbit != 8 && pdst->nbit!=10 && pdst->nbit!=16) ||
        (pdst->nbit < pdst->nlsb)) {
        printf("@cmdl>> Err : invalid bitdepth (%d/%d) for dst\n", 
                pdst->nlsb, pdst->nbit);
        return -1;
    }
    
    psrc->nlsb = psrc->nlsb ? psrc->nlsb : psrc->nbit;
    pdst->nlsb = pdst->nlsb ? pdst->nlsb : pdst->nbit;
    
    LEAVE_FUNC;
    
    return 0;
}

int cvt_arg_help()
{
    printf("-dst name<%%s> {...yuv props...} \n");
    printf("-src name<%%s> {...yuv props...} \n");
    printf("[-frame <%%d>]\n");
    printf("\n...yuv props...\n");
    printf("\t [-wxh <%%dx%%d>]\n");
    printf("\t [-fmt <420p,420sp,uyvy,422p>]\n");
    printf("\t [-stride <%%d>]\n");
    printf("\t [-fsize <%%d>]\n");
    printf("\t [-b10]\n");
    printf("\t [-btile]\n");
    return 0;
}

int yuv_cvt(int argc, char **argv)
{
    int         r, i;
    cvt_opt_t   cfg;
    yuv_seq_t   seq[2];

    memset(seq, 0, sizeof(seq));
    memset(&cfg, 0, sizeof(cfg));
    r = cvt_arg_parse(&cfg, argc, argv);
    if (r < 0) {
        cvt_arg_help();
        return 1;
    }
    r = cvt_arg_check(&cfg, argc, argv);
    if (r < 0) {
        printf("\n****src****\n");  show_yuv_prop(&cfg.src);
        printf("\n****dst****\n");  show_yuv_prop(&cfg.dst);
        return 1;
    }
    
    cfg.dst.fp = fopen(cfg.dst.path, "wb");
    cfg.src.fp = fopen(cfg.src.path, "rb");
    if( !cfg.dst.fp || !cfg.src.fp )
    {
        printf("error : open %s %s fail\n", 
                cfg.dst.fp ? "" : cfg.dst.path, 
                cfg.src.fp ? "" : cfg.src.path);
        return -1;
    }
    
    int w_align = bit_sat(6, cfg.src.width);
    int h_align = bit_sat(6, cfg.src.height);
    int nbyte   = 1 + (cfg.src.nbit > 8 || cfg.src.nbit > 8);
    for (i=0; i<2; ++i) {
        seq[i].buf_size = w_align * h_align * 3 * nbyte;
        seq[i].pbuf = (uint8_t *)malloc(seq[i].buf_size);
        if(!seq[i].pbuf) {
            printf("error: malloc seq[%d] fail\n", i);
            return -1;
        }
    }

    /*************************************************************************
     *                          frame loop
     ************************************************************************/
    for (i=cfg.frame_range[0]; i<cfg.frame_range[1]; i++) 
    {
        printf("\n@frm> **** %d ****\n", i);
        
        set_yuv_prop_by_copy(&seq[0], &cfg.src);
        
        r=fseek(cfg.src.fp, seq[0].io_size * i, SEEK_SET);
        if (r) {
            printf("fseek %d error\n", seq[0].io_size * i);
            return -1;
        }
        r = fread(seq[0].pbuf, seq[0].io_size, 1, cfg.src.fp);
        if (r<1) {
            if ( feof(cfg.src.fp) ) {
                printf("reach file end, force stop\n");
            } else {
                printf("error reading file\n");
            }
            break;
        }
        
        set_yuv_prop_by_copy(&seq[1], &cfg.dst);
        yuv_seq_t *pdst = yuv_cvt_frame(&seq[1], &seq[0]);
        
        r = fwrite(pdst->pbuf, pdst->io_size, 1, cfg.dst.fp);
        if (r<1) {
            printf("error writing file\n");
            break;
        }
    } // end frame loop
    
    for (i=0; i<2; ++i) {
        if (seq[i].pbuf)    free(seq[i].pbuf);
    }
    
    if (cfg.dst.fp)     fclose(cfg.dst.fp);
    if (cfg.src.fp)     fclose(cfg.src.fp);

    return 0;
}