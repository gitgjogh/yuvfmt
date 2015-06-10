yuv format convertor
=====================

A YUV tool which (may/would) support format conversion during

	- 400P/420P/420SP/420ASP/422P/UYVY/YUVY
	- 8bit/10bit/16bit
	- tile/raster scan

LICENSE
-------

The Apache License 2.0 applies to all codes in this repository.

   Copyright 2014~2015 Jeff <ggjogh@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
INSTALL
-------

Under shell
>  $make

USAGE
-------

$ ./yuv cvt -h
yuv format convertor. Options:
         -i|-dst name<%s> {...props...}
         -o|-src name<%s> {...props...}
         -f   <%d~%d>

set yuv props as follow:
         [-wxh <%dx%d>]
         [-fmt <%420p,%420sp,%uyvy,%422p>]
         [-stride <%d>]
         [-iosize <%d>]  //frame buf size
         [-b10]
         [-btile|-tile|-t]

set frame range as follow:
         [-f-range|-f <%d~%d>]
         [-f-start    <%d>]
         [-n-frame|-n <%d>]

-wxh option can be short as follow:
         -%qcif = "-wxh  176x144 "
         -%cif  = "-wxh  352x288 "
         -%360  = "-wxh  640x360 "
         -%480  = "-wxh  720x480 "
         -%720  = "-wxh 1280x720 "
         -%1080 = "-wxh 1920x1080"
         -%2k   = "-wxh 1920x1080"
         -%1088 = "-wxh 1920x1088"
         -%2k+  = "-wxh 1920x1088"
         -%2160 = "-wxh 3840x2160"
         -%4k   = "-wxh 3840x2160"
         -%2176 = "-wxh 3840x2176"
         -%4k+  = "-wxh 3840x2176"

-fmt option can be short as follow:
         -%400p    = "-fmt 0 (%400p   )"
         -%420p    = "-fmt 1 (%420p   )"
         -%420sp   = "-fmt 2 (%420sp  )"
         -%420spa  = "-fmt 3 (%420spa )"
         -%422p    = "-fmt 4 (%422p   )"
         -%422sp   = "-fmt 5 (%422sp  )"
         -%422spa  = "-fmt 6 (%422spa )"
         -%uyvy    = "-fmt 7 (%uyvy   )"
         -%yuyv    = "-fmt 8 (%yuyv   )"
