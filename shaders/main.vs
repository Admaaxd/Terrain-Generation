#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 gVP;
uniform vec3 gCameraWorldPos;

void main()
{
    vec3 adjustedPos = aPos + vec3(gCameraWorldPos.x, 0.0, gCameraWorldPos.z);

    gl_Position = gVP * vec4(adjustedPos, 1.0);

    WorldPos = adjustedPos;
}
