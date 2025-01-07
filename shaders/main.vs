#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

out float visibility;

uniform mat4 gVP;
uniform vec3 gCameraWorldPos;

const float density = 0.008;
const float gradient = 10.5;

void main()
{
    vec3 adjustedPos = aPos + vec3(gCameraWorldPos.x, 0.0, gCameraWorldPos.z);

    vec4 positionRelativeToCam = gVP * vec4(adjustedPos, 1.0);
    gl_Position = positionRelativeToCam;

    float distance = length(positionRelativeToCam.xyz);
    float fogFactor = distance * density;
    visibility = exp(-pow(fogFactor, gradient));
    visibility = clamp(visibility, 0.0, 1.0);

    WorldPos = adjustedPos;
}
