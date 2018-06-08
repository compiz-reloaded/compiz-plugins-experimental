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
#include "fish.h"

void
AnimateFish(float t)
{
    int   ti = (int) t;
    float w = 2*PI*(t-ti);
    FishPoints[2+3*5] =-386.536+627.23804*sinf(w-6.2204614);
    FishPoints[2+3*8] =632.03595+572.7439*sinf(w-6.225911);
    FishPoints[2+3*36] =92.51199+211.3501*sinf(w-6.26205);
    FishPoints[2+3*51] =160.572+936.5581*sinf(w-6.1895294);
    FishPoints[2+3*71] =5.152+832.3779*sinf(w-6.1999474);
    FishPoints[2+3*316] =29.880001+149.23804*sinf(w-6.2682614);
    FishPoints[2+3*326] = FishPoints[2+3*316];
    FishPoints[2+3*381] =-68.796+624.1221*sinf(w-6.220773);
    FishPoints[2+3*382] =-23.82+2785.5962*sinf(w-6.004626);
    FishPoints[2+3*383] =-74.631996+3179.0142*sinf(w-5.965284);
    FishPoints[2+3*384] =-27.076+697.8159*sinf(w-6.2134037);
    FishPoints[2+3*385] =-32.272+3485.4321*sinf(w-5.9346423);
    FishPoints[2+3*386] =-23.824001+3429.102*sinf(w-5.940275);
    FishPoints[2+3*387] = FishPoints[2+3*384];
    FishPoints[2+3*388] = FishPoints[2+3*386];
    FishPoints[2+3*389] =-11.12+3014.1416*sinf(w-5.981771);
    FishPoints[2+3*390] = FishPoints[2+3*384];
    FishPoints[2+3*391] = FishPoints[2+3*389];
    FishPoints[2+3*392] =-6.884+2005.706*sinf(w-6.082615);
    FishPoints[2+3*393] = FishPoints[2+3*384];
    FishPoints[2+3*394] = FishPoints[2+3*392];
    FishPoints[2+3*395] =-11.12+1026.46*sinf(w-6.180539);
    FishPoints[2+3*396] =-25.284+3129.206*sinf(w-5.970265);
    FishPoints[2+3*397] =64.871994+678.3181*sinf(w-6.2153535);
    FishPoints[2+3*398] =-69.856+674.24*sinf(w-6.215761);
    FishPoints[2+3*399] =65.119995+638.29395*sinf(w-6.219356);
    FishPoints[2+3*400] =-69.852005+633.33984*sinf(w-6.2198515);
    FishPoints[2+3*401] =-82.524+3260.712*sinf(w-5.957114);
    FishPoints[2+3*402] =-50.124+651.2622*sinf(w-6.218059);
    FishPoints[2+3*403] =61.896+648.6819*sinf(w-6.218317);
    FishPoints[2+3*404] =35.456+2975.998*sinf(w-5.9855857);
    FishPoints[2+3*405] =-25.744001+3219.314*sinf(w-5.961254);
    FishPoints[2+3*406] =-69.848+639.1841*sinf(w-6.219267);
    FishPoints[2+3*407] = FishPoints[2+3*399];
    FishPoints[2+3*408] =-24.4+2638.6577*sinf(w-6.0193195);
    FishPoints[2+3*409] =75.19199+665.4758*sinf(w-6.2166376);
    FishPoints[2+3*410] =-69.852005+662.55396*sinf(w-6.21693);
    FishPoints[2+3*411] =-23.6+2713.648*sinf(w-6.0118203);
    FishPoints[2+3*412] =35.456+2708.5278*sinf(w-6.0123324);
    FishPoints[2+3*413] =75.188+671.3181*sinf(w-6.2160535);
    FishPoints[2+3*414] = FishPoints[2+3*384];
    FishPoints[2+3*415] =6.7400002+692.67993*sinf(w-6.2139173);
    FishPoints[2+3*416] =-5.8960004+3352.5718*sinf(w-5.947928);
    FishPoints[2+3*417] =-23.332+3019.3022*sinf(w-5.981255);
    FishPoints[2+3*418] =67.644+643.5161*sinf(w-6.218834);
    FishPoints[2+3*419] =35.456+2983.812*sinf(w-5.984804);
    FishPoints[2+3*420] = FishPoints[2+3*384];
    FishPoints[2+3*421] = FishPoints[2+3*416];
    FishPoints[2+3*422] = FishPoints[2+3*385];
    FishPoints[2+3*423] =69.523994+627.49194*sinf(w-6.220436);
    FishPoints[2+3*424] = FishPoints[2+3*401];
    FishPoints[2+3*425] = FishPoints[2+3*400];
    FishPoints[2+3*426] =-100.02+2737.7021*sinf(w-6.009415);
    FishPoints[2+3*427] =75.19199+659.6321*sinf(w-6.217222);
    FishPoints[2+3*428] =35.456+2619.7861*sinf(w-6.021207);
    FishPoints[2+3*429] = FishPoints[2+3*411];
    FishPoints[2+3*430] =-69.852005+668.396*sinf(w-6.216346);
    FishPoints[2+3*431] =-83.1+2754.706*sinf(w-6.0077147);
    FishPoints[2+3*432] =-24.684+2673.6265*sinf(w-6.015823);
    FishPoints[2+3*433] =28.596+654.9358*sinf(w-6.217692);
    FishPoints[2+3*434] =-83.1+2777.0298*sinf(w-6.005482);
    FishPoints[2+3*435] = FishPoints[2+3*405];
    FishPoints[2+3*436] = FishPoints[2+3*399];
    FishPoints[2+3*437] =35.456+3156.4238*sinf(w-5.967543);
    FishPoints[2+3*438] = FishPoints[2+3*416];
    FishPoints[2+3*439] = FishPoints[2+3*415];
    FishPoints[2+3*440] =-32.236+3465.0083*sinf(w-5.9366846);
    FishPoints[2+3*441] = FishPoints[2+3*432];
    FishPoints[2+3*442] = FishPoints[2+3*427];
    FishPoints[2+3*443] = FishPoints[2+3*433];
    FishPoints[2+3*444] = FishPoints[2+3*418];
    FishPoints[2+3*445] = FishPoints[2+3*406];
    FishPoints[2+3*446] =-82.368004+3253.542*sinf(w-5.957831);
    FishPoints[2+3*447] = FishPoints[2+3*396];
    FishPoints[2+3*448] = FishPoints[2+3*398];
    FishPoints[2+3*449] =-96.064+3134.646*sinf(w-5.969721);
    FishPoints[2+3*450] = FishPoints[2+3*408];
    FishPoints[2+3*451] =51.031998+2553.2256*sinf(w-6.0278625);
    FishPoints[2+3*452] = FishPoints[2+3*409];
    FishPoints[2+3*453] =13.415999+687.7261*sinf(w-6.2144127);
    FishPoints[2+3*454] = FishPoints[2+3*440];
    FishPoints[2+3*455] = FishPoints[2+3*415];
    FishPoints[2+3*456] = FishPoints[2+3*403];
    FishPoints[2+3*457] =-69.856+645.0261*sinf(w-6.218683);
    FishPoints[2+3*458] =-82.484+3081.8223*sinf(w-5.9750032);
    FishPoints[2+3*459] = FishPoints[2+3*432];
    FishPoints[2+3*460] = FishPoints[2+3*428];
    FishPoints[2+3*461] = FishPoints[2+3*427];
    FishPoints[2+3*462] = FishPoints[2+3*381];
    FishPoints[2+3*463] =34.792+3136.588*sinf(w-5.9695263);
    FishPoints[2+3*464] = FishPoints[2+3*423];
    FishPoints[2+3*465] = FishPoints[2+3*417];
    FishPoints[2+3*466] = FishPoints[2+3*457];
    FishPoints[2+3*467] = FishPoints[2+3*418];
    FishPoints[2+3*468] = FishPoints[2+3*453];
    FishPoints[2+3*469] =-15.356+3333.288*sinf(w-5.9498563);
    FishPoints[2+3*470] = FishPoints[2+3*440];
    FishPoints[2+3*471] = FishPoints[2+3*403];
    FishPoints[2+3*472] = FishPoints[2+3*458];
    FishPoints[2+3*473] = FishPoints[2+3*404];
    FishPoints[2+3*474] = FishPoints[2+3*423];
    FishPoints[2+3*475] = FishPoints[2+3*463];
    FishPoints[2+3*476] = FishPoints[2+3*401];
    FishPoints[2+3*477] =-25.232+3388.854*sinf(w-5.9442997);
    FishPoints[2+3*478] = FishPoints[2+3*469];
    FishPoints[2+3*479] = FishPoints[2+3*453];
    FishPoints[2+3*480] = FishPoints[2+3*451];
    FishPoints[2+3*481] = FishPoints[2+3*430];
    FishPoints[2+3*482] = FishPoints[2+3*409];
    FishPoints[2+3*483] = FishPoints[2+3*411];
    FishPoints[2+3*484] = FishPoints[2+3*413];
    FishPoints[2+3*485] = FishPoints[2+3*430];
    FishPoints[2+3*486] = FishPoints[2+3*477];
    FishPoints[2+3*487] = FishPoints[2+3*453];
    FishPoints[2+3*488] =-16.188+3417.1763*sinf(w-5.941468);
    FishPoints[2+3*489] = FishPoints[2+3*381];
    FishPoints[2+3*490] =-23.82+929.4241*sinf(w-6.190243);
    FishPoints[2+3*491] =-6.884+1842.5039*sinf(w-6.098935);
    FishPoints[2+3*492] = FishPoints[2+3*381];
    FishPoints[2+3*493] = FishPoints[2+3*383];
    FishPoints[2+3*494] =-94.336006+3240.5679*sinf(w-5.9591284);
    FishPoints[2+3*495] =-23.84+3048.9902*sinf(w-5.9782863);
    FishPoints[2+3*496] = FishPoints[2+3*433];
    FishPoints[2+3*497] = FishPoints[2+3*402];
    FishPoints[2+3*498] = FishPoints[2+3*381];
    FishPoints[2+3*499] = FishPoints[2+3*491];
    FishPoints[2+3*500] = FishPoints[2+3*382];
    FishPoints[2+3*501] =88.892+683.448*sinf(w-6.2148404);
    FishPoints[2+3*502] = FishPoints[2+3*488];
    FishPoints[2+3*503] = FishPoints[2+3*453];
    FishPoints[2+3*504] = FishPoints[2+3*418];
    FishPoints[2+3*505] = FishPoints[2+3*446];
    FishPoints[2+3*506] = FishPoints[2+3*419];
    FishPoints[2+3*507] = FishPoints[2+3*433];
    FishPoints[2+3*508] =32.732+2941.5264*sinf(w-5.9890327);
    FishPoints[2+3*509] = FishPoints[2+3*434];
    FishPoints[2+3*510] = FishPoints[2+3*495];
    FishPoints[2+3*511] = FishPoints[2+3*508];
    FishPoints[2+3*512] = FishPoints[2+3*433];
    FishPoints[2+3*513] = FishPoints[2+3*426];
    FishPoints[2+3*514] = FishPoints[2+3*410];
    FishPoints[2+3*515] = FishPoints[2+3*427];
    FishPoints[2+3*516] = FishPoints[2+3*501];
    FishPoints[2+3*517] =48.156002+3121.374*sinf(w-5.971048);
    FishPoints[2+3*518] = FishPoints[2+3*488];
    FishPoints[2+3*519] = FishPoints[2+3*410];
    FishPoints[2+3*520] = FishPoints[2+3*426];
    FishPoints[2+3*521] = FishPoints[2+3*408];
    FishPoints[2+3*522] = FishPoints[2+3*405];
    FishPoints[2+3*523] = FishPoints[2+3*446];
    FishPoints[2+3*524] = FishPoints[2+3*406];
    FishPoints[2+3*525] = FishPoints[2+3*417];
    FishPoints[2+3*526] = FishPoints[2+3*458];
    FishPoints[2+3*527] = FishPoints[2+3*457];
    FishPoints[2+3*528] =-23.016+3148.838*sinf(w-5.9683013);
    FishPoints[2+3*529] = FishPoints[2+3*501];
    FishPoints[2+3*530] =-71.528+680.0139*sinf(w-6.2151837);
    FishPoints[2+3*531] = FishPoints[2+3*402];
    FishPoints[2+3*532] = FishPoints[2+3*404];
    FishPoints[2+3*533] =-82.504+3102.292*sinf(w-5.972956);
    FishPoints[2+3*534] = FishPoints[2+3*528];
    FishPoints[2+3*535] = FishPoints[2+3*530];
    FishPoints[2+3*536] =-82.716+3204.4316*sinf(w-5.9627423);
    FishPoints[2+3*537] = FishPoints[2+3*396];
    FishPoints[2+3*538] =35.456+3074.2563*sinf(w-5.9757595);
    FishPoints[2+3*539] = FishPoints[2+3*397];
    FishPoints[2+3*540] = FishPoints[2+3*495];
    FishPoints[2+3*541] = FishPoints[2+3*402];
    FishPoints[2+3*542] = FishPoints[2+3*533];
    FishPoints[2+3*543] = FishPoints[2+3*451];
    FishPoints[2+3*544] = FishPoints[2+3*431];
    FishPoints[2+3*545] = FishPoints[2+3*430];
    FishPoints[2+3*546] = FishPoints[2+3*528];
    FishPoints[2+3*547] = FishPoints[2+3*517];
    FishPoints[2+3*548] = FishPoints[2+3*501];
    FishPoints[2+3*549] = FishPoints[2+3*397];
    FishPoints[2+3*550] = FishPoints[2+3*536];
    FishPoints[2+3*551] = FishPoints[2+3*530];
    FishPoints[2+3*552] = FishPoints[2+3*399];
    FishPoints[2+3*553] = FishPoints[2+3*401];
    FishPoints[2+3*554] = FishPoints[2+3*437];
    FishPoints[2+3*555] = FishPoints[2+3*449];
    FishPoints[2+3*556] = FishPoints[2+3*398];
    FishPoints[2+3*557] = FishPoints[2+3*413];
    FishPoints[2+3*558] = FishPoints[2+3*381];
    FishPoints[2+3*559] = FishPoints[2+3*494];
    FishPoints[2+3*560] = FishPoints[2+3*463];
    FishPoints[2+3*561] = FishPoints[2+3*449];
    FishPoints[2+3*562] = FishPoints[2+3*413];
    FishPoints[2+3*563] = FishPoints[2+3*412];
    FishPoints[2+3*564] = FishPoints[2+3*397];
    FishPoints[2+3*565] = FishPoints[2+3*538];
    FishPoints[2+3*566] = FishPoints[2+3*536];
}


void
DrawAnimatedFish()
{
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, FishPoints);
    glNormalPointer (GL_FLOAT, 0, FishNormals);

    //Body
    glDisable (GL_COLOR_MATERIAL);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, UserDefined_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, UserDefined_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, UserDefined_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, UserDefined_specular);
    glDrawElements(GL_TRIANGLES, 408, GL_UNSIGNED_INT, &(FishIndices[0]));

    //TopSideFin
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 1.0f);
    glDisable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, UserDefined_DS_80_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, UserDefined_DS_80_ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, UserDefined_DS_80_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, UserDefined_DS_80_specular);
    glDrawElements(GL_TRIANGLES, 93, GL_UNSIGNED_INT, &(FishIndices[408]));

    //BottomSideFin
    glDrawElements(GL_TRIANGLES, 93, GL_UNSIGNED_INT, &(FishIndices[501]));

    //TopFin
    glDrawElements(GL_TRIANGLES, 123, GL_UNSIGNED_INT, &(FishIndices[594]));

    //TailFin
    glDrawElements(GL_TRIANGLES, 186, GL_UNSIGNED_INT, &(FishIndices[717]));

    //Eye
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);
    glEnable(GL_CULL_FACE);
    glMaterialfv (GL_FRONT, GL_SHININESS, EyeWhite_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, EyeWhite_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, EyeWhite_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, EyeWhite_specular);
    glDrawElements(GL_TRIANGLES, 324, GL_UNSIGNED_INT, &(FishIndices[903]));

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMaterialfv (GL_FRONT, GL_SHININESS, EyePupil_shininess);
    glMaterialfv (GL_FRONT, GL_AMBIENT, EyePupil_ambient);
    glMaterialfv (GL_FRONT, GL_DIFFUSE, EyePupil_diffuse);
    glMaterialfv (GL_FRONT, GL_SPECULAR, EyePupil_specular);
    glDrawElements(GL_TRIANGLES, 60, GL_UNSIGNED_INT, &(FishIndices[1227]));
    glDisableClientState (GL_NORMAL_ARRAY);
    glPopAttrib();
}

void
initDrawFish(float *color)
{
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glEnable(GL_CULL_FACE);

    copyColor((float *)UserDefined_DS_80_ambient, color, 0.64);
    copyColor((float *)UserDefined_DS_80_diffuse, color, 0.64);
    copyColor((float *)UserDefined_DS_80_specular, color, 0.08);
    copyColor((float *)UserDefined_ambient, color, 0.8);
    copyColor((float *)UserDefined_diffuse, color, 0.8);
    copyColor((float *)UserDefined_specular, color, 0.1);
}

void
finDrawFish()
{
    glDisable(GL_CULL_FACE);
}
