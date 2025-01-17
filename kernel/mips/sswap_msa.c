/*******************************************************************************
Copyright (c) 2016, The OpenBLAS Project
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the OpenBLAS project nor the names of
its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "common.h"
#include "macros_msa.h"

int CNAME(BLASLONG n, BLASLONG dummy0, BLASLONG dummy1, FLOAT dummy3,
          FLOAT *srcx, BLASLONG inc_x, FLOAT *srcy, BLASLONG inc_y,
          FLOAT *dummy, BLASLONG dummy2)
{
    BLASLONG i = 0, pref_offsetx, pref_offsety;
    FLOAT *px, *py;
    FLOAT x0, x1, x2, x3, x4, x5, x6, x7;
    FLOAT y0, y1, y2, y3, y4, y5, y6, y7;
    v4f32 xv0, xv1, xv2, xv3, xv4, xv5, xv6, xv7;
    v4f32 yv0, yv1, yv2, yv3, yv4, yv5, yv6, yv7;

    if (n < 0)  return (0);

    pref_offsetx = (BLASLONG)srcx & (L1_DATA_LINESIZE - 1);
    if (pref_offsetx > 0)
    {
        pref_offsetx = L1_DATA_LINESIZE - pref_offsetx;
        pref_offsetx = pref_offsetx / sizeof(FLOAT);
    }

    pref_offsety = (BLASLONG)srcy & (L1_DATA_LINESIZE - 1);
    if (pref_offsety > 0)
    {
        pref_offsety = L1_DATA_LINESIZE - pref_offsety;
        pref_offsety = pref_offsety / sizeof(FLOAT);
    }

    px = srcx;
    py = srcy;

    if ((1 == inc_x) && (1 == inc_y))
    {
        if (n >> 5)
        {
            LD_SP8_INC(px, 4, xv0, xv1, xv2, xv3, xv4, xv5, xv6, xv7);

            for (i = (n >> 5) - 1; i--;)
            {
                PREFETCH(px + pref_offsetx + 32);
                PREFETCH(px + pref_offsetx + 40);
                PREFETCH(px + pref_offsetx + 48);
                PREFETCH(px + pref_offsetx + 56);

                PREFETCH(py + pref_offsety + 32);
                PREFETCH(py + pref_offsety + 40);
                PREFETCH(py + pref_offsety + 48);
                PREFETCH(py + pref_offsety + 56);

                yv0 = LD_SP(py); py += 4;
                ST_SP(xv0, srcy); srcy += 4;
                yv1 = LD_SP(py); py += 4;
                ST_SP(xv1, srcy); srcy += 4;
                yv2 = LD_SP(py); py += 4;
                ST_SP(xv2, srcy); srcy += 4;
                yv3 = LD_SP(py); py += 4;
                ST_SP(xv3, srcy); srcy += 4;
                yv4 = LD_SP(py); py += 4;
                ST_SP(xv4, srcy); srcy += 4;
                yv5 = LD_SP(py); py += 4;
                ST_SP(xv5, srcy); srcy += 4;
                yv6 = LD_SP(py); py += 4;
                ST_SP(xv6, srcy); srcy += 4;
                yv7 = LD_SP(py); py += 4;
                ST_SP(xv7, srcy); srcy += 4;

                xv0 = LD_SP(px); px += 4;
                ST_SP(yv0, srcx); srcx += 4;
                xv1 = LD_SP(px); px += 4;
                ST_SP(yv1, srcx); srcx += 4;
                xv2 = LD_SP(px); px += 4;
                ST_SP(yv2, srcx); srcx += 4;
                xv3 = LD_SP(px); px += 4;
                ST_SP(yv3, srcx); srcx += 4;
                xv4 = LD_SP(px); px += 4;
                ST_SP(yv4, srcx); srcx += 4;
                xv5 = LD_SP(px); px += 4;
                ST_SP(yv5, srcx); srcx += 4;
                xv6 = LD_SP(px); px += 4;
                ST_SP(yv6, srcx); srcx += 4;
                xv7 = LD_SP(px); px += 4;
                ST_SP(yv7, srcx); srcx += 4;
            }

            LD_SP8_INC(py, 4, yv0, yv1, yv2, yv3, yv4, yv5, yv6, yv7);
            ST_SP8_INC(xv0, xv1, xv2, xv3, xv4, xv5, xv6, xv7, srcy, 4);
            ST_SP8_INC(yv0, yv1, yv2, yv3, yv4, yv5, yv6, yv7, srcx, 4);
        }

        if (n & 31)
        {
            if ((n & 16) && (n & 8) && (n & 4))
            {
                LD_SP7_INC(px, 4, xv0, xv1, xv2, xv3, xv4, xv5, xv6);
                LD_SP7_INC(py, 4, yv0, yv1, yv2, yv3, yv4, yv5, yv6);
                ST_SP7_INC(xv0, xv1, xv2, xv3, xv4, xv5, xv6, srcy, 4);
                ST_SP7_INC(yv0, yv1, yv2, yv3, yv4, yv5, yv6, srcx, 4);
            }
            else if ((n & 16) && (n & 8))
            {
                LD_SP6_INC(px, 4, xv0, xv1, xv2, xv3, xv4, xv5);
                LD_SP6_INC(py, 4, yv0, yv1, yv2, yv3, yv4, yv5);
                ST_SP6_INC(xv0, xv1, xv2, xv3, xv4, xv5, srcy, 4);
                ST_SP6_INC(yv0, yv1, yv2, yv3, yv4, yv5, srcx, 4);
            }
            else if ((n & 16) && (n & 4))
            {
                LD_SP5_INC(px, 4, xv0, xv1, xv2, xv3, xv4);
                LD_SP5_INC(py, 4, yv0, yv1, yv2, yv3, yv4);
                ST_SP5_INC(xv0, xv1, xv2, xv3, xv4, srcy, 4);
                ST_SP5_INC(yv0, yv1, yv2, yv3, yv4, srcx, 4);
            }
            else if ((n & 8) && (n & 4))
            {
                LD_SP3_INC(px, 4, xv0, xv1, xv2);
                LD_SP3_INC(py, 4, yv0, yv1, yv2);
                ST_SP3_INC(xv0, xv1, xv2, srcy, 4);
                ST_SP3_INC(yv0, yv1, yv2, srcx, 4);
            }
            else if (n & 16)
            {
                LD_SP4_INC(px, 4, xv0, xv1, xv2, xv3);
                LD_SP4_INC(py, 4, yv0, yv1, yv2, yv3);
                ST_SP4_INC(xv0, xv1, xv2, xv3, srcy, 4);
                ST_SP4_INC(yv0, yv1, yv2, yv3, srcx, 4);
            }
            else if (n & 8)
            {
                LD_SP2_INC(px, 4, xv0, xv1);
                LD_SP2_INC(py, 4, yv0, yv1);
                ST_SP2_INC(xv0, xv1, srcy, 4);
                ST_SP2_INC(yv0, yv1, srcx, 4);
            }
            else if (n & 4)
            {
                xv0 = LD_SP(px);
                yv0 = LD_SP(py);

                px += 4;
                py += 4;

                ST_SP(xv0, srcy);
                ST_SP(yv0, srcx);

                srcx += 4;
                srcy += 4;
            }

            if ((n & 2) && (n & 1))
            {
                LD_GP3_INC(px, 1, x0, x1, x3);
                LD_GP3_INC(py, 1, y0, y1, y3);
                ST_GP3_INC(x0, x1, x3, srcy, 1);
                ST_GP3_INC(y0, y1, y3, srcx, 1);
            }
            else if (n & 2)
            {
                LD_GP2_INC(px, 1, x0, x1);
                LD_GP2_INC(py, 1, y0, y1);
                ST_GP2_INC(x0, x1, srcy, 1);
                ST_GP2_INC(y0, y1, srcx, 1);
            }
            else if (n & 1)
            {
                x0 = px[0];
                y0 = py[0];
                srcx[0] = y0;
                srcy[0] = x0;
            }
        }
    }
    else if ((inc_x != 0) && (inc_y != 0))
    {
        for (i = (n >> 3); i--;)
        {
            LD_GP8_INC(px, inc_x, x0, x1, x2, x3, x4, x5, x6, x7);
            LD_GP8_INC(py, inc_y, y0, y1, y2, y3, y4, y5, y6, y7);
            ST_GP8_INC(x0, x1, x2, x3, x4, x5, x6, x7, srcy, inc_y);
            ST_GP8_INC(y0, y1, y2, y3, y4, y5, y6, y7, srcx, inc_x);
        }

        if (n & 7)
        {
            if ((n & 4) && (n & 2) && (n & 1))
            {
                LD_GP7_INC(px, inc_x, x0, x1, x2, x3, x4, x5, x6);
                LD_GP7_INC(py, inc_y, y0, y1, y2, y3, y4, y5, y6);
                ST_GP7_INC(x0, x1, x2, x3, x4, x5, x6, srcy, inc_y);
                ST_GP7_INC(y0, y1, y2, y3, y4, y5, y6, srcx, inc_x);
            }
            else if ((n & 4) && (n & 2))
            {
                LD_GP6_INC(px, inc_x, x0, x1, x2, x3, x4, x5);
                LD_GP6_INC(py, inc_y, y0, y1, y2, y3, y4, y5);
                ST_GP6_INC(x0, x1, x2, x3, x4, x5, srcy, inc_y);
                ST_GP6_INC(y0, y1, y2, y3, y4, y5, srcx, inc_x);
            }
            else if ((n & 4) && (n & 1))
            {
                LD_GP5_INC(px, inc_x, x0, x1, x2, x3, x4);
                LD_GP5_INC(py, inc_y, y0, y1, y2, y3, y4);
                ST_GP5_INC(x0, x1, x2, x3, x4, srcy, inc_y);
                ST_GP5_INC(y0, y1, y2, y3, y4, srcx, inc_x);
            }
            else if ((n & 2) && (n & 1))
            {
                LD_GP3_INC(px, inc_x, x0, x1, x2);
                LD_GP3_INC(py, inc_y, y0, y1, y2);
                ST_GP3_INC(x0, x1, x2, srcy, inc_y);
                ST_GP3_INC(y0, y1, y2, srcx, inc_x);
            }
            else if (n & 4)
            {
                LD_GP4_INC(px, inc_x, x0, x1, x2, x3);
                LD_GP4_INC(py, inc_y, y0, y1, y2, y3);
                ST_GP4_INC(x0, x1, x2, x3, srcy, inc_y);
                ST_GP4_INC(y0, y1, y2, y3, srcx, inc_x);
            }
            else if (n & 2)
            {
                LD_GP2_INC(px, inc_x, x0, x1);
                LD_GP2_INC(py, inc_y, y0, y1);
                ST_GP2_INC(x0, x1, srcy, inc_y);
                ST_GP2_INC(y0, y1, srcx, inc_x);
            }
            else if (n & 1)
            {
                x0 = *srcx;
                y0 = *srcy;

                *srcx = y0;
                *srcy = x0;
            }
        }
    }
    else
    {
        if (inc_x == inc_y)
        {
            if (n & 1)
            {
                x0 = *srcx;
                *srcx  = *srcy;
                *srcy  = x0;
            }
            else
                return (0);
        }
        else
        {
            BLASLONG ix = 0, iy = 0;
            while (i < n)
            {
                x0 = srcx[ix];
                srcx[ix] = srcy[iy];
                srcy[iy] = x0;
                ix += inc_x;
                iy += inc_y;
                i++;
            }
        }
    }

    return (0);
}
