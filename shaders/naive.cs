#version 460

//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

precision highp float;
precision highp int;


layout(local_size_x = 16, local_size_y = 16) in;


struct InnerOctant {
    uint children[8];
};

struct Leaf {
	uint color;
};

layout(binding = 0, std430) restrict readonly buffer InnerOctants {
    InnerOctant innerOctants[];
};

layout(binding = 1, std430) restrict readonly buffer Leaves {
    Leaf leaves[];
};


uniform ivec2 resolution;
uniform vec3 cameraPos;
uniform mat3 cameraMat;

layout(rgba8) restrict writeonly uniform image2D texture;


const float MIN_STEP = 0.00001f;

const uint leafBit = 0x80000000, leafData = 0x7FFFFFFF;


float minComponent(in vec3 v) {
    return min(min(v.x, v.y), v.z);
}

float maxComponent(in vec3 v) {
    return max(max(v.x, v.y), v.z);
}

uint find(in vec3 pos, out vec3 base, out float side) {
    float extent = 1.f;

    uint index = 0;  // root
    vec3 offset = vec3(0.f);

    while (true) {
		extent *= 0.5f;
        bvec3 mask = greaterThanEqual(pos, offset + extent);
        int child = int(mask.x) + int(mask.y) * 2 + int(mask.z) * 4;

		offset += vec3(mask) * vec3(extent);

		// return leaf index if child is leaf
		if (innerOctants[index].children[child] >= leafBit) {
			base = offset;
			side = extent;
			return (innerOctants[index].children[child] & leafData);
		}

        index = innerOctants[index].children[child];
    }
}

vec3 castRayOpaque(in vec3 origin, in vec3 dir, in bool drawBlank)
{
    vec3 inverseDir = 1.f / dir;
    vec3 bias = inverseDir * origin;

    vec3 boundsMin = -bias;
    vec3 boundsMax = inverseDir - bias;

    float tMin = maxComponent(min(boundsMin, boundsMax));
    float tMax = minComponent(max(boundsMin, boundsMax));

	// Ray misses root cube
    if (tMin > tMax)
        return vec3(0.f);

    tMin = max(tMin, 0.f);

    float t = tMin + MIN_STEP;

    while (t < tMax)
	{
        vec3 pos = t * dir + origin;
        vec3 offset;
        float side;
        uint node = find(pos, offset, side);

        vec3 nodeMin = offset * inverseDir - bias;
        vec3 nodeMax = (offset + side) * inverseDir - bias;

        float uMin = maxComponent(min(nodeMin, nodeMax));
        float uMax = minComponent(max(nodeMin, nodeMax));

        uMin = max(uMin, 0.f);
        float step = max(uMax - uMin, MIN_STEP);
        t += step;

		if(drawBlank)
			return unpackUnorm4x8(leaves[node].color).rgb;

		vec4 color = unpackUnorm4x8(leaves[node].color);
		if(color.a > 0.f)
			return color.rgb;

		//return vec3(t/tMax);
    }

    return vec3(0.f);
}

vec3 castRayTrans(in vec3 origin, in vec3 dir) {
    vec3 inverseDir = 1.f / dir;
    vec3 bias = inverseDir * origin;

    vec3 boxMin = -bias;
    vec3 boxMax = inverseDir - bias;

    float tMin = maxComponent(min(boxMin, boxMax));
    float tMax = minComponent(max(boxMin, boxMax));

	// Ray misses root cube
    if (tMin > tMax)
        return vec3(0.f);

    tMin = max(tMin, 0.f);

	vec4 finalColor = vec4(0.f);

    float t = tMin + MIN_STEP;

    while (t < tMax)
	{
        vec3 pos = t * dir + origin;
        vec3 offset;
        float side;
        uint node = find(pos, offset, side);

        vec3 nodeMin = offset * inverseDir - bias;
        vec3 nodeMax = (offset + side) * inverseDir - bias;

        float uMin = maxComponent(min(nodeMin, nodeMax));
        float uMax = minComponent(max(nodeMin, nodeMax));

        uMin = max(uMin, 0.f);
        float step = max(uMax - uMin, MIN_STEP);
        t += step;

        vec4 color = unpackUnorm4x8(leaves[node].color);

		// add transparent objects together
		//finalColor += color.rgb * color.a;
		//float finalAlpha = finalColor.a * 0.5f + color.a * 0.5f;
		finalColor = finalColor.a * finalColor + (1.f - finalColor.a) * color;

		// if object is 100% opaque return final color
		if(color.a == 1.f)
			return finalColor.rgb * finalColor.a;
    }

    return finalColor.rgb * finalColor.a;
}

void main() {
    vec3 rayVec = normalize(vec3((gl_GlobalInvocationID.xy-(0.5f*resolution))/resolution.y, 1.f)) * cameraMat;

    //vec3 color = castRayOpaque(cameraPos, rayVec, false);
    vec3 color = castRayTrans(cameraPos, rayVec);

    imageStore(texture, ivec2(gl_GlobalInvocationID.xy), vec4(color, 0.f));
}
