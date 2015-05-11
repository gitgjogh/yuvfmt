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

int fmt_arg_parse(cvt_opt_t *cfg, int argc, char *argv[]);
int fmt_arg_check(cvt_opt_t *cfg, int argc, char *argv[]);
int fmt_arg_help();

int yuv_fmt(int argc, char **argv);


#endif  // __YUVFMT_H__