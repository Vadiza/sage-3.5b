/*
 * VLC constants for DV codec
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * VLC constants for DV codec.
 */

#ifndef AVCODEC_DV_VLC_DATA_H
#define AVCODEC_DV_VLC_DATA_H

#include <stdint.h>

#define NB_DV_VLC 409

/*
 * There's a catch about the following three tables: the mapping they establish
 * between (run, level) and vlc is not 1-1. So you have to watch out for that
 * when building misc. tables. E.g. (1, 0) can be either 0x7cf or 0x1f82.
 */
static const uint16_t dv_vlc_bits[NB_DV_VLC] = {
  0x0000, 0x0002, 0x0007, 0x0008, 0x0009, 0x0014, 0x0015, 0x0016,
  0x0017, 0x0030, 0x0031, 0x0032, 0x0033, 0x0068, 0x0069, 0x006a,
  0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x00e0, 0x00e1, 0x00e2,
  0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea,
  0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x01e0, 0x01e1, 0x01e2,
  0x01e3, 0x01e4, 0x01e5, 0x01e6, 0x01e7, 0x01e8, 0x01e9, 0x01ea,
  0x01eb, 0x01ec, 0x01ed, 0x01ee, 0x01ef, 0x03e0, 0x03e1, 0x03e2,
  0x03e3, 0x03e4, 0x03e5, 0x03e6, 0x07ce, 0x07cf, 0x07d0, 0x07d1,
  0x07d2, 0x07d3, 0x07d4, 0x07d5, 0x0fac, 0x0fad, 0x0fae, 0x0faf,
  0x0fb0, 0x0fb1, 0x0fb2, 0x0fb3, 0x0fb4, 0x0fb5, 0x0fb6, 0x0fb7,
  0x0fb8, 0x0fb9, 0x0fba, 0x0fbb, 0x0fbc, 0x0fbd, 0x0fbe, 0x0fbf,
  0x1f80, 0x1f81, 0x1f82, 0x1f83, 0x1f84, 0x1f85, 0x1f86, 0x1f87,
  0x1f88, 0x1f89, 0x1f8a, 0x1f8b, 0x1f8c, 0x1f8d, 0x1f8e, 0x1f8f,
  0x1f90, 0x1f91, 0x1f92, 0x1f93, 0x1f94, 0x1f95, 0x1f96, 0x1f97,
  0x1f98, 0x1f99, 0x1f9a, 0x1f9b, 0x1f9c, 0x1f9d, 0x1f9e, 0x1f9f,
  0x1fa0, 0x1fa1, 0x1fa2, 0x1fa3, 0x1fa4, 0x1fa5, 0x1fa6, 0x1fa7,
  0x1fa8, 0x1fa9, 0x1faa, 0x1fab, 0x1fac, 0x1fad, 0x1fae, 0x1faf,
  0x1fb0, 0x1fb1, 0x1fb2, 0x1fb3, 0x1fb4, 0x1fb5, 0x1fb6, 0x1fb7,
  0x1fb8, 0x1fb9, 0x1fba, 0x1fbb, 0x1fbc, 0x1fbd, 0x1fbe, 0x1fbf,
  0x7f00, 0x7f01, 0x7f02, 0x7f03, 0x7f04, 0x7f05, 0x7f06, 0x7f07,
  0x7f08, 0x7f09, 0x7f0a, 0x7f0b, 0x7f0c, 0x7f0d, 0x7f0e, 0x7f0f,
  0x7f10, 0x7f11, 0x7f12, 0x7f13, 0x7f14, 0x7f15, 0x7f16, 0x7f17,
  0x7f18, 0x7f19, 0x7f1a, 0x7f1b, 0x7f1c, 0x7f1d, 0x7f1e, 0x7f1f,
  0x7f20, 0x7f21, 0x7f22, 0x7f23, 0x7f24, 0x7f25, 0x7f26, 0x7f27,
  0x7f28, 0x7f29, 0x7f2a, 0x7f2b, 0x7f2c, 0x7f2d, 0x7f2e, 0x7f2f,
  0x7f30, 0x7f31, 0x7f32, 0x7f33, 0x7f34, 0x7f35, 0x7f36, 0x7f37,
  0x7f38, 0x7f39, 0x7f3a, 0x7f3b, 0x7f3c, 0x7f3d, 0x7f3e, 0x7f3f,
  0x7f40, 0x7f41, 0x7f42, 0x7f43, 0x7f44, 0x7f45, 0x7f46, 0x7f47,
  0x7f48, 0x7f49, 0x7f4a, 0x7f4b, 0x7f4c, 0x7f4d, 0x7f4e, 0x7f4f,
  0x7f50, 0x7f51, 0x7f52, 0x7f53, 0x7f54, 0x7f55, 0x7f56, 0x7f57,
  0x7f58, 0x7f59, 0x7f5a, 0x7f5b, 0x7f5c, 0x7f5d, 0x7f5e, 0x7f5f,
  0x7f60, 0x7f61, 0x7f62, 0x7f63, 0x7f64, 0x7f65, 0x7f66, 0x7f67,
  0x7f68, 0x7f69, 0x7f6a, 0x7f6b, 0x7f6c, 0x7f6d, 0x7f6e, 0x7f6f,
  0x7f70, 0x7f71, 0x7f72, 0x7f73, 0x7f74, 0x7f75, 0x7f76, 0x7f77,
  0x7f78, 0x7f79, 0x7f7a, 0x7f7b, 0x7f7c, 0x7f7d, 0x7f7e, 0x7f7f,
  0x7f80, 0x7f81, 0x7f82, 0x7f83, 0x7f84, 0x7f85, 0x7f86, 0x7f87,
  0x7f88, 0x7f89, 0x7f8a, 0x7f8b, 0x7f8c, 0x7f8d, 0x7f8e, 0x7f8f,
  0x7f90, 0x7f91, 0x7f92, 0x7f93, 0x7f94, 0x7f95, 0x7f96, 0x7f97,
  0x7f98, 0x7f99, 0x7f9a, 0x7f9b, 0x7f9c, 0x7f9d, 0x7f9e, 0x7f9f,
  0x7fa0, 0x7fa1, 0x7fa2, 0x7fa3, 0x7fa4, 0x7fa5, 0x7fa6, 0x7fa7,
  0x7fa8, 0x7fa9, 0x7faa, 0x7fab, 0x7fac, 0x7fad, 0x7fae, 0x7faf,
  0x7fb0, 0x7fb1, 0x7fb2, 0x7fb3, 0x7fb4, 0x7fb5, 0x7fb6, 0x7fb7,
  0x7fb8, 0x7fb9, 0x7fba, 0x7fbb, 0x7fbc, 0x7fbd, 0x7fbe, 0x7fbf,
  0x7fc0, 0x7fc1, 0x7fc2, 0x7fc3, 0x7fc4, 0x7fc5, 0x7fc6, 0x7fc7,
  0x7fc8, 0x7fc9, 0x7fca, 0x7fcb, 0x7fcc, 0x7fcd, 0x7fce, 0x7fcf,
  0x7fd0, 0x7fd1, 0x7fd2, 0x7fd3, 0x7fd4, 0x7fd5, 0x7fd6, 0x7fd7,
  0x7fd8, 0x7fd9, 0x7fda, 0x7fdb, 0x7fdc, 0x7fdd, 0x7fde, 0x7fdf,
  0x7fe0, 0x7fe1, 0x7fe2, 0x7fe3, 0x7fe4, 0x7fe5, 0x7fe6, 0x7fe7,
  0x7fe8, 0x7fe9, 0x7fea, 0x7feb, 0x7fec, 0x7fed, 0x7fee, 0x7fef,
  0x7ff0, 0x7ff1, 0x7ff2, 0x7ff3, 0x7ff4, 0x7ff5, 0x7ff6, 0x7ff7,
  0x7ff8, 0x7ff9, 0x7ffa, 0x7ffb, 0x7ffc, 0x7ffd, 0x7ffe, 0x7fff,
  0x0006,
};

static const uint8_t dv_vlc_len[NB_DV_VLC] = {
  2,  3,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,
  7,  7,  7,  7,  7,  8,  8,  8,
  8,  8,  8,  8,  8,  8,  8,  8,
  8,  8,  8,  8,  8,  9,  9,  9,
  9,  9,  9,  9,  9,  9,  9,  9,
  9,  9,  9,  9,  9, 10, 10, 10,
  10, 10, 10, 10, 11, 11, 11, 11,
  11, 11, 11, 11, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  4,
};

static const uint8_t dv_vlc_run[NB_DV_VLC] = {
  0,  0,  1,  0,  0,  2,  1,  0,
  0,  3,  4,  0,  0,  5,  6,  2,
  1,  1,  0,  0,  0,  7,  8,  9,
  10,  3,  4,  2,  1,  1,  1,  0,
  0,  0,  0,  0,  0, 11, 12, 13,
  14,  5,  6,  3,  4,  2,  2,  1,
  0,  0,  0,  0,  0,  5,  3,  3,
  2,  1,  1,  1,  0,  1,  6,  4,
  3,  1,  1,  1,  2,  3,  4,  5,
  7,  8,  9, 10,  7,  8,  4,  3,
  2,  2,  2,  2,  2,  1,  1,  1,
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55,
  56, 57, 58, 59, 60, 61, 62, 63,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  127,
};

static const uint8_t dv_vlc_level[NB_DV_VLC] = {
  1,   2,   1,   3,   4,   1,   2,   5,
  6,   1,   1,   7,   8,   1,   1,   2,
  3,   4,   9,  10,  11,   1,   1,   1,
  1,   2,   2,   3,   5,   6,   7,  12,
  13,  14,  15,  16,  17,   1,   1,   1,
  1,   2,   2,   3,   3,   4,   5,   8,
  18,  19,  20,  21,  22,   3,   4,   5,
  6,   9,  10,  11,   0,   0,   3,   4,
  6,  12,  13,  14,   0,   0,   0,   0,
  2,   2,   2,   2,   3,   3,   5,   7,
  7,   8,   9,  10,  11,  15,  16,  17,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,  10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,
  40,  41,  42,  43,  44,  45,  46,  47,
  48,  49,  50,  51,  52,  53,  54,  55,
  56,  57,  58,  59,  60,  61,  62,  63,
  64,  65,  66,  67,  68,  69,  70,  71,
  72,  73,  74,  75,  76,  77,  78,  79,
  80,  81,  82,  83,  84,  85,  86,  87,
  88,  89,  90,  91,  92,  93,  94,  95,
  96,  97,  98,  99, 100, 101, 102, 103,
  104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 124, 125, 126, 127,
  128, 129, 130, 131, 132, 133, 134, 135,
  136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 146, 147, 148, 149, 150, 151,
  152, 153, 154, 155, 156, 157, 158, 159,
  160, 161, 162, 163, 164, 165, 166, 167,
  168, 169, 170, 171, 172, 173, 174, 175,
  176, 177, 178, 179, 180, 181, 182, 183,
  184, 185, 186, 187, 188, 189, 190, 191,
  192, 193, 194, 195, 196, 197, 198, 199,
  200, 201, 202, 203, 204, 205, 206, 207,
  208, 209, 210, 211, 212, 213, 214, 215,
  216, 217, 218, 219, 220, 221, 222, 223,
  224, 225, 226, 227, 228, 229, 230, 231,
  232, 233, 234, 235, 236, 237, 238, 239,
  240, 241, 242, 243, 244, 245, 246, 247,
  248, 249, 250, 251, 252, 253, 254, 255,
  0,
};

#endif /* AVCODEC_DV_VLC_DATA_H */