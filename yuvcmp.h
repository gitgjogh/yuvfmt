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

#ifndef __YUVCMP_H__
#define __YUVCMP_H__

typedef struct _yuv_cmp_opt
{
    yuv_seq_t   seq[3];     /* src1,src2,diff */
    int         blksz;
    int     frame_range[2];
    
} cmp_opt_t;

typedef struct _diff_stat
{
    uint64_t    cnt;
    uint64_t    sad;
    uint64_t    ssd;
    
}dstat_t;


dstat_t b8_rect_diff(int w, int h, uint8_t *base[3], 
                     int stride[3], dstat_t *stat);
                     
dstat_t b16_rect_diff(int w, int h, uint8_t *base[3], 
                      int stride[3], dstat_t *stat);
                      
dstat_t yuv_diff(yuv_seq_t *seq1, yuv_seq_t *seq2, 
                 yuv_seq_t *diff, dstat_t *stat);
                 
int cmp_arg_init (cmp_opt_t *cfg, int argc, char *argv[]);
int cmp_arg_parse(cmp_opt_t *cfg, int argc, char *argv[]);
int cmp_arg_check(cmp_opt_t *cfg, int argc, char *argv[]);
int cmp_arg_help();

int yuv_cmp(int argc, char **argv);


#endif  // __YUVCMP_H__