#version 460

#extension GL_ARB_gpu_shader_int64 : enable


layout(triangles) in;
layout (triangle_strip, max_vertices = 3) out;


in vec3 vsNormals[];
in vec2 vsTexCoords[];

uniform float voxelResolution;

layout(offset = 0, binding = 0) uniform atomic_uint voxelIndex;
layout(std430, binding = 0) writeonly buffer voxelBuffer { uvec4 voxels[]; };

layout(binding = 1) uniform sampler2D diffuseTex;
// layout(binding = 2) uniform sampler2D aoTex;
// layout(binding = 3) uniform sampler2D emissiveTex;
// layout(binding = 4) uniform sampler2D metalRoughnessTex;
// layout(binding = 5) uniform sampler2D normlTex;


void barycentric(in const vec3 p, in const mat3 vertices, out vec3 projCoord);
void calcNormalAndAxis( inout mat3 vertices, out vec3 normal, out mat3 rotationMatrix);

uint64_t encodeComponent(in const uint c);
uvec2 morton(in const uvec3 pos);
bool writeVoxels(in const uvec3 coord, in const vec4 color, in const vec3 normal);

void voxelize(in const mat3 vertices, in const vec3 normal, in const mat3 rotationMatrix, in const uvec3 minVoxIndex, in const uvec3 maxVoxIndex);


void main() {
    mat3 vertices = { gl_in[0].gl_Position.xyz,
                      gl_in[1].gl_Position.xyz,
                      gl_in[2].gl_Position.xyz };

    vec3 normal;
    mat3 rotationMatrix;
    
    calcNormalAndAxis(vertices, normal, rotationMatrix);

    // limit triangle overlap test to the voxel space of triangle's AABB for efficiency
    const vec3 AABBmin = min(min(vertices[0], vertices[1]), vertices[2]);
    const vec3 AABBmax = max(max(vertices[0], vertices[1]), vertices[2]);

    const uvec3 minVoxIndex = uvec3(clamp(floor(AABBmin), uvec3(0), uvec3(voxelResolution)));
    const uvec3 maxVoxIndex = uvec3(clamp( ceil(AABBmax), uvec3(0), uvec3(voxelResolution)));

    voxelize(vertices, normal, rotationMatrix, minVoxIndex, maxVoxIndex);
}


void barycentric(in const vec3 p, in const mat3 vertices, out vec3 projCoord) {
    const vec3 v0 = vertices[1] - vertices[0],
               v1 = vertices[2] - vertices[0],
               v2 =           p - vertices[0];

    const float den = v0.x * v1.y - v1.x * v0.y;

    projCoord.y = (v2.x * v1.y - v1.x * v2.y) / den;
    projCoord.z = (v0.x * v2.y - v2.x * v0.y) / den;
    projCoord.x = 1.f - projCoord.y - projCoord.z;
}

void calcNormalAndAxis( inout mat3 vertices, out vec3 normal, out mat3 rotationMatrix) {
    normal = normalize(cross(vertices[1] - vertices[0], vertices[2] - vertices[1]));
    const vec3 absN = abs(normal);
    const float maxAbsN = max(max(absN.x, absN.y), absN.z);

    vertices[0] = (absN.x == maxAbsN) ? vertices[0].yzx : (absN.y == maxAbsN) ? vertices[0].zxy : vertices[0];
    vertices[1] = (absN.x == maxAbsN) ? vertices[1].yzx : (absN.y == maxAbsN) ? vertices[1].zxy : vertices[1];
    vertices[2] = (absN.x == maxAbsN) ? vertices[2].yzx : (absN.y == maxAbsN) ? vertices[2].zxy : vertices[2];

    normal = (absN.x == maxAbsN) ? normal.yzx : (absN.y == maxAbsN) ? normal.zxy : normal;

    rotationMatrix = (absN.x == maxAbsN) ? mat3(0,1,0,
                                                0,0,1,
                                                1,0,0) : (absN.y == maxAbsN) ? mat3(0,0,1,
                                                                                    1,0,0,
                                                                                    0,1,0) : mat3(1);
}


uint64_t encodeComponent(in const uint c) {
    uint64_t morton = c & 0x3FFFFFu;

    morton = (morton ^ (morton << 32)) & 0x1F00000000FFFFl;
    morton = (morton ^ (morton << 16)) & 0x1F0000FF0000FFl;
    morton = (morton ^ (morton << 8))  & 0x100F00F00F00F00Fl;
    morton = (morton ^ (morton << 4))  & 0x10C30C30C30C30C3l;
    
    return   (morton ^ (morton << 2))  & 0x1249249249249249l;
}

uvec2 morton(in const uvec3 pos) {
    uint64_t x = encodeComponent(pos.x);
    uint64_t y = encodeComponent(pos.y);
    uint64_t z = encodeComponent(pos.z);

    uint64_t morton = x | (y << 1) | (z << 2);

    return uvec2(morton >> 32, morton & 0xFFFFFFFFu);
}


bool writeVoxels(in const uvec3 coord, in const vec4 color, in const vec3 normal) {
    const uint idx = atomicCounterIncrement(voxelIndex);
    if (idx >= 0x1FFFFFFFu) {
        atomicCounterExchange(voxelIndex, 0x1FFFFFFFu);
        return false;
    }

    voxels[idx] = uvec4( morton(coord),
                         packUnorm4x8 (color),
                         packSnorm4x8 (vec4(normal, 0)) );

    return true;
}


// using implementation from https://jcgt.org/published/0002/01/02/, figure 17
void voxelize( in const mat3 vertices,
               in const vec3 normal,
               in const mat3 rotationMatrix,
               in const uvec3 minVoxIndex,
               in const uvec3 maxVoxIndex )
{
    vec3 e0 = vertices[1] - vertices[0];
    vec3 e1 = vertices[2] - vertices[1];
    vec3 e2 = vertices[0] - vertices[2];

    // line 4
    vec2 n_e0_xy = (normal.z >= 0) ? vec2(-e0.y, e0.x) : vec2(e0.y, -e0.x);
    vec2 n_e1_xy = (normal.z >= 0) ? vec2(-e1.y, e1.x) : vec2(e1.y, -e1.x);
    vec2 n_e2_xy = (normal.z >= 0) ? vec2(-e2.y, e2.x) : vec2(e2.y, -e2.x);

    // line 5
    vec2 n_e0_yz = (normal.x >= 0) ? vec2(-e0.z, e0.y) : vec2(e0.z, -e0.y);
    vec2 n_e1_yz = (normal.x >= 0) ? vec2(-e1.z, e1.y) : vec2(e1.z, -e1.y);
    vec2 n_e2_yz = (normal.x >= 0) ? vec2(-e2.z, e2.y) : vec2(e2.z, -e2.y);

    // line 6
    vec2 n_e0_zx = (normal.y >= 0) ? vec2(-e0.x, e0.z) : vec2(e0.x, -e0.z);
    vec2 n_e1_zx = (normal.y >= 0) ? vec2(-e1.x, e1.z) : vec2(e1.x, -e1.z);
    vec2 n_e2_zx = (normal.y >= 0) ? vec2(-e2.x, e2.z) : vec2(e2.x, -e2.z);

    
    // line 7
    const float d_e0_xy = -dot(n_e0_xy, vertices[0].xy) + max(0.0f, n_e0_xy.x) + max(0.0f, n_e0_xy.y);
    const float d_e1_xy = -dot(n_e1_xy, vertices[1].xy) + max(0.0f, n_e1_xy.x) + max(0.0f, n_e1_xy.y);
    const float d_e2_xy = -dot(n_e2_xy, vertices[2].xy) + max(0.0f, n_e2_xy.x) + max(0.0f, n_e2_xy.y);

    // line 8
    const float d_e0_yz = -dot(n_e0_yz, vertices[0].yz) + max(0.0f, n_e0_yz.x) + max(0.0f, n_e0_yz.y);
    const float d_e1_yz = -dot(n_e1_yz, vertices[1].yz) + max(0.0f, n_e1_yz.x) + max(0.0f, n_e1_yz.y);
    const float d_e2_yz = -dot(n_e2_yz, vertices[2].yz) + max(0.0f, n_e2_yz.x) + max(0.0f, n_e2_yz.y);

    // line 9
    const float d_e0_zx = -dot(n_e0_zx, vertices[0].zx) + max(0.0f, n_e0_zx.x) + max(0.0f, n_e0_zx.y);
    const float d_e1_zx = -dot(n_e1_zx, vertices[1].zx) + max(0.0f, n_e1_zx.x) + max(0.0f, n_e1_zx.y);
    const float d_e2_zx = -dot(n_e2_zx, vertices[2].zx) + max(0.0f, n_e2_zx.x) + max(0.0f, n_e2_zx.y);


    // line 10
    const vec3 nProj = (normal.z < 0.0) ? -normal : normal;

    const float dTri = dot(nProj, vertices[0]);

    const float dTriFatMin = dTri - max(nProj.x, 0) - max(nProj.y, 0);	//line 11
    const float dTriFatMax = dTri - min(nProj.x, 0) - min(nProj.y, 0);	//line 12

    const float nzInv = 1.0 / nProj.z;
    
    uvec3 pos;
    uint zMin, zMax;
    for (pos.x = minVoxIndex.x; pos.x < maxVoxIndex.x; pos.x++) {
        for (pos.y = minVoxIndex.y; pos.y < maxVoxIndex.y; pos.y++) {
            float dd_e0_xy = d_e0_xy + dot(n_e0_xy, pos.xy);
            float dd_e1_xy = d_e1_xy + dot(n_e1_xy, pos.xy);
            float dd_e2_xy = d_e2_xy + dot(n_e2_xy, pos.xy);
        
            bool xy_overlap = (dd_e0_xy >= 0) && (dd_e1_xy >= 0) && (dd_e2_xy >= 0);

            if (xy_overlap) {
                float dot_n_p = dot(nProj.xy, pos.xy);

                const float zMinInt = (-dot_n_p + dTriFatMin) * nzInv;
                const float zMaxInt = (-dot_n_p + dTriFatMax) * nzInv;

                const float zMinFloor = floor(zMinInt);
                const float zMaxCeil  =  ceil(zMaxInt);

                zMin = int(zMinFloor) - int(zMinFloor == zMinInt);
                zMax = int(zMaxCeil ) + int(zMaxCeil  == zMaxInt);

                zMin = max(minVoxIndex.z, zMin);
                zMax = min(maxVoxIndex.z, zMax);
                
                for (pos.z = zMin; pos.z < zMax; pos.z++) {
                    float dd_e0_yz = d_e0_yz + dot(n_e0_yz, pos.yz);
                    float dd_e1_yz = d_e1_yz + dot(n_e1_yz, pos.yz);
                    float dd_e2_yz = d_e2_yz + dot(n_e2_yz, pos.yz);

                    float dd_e0_zx = d_e0_zx + dot(n_e0_zx, pos.zx);
                    float dd_e1_zx = d_e1_zx + dot(n_e1_zx, pos.zx);
                    float dd_e2_zx = d_e2_zx + dot(n_e2_zx, pos.zx);

                    bool yz_overlap = (dd_e0_yz >= 0) && (dd_e1_yz >= 0) && (dd_e2_yz >= 0);
                    bool zx_overlap = (dd_e0_zx >= 0) && (dd_e1_zx >= 0) && (dd_e2_zx >= 0);

                    if(yz_overlap && zx_overlap) {
                        vec3 projCoord;
                        barycentric(pos, vertices, projCoord);

                        const vec2 texCoords = projCoord.x * vsTexCoords[0] + projCoord.y * vsTexCoords[1] + projCoord.z * vsTexCoords[2];

                        const uvec3 pos = uvec3((rotationMatrix * pos) / voxelResolution);
                        const vec4 color = texture(diffuseTex, texCoords);
                        vec3 n = projCoord.x * vsNormals[0] + projCoord.y * vsNormals[1] + projCoord.z * vsNormals[2];

                        if (!writeVoxels(pos, color, normalize(n)))
                            return;
                    }
                }
            }
        }
    }
}