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

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <limits.h>

#include "yuvfmt.h"

enum {
    B10_2_B16   = 0,
    B16_2_B10   = 1,
    
    B16_2_B8    = 0,
    B8_2_B16    = 1,
    
    TILE_0      = 0,
    TILE_1      = 1,

    LINE2RECT   = 0,
    RECT2LINE   = 1,    
    TILE2RECT   = LINE2RECT,
    RECT2TILE   = RECT2LINE,
    
    BIT_8       = 8,
    BIT_10      = 10,
    BIT_16      = 16,
    
    SPLITTING   = 0,
    INTERLACING = 1,
};

typedef struct resolution {
    const char *name; 
    int         w, h;
} res_t;
const static res_t cmn_res[] = {
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

typedef struct yuvformat {
    const char *name; 
    int         ifmt;
} fmt_t;
const static fmt_t cmn_fmt[] = {
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
static const char* show_fmt(int ifmt)
{
    int j;
    for (j=0; j<ARRAY_SIZE(cmn_fmt); ++j) {
        if (ifmt == cmn_fmt[j].ifmt) {
            return cmn_fmt[j].name;
        }
    }
    return "unknown";
}

static int fcall_layer = 0;
#define ENTER_FUNC  printf("@+++> %-2d: %s(+)\n", fcall_layer++, __FUNCTION__)
#define LEAVE_FUNC  printf("@---> %-2d: %s(-)\n", --fcall_layer, __FUNCTION__)

inline int sat_div(int num, int den)
{
    return (num + den - 1)/den;
}

int bit_sat(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val+pad) & (~pad);
}

int is_bit_aligned(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val&pad)==0;
}

inline
int is_mch_420(int fmt)
{
    return (fmt == YUVFMT_420P
            || fmt == YUVFMT_420SP
            || fmt == YUVFMT_420SPA) ? 1 : 0;
}

inline
int is_mch_422(int fmt)
{
    return (fmt == YUVFMT_422P
            || fmt == YUVFMT_422SP
            || fmt == YUVFMT_422SPA
            || fmt == YUVFMT_UYVY
            || fmt == YUVFMT_YUYV) ? 1 : 0;
}

inline 
int is_mch_mixed(int fmt)
{
    return (fmt == YUVFMT_UYVY 
            || fmt == YUVFMT_YUYV) ? 1 : 0;
}

inline
int is_mch_planar(int fmt)
{
    return (fmt == YUVFMT_420P 
            || fmt == YUVFMT_422P) ? 1 : 0;
}

inline
int is_semi_planar(int fmt)
{
    return (fmt == YUVFMT_420SP
            || fmt == YUVFMT_420SPA
            || fmt == YUVFMT_422SP
            || fmt == YUVFMT_422SPA) ? 1 : 0;
}

inline
int is_mono_planar(int fmt)
{
    return (fmt == YUVFMT_400P) ? 1 : 0;
}

int get_spl_fmt(int fmt)
{
    if (fmt == YUVFMT_400P)     return YUVFMT_400P; 
    else if (is_mch_420(fmt))   return YUVFMT_420P;
    else if (is_mch_422(fmt))   return YUVFMT_422P;
    else                        return YUVFMT_UNSUPPORT;
}

void swap_uv(uint8_t **u, uint8_t **v)
{
    uint8_t *m = *u;
    *u = *v;
    *v = m;
}

int get_uv_width(yuv_seq_t *yuv)
{
    int fmt = yuv->yuvfmt;
    
    if       (is_semi_planar(fmt)) {
        return yuv->width;
    } else if (is_mch_planar(fmt)) {
        return yuv->width / 2;
    }
    
    return 0;
}

int get_uv_height(yuv_seq_t *yuv)
{
    int fmt = yuv->yuvfmt;

    if       (is_mch_422(fmt)) {
        return yuv->height;
    } else if (is_mch_420(fmt)) {
        return yuv->height / 2;
    }

    return 0;
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

void b8_linear_2_rect_align_0(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; ++i) {
            *((uint8_t*)(rect+i)) = *((uint8_t*)(line+i));
        }
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect_align_4(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=4) {
            *((uint32_t*)(rect+i)) = *((uint32_t*)(line+i));
        }
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect_align_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=8) {
            *((uint64_t*)(rect+i)) = *((uint64_t*)(line+i));
        }
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect_width_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        *((uint64_t*)(rect)) = *((uint64_t*)(line));
        line += w;
        rect += s;
    }
}


void b8_rect_2_linear_align_0(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; ++i) {
            *((uint8_t*)(line+i)) = *((uint8_t*)(rect+i));
        }
        line += w;
        rect += s;
    }
}

void b8_rect_2_linear_align_4(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=4) {
            *((uint32_t*)(line+i)) = *((uint32_t*)(rect+i));
        }
        line += w;
        rect += s;
    }
}

void b8_rect_2_linear_align_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=8) {
            *((uint64_t*)(line+i)) = *((uint64_t*)(rect+i));
        }
        line += w;
        rect += s;
    }
}

void b8_rect_2_linear_width_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        *((uint64_t*)(line)) = *((uint64_t*)(rect));
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect(int dir, uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    void (*map_func_p)(uint8_t* line, uint8_t* rect, int w, int h, int s);
    int b_l2r = (dir==LINE2RECT);
    
    if        ( ((int)line&3) || (w&3) || ((int)rect&3) || (s&3) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_0 : b8_rect_2_linear_align_0;
    } else if ( ((int)line&7) || (w&7) || ((int)rect&7) || (s&7) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_4 : b8_rect_2_linear_align_4;
    } else if ( w == 8 )  {
        map_func_p = b_l2r ? b8_linear_2_rect_width_8 : b8_rect_2_linear_width_8; 
    } else {
        map_func_p = b_l2r ? b8_linear_2_rect_align_8 : b8_rect_2_linear_align_8;
    }
    
    map_func_p(line, rect, w, h, s);
}

void b8_tile_2_rect
(
    int b_t2r, 
    uint8_t* pt, int tw, int th, int tsz, int ts, 
    uint8_t* pl, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    void (*map_func_p)(uint8_t* line, uint8_t* rect, int w, int h, int s);
    int b_l2r = (b_t2r == TILE2RECT);

    if        ( ((int)pt&3) || (tw&3) || (tsz&3) || (ts&3) || ((int)pl&3) || (s&3) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_0 : b8_rect_2_linear_align_0;
    } else if ( ((int)pl&7) || (tw&7) || (tsz&7) || (ts&7) || ((int)pl&7) || (s&7) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_4 : b8_rect_2_linear_align_4;
    } else if ( tw == 8 ) {
        map_func_p = b_l2r ? b8_linear_2_rect_width_8 : b8_rect_2_linear_width_8;  
    } else {
        map_func_p = b_l2r ? b8_linear_2_rect_align_8 : b8_rect_2_linear_align_8;    
    } 
    
    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* src = &pt[ts*ty + tsz*tx];
            uint8_t* dst = &pl[s * y + x];

            map_func_p(src, dst, tw, th, s);
        }
    } 
    
    return;
}

void b8_tile_2_rect_mch(yuv_seq_t *tile, yuv_seq_t *rect, int b_t2r)
{
    int fmt = tile->yuvfmt;
    
    int tw  = tile->tile.tw;
    int th  = tile->tile.th;
    int tsz = tile->tile.tsz;
    int ts  = tile->y_stride;
    int w   = rect->width;
    int h   = rect->height;
    int s   = rect->y_stride;
    uint8_t *pt = tile->pbuf;
    uint8_t *pl = rect->pbuf;
    
    ENTER_FUNC;
    
    assert (tile->nbit == 8);
    assert (rect->nbit == 8);
    assert (tile->width == rect->width);
    assert (tile->height == rect->height);
    
    if (fmt == YUVFMT_400P) {
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += tile->uv_size;
        pl += rect->uv_size;
        
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (is_semi_planar(fmt))
    {
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;

        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return;
}

/**
 * unpack 10-bit compact pixels to 16 (low 10-bit)
 *      - i32_0 is used for bitop
 *      - i32_1 is used for reading 
 *
 *   high (10-rbit)|00000000000000|- rbit -| 
 *    bit      high------------------------> low
 *    byte         |  3  |  2  |  1  |  0  | 
 *  
 *                  high (10-rbit)|00000000|- (32-rbit) --|
 *    bit      high------------------------> low
 *  
 */
void b10_linear_unpack_lte(void* b10_base, int n_byte, void* b16_base, int n16)
{
    register i64_pack_t bp;
    
    bp.i64   = 0;
    uint16_t v16;
    int rbit = 0;
    int i32  = 0;
    int n32  = n_byte >> 2;
    int i16  = 0;
    uint32_t* pb10 = (uint32_t*)b10_base;
    uint16_t* pb16 = (uint16_t*)b16_base;

    //assert( (n_byte & 0x08) == 0 );
    //assert( n_byte > (n16*5+3)/4 );
    
    while (1) 
    {
        if (rbit<10) {
            if (i32 < n32 && i16 < n16) {
                bp.i32_1 = pb10[i32++];
            } else {
                break;
            }
            v16  = (bp.i32_0 & 0x3ff);  bp.i64 >>= (32-rbit);   // low   rbit
            v16 += (bp.i32_0 & 0x3ff);  bp.i64 >>= 10;          // high (10-rbit)
            rbit += 22;                                         // rbit=32-(10-rbit)
        } else {
            v16 = (bp.i32_0 & 0x3ff);   bp.i64 >>= 10; 
            rbit -= 10;
        }
        pb16[i16++] = v16;
    }
}

/**
 * pack low 10-bit in 16 to 10-bit compact format
 *
 *                 |xxxxxxxxxxxxxx|- rbit -| 
 *    bit      high------------------------> low
 *    byte         |  3  |  2  |  1  |  0  | 
 *  
 */
void b10_linear_pack_lte(void* b10_base, int n_byte, void* b16_base, int n16)
{
    uint32_t v16 = 0;
    uint32_t v32 = 0;
    int rbit = 0;
    int i32  = 0;
    int n32  = n_byte >> 2;
    int i16  = 0;
    uint32_t* pb10 = (uint32_t*)b10_base;
    uint16_t* pb16 = (uint16_t*)b16_base;
    
    //assert( (n_byte & 0x08) == 0 );
    //assert( n_byte > (n16*5+3)/4 );
    
    while (i32 < n32 && i16 < n16) 
    {
        v16 = pb16[i16++];
        
        if (rbit>=22) {
            v32  += (v16 & 0x3ff) << rbit;                      // low (32-rbit)
            pb10[i32++] = v32;
            v32   = (v16 & 0x3ff) >> (32-rbit);                 // high 10-(32-rbit)
            rbit -= 22;                                         // rbit=10-(32-rbit)
        } else {
            v32  += (v16 & 0x3ff) << rbit;
            rbit += 10;
        }
    }
}

void b10_rect_unpack
(
    int   b_pack,
    void* b10_base, int b10_stride,
    void* b16_base, int b16_stride,
    int   rect_w,   int rect_h
)
{
    int x, y;
    void (*b10_pack_unpack_fp)(void* b10_base, int n_byte, void* b16_base, int n16);
    
    b10_pack_unpack_fp = (b_pack == B16_2_B10) ? b10_linear_pack_lte : b10_linear_unpack_lte;
    
    for (y=0; y<rect_h; ++y) 
    {
        b10_pack_unpack_fp(b10_base, b10_stride, b16_base, rect_w);
        b10_base += b10_stride;
        b16_base += b16_stride;
    }
    
    return;
}

int b10_rect_unpack_mch(yuv_seq_t *rect10, yuv_seq_t *rect16, int b_pack)
{
    int fmt = rect10->yuvfmt;
    
    uint8_t* b10_base   = rect10->pbuf; 
    uint8_t* b16_base   = rect16->pbuf; 
    int b10_stride  = rect10->y_stride; 
    int b16_stride  = rect16->y_stride;
    int w   = rect10->width; 
    int h   = rect10->height; 

    ENTER_FUNC;
    
    assert (rect10->nbit == 10);
    assert (rect16->nbit == 16);
    assert (rect16->nlsb == 10);
    assert (rect10->width == rect16->width);
    assert (rect10->height == rect16->height);
    assert (rect10->yuvfmt == rect16->yuvfmt);
    
    if      (fmt == YUVFMT_400P)
    {
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
        
        b10_base   += rect10->y_size;
        b16_base   += rect16->y_size; 
        b10_stride  = rect10->uv_stride;
        b16_stride  = rect16->uv_stride;
        
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
        
        b10_base   += rect10->uv_size;
        b16_base   += rect16->uv_size;
        
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    else if (is_semi_planar(fmt))
    {
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
        
        b10_base   += rect10->y_size;
        b16_base   += rect16->y_size; 
        b10_stride  = rect10->uv_stride;
        b16_stride  = rect16->uv_stride;
        
        h   = is_mch_422(fmt) ? h : h/2;
        
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    
    LEAVE_FUNC;

    return;
}

int b10_tile_unpack
(
    int      b_pack,
    uint8_t* tile10_base, int tw, int th, int tsz, int ts, 
    uint8_t* rect16_base, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    
    assert( (tsz & 7) == 0 );
    assert( ts >= (w*th*5+3)/4 );
    
    #define UNPACK_BUF_SIZE 4094
    uint8_t  unpack_buf[UNPACK_BUF_SIZE];
    uint8_t *unpack_base = unpack_buf;
    int size_needed = tw * th * sizeof(uint16_t);
    if (size_needed > UNPACK_BUF_SIZE)
    {
        unpack_base = (uint8_t*) malloc ( size_needed );
        if (unpack_base==0) {
            printf("%s : malloc fail!\n", __FUNCTION__);
            return 0;
        }
    }
    
    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* p10 = &tile10_base[ts*ty + tsz*tx];
            uint8_t* p16 = &rect16_base[s * y + sizeof(uint16_t) * x];
            
            if (b_pack==B16_2_B10) {
                b8_linear_2_rect(RECT2LINE, unpack_base, p16, tw*2, th, s);
                b16_rect_transpose(unpack_base, tw, th);
                b10_linear_pack_lte(p10, tsz, unpack_base, tw*th);
            } else {
                b10_linear_unpack_lte(p10, tsz, unpack_base, tw*th);
                b16_rect_transpose(unpack_base, tw, th);
                b8_linear_2_rect(LINE2RECT, unpack_base, p16, tw*2, th, s);
            }
        }
    }
    
    if (size_needed>UNPACK_BUF_SIZE)
    {
        free(unpack_base);
    }
    
    return w*h;
}

int b10_tile_unpack_mch(yuv_seq_t *tile10, yuv_seq_t *rect16, int b_pack)
{
    int fmt = tile10->yuvfmt;

    ENTER_FUNC;
    
    int tw  = tile10->tile.tw;
    int th  = tile10->tile.th;
    int tsz = tile10->tile.tsz;
    int ts  = tile10->y_stride;
    int w   = rect16->width;
    int h   = rect16->height;
    int s   = rect16->y_stride;
    uint8_t *pt = tile10->pbuf;
    uint8_t *pl = rect16->pbuf;
    
    assert (tile10->btile == 1);
    assert (tile10->nbit == 10);
    assert (rect16->nbit == 16);
    assert (rect16->nlsb == 10);
    assert (tile10->width == rect16->width);
    assert (tile10->height == rect16->height);
    assert (tile10->yuvfmt == rect16->yuvfmt);
    
    if      (fmt == YUVFMT_400P)
    {
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile10->uv_stride;
        s   = rect16->uv_stride;
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        pt += tile10->y_size;
        pl += rect16->y_size;
        
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += tile10->uv_size;
        pl += rect16->uv_size;
        
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (is_semi_planar(fmt))
    {
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile10->uv_stride;
        s   = rect16->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;

        pt += tile10->y_size;
        pl += rect16->y_size;
        
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return 0;
}

void set_yuv_prop(yuv_seq_t *yuv, int w, int h, int fmt, 
                    int nbit, int nlsb, int btile, 
                    int stride, int io_size)
{
    yuv->width      = w;
    yuv->height     = h;
    yuv->yuvfmt     = fmt;
    yuv->nbit       = nbit;
    yuv->nlsb       = nlsb;
    yuv->btile      = btile;

    if (btile) 
    {
        tile_t *t = &yuv->tile; 
        if (yuv->nbit == 10) { 
            t->tw = 3; t->th = 4; t->tsz = 16; 
        } else 
        if (yuv->nbit == 8) { 
            t->tw = 8; t->th = 4; t->tsz = 32;
        } else {
            printf("@err>> not support bitdepth (%d) for tile mode\n", yuv->nbit);
            t->tw = 8; t->th = 4; t->tsz = 64;
        }
        
        yuv->y_stride = sat_div(w, t->tw) * t->tsz;
        yuv->y_stride = max(stride,  yuv->y_stride);
        
        yuv->y_size = yuv->y_stride * sat_div(h, t->th);
    } 
    else 
    {
        yuv->y_stride = sat_div(yuv->width * yuv->nbit, 8);
        yuv->y_stride = max(stride,  yuv->y_stride);
        
        yuv->y_size = yuv->y_stride * yuv->height;
    }
    
    if      (fmt == YUVFMT_400P)
    {
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    else if (fmt == YUVFMT_420P)
    {
        yuv->uv_stride  = yuv->y_stride / 2;
        yuv->uv_size    = yuv->y_size   / 4;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_420SP)
    {
        assert( is_bit_aligned(1, yuv->height) );
        
        yuv->uv_stride  = yuv->y_stride;
        yuv->uv_size    = yuv->y_size   / 2;
        yuv->io_size    = yuv->y_size + yuv->uv_size;
    }
    else if (fmt == YUVFMT_422P)
    {
        yuv->uv_stride  = yuv->y_stride / 2;
        yuv->uv_size    = yuv->y_size   / 2;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        yuv->y_stride   = yuv->y_stride * 2; 
        yuv->y_size     = yuv->y_size   * 2;
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    
    yuv->io_size = max(io_size, yuv->io_size);
    assert(yuv->buf_size >= yuv->io_size);
    
    void show_yuv_prop(yuv_seq_t *yuv);
    //show_yuv_prop(yuv);

    return;
}

void show_yuv_prop(yuv_seq_t *yuv)
{
    printf("\n");
    printf("width       = %d\n" , yuv->width     );
    printf("height      = %d\n" , yuv->height    );
    printf("yuvfmt      = %s\n" , show_fmt(yuv->yuvfmt));
    printf("nlsb        = %d\n" , yuv->nlsb      );
    printf("nbit        = %d\n" , yuv->nbit      );
    printf("btile       = %d\n" , yuv->btile     );
    printf("y_stride    = %d\n" , yuv->y_stride  );
    printf("uv_stride   = %d\n" , yuv->uv_stride );
    printf("y_size      = %d\n" , yuv->y_size    );
    printf("uv_size     = %d\n" , yuv->uv_size   );
    printf("io_size     = %d\n" , yuv->io_size   );
    printf("buf_size    = %d\n" , yuv->buf_size  );
    
    printf("tile.tw     = %d\n" , yuv->tile.tw   );
    printf("tile.th     = %d\n" , yuv->tile.th   );
    printf("tile.tsz    = %d\n" , yuv->tile.tsz  );
    printf("\n");
}

static int arg_init (yuv_cfg_t *cfg, int argc, char *argv[]);
static int arg_parse(yuv_cfg_t *cfg, int argc, char *argv[]);
static int arg_check(yuv_cfg_t *cfg, int argc, char *argv[]);
static int arg_help()
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

int main(int argc, char **argv)
{
    int         r, i;
    yuv_cfg_t   cfg;
    yuv_seq_t   seq[2];
    yuv_seq_t   *psrc = &seq[0];
    yuv_seq_t   *pdst = &seq[1];
    #define SWAP_SRC_DST()  do { yuv_seq_t *ptmp=psrc; psrc=pdst; pdst=ptmp; } while(0)
    
    memset(seq, 0, sizeof(seq));
    memset(&cfg, 0, sizeof(cfg));
    r = arg_parse(&cfg, argc, argv);
    if (r < 0) {
        arg_help();
        return 1;
    }
    r = arg_check(&cfg, argc, argv);
    if (r < 0) {
        printf("\n****src****\n");  show_yuv_prop(&cfg.src);
        printf("\n****dst****\n");  show_yuv_prop(&cfg.dst);
        return 1;
    }
    
    cfg.dst_fp = fopen(cfg.dst_path, "wb");
    cfg.src_fp = fopen(cfg.src_path, "rb");
    if( !cfg.dst_fp || !cfg.src_fp )
    {
        printf("error : open %s %s fail\n", 
                cfg.dst_fp ? "" : cfg.dst_path, 
                cfg.src_fp ? "" : cfg.src_path);
        return -1;
    }

    for (i=0; i<2; ++i) {
        seq[i].width = bit_sat(6, cfg.src.width);
        seq[i].height = bit_sat(6, cfg.src.height);
        seq[i].buf_size = seq[i].width * seq[i].height * 4 * 2;
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
        
        /**
         *  read one frame
         */
        set_yuv_prop(pdst, cfg.src.width, cfg.src.height, cfg.src.yuvfmt, 
                cfg.src.nbit, cfg.src.nlsb, cfg.src.btile, 
                cfg.src.y_stride, cfg.src.io_size);
        
        r=fseek(cfg.src_fp, pdst->io_size * i, SEEK_SET);
        if (r) {
            printf("fseek %d error\n", pdst->io_size * i);
            return -1;
        }
        r = fread(pdst->pbuf, pdst->io_size, 1, cfg.src_fp);
        if (r<1) {
            if ( feof(cfg.src_fp) ) {
                printf("reach file end, force stop\n");
            } else {
                printf("error reading file\n");
            }
            break;
        }
        
        /**
         *  b10-untile/unpack, b8-untile
         */
        if (cfg.src.nbit==10) 
        {
            SWAP_SRC_DST();
            set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                    cfg.src.yuvfmt, BIT_16, BIT_10, TILE_0, 0, 0);
            if (cfg.src.btile) {
                b10_tile_unpack_mch(psrc, pdst, B10_2_B16);
            } else {
                b10_rect_unpack_mch(psrc, pdst, B10_2_B16);
            }
        } 
        else if (cfg.src.nbit==8)
        {
            if (cfg.src.btile) {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.src.yuvfmt, BIT_8, BIT_8, TILE_0, 0, 0);
                b8_tile_2_rect_mch(psrc, pdst, TILE2RECT);
            }
        }

        /**
         *  bit-shift
         */
        if (pdst->nbit != cfg.dst.nbit) {
            if (pdst->nbit==16 && cfg.dst.nbit==8) {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.src.yuvfmt, BIT_8, BIT_8, TILE_0, 0, 0);
                b16_n_b8_cvt_mch(psrc, pdst, B16_2_B8);
            } 
            else if (pdst->nbit==8 && cfg.dst.nbit>8) {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.src.yuvfmt, BIT_16, cfg.dst.nbit, TILE_0, 0, 0);
                pdst->nlsb = cfg.dst.nlsb;
                b16_n_b8_cvt_mch(pdst, psrc, B8_2_B16);
            }
        } else if (cfg.dst.nbit == 16) {
            if (pdst->nlsb != cfg.dst.nlsb) {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.src.yuvfmt, BIT_16, BIT_16, TILE_0, 0, 0);
                b16_mch_scale(psrc, pdst);
            } 
        }

        /**
         * fmt convertion. TODO: nbit==BIT_16 support
         */        
        if (cfg.src.yuvfmt != cfg.dst.yuvfmt) 
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
            if (cfg.src.yuvfmt != get_spl_fmt(cfg.src.yuvfmt)) { 
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        get_spl_fmt(cfg.src.yuvfmt), nbit, nlsb, TILE_0, 0, 0);
                if (is_semi_planar(cfg.src.yuvfmt)) {
                    mch_sp2p(psrc, pdst, SPLITTING);
                } else if (cfg.src.yuvfmt == YUVFMT_UYVY || cfg.src.yuvfmt == YUVFMT_YUYV) {
                    mch_yuyv2p(psrc, pdst, SPLITTING);
                }
            }
            
            // uv re-sample
            if (get_spl_fmt(cfg.src.yuvfmt) != get_spl_fmt(cfg.dst.yuvfmt))
            {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        get_spl_fmt(cfg.dst.yuvfmt), nbit, nlsb, TILE_0, 0, 0);
                mch_p2p(psrc, pdst); 
            }

            // uv interlace
            if (cfg.dst.yuvfmt != get_spl_fmt(cfg.dst.yuvfmt)) {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.dst.yuvfmt, nbit, nlsb, TILE_0, 0, 0);
                if (is_semi_planar(cfg.dst.yuvfmt)) {
                    mch_sp2p(pdst, psrc, INTERLACING);
                } else if (cfg.dst.yuvfmt == YUVFMT_UYVY  || cfg.dst.yuvfmt == YUVFMT_YUYV ) {
                    mch_yuyv2p(pdst, psrc, INTERLACING);
                }
            }
        }

        /**
         *  b10-tile/pack, b8-tile
         */
        if (cfg.dst.nbit==10) 
        {
            SWAP_SRC_DST();
            if (cfg.dst.btile) {
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.dst.yuvfmt, BIT_10, BIT_10, TILE_1, 0, 0);
                b10_tile_unpack_mch(pdst, psrc, B16_2_B10);
            } else {
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.dst.yuvfmt, BIT_10, BIT_10, TILE_0, 0, 0);
                b10_rect_unpack_mch(pdst, psrc, B16_2_B10);
            }
        }
        else if (cfg.dst.nbit==8)
        {
            if (cfg.dst.btile) {
                SWAP_SRC_DST();
                set_yuv_prop(pdst, cfg.src.width, cfg.src.height, 
                        cfg.dst.yuvfmt, BIT_8, BIT_8, TILE_1,
                        cfg.dst.y_stride, cfg.dst.io_size);
                b8_tile_2_rect_mch(pdst, psrc, RECT2TILE);
            }
        }
        else if (cfg.dst.nbit==16)
        {
            //TODO: 
        }
        
        r = fwrite(pdst->pbuf, pdst->io_size, 1, cfg.dst_fp);
      
        if (r<1) {
            printf("error writing file\n");
            break;
        }
    } // end frame loop
    
    for (i=0; i<2; ++i) {
        if (seq[i].pbuf)    free(seq[i].pbuf);
    }
    
    if (cfg.dst_fp)     fclose(cfg.dst_fp);
    if (cfg.src_fp)     fclose(cfg.src_fp);

    return 0;
}

#define GET_ARGV(idx, name) get_argv(argc, argv, idx, name)
static char *get_argv(int argc, char *argv[], int i, char *name)
{
    int s = i<argc ? argv[i][0] : 0;
    char *arg = (s != 0 && s != '-') ? argv[i] : 0;
    if (name) {
        printf("@cmdl>> get_argv[%s]=%s\n", name, arg?arg:"");
    }
    return arg;
}

static char* get_uint32 (char *str, uint32_t *out)
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

static int arg_parse_wxh(int i, int argc, char *argv[], yuv_seq_t *seq)
{
    int j;
    char *flag=0;
    char *last=0;
    
    char *arg = GET_ARGV(++ i, "wxh");
    if (!arg) {
        return -1;
    }
    
    for (j=0; j<ARRAY_SIZE(cmn_res); ++j) {
        if (0==strcmp(arg, cmn_res[j].name)) {
            seq->width  = cmn_res[j].w;  
            seq->height = cmn_res[j].h;
            return ++i;
        }
    }
    
    //seq->width = strtoul (arg, &flag, 10);
    flag = get_uint32 (arg, &seq->width);
    if (flag==0 || *flag != 'x') {
        printf("@cmdl>> Err : not (%%d)x(%%d)\n");
        return -1;
    }
    
    //seq->height = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, &seq->height);
    if (last == 0 || *last != 0 ) {
        printf("@cmdl>> Err : not (%%d)x(%%d)\n");
        return -1;
    }

    return -1;
}

static int arg_parse_fmt(int i, int argc, char *argv[], int *fmt)
{
    int j;
    
    char *arg = GET_ARGV(++ i, "yuvfmt");
    if (!arg) {
        return -1;
    }
    
    for (j=0; j<ARRAY_SIZE(cmn_fmt); ++j) {
        if (0==strcmp(arg, cmn_fmt[j].name)) {
            *fmt = cmn_fmt[j].ifmt;
            return ++i;
        }
    }
    
    printf("@cmdl>> Err : unrecognized yuvfmt `%s`\n", arg);
    return -1;
}

static int arg_parse_range(int i, int argc, char *argv[], int i_range[2])
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

static int arg_parse_str(int i, int argc, char *argv[], char **p)
{
    char *arg = GET_ARGV(++ i, "string");
    *p = arg ? arg : 0;
    return arg ? ++i : -1;
}

static int arg_parse_int(int i, int argc, char *argv[], int *p)
{
    char *arg = GET_ARGV(++ i, "int");
    *p = arg ? atoi(arg) : (-1);
    return arg ? ++i : -1;
}

static int arg_parse(yuv_cfg_t *cfg, int argc, char *argv[])
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
        
        for (j=0; j<ARRAY_SIZE(cmn_res); ++j) {
            if (0==strcmp(arg, cmn_res[j].name)) {
                seq->width  = cmn_res[j].w;
                seq->height = cmn_res[j].h;
                break;
            }
        }
        if (j<ARRAY_SIZE(cmn_res)) {
            ++i; continue;
        }
        
        for (j=0; j<ARRAY_SIZE(cmn_fmt); ++j) {
            if (0==strcmp(arg, cmn_fmt[j].name)) {
                seq->yuvfmt = cmn_fmt[j].ifmt;
                break;
            }
        }
        if (j<ARRAY_SIZE(cmn_fmt)) {
            ++i; continue;
        }
        
        if (0==strcmp(arg, "h") || 0==strcmp(arg, "help")) {
            arg_help();
            return -1;
        } else
        if (0==strcmp(arg, "src")) {
            seq = &cfg->src;
            i = arg_parse_str(i, argc, argv, &cfg->src_path);
        } else
        if (0==strcmp(arg, "dst")) {
            seq = &cfg->dst;
            i = arg_parse_str(i, argc, argv, &cfg->dst_path);
        } else
        if (0==strcmp(arg, "wxh")) {
            i = arg_parse_wxh(i, argc, argv, seq);
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

static int arg_check(yuv_cfg_t *cfg, int argc, char *argv[])
{
    yuv_seq_t *psrc = &cfg->src;
    yuv_seq_t *pdst = &cfg->dst;
    
    ENTER_FUNC;
    
    if (!cfg->src_path || !cfg->dst_path) {
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
