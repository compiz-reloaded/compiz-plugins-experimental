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
#include "fish2.h"

void
AnimateFish2(float t)
{
    int   ti = (int) t;
    float w = 2*PI*(t-ti);
    Fish2Points[2+3*2] =8.472+27.614014*sinf(w-6.2797337);
    Fish2Points[2+3*14] = Fish2Points[2+3*2];
    Fish2Points[2+3*257] =8.228001+685.45593*sinf(w-6.197503);
    Fish2Points[2+3*258] =-64.92+830.6361*sinf(w-6.1793556);
    Fish2Points[2+3*259] =-64.467995+3308.5679*sinf(w-5.869614);
    Fish2Points[2+3*260] =-9.048+743.78406*sinf(w-6.1902122);
    Fish2Points[2+3*261] =-173.584+2643.146*sinf(w-5.952792);
    Fish2Points[2+3*262] =-124.144005+1403.7742*sinf(w-6.1077137);
    Fish2Points[2+3*263] =-17.488+2293.32*sinf(w-5.9965205);
    Fish2Points[2+3*264] =-25.028+2843.0222*sinf(w-5.9278073);
    Fish2Points[2+3*265] =-59.236+801.5239*sinf(w-6.182995);
    Fish2Points[2+3*266] =18.132+742.0499*sinf(w-6.190429);
    Fish2Points[2+3*267] =-81.304+2823.6082*sinf(w-5.9302344);
    Fish2Points[2+3*268] =-49.635998+2374.306*sinf(w-5.9863973);
    Fish2Points[2+3*269] =-23.664+686.854*sinf(w-6.1973286);
    Fish2Points[2+3*270] =-3.3400002+3498.526*sinf(w-5.8458695);
    Fish2Points[2+3*271] =-5.844+3508.1099*sinf(w-5.8446717);
    Fish2Points[2+3*272] =-34.212+998.92993*sinf(w-6.158319);
    Fish2Points[2+3*273] = Fish2Points[2+3*271];
    Fish2Points[2+3*274] =62.584+3194.708*sinf(w-5.8838468);
    Fish2Points[2+3*275] = Fish2Points[2+3*265];
    Fish2Points[2+3*276] = Fish2Points[2+3*264];
    Fish2Points[2+3*277] =-17.64+3378.4058*sinf(w-5.8608847);
    Fish2Points[2+3*278] = Fish2Points[2+3*272];
    Fish2Points[2+3*279] = Fish2Points[2+3*269];
    Fish2Points[2+3*280] = Fish2Points[2+3*271];
    Fish2Points[2+3*281] = Fish2Points[2+3*265];
    Fish2Points[2+3*282] =0.724+1415.1799*sinf(w-6.106288);
    Fish2Points[2+3*283] = Fish2Points[2+3*263];
    Fish2Points[2+3*284] = Fish2Points[2+3*266];
    Fish2Points[2+3*285] =6.0959997+1936.6799*sinf(w-6.0411005);
    Fish2Points[2+3*286] =11.648+1321.4961*sinf(w-6.117998);
    Fish2Points[2+3*287] =-52.8+3035.812*sinf(w-5.903709);
    Fish2Points[2+3*288] =-133.184+1432.0901*sinf(w-6.104174);
    Fish2Points[2+3*289] = Fish2Points[2+3*257];
    Fish2Points[2+3*290] =-87.284+2772.508*sinf(w-5.9366217);
    Fish2Points[2+3*291] =-124.035995+1555.332*sinf(w-6.088769);
    Fish2Points[2+3*292] =-0.604+677.182*sinf(w-6.1985373);
    Fish2Points[2+3*293] =-48.868+3261.3838*sinf(w-5.875512);
    Fish2Points[2+3*294] = Fish2Points[2+3*266];
    Fish2Points[2+3*295] =30.212+3307.566*sinf(w-5.8697395);
    Fish2Points[2+3*296] = Fish2Points[2+3*263];
    Fish2Points[2+3*297] = Fish2Points[2+3*282];
    Fish2Points[2+3*298] =-12.172+1906.4141*sinf(w-6.0448837);
    Fish2Points[2+3*299] = Fish2Points[2+3*258];
    Fish2Points[2+3*300] =-41.54+3162.44*sinf(w-5.8878803);
    Fish2Points[2+3*301] = Fish2Points[2+3*259];
    Fish2Points[2+3*302] = Fish2Points[2+3*266];
    Fish2Points[2+3*303] =-111.52+3457.7358*sinf(w-5.8509684);
    Fish2Points[2+3*304] = Fish2Points[2+3*295];
    Fish2Points[2+3*305] = Fish2Points[2+3*266];
    Fish2Points[2+3*306] = Fish2Points[2+3*268];
    Fish2Points[2+3*307] = Fish2Points[2+3*285];
    Fish2Points[2+3*308] =38.868+1401.502*sinf(w-6.1079974);
    Fish2Points[2+3*309] = Fish2Points[2+3*277];
    Fish2Points[2+3*310] =48.524+3463.4404*sinf(w-5.8502555);
    Fish2Points[2+3*311] = Fish2Points[2+3*259];
    Fish2Points[2+3*312] = Fish2Points[2+3*287];
    Fish2Points[2+3*313] = Fish2Points[2+3*257];
    Fish2Points[2+3*314] = Fish2Points[2+3*260];
    Fish2Points[2+3*315] =-56.904+2984.6318*sinf(w-5.910106);
    Fish2Points[2+3*316] = Fish2Points[2+3*261];
    Fish2Points[2+3*317] = Fish2Points[2+3*267];
    Fish2Points[2+3*318] = Fish2Points[2+3*266];
    Fish2Points[2+3*319] = Fish2Points[2+3*293];
    Fish2Points[2+3*320] = Fish2Points[2+3*272];
    Fish2Points[2+3*321] = Fish2Points[2+3*274];
    Fish2Points[2+3*322] =116.548004+1603.0479*sinf(w-6.082804);
    Fish2Points[2+3*323] = Fish2Points[2+3*269];
    Fish2Points[2+3*324] =38.724003+3138.046*sinf(w-5.8909297);
    Fish2Points[2+3*325] = Fish2Points[2+3*270];
    Fish2Points[2+3*326] =-87.647995+928.5459*sinf(w-6.167117);
    Fish2Points[2+3*327] = Fish2Points[2+3*303];
    Fish2Points[2+3*328] = Fish2Points[2+3*266];
    Fish2Points[2+3*329] = Fish2Points[2+3*315];
    Fish2Points[2+3*330] = Fish2Points[2+3*260];
    Fish2Points[2+3*331] =-26.915998+3044.708*sinf(w-5.902597);
    Fish2Points[2+3*332] = Fish2Points[2+3*308];
    Fish2Points[2+3*333] = Fish2Points[2+3*265];
    Fish2Points[2+3*334] = Fish2Points[2+3*277];
    Fish2Points[2+3*335] = Fish2Points[2+3*290];
    Fish2Points[2+3*336] = Fish2Points[2+3*292];
    Fish2Points[2+3*337] =-37.312+2777.5*sinf(w-5.935998);
    Fish2Points[2+3*338] = Fish2Points[2+3*292];
    Fish2Points[2+3*339] =-64.952+1454.354*sinf(w-6.101391);
    Fish2Points[2+3*340] = Fish2Points[2+3*337];
    Fish2Points[2+3*341] =-100.708+2697.448*sinf(w-5.9460044);
    Fish2Points[2+3*342] = Fish2Points[2+3*337];
    Fish2Points[2+3*343] = Fish2Points[2+3*339];
    Fish2Points[2+3*344] = Fish2Points[2+3*260];
    Fish2Points[2+3*345] =-40.440002+1561.1921*sinf(w-6.0880365);
    Fish2Points[2+3*346] = Fish2Points[2+3*331];
    Fish2Points[2+3*347] = Fish2Points[2+3*331];
    Fish2Points[2+3*348] = Fish2Points[2+3*345];
    Fish2Points[2+3*349] =-116.116005+2728.93*sinf(w-5.942069);
    Fish2Points[2+3*368] =-39.843998+386.25*sinf(w-6.2349043);
    Fish2Points[2+3*370] =182.37201+62.35205*sinf(w-6.275391);
    Fish2Points[2+3*401] =-120.659996+1114.71*sinf(w-6.1438465);
    Fish2Points[2+3*402] =-62.452+1218.2461*sinf(w-6.1309047);
    Fish2Points[2+3*407] =128.42401+409.07202*sinf(w-6.2320514);
    Fish2Points[2+3*429] =109.272+1242.75*sinf(w-6.1278415);
    Fish2Points[2+3*430] =-37.024+1098.196*sinf(w-6.1459107);
    Fish2Points[2+3*431] =-140.92401+391.776*sinf(w-6.2342134);
}


void
DrawAnimatedFish2()
{
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, Fish2Points);
    glNormalPointer (GL_FLOAT, 0, Fish2Normals);

    //TopFin
    glDisable (GL_COLOR_MATERIAL);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 1.0f);
    glDisable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, UserDefined_DS_85_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, UserDefined_DS_85_ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, UserDefined_DS_85_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, UserDefined_DS_85_specular);
    glDrawElements(GL_TRIANGLES, 84, GL_UNSIGNED_INT, &(Fish2Indices[0]));

    //Eye
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);
    glEnable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT, GL_SHININESS, EyePupil_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, EyePupil_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, EyePupil_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, EyePupil_specular);
    glDrawElements(GL_TRIANGLES, 48, GL_UNSIGNED_INT, &(Fish2Indices[84]));

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, EyeWhite_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, EyeWhite_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, EyeWhite_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, EyeWhite_specular);
    glDrawElements(GL_TRIANGLES, 408, GL_UNSIGNED_INT, &(Fish2Indices[132]));

    //BottomFin
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 1.0f);
    glDisable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, UserDefined_DS_85_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, UserDefined_DS_85_ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, UserDefined_DS_85_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, UserDefined_DS_85_specular);
    glDrawElements(GL_TRIANGLES, 93, GL_UNSIGNED_INT, &(Fish2Indices[540]));

    //TailFin
    glDrawElements(GL_TRIANGLES, 93, GL_UNSIGNED_INT, &(Fish2Indices[633]));

    //Body
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);
    glEnable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT, GL_SHININESS, UserDefined_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, UserDefined_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, UserDefined_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, UserDefined_specular);
    glDrawElements(GL_TRIANGLES, 468, GL_UNSIGNED_INT, &(Fish2Indices[726]));
    glDisableClientState (GL_NORMAL_ARRAY);
    glPopAttrib();
}

void
initDrawFish2(float *color)
{
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glEnable(GL_CULL_FACE);

    copyColor((float *)UserDefined_DS_85_ambient, color, 0.68);
    copyColor((float *)UserDefined_DS_85_diffuse, color, 0.68);
    copyColor((float *)UserDefined_DS_85_specular, color, 0.085);
    copyColor((float *)UserDefined_ambient, color, 0.8);
    copyColor((float *)UserDefined_diffuse, color, 0.8);
    copyColor((float *)UserDefined_specular, color, 0.1);
}

void
finDrawFish2()
{
    glDisable(GL_CULL_FACE);
}
