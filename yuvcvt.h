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

typedef struct _yuv_cvt_opt
{
    int     frame_range[2];

    yuv_seq_t   src;
    yuv_seq_t   dst;
    
} cvt_opt_t;

int cvt_arg_init (cvt_opt_t *cfg, int argc, char *argv[]);
int cvt_arg_parse(cvt_opt_t *cfg, int argc, char *argv[]);
int cvt_arg_check(cvt_opt_t *cfg, int argc, char *argv[]);
int cvt_arg_help();

int yuv_cvt(int argc, char **argv);


#endif  // __YUVCVT_H__