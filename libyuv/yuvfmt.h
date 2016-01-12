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

#ifndef __YUVFMT_H__
#define __YUVFMT_H__


typedef struct yuv_arg
{
    char       *path;
    yuv_seq_t   seq;
}
yuv_arg_t;

typedef struct _fmt_cvt_opt
{
    ios_t   ios[2];
    int     nframe;
    int     frame_range[2];

    yuv_arg_t   src;
    yuv_arg_t   dst;
    
} fmt_opt_t;

extern const opt_enum_t cmn_fmt[];
extern const int      n_cmn_fmt;

int fmt_arg_check(fmt_opt_t *cfg, int argc, char *argv[]);
int fmt_arg_help (fmt_opt_t *cfg, int argc, char *argv[]);

int yuv_fmt(int argc, char **argv);


#endif  // __YUVFMT_H__