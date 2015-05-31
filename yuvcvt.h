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

#ifndef __YUVCVT_H__
#define __YUVCVT_H__

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

extern const res_t cmn_res[];
extern const int n_cmn_res;
extern const opt_enum_t cmn_fmt[];
extern const int n_cmn_fmt;
const char* show_fmt(int ifmt);
int arg_parse_wxh(int i, int argc, char *argv[], int *pw, int *ph);
int arg_parse_fmt(int i, int argc, char *argv[], int *fmt);

enum cvt_ios_channel {
    CVT_IOS_DST = 0,
    CVT_IOS_SRC = 1,
    CVT_IOS_CNT,
};

typedef struct _yuv_cvt_opt
{
    ios_t   ios[2];
    int     frame_range[2];

    yuv_seq_t   src;
    yuv_seq_t   dst;
    
} cvt_opt_t;

yuv_seq_t *yuv_cvt_frame(yuv_seq_t *pdst, yuv_seq_t *psrc);

int cvt_arg_init (cvt_opt_t *cfg, int argc, char *argv[]);
int cvt_arg_parse(cvt_opt_t *cfg, int argc, char *argv[]);
int cvt_arg_check(cvt_opt_t *cfg, int argc, char *argv[]);
int cvt_arg_help();

int yuv_cvt(int argc, char **argv);


int yuv_copy_rect(int w, int h, uint8_t *dst, int dst_stride, 
                                uint8_t *src, int src_stride);
int yuv_copy_frame(yuv_seq_t *pdst, yuv_seq_t *psrc);

#endif  // __YUVCVT_H__