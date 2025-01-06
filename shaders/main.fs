#version 330 core
out vec4 FragColor;

in vec3 WorldPos;

uniform float gGridMinPixelsBetweenCells = 2.0;
uniform float gGridCellSize = 0.025;
uniform vec4 gGridColorThin = vec4(0.5, 0.5, 0.5, 1.0);
uniform vec4 gGridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

void main()
{
    vec2 dvx = vec2(dFdx(WorldPos.x), dFdy(WorldPos.x)); 
    vec2 dvy = vec2(dFdx(WorldPos.z), dFdy(WorldPos.z)); 

    float lx = length(dvx);
    float ly = length(dvy);

    vec2 dudv = vec2(lx, ly);

    float l = length(dudv);

    float LOD = max(0.0, log(l * gGridMinPixelsBetweenCells / gGridCellSize) / log(10.0) + 1.0);

    float GridCellSizeLod0 = gGridCellSize * pow(10.0, floor(LOD));
    float GridCellSizeLod1 = GridCellSizeLod0 * 10.0;
    float GridCellSizeLod2 = GridCellSizeLod1 * 10.0;

    dudv *= 4.0;
    
    vec2 modDivDudv = mod(WorldPos.xz, GridCellSizeLod0) / dudv;
    vec2 result1 = abs(modDivDudv * 2.0 - vec2(1.0));
    float Lod0a = max(1.0 - result1.x, 1.0 - result1.y);

    modDivDudv = mod(WorldPos.xz, GridCellSizeLod1) / dudv;
    vec2 result2 = abs(modDivDudv * 2.0 - vec2(1.0));
    float Lod1a = max(1.0 - result2.x, 1.0 - result2.y);

    modDivDudv = mod(WorldPos.xz, GridCellSizeLod2) / dudv;
    vec2 result3 = abs(modDivDudv * 2.0 - vec2(1.0));
    float Lod2a = max(1.0 - result3.x, 1.0 - result3.y);

    float LOD_fade = fract(LOD);

    vec4 Color;

    if (Lod2a > 0.0){
        Color = gGridColorThick;
        Color.a *= Lod2a;
    } else {
            if (Lod1a > 0.0){
                    Color = mix(gGridColorThick, gGridColorThin, LOD_fade);
                    Color.a *= Lod1a;
                } else {
                    Color = gGridColorThin;
                    Color.a *= (Lod0a * (1.0 - LOD_fade));
             }
        }
    
    FragColor = Color;
}
