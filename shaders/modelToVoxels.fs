#version 460

layout(offset = 0, binding = 0) uniform atomic_uint voxelIndex;
layout(std430, binding = 0) writeonly buffer voxelBuffer { uvec4 voxels[]; };
layout(binding = 1) uniform sampler2D diffuseTex;

in vec3 gsVoxelPos;
in vec2 gsTexCoords;

//uniform ivec3 voxelResolution;

void main()
{
	vec4 color = texture(diffuseTex, gsTexCoords);
	if(color.a != 1.f) {
		discard;
		return;
	}

	uvec3 voxelCoord = uvec3(gsVoxelPos);

	uint idx = atomicCounterIncrement(voxelIndex);
	if(idx == 0xFFFFFFFFu) {
		discard;
		return;
	}

	voxels[idx] = uvec4(voxelCoord, packUnorm4x8(color));
	//voxels[idx] = uvec4(voxelCoord, packUnorm4x8(vec4(1.f)));
}

//
// #version 460
//
// layout(offset = 0, binding = 0) uniform atomic_uint voxelIndex;
// layout(std430, binding = 0) writeonly buffer voxelBuffer { uvec4 voxels[]; };
// layout(binding = 1) uniform sampler2D diffuseTex;
//
// in vec3 gsVoxelPos;
// in vec2 gsTexCoords;
//
// uniform ivec3 voxelResolution;
//
// void main()
// {
// 	vec4 color = texture(diffuseTex, gsTexCoords);
// 	//uvec3 voxelCoord = uvec3(round(gsVoxelPos));
//
// 	//if(any(lessThan(voxelCoord, uvec3(0u))) || any(greaterThanEqual(voxelCoord, voxelResolution)))
// 	//	return;
//
//
// 	uint idx = atomicCounterIncrement(voxelIndex);
// 	if(idx == 0xFFFFFFFFu)
// 		return;
//
// 	//uvec3 uvoxel_pos = clamp(uvec3(gsVoxelPos * voxelResolution), uvec3(0u), uvec3(voxelResolution - 1u));
// 	voxels[idx] = uvec4(gsVoxelPos, color);
//
// 	//voxels[idx] = uvec4(voxelCoord, packUnorm4x8(texture(diffuseTex, gsTexCoords)));
// 	//voxels[idx] = uvec4(voxelCoord, packUnorm4x8(vec4(1.f)));
// }
