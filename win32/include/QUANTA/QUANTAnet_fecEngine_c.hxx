/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

/* File: QUANTAnet_fecEngine_c.hxx
   Description:  The header file for QUANTAnet_fecEngine class
*/

#ifndef _QUANTANET_FECENGINE_C
#define _QUANTANET_FECENGINE_C

#ifndef GF_BITS
#define GF_BITS  8  /* code over GF(2**GF_BITS) */
#endif

#if (GF_BITS < 2  && GF_BITS >16)
#error "GF_BITS must be 2 .. 16"
#endif
#if (GF_BITS <= 8)
typedef unsigned char gf;
#else
typedef unsigned short gf;
#endif

#define GF_SIZE ((1 << GF_BITS) - 1)

typedef struct {
  unsigned long magic ;
  int k, n ;    /* parameters of the code */
  unsigned char *enc_matrix ;
} fec_parms;

/**
   FEC Engine Class.  This class implements the core algorithm of FEC over UDP protocol.

*/


class QUANTAnet_fecEngine_c
{
public:
  QUANTAnet_fecEngine_c();
  ~QUANTAnet_fecEngine_c();
  void release(fec_parms *p) ;
  fec_parms * init(int k, int n) ;
  void encode(fec_parms *code, void *src[], void *dst, int index, int sz, int k) ;
  int decode(fec_parms *code, void *pkt[], int index[], int sz, int k) ;

};


#endif /* _QUANTANET_FECENGINE_C */
