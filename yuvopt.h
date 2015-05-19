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

#ifndef __YUVOPT_H__
#define __YUVOPT_H__

typedef struct resolution {
    const char *name; 
    int         w, h;
} res_t;

typedef struct yuvformat {
    const char *name; 
    int         ifmt;
} fmt_t;

extern const res_t cmn_res[];
extern const int n_cmn_res;
extern const fmt_t cmn_fmt[];
extern const int n_cmn_fmt;
const char* show_fmt(int ifmt);


#define GET_ARGV(idx, name) get_argv(argc, argv, idx, name);
char *get_argv(int argc, char *argv[], int i, char *name);
char* get_uint32 (char *str, uint32_t *out);

int arg_parse_wxh(int i, int argc, char *argv[], int *pw, int *ph);
int arg_parse_fmt(int i, int argc, char *argv[], int *fmt);
int arg_parse_range(int i, int argc, char *argv[], int i_range[2]);
int arg_parse_str(int i, int argc, char *argv[], char **p);
int arg_parse_int(int i, int argc, char *argv[], int *p);


#endif  // __YUVOPT_H__