#pragma once
#include "PreMadeStencil.h"
#include "GlobalSettings.h"
#include "Mesh.h"
#include <iostream>
#include <sstream>
#include <string>


PreMadeStencil::PreMadeStencil(const VulkanContextInfo& contextInfo, const uint32_t qualityIndex, const StencilType type)
	: type(type), qualityIndex(qualityIndex), qualityScale(contextInfo.camera.vrScalings[qualityIndex]) 


	//set to false to just read the mask from disk
	, writeStencil(false)
{
	if (!writeStencil) { genFileName(); return; }
	if (StencilType::RadialDensityMask == type) {
		createRadialDensityStencilMask(contextInfo);
	} else if (StencilType::FixedFoveated == type) {
		createFixedFoveatedStencilMask(contextInfo);
	} else if (StencilType::PreCalcBarrelSamplingMask == type) {
		createPreCalcBarrelSamplingStencilMask(contextInfo);
	}
}

PreMadeStencil::PreMadeStencil() {
}

PreMadeStencil::~PreMadeStencil() {
}
void PreMadeStencil::createFixedFoveatedStencilMask(const VulkanContextInfo& contextInfo) {
	width = pretendStartsVR ? contextInfo.camera.renderTargetExtent.width : hmdWidth;
	height = pretendStartsVR ? contextInfo.camera.renderTargetExtent.height : hmdHeight;
	width	= (width &  1) == 1 ? width  - 1 : width;
	height	= (height & 1) == 1 ? height - 1 : height;


}

//if this works should be able to avoid having to hole fill like the other radial quad masking technuiques
void PreMadeStencil::createPreCalcBarrelSamplingStencilMask(const VulkanContextInfo& contextInfo) {
	//width = pretendStartsVR ? contextInfo.camera.renderTargetExtent[qualityIndex].width : hmdWidth;
	//height = pretendStartsVR ? contextInfo.camera.renderTargetExtent[qualityIndex].height : hmdHeight;
	width = pretendStartsVR ? qualityScale * hmdWidth : hmdWidth;
	height = pretendStartsVR ? qualityScale * hmdHeight : hmdHeight;

	//TODO: should get rid of this, have camera have all of its dims for every quality setting so the odd number check is in one place
	//ensure that dims are even to avoid stencil issues
	width	= (width &  1) == 1 ? width  - 1 : width;
	height	= (height & 1) == 1 ? height - 1 : height;

	const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float invHMDWidth = 1.f / hmdWidth;
	const float invHMDHeight = 1.f / hmdHeight;
	std::vector<std::vector<uint8_t>>radialDensityMask(3);
	radialDensityMask[0].resize(width*height);//left
	radialDensityMask[1].resize(width*height);//right
	radialDensityMask[2].resize(width*height);//combined (xor)

	//go through all 2x2 set of pixels (center of group) and determine if that point samples outside of
	//the UV space for that eye, if so mark as 0 (z-near in vulkan). make a once that is blank for non vr mode?
	for (int camIndex = 0; camIndex <= 1; ++camIndex) {
		for (int hmdY = 0; hmdY < hmdHeight; ++hmdY) {
			for (int hmdX = 0; hmdX < hmdWidth; ++hmdX) {
				/////////////////////////////////////////////
				//////////CHECK IF WITHIN RADIUS/////////////
				/////////////////////////////////////////////
				//determine if im inside the radius
				//dont distinguish between checker band and center
				//maybe there's a quad that doesnt get sampled in the middle for certain HMD's lenses because it gets blown out so much
				//glm::vec2 uv(hmdX*invHMDWidth, hmdY*invHMDHeight);
				//Should probably be this:
				glm::vec2 uv((hmdX+0.5f)*invHMDWidth, (hmdY+0.5f)*invHMDHeight);

				//convert this uv to ndc based on camIndex
				glm::vec2 equivNDC = glm::vec2((uv.x - 0.5f*camIndex)*4.f - 1.f, uv.y*2.f - 1.f);
				equivNDC *= glm::vec2(1.f , hmdHeight/(hmdWidth*0.5f));//normalize y ndc against half width (eye viewport size) so we get circles and not long vertical ellipses if Y is greater than vr eye viewport x (width/2)
				float radius = glm::length(equivNDC - ndcCenter[camIndex]);

				if (radius < (1.f + extraRadius)) {
					//if so use Mesh::getSourceUV to get all 3 UV channels needed
					glm::vec2 tcRed, tcGreen, tcBlue;
					Mesh::getSourceUV(camIndex, uv, tcRed, tcGreen, tcBlue);//calls Brown-Conrady distortion, to get sampling uv's

					//convert these UV's to pixels for the scaled vr source image
					glm::ivec2 groupRed	  = tcRed	* glm::vec2(width, height);
					glm::ivec2 groupGreen = tcGreen * glm::vec2(width, height);
					glm::ivec2 groupBlue  = tcBlue  * glm::vec2(width, height);


					//snap to a pixel quad for each 
					groupRed.x		= ((groupRed.x		& 1) == 1) ? groupRed.x		: groupRed.x 	+ 1;
					groupRed.y 		= ((groupRed.y 		& 1) == 1) ? groupRed.y 	: groupRed.y 	+ 1;
					groupGreen.x	= ((groupGreen.x	& 1) == 1) ? groupGreen.x 	: groupGreen.x 	+ 1;
					groupGreen.y 	= ((groupGreen.y 	& 1) == 1) ? groupGreen.y 	: groupGreen.y 	+ 1;
					groupBlue.x 	= ((groupBlue.x 	& 1) == 1) ? groupBlue.x 	: groupBlue.x 	+ 1;
					groupBlue.y 	= ((groupBlue.y 	& 1) == 1) ? groupBlue.y 	: groupBlue.y 	+ 1;

					
					//tag all 4 pixels in each of teh 3 channel's pixel quads
					//if ((((groupRed.x - 1) & 0x3) == 0) && (((groupRed.y - 1) & 0x3) == 0) //both divis by 4
					// || (((groupRed.x - 3) & 0x3) == 0) && (((groupRed.y - 3) & 0x3) == 0)) { //shift the above pattern right and down to get checker
						  //set group of 4 to stencilMask. uv as it is resolves to lower right pixel
						radialDensityMask[camIndex][(groupRed.y    )*width	+ groupRed.x    ] = stencilMaskVal;//lowerright
						radialDensityMask[camIndex][(groupRed.y - 1)*width	+ groupRed.x    ] = stencilMaskVal;//upperright
						radialDensityMask[camIndex][(groupRed.y - 1)*width	+ groupRed.x - 1] = stencilMaskVal;//upperleft
						radialDensityMask[camIndex][(groupRed.y	   )*width	+ groupRed.x - 1] = stencilMaskVal;//lowerleft
					//}

					//if ((((groupGreen.x - 1) & 0x3) == 0) && (((groupGreen.y - 1) & 0x3) == 0) //both divis by 4
					// || (((groupGreen.x - 3) & 0x3) == 0) && (((groupGreen.y - 3) & 0x3) == 0)) { //shift the above pattern right and down to get checker
						  //set group of 4 to stencilMask. uv as it is resolves to lower right pixel
						radialDensityMask[camIndex][(groupGreen.y    )*width	+ groupGreen.x    ] = stencilMaskVal;//lowerright
						radialDensityMask[camIndex][(groupGreen.y - 1)*width	+ groupGreen.x    ] = stencilMaskVal;//upperright
						radialDensityMask[camIndex][(groupGreen.y - 1)*width	+ groupGreen.x - 1] = stencilMaskVal;//upperleft
						radialDensityMask[camIndex][(groupGreen.y	 )*width	+ groupGreen.x - 1] = stencilMaskVal;//lowerleft
					//}

					//if ((((groupBlue.x - 1) & 0x3) == 0) && (((groupBlue.y - 1) & 0x3) == 0) //both divis by 4
					// || (((groupBlue.x - 3) & 0x3) == 0) && (((groupBlue.y - 3) & 0x3) == 0)) { //shift the above pattern right and down to get checker
						  //set group of 4 to stencilMask. uv as it is resolves to lower right pixel
						radialDensityMask[camIndex][(groupBlue.y    )*width	+ groupBlue.x    ] = stencilMaskVal;//lowerright
						radialDensityMask[camIndex][(groupBlue.y - 1)*width	+ groupBlue.x    ] = stencilMaskVal;//upperright
						radialDensityMask[camIndex][(groupBlue.y - 1)*width	+ groupBlue.x - 1] = stencilMaskVal;//upperleft
						radialDensityMask[camIndex][(groupBlue.y	)*width	+ groupBlue.x - 1] = stencilMaskVal;//lowerleft
					//}
				} //if in radius
			}//x
		}//x
	}//camIndex

	//XOR values to get result
	for (int y = 0; y <height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int stencilIndex = (y*width + x);
			uint32_t xorResult = radialDensityMask[0][stencilIndex] ^ radialDensityMask[1][stencilIndex];
			//radialDensityMask[2][stencilIndex] = radialDensityMask[0][stencilIndex];
			radialDensityMask[2][stencilIndex] = xorResult;
		}
	}

	writeStencilToImage(radialDensityMask[2]);
}

void PreMadeStencil::createRadialDensityStencilMask(const VulkanContextInfo& contextInfo) {
	//width = pretendStartsVR ? contextInfo.camera.renderTargetExtent[qualityIndex].width : hmdWidth;
	//height = pretendStartsVR ? contextInfo.camera.renderTargetExtent[qualityIndex].height : hmdHeight;

	width = pretendStartsVR ? qualityScale * hmdWidth : hmdWidth;
	height = pretendStartsVR ? qualityScale * hmdHeight : hmdHeight;

	//ensure that dims are even to avoid stencil issues
	width	= (width &  1) == 1 ? width  - 1 : width;
	height	= (height & 1) == 1 ? height - 1 : height;

	const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	std::vector<std::vector<uint8_t>>radialDensityMask(3);
	radialDensityMask[0].resize(width*height);
	radialDensityMask[1].resize(width*height);
	radialDensityMask[2].resize(width*height);

	//go through all 2x2 set of pixels (center of group) and determine if that point samples outside of
	//the UV space for that eye, if so mark as 0 (z-near in vulkan). make a once that is blank for non vr mode?
	for (int camIndex = 0; camIndex <= 1; ++camIndex) {
		
		//x,y correspond to pixel number where 0,0 is upper left in vulkan
		//loop over entire render area for each eye, if outside of r=1.15 or render area set to stencilMask(need to XOR later to get middle cutout)
		for (int y = 1; y < height; y+=2) {
			for (int x = 1; x < width; x+=2) {
				//if (x == 881 && y == 601 && camIndex == 1) {
				//if (x == 1367 && y == 1 && camIndex == 1) {
				//	int adinvow = 1;
				//}

				//convert to uv
				const glm::vec2 uv(x*invWidth, y*invHeight);

				//convert this uv to ndc based on camIndex
				glm::vec2 equivNDC = glm::vec2((uv.x - 0.5f*camIndex)*4.f - 1.f, uv.y*2.f - 1.f);
				equivNDC *= glm::vec2(1.f , height/(width*0.5f));//normalize y ndc against half width (eye viewport size) so we get circles and not long vertical ellipses if Y is greater than vr eye viewport x (width/2)
				float radius = glm::length(equivNDC - ndcCenter[camIndex]);



				if (radius < (1.f + extraRadius)) {
					if (radius > middleRegionRadius) {//middle region checkerboard 2x2
						if ( (((x - 1) & 0x3) == 0) && (((y - 1) & 0x3) == 0) //both divis by 4
					      || (((x - 3) & 0x3) == 0) && (((y - 3) & 0x3) == 0) ) { //shift the above pattern right and down to get checker
							//set group of 4 to stencilMask. uv as it is resolves to lower right pixel
							radialDensityMask[camIndex][(y  )*width + x  ] = stencilMaskVal;//lowerright
							radialDensityMask[camIndex][(y-1)*width + x  ] = stencilMaskVal;//upperright
							radialDensityMask[camIndex][(y-1)*width + x-1] = stencilMaskVal;//upperleft
							radialDensityMask[camIndex][(y  )*width + x-1] = stencilMaskVal;//lowerleft
						}
					} else { //center region
						//set group of 4 to stencilMask. uv as it is resolves to lower right pixel
						radialDensityMask[camIndex][(y  )*width + x  ] = stencilMaskVal;//lowerright
						radialDensityMask[camIndex][(y-1)*width + x  ] = stencilMaskVal;//upperright
						radialDensityMask[camIndex][(y-1)*width + x-1] = stencilMaskVal;//upperleft
						radialDensityMask[camIndex][(y  )*width + x-1] = stencilMaskVal;//lowerleft
					}
				} 

			}//x pixel ID
		}//y pixel ID
	}//camIndex

	//XOR values to get result
	for (int y = 0; y <height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int stencilIndex = (y*width + x);
			uint32_t xorResult = radialDensityMask[0][stencilIndex] ^ radialDensityMask[1][stencilIndex];
			//radialDensityMask[2][stencilIndex] = radialDensityMask[0][stencilIndex];
			radialDensityMask[2][stencilIndex] = xorResult;
		}
	}

	writeStencilToImage(radialDensityMask[2]);
}


void PreMadeStencil::writeStencilToImage(const std::vector<uint8_t>& maskData) {
	//stb write to an image to check it out
	const int NUM_CHANNELS = 4;
	uint8_t* rgb_image = (uint8_t*)malloc(width * height * NUM_CHANNELS);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int stencilIndex = (y*width + x);
			const int redindex = NUM_CHANNELS * stencilIndex;
			const uint8_t colorVal = 255 * maskData[stencilIndex];
			rgb_image[redindex + 0] = colorVal;
			rgb_image[redindex + 1] = colorVal;
			rgb_image[redindex + 2] = colorVal;
			rgb_image[redindex + 3] = 255;
		}
	}

	genFileName();
	std::cout << "\nWriting radialStencilMask image to "<< filename;
	stbi_write_bmp(filename.c_str(), width, height, NUM_CHANNELS, rgb_image);
	stbi_image_free(rgb_image);
}

void PreMadeStencil::genFileName() {
	std::string basename;
	if (type == StencilType::RadialDensityMask) {
		basename = "radialStencilMask";
	} else if (type == StencilType::FixedFoveated) {
		basename = "fixedFoveatedMask";
	} else if (type == StencilType::PreCalcBarrelSamplingMask) {
		basename = "preCalcBarrelSamplingMask";
	}

	std::stringstream ss; ss << basename << qualityScale << ".bmp";
	filename = ss.str();
}