/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This plugin renders a fish tank inside of the transparent cube,
 * replete with fish, crabs, sand, bubbles, and coral.
 *
 * Copyright : (C) 2007-2008 by David Mikos
 * Email     : infiniteloopcounter@gmail.com
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* Copyright (c) E. Lassauge, 1998. */

/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * The original code for this mode was written by Mark J. Kilgard
 * as a demo for openGL programming.
 *
 * Porting it to xlock  was possible by comparing the original Mesa's morph3d
 * demo with it's ported version to xlock, so thanks for Marcelo F. Vianna
 * (look at morph3d.c) for his indirect help.
 *
 * Thanks goes also to Brian Paul for making it possible and inexpensive
 * to use OpenGL at home.
 *
 * My e-mail address is lassauge@users.sourceforge.net
 *
 * Eric Lassauge  (May-13-1998)
 *
 */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include "atlantis-internal.h"
#include "shark.h"

void
DrawShark(fishRec * fish, int wire)
{
    float mat[4][4];
    int n;
    float seg1, seg2, seg3, seg4, segup;
    float thrash, chomp;
    GLenum cap;

    fish->htail = (int) (fish->htail - (int) (5.0 * fish->v)) % 360;

    thrash = 50.0 * fish->v;

    seg1 = 0.6 * thrash * sin(fish->htail * RRAD);
    seg2 = 1.8 * thrash * sin((fish->htail + 45.0) * RRAD);
    seg3 = 3.0 * thrash * sin((fish->htail + 90.0) * RRAD);
    seg4 = 4.0 * thrash * sin((fish->htail + 110.0) * RRAD);

    chomp = 0.0;
    if (fish->v > 2.0)
    {
	chomp = -(fish->v - 2.0) * 200.0;
    }
    P004[1] = iP004[1] + chomp;
    P007[1] = iP007[1] + chomp;
    P010[1] = iP010[1] + chomp;
    P011[1] = iP011[1] + chomp;

    P023[0] = iP023[0] + seg1;
    P024[0] = iP024[0] + seg1;
    P025[0] = iP025[0] + seg1;
    P026[0] = iP026[0] + seg1;
    P027[0] = iP027[0] + seg1;
    P028[0] = iP028[0] + seg1;
    P029[0] = iP029[0] + seg1;
    P030[0] = iP030[0] + seg1;
    P031[0] = iP031[0] + seg1;
    P032[0] = iP032[0] + seg1;
    P033[0] = iP033[0] + seg2;
    P034[0] = iP034[0] + seg2;
    P035[0] = iP035[0] + seg2;
    P036[0] = iP036[0] + seg2;
    P037[0] = iP037[0] + seg2;
    P038[0] = iP038[0] + seg2;
    P039[0] = iP039[0] + seg2;
    P040[0] = iP040[0] + seg2;
    P041[0] = iP041[0] + seg2;
    P042[0] = iP042[0] + seg2;
    P043[0] = iP043[0] + seg3;
    P044[0] = iP044[0] + seg3;
    P045[0] = iP045[0] + seg3;
    P046[0] = iP046[0] + seg3;
    P047[0] = iP047[0] + seg3;
    P048[0] = iP048[0] + seg3;
    P049[0] = iP049[0] + seg3;
    P050[0] = iP050[0] + seg3;
    P051[0] = iP051[0] + seg3;
    P052[0] = iP052[0] + seg3;
    P002[0] = iP002[0] + seg4;
    P061[0] = iP061[0] + seg4;
    P069[0] = iP069[0] + seg4;
    P070[0] = iP070[0] + seg4;

    fish->vtail += ((fish->dtheta - fish->vtail) * 0.1);

    if (fish->vtail > 0.5)
    {
	fish->vtail = 0.5;
    }
    else if (fish->vtail < -0.5)
    {
	fish->vtail = -0.5;
    }
    segup = thrash * fish->vtail;

    P023[1] = iP023[1] + segup;
    P024[1] = iP024[1] + segup;
    P025[1] = iP025[1] + segup;
    P026[1] = iP026[1] + segup;
    P027[1] = iP027[1] + segup;
    P028[1] = iP028[1] + segup;
    P029[1] = iP029[1] + segup;
    P030[1] = iP030[1] + segup;
    P031[1] = iP031[1] + segup;
    P032[1] = iP032[1] + segup;
    P033[1] = iP033[1] + segup * 5.0;
    P034[1] = iP034[1] + segup * 5.0;
    P035[1] = iP035[1] + segup * 5.0;
    P036[1] = iP036[1] + segup * 5.0;
    P037[1] = iP037[1] + segup * 5.0;
    P038[1] = iP038[1] + segup * 5.0;
    P039[1] = iP039[1] + segup * 5.0;
    P040[1] = iP040[1] + segup * 5.0;
    P041[1] = iP041[1] + segup * 5.0;
    P042[1] = iP042[1] + segup * 5.0;
    P043[1] = iP043[1] + segup * 12.0;
    P044[1] = iP044[1] + segup * 12.0;
    P045[1] = iP045[1] + segup * 12.0;
    P046[1] = iP046[1] + segup * 12.0;
    P047[1] = iP047[1] + segup * 12.0;
    P048[1] = iP048[1] + segup * 12.0;
    P049[1] = iP049[1] + segup * 12.0;
    P050[1] = iP050[1] + segup * 12.0;
    P051[1] = iP051[1] + segup * 12.0;
    P052[1] = iP052[1] + segup * 12.0;
    P002[1] = iP002[1] + segup * 17.0;
    P061[1] = iP061[1] + segup * 17.0;
    P069[1] = iP069[1] + segup * 17.0;
    P070[1] = iP070[1] + segup * 17.0;

    glPushMatrix();

    glTranslatef(0.0, 0.0, -3000.0);

    glGetFloatv(GL_MODELVIEW_MATRIX, &mat[0][0]);
    n = 0;
    if (mat[0][2] >= 0.0)
    {
	n += 1;
    }
    if (mat[1][2] >= 0.0)
    {
	n += 2;
    }
    if (mat[2][2] >= 0.0)
    {
	n += 4;
    }
    glScalef(2.0, 1.0, 1.0);

    glEnable(GL_CULL_FACE);
    cap = wire ? GL_LINE_LOOP : GL_POLYGON;
    switch (n)
    {
    case 0:
	Fish_1(cap);
	break;
    case 1:
	Fish_2(cap);
	break;
    case 2:
	Fish_3(cap);
	break;
    case 3:
	Fish_4(cap);
	break;
    case 4:
	Fish_5(cap);
	break;
    case 5:
	Fish_6(cap);
	break;
    case 6:
	Fish_7(cap);
	break;
    case 7:
	Fish_8(cap);
	break;
    }
    glDisable(GL_CULL_FACE);

    glPopMatrix();
}

