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
 * E-mail    : onestone@compiz.org
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
#include "bfish.h"

void
AnimateBFish(float t)
{
    int   ti = (int) t;
    float w = 2*PI*(t-ti);
    BFishPoints[2+3*0] =-82.18+402.5359*sinf(w-6.242932);
    BFishPoints[2+3*1] =-37.912+1161.102*sinf(w-6.167075);
    BFishPoints[2+3*2] =87.136+1161.478*sinf(w-6.1670375);
    BFishPoints[2+3*3] =149.72+403.23584*sinf(w-6.2428617);
    BFishPoints[2+3*4] =-212.568+139.08008*sinf(w-6.269277);
    BFishPoints[2+3*5] =286.456+140.58594*sinf(w-6.269127);
    BFishPoints[2+3*6] =89.54+1315.0881*sinf(w-6.1516767);
    BFishPoints[2+3*7] =302.104+303.52002*sinf(w-6.2528334);
    BFishPoints[2+3*9] =-232.14801+301.90796*sinf(w-6.2529945);
    BFishPoints[2+3*10] =-44.024002+1314.686*sinf(w-6.1517167);
    BFishPoints[2+3*12] =93.287994+1153.2058*sinf(w-6.167865);
    BFishPoints[2+3*13] =235.864+407.18384*sinf(w-6.242467);
    BFishPoints[2+3*14] =239.892+73.65991*sinf(w-6.2758193);
    BFishPoints[2+3*15] =-43.864002+1152.792*sinf(w-6.1679063);
    BFishPoints[2+3*16] =-168.416+405.96387*sinf(w-6.242589);
    BFishPoints[2+3*17] =-164.388+72.43994*sinf(w-6.2759414);
    BFishPoints[2+3*18] =57.148+1017.93384*sinf(w-6.1813917);
    BFishPoints[2+3*19] =-4.464+1017.74805*sinf(w-6.1814103);
    BFishPoints[2+3*21] =-42.984+110.73413*sinf(w-6.272112);
    BFishPoints[2+3*22] =117.572+111.21997*sinf(w-6.2720633);
    BFishPoints[2+3*24] =60.792+716.166*sinf(w-6.211569);
    BFishPoints[2+3*25] =287.344+66.98584*sinf(w-6.276487);
    BFishPoints[2+3*26] =-211.68+65.47998*sinf(w-6.276637);
    BFishPoints[2+3*27] =-0.82+715.98*sinf(w-6.2115874);
    BFishPoints[2+3*34] =5.0039997+265.08203*sinf(w-6.256677);
    BFishPoints[2+3*36] =-3.632+980.5979*sinf(w-6.1851254);
    BFishPoints[2+3*44] =65.856+265.26587*sinf(w-6.2566586);
    BFishPoints[2+3*45] =57.216+980.782*sinf(w-6.185107);
    BFishPoints[2+3*46] =287.344+66.98584*sinf(w-6.276487);
    BFishPoints[2+3*47] =286.456+140.58398*sinf(w-6.269127);
    BFishPoints[2+3*52] =-212.568+139.07788*sinf(w-6.2692776);
    BFishPoints[2+3*53] =-211.68+65.47998*sinf(w-6.276637);
    BFishPoints[2+3*74] =239.892+73.65991*sinf(w-6.2758193);
    BFishPoints[2+3*77] =-164.388+72.43994*sinf(w-6.2759414);
    BFishPoints[2+3*125] =-37.912+1161.0999*sinf(w-6.167075);
    BFishPoints[2+3*126] =22.928+1309.362*sinf(w-6.1522493);
    BFishPoints[2+3*127] =87.136+1161.478*sinf(w-6.1670375);
    BFishPoints[2+3*128] =20.880001+1475.4539*sinf(w-6.13564);
    BFishPoints[2+3*129] =89.54+1315.0881*sinf(w-6.1516767);
    BFishPoints[2+3*130] =22.88+1301.7122*sinf(w-6.153014);
    BFishPoints[2+3*131] =93.287994+1153.2058*sinf(w-6.167865);
    BFishPoints[2+3*132] =-43.864002+1152.792*sinf(w-6.1679063);
    BFishPoints[2+3*133] =-44.024002+1314.6841*sinf(w-6.1517167);
    BFishPoints[2+3*134] =57.148+1017.9319*sinf(w-6.181392);
    BFishPoints[2+3*135] =24.852+1141.512*sinf(w-6.169034);
    BFishPoints[2+3*136] =-4.464+1017.7461*sinf(w-6.181411);
    BFishPoints[2+3*137] =-42.984+110.73413*sinf(w-6.272112);
    BFishPoints[2+3*138] =35.332+273.35596*sinf(w-6.25585);
    BFishPoints[2+3*139] =117.572+111.21802*sinf(w-6.2720637);
    BFishPoints[2+3*140] =28.880001+807.60596*sinf(w-6.2024245);
    BFishPoints[2+3*141] =60.792+716.166*sinf(w-6.211569);
    BFishPoints[2+3*142] =-0.82+715.98*sinf(w-6.2115874);
    BFishPoints[2+3*143] =-3.632+980.59985*sinf(w-6.1851254);
    BFishPoints[2+3*144] =24.852+1141.512*sinf(w-6.169034);
    BFishPoints[2+3*145] =57.216+980.782*sinf(w-6.185107);
    BFishPoints[2+3*149] =65.856+265.26587*sinf(w-6.2566586);
    BFishPoints[2+3*150] =33.72+406.91992*sinf(w-6.242493);
    BFishPoints[2+3*155] =5.0039997+265.08203*sinf(w-6.256677);
}


void
DrawAnimatedBFish()
{
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, BFishPoints);
    glNormalPointer (GL_FLOAT, 0, BFishNormals);

    //26/yellow:body_9_up_Mesh.024
    glDisable (GL_COLOR_MATERIAL);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, fin_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, fin_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, fin_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, fin_specular);
    glDrawElements(GL_QUADS, 112, GL_UNSIGNED_INT, &(BFishIndices[0]));

    //19/yellow:body_5_low_Mesh.017
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, UserDefined_body_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, UserDefined_body_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, UserDefined_body_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, UserDefined_body_specular);
    glDrawElements(GL_QUADS, 120, GL_UNSIGNED_INT, &(BFishIndices[112]));

    //13/black:body_2_Mesh.011
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, shadow_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, shadow_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, shadow_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, shadow_specular);
    glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, &(BFishIndices[232]));

    //12/yellow:body_1_Mesh.010
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, UserDefined_body_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, UserDefined_body_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, UserDefined_body_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, UserDefined_body_specular);
    glDrawElements(GL_QUADS, 72, GL_UNSIGNED_INT, &(BFishIndices[256]));

    glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, &(BFishIndices[328]));

    //9/black:upper_Mesh.007
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, shadow_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, shadow_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, shadow_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, shadow_specular);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &(BFishIndices[346]));

    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, &(BFishIndices[349]));

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &(BFishIndices[357]));

    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, &(BFishIndices[360]));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &(BFishIndices[368]));

    glDrawElements(GL_QUADS, 16, GL_UNSIGNED_INT, &(BFishIndices[374]));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &(BFishIndices[390]));

    glDrawElements(GL_QUADS, 32, GL_UNSIGNED_INT, &(BFishIndices[396]));

    //6/yellow:side_Mesh.004
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 1.0f);
    glDisable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, userDefined_DS_fin_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, userDefined_DS_fin_ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, userDefined_DS_fin_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, userDefined_DS_fin_specular);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &(BFishIndices[428]));

    glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &(BFishIndices[431]));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &(BFishIndices[435]));

    glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &(BFishIndices[441]));

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &(BFishIndices[445]));

    //4/black:black_Mesh.002
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);
    glEnable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT, GL_SHININESS, shadow_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, shadow_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, shadow_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, shadow_specular);
    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, &(BFishIndices[448]));

    //2/white:white_Mesh
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, eyeborder_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, eyeborder_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, eyeborder_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, eyeborder_specular);
    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, &(BFishIndices[456]));
    glDisableClientState (GL_NORMAL_ARRAY);
    glPopAttrib();
}

void
initDrawBFish(float *color)
{
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glEnable(GL_CULL_FACE);

    copyColor((float *)UserDefined_body_ambient, color, 0.8);
    copyColor((float *)UserDefined_body_diffuse, color, 0.8);
    copyColor((float *)UserDefined_body_specular, color, 0.1);
}

void
finDrawBFish()
{
    glDisable(GL_CULL_FACE);
}
