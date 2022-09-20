#version 460

layout(offset = 0, binding = 0) uniform atomic_uint voxelIndex;
layout(std430, binding = 0) writeonly buffer voxelBuffer { uvec4 voxels[]; };
layout(binding = 1) uniform sampler2D diffuseTex;

in vec3 gsVoxelPos;
in vec2 gsTexCoords;


void main()
{
	vec4 color = texture(diffuseTex, gsTexCoords);
	if(color.a < .1f)
		discard;
		
	uint idx = atomicCounterIncrement(voxelIndex);
	if(idx > 0x20000000u)
		discard;

	voxels[idx] = uvec4(uvec3(gsVoxelPos), packUnorm4x8(color));
}
