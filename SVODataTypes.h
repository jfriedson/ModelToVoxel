#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>


struct MaskType {
	const uint32_t leafBit{ 0x80000000U },
				   nodeIdx{ 0x7FFFFFFFU };
};


struct InnerOctant {
	uint32_t children[8];    // children indices
	//uint32_t color;    // RGBA format - 8 bits per channel

	InnerOctant() {
		for (unsigned int c = 0; c < 8; ++c)
			children[c] = 0;
	}

	InnerOctant(const uint32_t val) {
		for (unsigned int c = 0; c < 8; ++c)
			children[c] = val;
	}
};

struct Leaf {
	uint32_t color;    // RGBA format - 8 bits per channel
	uint32_t normal;

	// add metallic, roughness, specular

	Leaf() : color{ 0 }, normal{ 0 }
	{}

	Leaf(const uint32_t color, const uint32_t normal) :
		color{ color },
		normal{ normal }
	{}
};
