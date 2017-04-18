varying vec3 normal, halfVect, lightDir;
varying vec4 ambient, diffuse;

void main()
{
    /* Transformation in eye coordinates and normalization */
    normal = normalize (gl_NormalMatrix * gl_Normal);
    halfVect = normalize (gl_LightSource[1].halfVector.xyz);
    lightDir = normalize (vec3(gl_LightSource[1].position));

    /* Light */
    ambient = (gl_LightSource[1].ambient + gl_LightModel.ambient) * gl_FrontMaterial.ambient;
    diffuse = gl_LightSource[1].diffuse * gl_FrontMaterial.diffuse;

    /* Texture */
    gl_TexCoord[0] = gl_MultiTexCoord0;

    gl_Position = ftransform ();
}