#version 460


in vec3 position;
in vec2 texCoords;
in vec3 normal;

uniform float voxelResolution;

out vec3 vsNormals;
out vec2 vsTexCoords;


void main() {
    vsNormals = normal;
    vsTexCoords = texCoords;

    gl_Position = vec4(position * voxelResolution, 1);
}
