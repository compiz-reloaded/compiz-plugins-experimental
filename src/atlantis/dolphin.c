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
 * My e-mail address is lassauge@sagem.fr
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
#include "dolphin.h"

void
DrawDolphin(fishRec * fish, int wire)
{
    float seg0, seg1, seg2, seg3, seg4, seg5, seg6, seg7;
    float pitch, thrash, chomp;
    GLenum cap;

    fish->htail = (int) (fish->htail - (int) (10.0 * fish->v)) % 360;

    thrash = 70.0 * fish->v;

    seg0 = 1.0 * thrash * sin((fish->htail) * RRAD);
    seg3 = 1.0 * thrash * sin((fish->htail) * RRAD);
    seg1 = 2.0 * thrash * sin((fish->htail + 4.0) * RRAD);
    seg2 = 3.0 * thrash * sin((fish->htail + 6.0) * RRAD);
    seg4 = 4.0 * thrash * sin((fish->htail + 10.0) * RRAD);
    seg5 = 4.5 * thrash * sin((fish->htail + 15.0) * RRAD);
    seg6 = 5.0 * thrash * sin((fish->htail + 20.0) * RRAD);
    seg7 = 6.0 * thrash * sin((fish->htail + 30.0) * RRAD);

    pitch = fish->v * sin((fish->htail + 180.0) * RRAD);

    if (fish->v > 2.0)
    {
	chomp = -(fish->v - 2.0) * 200.0;
    }
    chomp = 100.0;

    P012[1] = iP012[1] + seg5;
    P013[1] = iP013[1] + seg5;
    P014[1] = iP014[1] + seg5;
    P015[1] = iP015[1] + seg5;
    P016[1] = iP016[1] + seg5;
    P017[1] = iP017[1] + seg5;
    P018[1] = iP018[1] + seg5;
    P019[1] = iP019[1] + seg5;

    P020[1] = iP020[1] + seg4;
    P021[1] = iP021[1] + seg4;
    P022[1] = iP022[1] + seg4;
    P023[1] = iP023[1] + seg4;
    P024[1] = iP024[1] + seg4;
    P025[1] = iP025[1] + seg4;
    P026[1] = iP026[1] + seg4;
    P027[1] = iP027[1] + seg4;

    P028[1] = iP028[1] + seg2;
    P029[1] = iP029[1] + seg2;
    P030[1] = iP030[1] + seg2;
    P031[1] = iP031[1] + seg2;
    P032[1] = iP032[1] + seg2;
    P033[1] = iP033[1] + seg2;
    P034[1] = iP034[1] + seg2;
    P035[1] = iP035[1] + seg2;

    P036[1] = iP036[1] + seg1;
    P037[1] = iP037[1] + seg1;
    P038[1] = iP038[1] + seg1;
    P039[1] = iP039[1] + seg1;
    P040[1] = iP040[1] + seg1;
    P041[1] = iP041[1] + seg1;
    P042[1] = iP042[1] + seg1;
    P043[1] = iP043[1] + seg1;

    P044[1] = iP044[1] + seg0;
    P045[1] = iP045[1] + seg0;
    P046[1] = iP046[1] + seg0;
    P047[1] = iP047[1] + seg0;
    P048[1] = iP048[1] + seg0;
    P049[1] = iP049[1] + seg0;
    P050[1] = iP050[1] + seg0;
    P051[1] = iP051[1] + seg0;

    P009[1] = iP009[1] + seg6;
    P010[1] = iP010[1] + seg6;
    P075[1] = iP075[1] + seg6;
    P076[1] = iP076[1] + seg6;

    P001[1] = iP001[1] + seg7;
    P011[1] = iP011[1] + seg7;
    P068[1] = iP068[1] + seg7;
    P069[1] = iP069[1] + seg7;
    P070[1] = iP070[1] + seg7;
    P071[1] = iP071[1] + seg7;
    P072[1] = iP072[1] + seg7;
    P073[1] = iP073[1] + seg7;
    P074[1] = iP074[1] + seg7;

    P091[1] = iP091[1] + seg3;
    P092[1] = iP092[1] + seg3;
    P093[1] = iP093[1] + seg3;
    P094[1] = iP094[1] + seg3;
    P095[1] = iP095[1] + seg3;
    P122[1] = iP122[1] + seg3 * 1.5;

    P097[1] = iP097[1] + chomp;
    P098[1] = iP098[1] + chomp;
    P102[1] = iP102[1] + chomp;
    P110[1] = iP110[1] + chomp;
    P111[1] = iP111[1] + chomp;
    P121[1] = iP121[1] + chomp;
    P118[1] = iP118[1] + chomp;
    P119[1] = iP119[1] + chomp;

    glPushMatrix();

    glRotatef(pitch, 1.0, 0.0, 0.0);

    glTranslatef(0.0, 0.0, 7000.0);

    glRotatef(180.0, 0.0, 1.0, 0.0);

    glEnable(GL_CULL_FACE);
    cap = wire ? GL_LINE_LOOP : GL_POLYGON;
    Dolphin014(cap);
    Dolphin010(cap);
    Dolphin009(cap);
    Dolphin012(cap);
    Dolphin013(cap);
    Dolphin006(cap);
    Dolphin002(cap);
    Dolphin001(cap);
    Dolphin003(cap);
    Dolphin015(cap);
    Dolphin004(cap);
    Dolphin005(cap);
    Dolphin007(cap);
    Dolphin008(cap);
    Dolphin011(cap);
    Dolphin016(cap);
    glDisable(GL_CULL_FACE);

    glPopMatrix();
}
