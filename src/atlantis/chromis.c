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

/*
 * This file is part of an automatically generated output from
 * executing objToC.java on a .obj 3D model. The output was
 * split manually into this code and a header with model data.
 */

#include "atlantis-internal.h"
#include "chromis.h"

void
AnimateChromis(float t)
{
    int   ti = (int) t;
    float w = 2*PI*(t-ti);
    ChromisPoints[2+3*26] =-27.047998+117.52197*sinf(w-6.2782884);
    ChromisPoints[2+3*27] =-27.012+166.97998*sinf(w-6.276228);
    ChromisPoints[2+3*28] =-26.892+336.7561*sinf(w-6.2691536);
    ChromisPoints[2+3*43] =-26.964+228.8999*sinf(w-6.273648);
    ChromisPoints[2+3*48] =-26.964+228.8999*sinf(w-6.273648);
    ChromisPoints[2+3*49] =-25.536+2332.356*sinf(w-6.1860037);
    ChromisPoints[2+3*50] =-165.828+150.85791*sinf(w-6.2768993);
    ChromisPoints[2+3*51] =111.792+150.80981*sinf(w-6.2769017);
    ChromisPoints[2+3*52] =-25.536+2332.356*sinf(w-6.1860037);
    ChromisPoints[2+3*85] =-26.196001+1366.7458*sinf(w-6.226238);
    ChromisPoints[2+3*86] =-26.868+610.80615*sinf(w-6.2577353);
    ChromisPoints[2+3*87] =-26.196001+1366.7458*sinf(w-6.226238);
}


void
DrawAnimatedChromis()
{
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, ChromisPoints);
    glNormalPointer (GL_FLOAT, 0, ChromisNormals);

    //Eye
    glDisable (GL_COLOR_MATERIAL);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, eye_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, eye_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, eye_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, eye_specular);
    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, &(ChromisIndices[0]));

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, eye_outer_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, eye_outer_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, eye_outer_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, eye_outer_specular);
    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, &(ChromisIndices[8]));

    //Fins
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 1.0f);
    glDisable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, userDefined_DS_fin_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, userDefined_DS_fin_ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, userDefined_DS_fin_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, userDefined_DS_fin_specular);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &(ChromisIndices[16]));

    glDrawElements(GL_QUADS, 16, GL_UNSIGNED_INT, &(ChromisIndices[22]));

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &(ChromisIndices[38]));

    glDrawElements(GL_QUADS, 12, GL_UNSIGNED_INT, &(ChromisIndices[41]));

    //BodyAndTail
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);
    glEnable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT, GL_SHININESS, userDefined_tailfin_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, userDefined_tailfin_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, userDefined_tailfin_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, userDefined_tailfin_specular);
    glDrawElements(GL_QUADS, 32, GL_UNSIGNED_INT, &(ChromisIndices[53]));

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, userDefined_border_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, userDefined_border_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, userDefined_border_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, userDefined_border_specular);
    glDrawElements(GL_QUADS, 64, GL_UNSIGNED_INT, &(ChromisIndices[85]));

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, userDefined_body_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, userDefined_body_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, userDefined_body_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, userDefined_body_specular);
    glDrawElements(GL_QUADS, 80, GL_UNSIGNED_INT, &(ChromisIndices[149]));

    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, &(ChromisIndices[229]));

    glDrawElements(GL_QUADS, 16, GL_UNSIGNED_INT, &(ChromisIndices[241]));
    glDisableClientState (GL_NORMAL_ARRAY);
    glPopAttrib();
}

void
initDrawChromis(float *color)
{
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glEnable(GL_CULL_FACE);

    copyColor((float *)userDefined_body_ambient, color, 0.95);
    copyColor((float *)userDefined_body_diffuse, color, 0.95);
    copyColor((float *)userDefined_body_specular, color, 0.1);
    copyColor((float *)userDefined_border_ambient, color, 0.7);
    copyColor((float *)userDefined_border_diffuse, color, 0.7);
    copyColor((float *)userDefined_border_specular, color, 0.1);
    copyColor((float *)userDefined_tailfin_ambient, color, 0.6);
    copyColor((float *)userDefined_tailfin_diffuse, color, 0.6);
    copyColor((float *)userDefined_tailfin_specular, color, 0.1);
    copyColor((float *)userDefined_DS_fin_ambient, color, 0.6);
    copyColor((float *)userDefined_DS_fin_diffuse, color, 0.6);
    copyColor((float *)userDefined_DS_fin_specular, color, 0.1);
}

void
initDrawChromis2(float *color)
{
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glEnable(GL_CULL_FACE);

    copyColor((float *)userDefined_body_ambient, color, 0.95);
    copyColor((float *)userDefined_body_diffuse, color, 0.95);
    copyColor((float *)userDefined_body_specular, color, 0.1);
    copyColor((float *)userDefined_border_ambient, color, 0.95);
    copyColor((float *)userDefined_border_diffuse, color, 0.95);
    copyColor((float *)userDefined_border_specular, color, 0.1);
    copyColor((float *)userDefined_tailfin_ambient, color, 0.6);
    copyColor((float *)userDefined_tailfin_diffuse, color, 0.6);
    copyColor((float *)userDefined_tailfin_specular, color, 0.1);
    copyColor((float *)userDefined_DS_fin_ambient, color, 0.6);
    copyColor((float *)userDefined_DS_fin_diffuse, color, 0.6);
    copyColor((float *)userDefined_DS_fin_specular, color, 0.1);
}

void
initDrawChromis3(float *color)
{
    static float COLOR_WHITE[] = { 1.0, 1.0, 1.0, 1.0 };

    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glEnable(GL_CULL_FACE);

    copyColor((float *)userDefined_body_ambient, color, 0.95);
    copyColor((float *)userDefined_body_diffuse, color, 0.95);
    copyColor((float *)userDefined_body_specular, color, 0.1);
    copyColor((float *)userDefined_border_ambient, COLOR_WHITE, 1.0);
    copyColor((float *)userDefined_border_diffuse, COLOR_WHITE, 1.0);
    copyColor((float *)userDefined_border_specular, COLOR_WHITE, 0.1);
    copyColor((float *)userDefined_tailfin_ambient, color, 0.6);
    copyColor((float *)userDefined_tailfin_diffuse, color, 0.6);
    copyColor((float *)userDefined_tailfin_specular, color, 0.1);
    copyColor((float *)userDefined_DS_fin_ambient, color, 0.6);
    copyColor((float *)userDefined_DS_fin_diffuse, color, 0.6);
    copyColor((float *)userDefined_DS_fin_specular, color, 0.1);
}

void
finDrawChromis()
{
    glDisable(GL_CULL_FACE);
}
