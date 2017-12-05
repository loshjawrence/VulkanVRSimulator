#pragma once

#include "VulkanContextInfo.h"
#include "stb_image_write.h"
#include "stb_image.h"
#include <string>
enum class StencilType {
	RadialDensityMask = 0, FixedFoveated, 
	PreCalcBarrelSamplingMask
};
class PreMadeStencil {
public:
	PreMadeStencil(const VulkanContextInfo& contextInfo, const uint32_t qualityIndex, const StencilType type);
	PreMadeStencil();
	~PreMadeStencil();
	void createPreCalcBarrelSamplingStencilMask(const VulkanContextInfo& contextInfo);
	void createRadialDensityStencilMask(const VulkanContextInfo& contextInfo);
	void createFixedFoveatedStencilMask(const VulkanContextInfo& contextInfo);
	void writeStencilToImage(const std::vector<uint8_t>& maskData);
	void genFileName();

public:
	bool writeStencil;

	StencilType type;
	float qualityScale;
	uint32_t qualityIndex;
	std::string filename;
	uint32_t width;
	uint32_t height;
	
	bool pretendStartsVR = true;
	uint32_t stencilMaskVal = 1;
	float middleRegionRadius = 0.52;//roughly 0.52
	float NDCcenterOffset = 0.1425;//0.15 ndc centeer UV center offset 0.0375
	float extraRadius = NDCcenterOffset*0.5f;//same as NDCcenterOffset?
	std::vector<glm::vec2> ndcCenter = { glm::vec2( NDCcenterOffset, 0.f), 
											   glm::vec2(-NDCcenterOffset, 0.f) };

};

