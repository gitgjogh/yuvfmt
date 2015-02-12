#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<assert.h>

#include "yuvfmt.h"

static int fcall_layer = 0;
#define ENTER_FUNC  printf("%-2d: %s +++\n", fcall_layer++, __FUNCTION__)
#define LEAVE_FUNC  printf("%-2d: %s ---\n", --fcall_layer, __FUNCTION__)

inline int sat_div(int num, int den)
{
    return (num + den - 1)/den;
}

int bit_saturate(int nbit, int val)
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
int is_mch_pl(int fmt)
{
    return (fmt == YUVFMT_420P 
            || fmt == YUVFMT_422P) ? 1 : 0;
}

inline
int is_mch_sp(int fmt)
{
    return (fmt == YUVFMT_420SP
            || fmt == YUVFMT_420SPA
            || fmt == YUVFMT_422SP
            || fmt == YUVFMT_422SPA) ? 1 : 0;
}

inline
int is_mono_pl(int fmt)
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

void b8_linear_2_rect_align_0(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; ++i) {
            *((uint8_t*)(rect+i)) = *((uint8_t*)(tile+i));
        }
        tile += tw;
        rect += s;
    }
}

void b8_linear_2_rect_align_4(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; i+=4) {
            *((uint32_t*)(rect+i)) = *((uint32_t*)(tile+i));
        }
        tile += tw;
        rect += s;
    }
}

void b8_linear_2_rect_align_8(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; i+=8) {
            *((uint64_t*)(rect+i)) = *((uint64_t*)(tile+i));
        }
        tile += tw;
        rect += s;
    }
}

void b8_linear_2_rect_width_8(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        *((uint64_t*)(rect)) = *((uint64_t*)(tile));
        tile += tw;
        rect += s;
    }
}

void b8_linear_2_rect(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    if      ( ((int)tile&3) || (tw&3) || ((int)rect&3) || (s&3) )
        b8_linear_2_rect_align_0(tile, tw, th, rect, s);
    else if ( ((int)tile&7) || (tw&7) || ((int)rect&7) || (s&7) )
        b8_linear_2_rect_align_4(tile, tw, th, rect, s);
    else if ( tw == 8 )
        b8_linear_2_rect_width_8(tile, tw, th, rect, s); 
    else
        b8_linear_2_rect_align_8(tile, tw, th, rect, s);   
}

void b8_tile_2_rect
(
    uint8_t* pt, int tw, int th, int tsz, int ts, 
    uint8_t* pl, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    void (*tile2rect_func_p)(uint8_t* src, int tw, int th, uint8_t* dst, int s);
    
    ENTER_FUNC;

    if      ( ((int)pt&3) || (tw&3) || (tsz&3) || (ts&3) || ((int)pl&3) || (s&3) )
        tile2rect_func_p = b8_linear_2_rect_align_0;
    else if ( ((int)pl&7) || (tw&7) || (tsz&7) || (ts&7) || ((int)pl&7) || (s&7) )
        tile2rect_func_p = b8_linear_2_rect_align_4;
    else if ( tw == 8 )
        tile2rect_func_p = b8_linear_2_rect_width_8; 
    else
        tile2rect_func_p = b8_linear_2_rect_align_8;     

    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* src = &pt[ts*ty + tsz*tx];
            uint8_t* dst = &pl[s * y + x];

            tile2rect_func_p(src, tw, th, dst, s);
        }
    } 
    
    LEAVE_FUNC;
}

void b8_tile_2_rect_mch(yuv_seq_t *tile, yuv_seq_t *rect)
{
    int fmt = tile->yuvfmt;
    
    int tw  = tile->tile.tw;
    int th  = tile->tile.th;
    int tsz = tile->tile.tsz;
    int ts  = tile->y_stride;
    int w   = rect->w_align;
    int h   = rect->h_align;
    int s   = rect->y_stride;
    uint8_t *pt = tile->pbuf;
    uint8_t *pl = rect->pbuf;
    
    ENTER_FUNC;
    
    assert (tile->b10     == rect->b10);
    assert (tile->w_align == rect->w_align);
    assert (tile->h_align == rect->h_align);
    
    if (fmt == YUVFMT_400P) {
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += tile->uv_size;
        pl += rect->uv_size;
        
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (is_mch_sp(fmt))
    {
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;

        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b8_tile_2_rect(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return;
}

void b8_rect_2_linear_align_0(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; ++i) {
            *((uint8_t*)(tile+i)) = *((uint8_t*)(rect+i));
        }
        tile += tw;
        rect += s;
    }
}

void b8_rect_2_linear_align_4(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; i+=4) {
            *((uint32_t*)(tile+i)) = *((uint32_t*)(rect+i));
        }
        tile += tw;
        rect += s;
    }
}

void b8_rect_2_linear_align_8(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; i+=8) {
            *((uint64_t*)(tile+i)) = *((uint64_t*)(rect+i));
        }
        tile += tw;
        rect += s;
    }
}

void b8_rect_2_linear_width_8(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        *((uint64_t*)(tile)) = *((uint64_t*)(rect));
        tile += tw;
        rect += s;
    }
}

void b8_rect_2_linear(uint8_t* tile, int tw, int th, uint8_t* rect, int s)
{
    if      ( ((int)tile&3) || (tw&3) || ((int)rect&3) || (s&3) )
        b8_rect_2_linear_align_0(tile, tw, th, rect, s);
    else if ( ((int)tile&7) || (tw&7) || ((int)rect&7) || (s&7) )
        b8_rect_2_linear_align_4(tile, tw, th, rect, s);
    else if ( tw == 8 )
        b8_rect_2_linear_width_8(tile, tw, th, rect, s); 
    else
        b8_rect_2_linear_align_8(tile, tw, th, rect, s);   
}

void b8_rect_2_tile
(
    uint8_t* pt, int tw, int th, int tsz, int ts, 
    uint8_t* pl, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    void (*rect2tile_func_p)(uint8_t* src, int tw, int th, uint8_t* dst, int s);
    
    ENTER_FUNC;

    if      ( ((int)pt&3) || (tw&3) || (tsz&3) || (ts&3) || ((int)pl&3) || (s&3) )
        rect2tile_func_p = b8_rect_2_linear_align_0;
    else if ( ((int)pl&7) || (tw&7) || (tsz&7) || (ts&7) || ((int)pl&7) || (s&7) )
        rect2tile_func_p = b8_rect_2_linear_align_4;
    else if ( tw == 8 )
        rect2tile_func_p = b8_rect_2_linear_width_8; 
    else
        rect2tile_func_p = b8_rect_2_linear_align_8;     

    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* tile = &pt[ts*ty + tsz*tx];
            uint8_t* rect = &pl[s * y + x];

            rect2tile_func_p(tile, tw, th, rect, s);
        }
    } 
    
    LEAVE_FUNC;
}

void b8_rect_2_tile_mch(yuv_seq_t *tile, yuv_seq_t *rect)
{
    int fmt = tile->yuvfmt;
    
    int tw  = tile->tile.tw;
    int th  = tile->tile.th;
    int tsz = tile->tile.tsz;
    int ts  = tile->y_stride;
    int w   = rect->w_align;
    int h   = rect->h_align;
    int s   = rect->y_stride;
    uint8_t *pt = tile->pbuf;
    uint8_t *pl = rect->pbuf;
    
    ENTER_FUNC;
    
    assert (tile->b10     == rect->b10);
    assert (tile->w_align == rect->w_align);
    assert (tile->h_align == rect->h_align);
    
    if (fmt == YUVFMT_400P) {
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += tile->uv_size;
        pl += rect->uv_size;
        
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (is_mch_sp(fmt))
    {
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;

        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b8_rect_2_tile(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return;
}

/**
 * little endian: 
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
void b10_linear_unpack(void* b10_base, int n_byte, void* b16_base, int n16)
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
 * little endian: 
 *
 *   high (10-rbit)|xxxxxxxxxxxxxx|- rbit -| 
 *    bit      high------------------------> low
 *    byte         |  3  |  2  |  1  |  0  | 
 *  
 *                  high (10-rbit)|00000000|- (32-rbit) --|
 *    bit      high------------------------> low
 *  
 */
void b10_linear_pack(void* b10_base, int n_byte, void* b16_base, int n16)
{
    uint32_t v16 = 0;
    uint32_t v32 = 0;
    int rbit = 32;
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
        
        if (rbit<10) {
            v32   = (v16 & 0x3ff) >> (10 - rbit);               // high rbit
            pb10[i32++] = v32;
            rbit += 22;                                         // rbit=32-(10-rbit)
            v32   = (v16 & 0x3ff) << rbit;                      // low (10-rbit)
        } else {
            v32  += (v16 & 0x3ff) << (rbit - 10);
            rbit -= 10;
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
    
    ENTER_FUNC;
    
    b10_pack_unpack_fp = b_pack ? b10_linear_pack : b10_linear_unpack;
    
    for (y=0; y<rect_h; ++y) 
    {
        b10_pack_unpack_fp(b10_base, b10_stride, b16_base, rect_w);
        b10_base += b10_stride;
        b16_base += b16_stride;
    }
    
    LEAVE_FUNC;
}

int b10_rect_unpack_mch(yuv_seq_t *rect10, yuv_seq_t *rect16, int b_pack)
{
    int fmt = rect10->yuvfmt;
    
    uint8_t* b10_base   = rect10->pbuf; 
    uint8_t* b16_base   = rect16->pbuf; 
    int b10_stride  = rect10->y_stride; 
    int b16_stride  = rect16->y_stride;
    int w   = rect10->w_align; 
    int h   = rect10->h_align; 

    ENTER_FUNC;
    
    assert (rect10->b10     == rect16->b10);
    assert (rect10->w_align == rect16->w_align);
    assert (rect10->h_align == rect16->h_align);
    
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
    else if (is_mch_sp(fmt))
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

int b10_tile_unpack
(
    int      b_pack,
    uint8_t* tile10_base, int tw, int th, int tsz, int ts, 
    uint8_t* rect16_base, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    
    ENTER_FUNC;
    
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
            
            if (b_pack) {
                b8_rect_2_linear(unpack_base, tw*2, th, p16, s);
                b16_rect_transpose(unpack_base, tw, th);
                b10_linear_pack(p10, tsz, unpack_base, tw*th);
            } else {
                b10_linear_unpack(p10, tsz, unpack_base, tw*th);
                b16_rect_transpose(unpack_base, tw, th);
                b8_linear_2_rect(unpack_base, tw*2, th, p16, s);
            }
        }
    }
    
    if (size_needed>UNPACK_BUF_SIZE)
    {
        free(unpack_base);
    }
    
    LEAVE_FUNC;
    
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
    int w   = rect16->w_align;
    int h   = rect16->h_align;
    int s   = rect16->y_stride;
    uint8_t *pt = tile10->pbuf;
    uint8_t *pl = rect16->pbuf;
    
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
    else if (is_mch_sp(fmt))
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

    return;
}

/**
 *  8bit, lama raster, not tiled
 */
int set_bufsz_aligned_b8(yuv_seq_t *yuv, int bufw, int bufh, int w_align_bit, int h_align_bit)
{
    int fmt = yuv->yuvfmt;

    ENTER_FUNC;
    
    bufw = bit_saturate(w_align_bit, bufw);
    bufh = bit_saturate(h_align_bit, bufh);
    assert( is_bit_aligned(1, bufw) );
    
    yuv->y_stride   = bufw;
    yuv->y_size     = bufw * bufh;
    
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
        assert( is_bit_aligned(1, bufh) );
        
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
    
    yuv->buf_size = bit_saturate(7, yuv->io_size);
    
    LEAVE_FUNC;

    return yuv->buf_size;
}

int set_bufsz_aligned(yuv_seq_t *yuv)
{
    ENTER_FUNC;

    set_bufsz_aligned_b8(yuv, yuv->w_align, yuv->h_align, 0, 0);
    
    if (yuv->b10) {
        yuv->y_stride   *= 2;
        yuv->y_size     *= 2;
        yuv->uv_stride  *= 2;
        yuv->uv_size    *= 2;
        yuv->io_size    *= 2;
        yuv->buf_size   = bit_saturate(8, yuv->io_size);
    }
    
    LEAVE_FUNC;
    
    return yuv->io_size;
}

/**
 *  luma raster
 */
int set_bufsz_src_raster(yuv_seq_t *yuv)
{
    int bufw = yuv->w_align;
    int bufh = yuv->h_align;
    
    ENTER_FUNC;

    if (yuv->b10) {
        bufw = bit_saturate(2, bufw * 5 / 4);
    }
    
    set_bufsz_aligned_b8(yuv, bufw, bufh, 0, 0);
    
    LEAVE_FUNC;
    
    return yuv->io_size;
}

int set_bufsz_src_tile(yuv_seq_t *yuv)
{
    ENTER_FUNC;
    
    int tw      = 0;
    int th      = 0;
    int tsz     = 0; 
    if (yuv->b10) { 
        tw = 3; th = 4; tsz = 16; 
    } else { 
        tw = 8; th = 4; tsz = 32; 
    }
    
    int fmt = yuv->yuvfmt;
    yuv->tile.tw    = tw;
    yuv->tile.th    = th;
    yuv->tile.tsz   = tsz;
    
    yuv->y_stride   = sat_div(yuv->w_align, tw) * tsz; 
    yuv->y_size     = sat_div(yuv->h_align, th) * yuv->y_stride;
    
    if      (fmt == YUVFMT_400P)
    {
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    else if (fmt == YUVFMT_420P)
    {
        yuv->uv_stride  = sat_div(yuv->w_align / 2, tw) * tsz;
        yuv->uv_size    = sat_div(yuv->h_align / 2, th) * yuv->uv_stride;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_420SP)
    {
        yuv->uv_stride  = yuv->y_stride; 
        yuv->uv_size    = sat_div(yuv->h_align / 2, th) * yuv->uv_stride; 
        yuv->io_size    = yuv->y_size + yuv->uv_size;
    }

    else if (fmt == YUVFMT_422P)  
    {
        yuv->uv_stride  = sat_div(yuv->w_align / 2, tw) * tsz;
        yuv->uv_size    = sat_div(yuv->h_align    , th) * yuv->uv_stride;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        yuv->y_stride   = sat_div(yuv->w_align * 2, tw) * tsz; 
        yuv->y_size     = sat_div(yuv->h_align * 2, th) * yuv->y_stride;
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    
    yuv->buf_size = bit_saturate(7, yuv->io_size);
    
    LEAVE_FUNC;
    
    return yuv->io_size;
}

void set_seq_info(yuv_seq_t *yuv, int w, int h, int fmt, int b10, int btile, int w_align_bit, int h_align_bit)
{
    ENTER_FUNC;
    
    yuv->width      = w;
    yuv->height     = h;
    yuv->w_align    = bit_saturate( w_align_bit, yuv->width  );
    yuv->h_align    = bit_saturate( h_align_bit, yuv->height );
    yuv->yuvfmt     = fmt;
    yuv->b10        = b10;
    yuv->btile      = btile;
    
    set_bufsz_aligned(yuv);

    LEAVE_FUNC;
}

void show_yuv_info(yuv_seq_t *yuv)
{
    printf("\n");
    printf("width       = %d\n" , yuv->width     );
    printf("height      = %d\n" , yuv->height    );
    printf("w_align     = %d\n" , yuv->w_align   );
    printf("h_align     = %d\n" , yuv->h_align   );
    printf("yuvfmt      = %d\n" , yuv->yuvfmt    );
    printf("b10         = %d\n" , yuv->b10       );
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

int b16_rect_clip_10to8
(
    uint8_t* src8, int src_stride, 
    uint8_t* dst8, int dst_stride,
    int w,  int h
)
{
    int x, y;

    for (y=0; y<h; ++y) 
    {
        uint16_t* src = (uint16_t*)(src8 + y * src_stride);
        uint8_t*  dst = (uint8_t* )(dst8 + y * dst_stride);
        
        for (x=0; x<w; ++x) 
        {
            //dst[x] = (uint8_t)(src[x] >> 2);
            *(dst++) = (uint8_t)(*(src++) >> 2);
        }
    }
    
    return w*h;
}

int b16_rect_clip_10to8_mch(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    uint8_t* src8   = psrc->pbuf; 
    uint8_t* dst8   = pdst->pbuf; 
    int src_stride  = psrc->y_stride; 
    int dst_stride  = pdst->y_stride;
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 

    ENTER_FUNC;
    
    assert (psrc->yuvfmt  == pdst->yuvfmt);
    assert (psrc->w_align == pdst->w_align);
    assert (psrc->h_align == pdst->h_align);
    
    if      (fmt == YUVFMT_400P)
    {
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
        
        src8       += psrc->y_size;
        dst8       += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
        
        src8 += psrc->uv_size;
        dst8 += pdst->uv_size;
        
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (is_mch_sp(fmt))
    {
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
        
        src8       += psrc->y_size;
        dst8       += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b16_rect_clip_10to8(src8, src_stride, dst8, dst_stride, w, h);
    }
    
    LEAVE_FUNC;

    return 0;
}

int b8mch_p2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;

    uint8_t* src_y_base = psrc->pbuf;
    uint8_t* src_u_base = src_y_base + psrc->y_size;
    uint8_t* src_v_base = src_u_base + psrc->uv_size;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;
    
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;
    
    for (y=0; y<h; ++y) {
        uint8_t* src_y = src_y_base + y * psrc->y_stride;
        uint8_t* dst_y = dst_y_base + y * pdst->y_stride;
        memcpy(dst_y, src_y, w);
    }
    
    if (fmt==YUVFMT_400P)
        return 0;
    
    w   = w/2;
    h   = is_mch_422(fmt) ? h : h/2;
    
    for (y=0; y<h; ++y) {
        uint8_t* src_u = src_u_base + y * psrc->uv_stride;
        uint8_t* src_v = src_v_base + y * psrc->uv_stride;
        uint8_t* dst_u = dst_u_base + y * pdst->uv_stride;
        uint8_t* dst_v = dst_v_base + y * pdst->uv_stride;
        
        memcpy(dst_u, src_u, w);
        memcpy(dst_v, src_v, w);
    }
    
    LEAVE_FUNC;
    
    return 0;
}

int b8mch_sp2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    uint8_t* src_y_base = psrc->pbuf;
    uint8_t* src_u_base = src_y_base + psrc->y_size;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;
    
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;
    
    for (y=0; y<h; ++y) {
        uint8_t* src_y = src_y_base + y * psrc->y_stride;
        uint8_t* dst_y = dst_y_base + y * pdst->y_stride;
        memcpy(dst_y, src_y, w);
    }

    w   = w/2;
    h   = is_mch_422(fmt) ? h : h/2;
    
    for (y=0; y<h; ++y) {
        uint8_t* src_u = src_u_base + y * psrc->uv_stride;
        uint8_t* dst_u = dst_u_base + y * pdst->uv_stride;
        uint8_t* dst_v = dst_v_base + y * pdst->uv_stride;

        for (x=0; x<w; ++x) {
            *(dst_u++) = *(src_u++);
            *(dst_v++) = *(src_u++);
        }
    }
    
    LEAVE_FUNC;
    
    return 0;
}

int b8mch_yuyv2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    uint8_t* src_y_base = psrc->pbuf;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;

    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;

    w   = w/2;

    for (y=0; y<h; ++y) 
    {
        uint8_t* src_y  = src_y_base + y * psrc->y_stride;
        uint8_t* dst_y  = dst_y_base + y * pdst->y_stride;
        uint8_t* dst_u  = dst_u_base + y * pdst->uv_stride;
        uint8_t* dst_v  = dst_v_base + y * pdst->uv_stride;

        if (psrc->yuvfmt == YUVFMT_YUYV) {
            for (x=0; x<w; ++x) {
                *(dst_y++) = *(src_y++);
                *(dst_u++) = *(src_y++);
                *(dst_y++) = *(src_y++);
                *(dst_v++) = *(src_y++);
            }
        } else {
            for (x=0; x<w; ++x) {
                *(dst_u++) = *(src_y++);
                *(dst_y++) = *(src_y++);
                *(dst_v++) = *(src_y++);
                *(dst_y++) = *(src_y++);
            }
        }
    }   /* end for y*/

    LEAVE_FUNC;
    
    return 0;
}

int b8mch_spliting(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    ENTER_FUNC;

    assert (psrc->b10     == pdst->b10);
    assert (psrc->w_align == pdst->w_align);
    assert (psrc->h_align == pdst->h_align);
    assert (pdst->yuvfmt  == get_spl_fmt(fmt));

    if (fmt == YUVFMT_400P || fmt == YUVFMT_420P  || fmt == YUVFMT_422P ) {
        b8mch_p2p(psrc, pdst);    
    } else if (is_mch_sp(fmt)) {
        b8mch_sp2p(psrc, pdst);
    } else if (fmt == YUVFMT_UYVY  || fmt == YUVFMT_YUYV ) {
        b8mch_yuyv2p(psrc, pdst);
    }
    
    LEAVE_FUNC;
    
    return 0;
}

int main(int argc, char **argv)
{
    int r;
    if (argc<=7)
    {
        printf("prog [1]dst.yuv [2]src.yuv [3]w<int> [4]h<int> [5]start<int> [6]nfrm<int> \n");
        printf("     [7]srcfmt<str>, [8]b10, [9]btile\n");
        return -1;
    } 

    yuv_seq_t   seq_src;
    yuv_seq_t   seq_b16;
    yuv_seq_t   seq_dst;
    yuv_seq_t   seq_spl;
    memset(&seq_src, 0, sizeof(yuv_seq_t));
    memset(&seq_b16, 0, sizeof(yuv_seq_t));
    memset(&seq_dst, 0, sizeof(yuv_seq_t));
    memset(&seq_spl, 0, sizeof(yuv_seq_t));
    
    seq_dst.fp = fopen(argv[1],"wb");
    seq_src.fp = fopen(argv[2],"rb");
    if( !seq_dst.fp || !seq_src.fp )
    {
        printf("error : open %s %s fail\n", seq_dst.fp?"":argv[1], seq_src.fp?"":argv[2]);
        return -1;
    }

    int w       = atoi(argv[3]);
    int h       = atoi(argv[4]);
    int start   = atoi(argv[5]);
    int nfrm    = atoi(argv[6]);
    
    printf("w=%d, h=%d, start=%d, nfrm=%d\n", w, h, start, nfrm);

    int fmt     = YUVFMT_420P;
    int b10     = 0;
    int btile   = 0;
    
    if (argc>=8) {
        if      ( !strcmp(argv[7], "420p" ) )  fmt = YUVFMT_420P;
        else if ( !strcmp(argv[7], "420sp") )  fmt = YUVFMT_420SP;
        else if ( !strcmp(argv[7], "422p" ) )  fmt = YUVFMT_422P;
        else if ( !strcmp(argv[7], "422sp") )  fmt = YUVFMT_422SP;
        else if ( !strcmp(argv[7], "uyvy" ) )  fmt = YUVFMT_UYVY;
        else if ( !strcmp(argv[7], "yuyv" ) )  fmt = YUVFMT_YUYV;
        else    { printf("error fmt\n");        return -1;  }
    }
    if (argc>=9)  { b10   = atoi(argv[8]); }
    if (argc>=10) { btile = atoi(argv[9]); }

    set_seq_info( &seq_src, w, h, fmt,  b10, btile, 3, 3);
    set_seq_info( &seq_dst, w, h, fmt,  0,      0 , 3, 3);
    
    if (btile)  set_bufsz_src_tile(&seq_src);
    else        set_bufsz_src_raster(&seq_src);
    
    printf("\n seq_src:\n");       show_yuv_info(&seq_src);
    printf("\n seq_dst:\n");       show_yuv_info(&seq_dst);

    seq_src.pbuf = (uint8_t *)malloc(seq_src.buf_size);
    seq_dst.pbuf = (uint8_t *)malloc(seq_dst.buf_size);
    if(!seq_src.pbuf || !seq_dst.pbuf)
    {
        printf("error: malloc %s %s fail\n", 
                seq_src.pbuf ? (free(seq_src.pbuf), "") : "src.pbuf", 
                seq_dst.pbuf ? (free(seq_dst.pbuf), "") : "dst.pbuf");
        return -1;
    }

    if (b10) 
    {  
        set_seq_info( &seq_b16, w, h, fmt, 1, 0, 3, 3 );
        printf("\n seq_b16:\n");    
        show_yuv_info(&seq_b16);

        seq_b16.pbuf = (uint8_t *)malloc(seq_b16.buf_size);
        if(!seq_b16.pbuf)
        {
            printf("error: malloc %s fail\n", "seq_b16.pbuf");
            return -1;
        }
    } 
    //if (fmt != YUVFMT_420P && fmt != YUVFMT_422P ) 
    {  
        set_seq_info( &seq_spl, w, h, get_spl_fmt(fmt), 0, 0, 3, 3 );
        printf("\n seq_spl:\n");        
        show_yuv_info(&seq_spl);

        seq_spl.pbuf = (uint8_t *)malloc(seq_spl.buf_size);
        if(!seq_spl.pbuf)
        {
            printf("error: malloc %s fail\n", "seq_spl.pbuf");
            return -1;
        }
    }      

    int i;
    for (i=0; i<nfrm; i++) 
    {
        int r=fseek(seq_src.fp, seq_src.io_size * i, SEEK_SET);
        if (r) {
            printf("fseek %d error\n", seq_src.io_size * i);
            return -1;
        }
        r = fread(seq_src.pbuf, seq_src.io_size, 1, seq_src.fp);
        if (r<1) {
            if ( feof(seq_src.fp) ) {
                printf("reach file end, force stop\n");
            } else {
                printf("error reading file\n");
            }
            break;
        }

        if (b10) {
            if (btile) {
                b10_tile_unpack_mch(&seq_src, &seq_b16, 0);
            } else {
                b10_rect_unpack_mch(&seq_src, &seq_b16, 0);
            }
            
/*
            seq_b16.fp = fopen("seq_b16.yuv","wb");
            r = fwrite(seq_b16.pbuf, seq_b16.io_size, 1, seq_b16.fp);
            if (r<1) {
                printf("error writing file\n");
                break;
            }
            fclose(seq_b16.fp);
*/
            
            b16_rect_clip_10to8_mch(&seq_b16, &seq_dst);
            b8mch_spliting(&seq_dst, &seq_spl);
        } else {
            if (btile) {
                b8_tile_2_rect_mch(&seq_src, &seq_dst);
                b8mch_spliting(&seq_dst, &seq_spl);
            } else {
                b8mch_spliting(&seq_src, &seq_spl);
            }
        }
        
        r = fwrite(seq_spl.pbuf, seq_spl.io_size, 1, seq_dst.fp);
      
        if (r<1) {
            printf("error writing file\n");
            break;
        }
    }

    if(seq_src.pbuf)    free(seq_src.pbuf);
    if(seq_b16.pbuf)    free(seq_b16.pbuf);
    if(seq_dst.pbuf)    free(seq_dst.pbuf);
    if(seq_spl.pbuf)    free(seq_spl.pbuf);
    
    if(seq_src.fp)      fclose(seq_src.fp);
    if(seq_dst.fp)      fclose(seq_dst.fp);

    return 0;
}
