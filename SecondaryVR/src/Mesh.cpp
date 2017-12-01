#pragma once
#include "Mesh.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "VulkanBuffer.h"


Mesh::Mesh(const VulkanContextInfo& contextInfo, const MESHTYPE meshtype, uint32_t camIndex) {
	if (meshtype == MESHTYPE::NDCTRIANGLE)					createNDCTriangle(contextInfo);
	else if (meshtype == MESHTYPE::NDCBARRELMESH)			createNDCBarrelMesh(contextInfo, camIndex);
	else if (meshtype == MESHTYPE::NDCBARRELMESH_PRECALC)	createNDCBarrelMeshPreCalc(contextInfo,camIndex);
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, 
	const std::vector<Texture>& textures, const VulkanContextInfo& contextInfo)
	: mVertices(vertices), mIndices(indices), mTextures(textures)
{
	setupVulkanBuffers(contextInfo);
}

Mesh::Mesh() {
}

Mesh::~Mesh() {
}

void Mesh::createDescriptor(const VulkanContextInfo& contextInfo, const VkBuffer& ubo, const uint32_t sizeofUBOstruct) {
	//bind textures for this mesh
	if (mTextures.size() > 4) {
		std::cout << "\nMore than 4 textures in mesh" << std::endl;
	}
	for (unsigned int i = 0; i < mTextures.size(); ++i) {
		std::string name = mTextures[i].type;
		if (name == "texture_diffuse") {
			diffuseindices.push_back(i);
		} else if (name == "texture_specular") {
			specindices.push_back(i);
		} else if (name == "texture_normal") {
			norindices.push_back(i);
		} else if (name == "texture_height") {
			heightindices.push_back(i);
		} else {
			std::cout << "Not sure which texture type this is: " << name << std::endl;
		}
	}

	descriptor.determineNumImageSamplersAndTextureMapFlags(this);
	//VulkanDescriptor has static members for the differnt layouts which get created at init
	//the set is created based on the type
	descriptor.createDescriptorSetLayout(contextInfo);
	descriptor.createDescriptorPool(contextInfo);//each should have their own pool
	descriptor.createDescriptorSet(contextInfo,ubo,sizeofUBOstruct,this);

}


void Mesh::setupVulkanBuffers(const VulkanContextInfo& contextInfo) {
	//setup vulkan buffers for the geometry
	VulkanBuffer::createVertexBuffer(contextInfo, mVertices, vertexBuffer, vertexBufferMemory);
	VulkanBuffer::createIndexBuffer(contextInfo, mIndices, indexBuffer, indexBufferMemory);
}

void Mesh::createNDCTriangle(const VulkanContextInfo& contextInfo) {

	//top left (vulk top left of screen is -1,-1 and uv 0, 0) also the front face is set to ccw
	Vertex tempVertexInfo;
	tempVertexInfo.pos = glm::vec3(-1.0f, -1.0f, 0.5f);
	tempVertexInfo.color = glm::vec3(1.0f);
	tempVertexInfo.nor = glm::vec3(1.0f);
	tempVertexInfo.uv = glm::vec2(0.0f, 0.0f);
	mVertices.push_back(tempVertexInfo);
	mIndices.push_back(0);

	//bottom left screen below screen
	tempVertexInfo.pos = glm::vec3(-1.0f, 3.0f, 0.5f);
	tempVertexInfo.color = glm::vec3(1.0f);
	tempVertexInfo.nor = glm::vec3(1.0f);
	tempVertexInfo.uv = glm::vec2(0.0f, 2.0f);
	mVertices.push_back(tempVertexInfo);
	mIndices.push_back(1);

	//top right off screen
	tempVertexInfo.pos = glm::vec3(3.0f, -1.0f, 0.5f);
	tempVertexInfo.color = glm::vec3(1.0f);
	tempVertexInfo.nor = glm::vec3(1.0f);
	tempVertexInfo.uv = glm::vec2(2.0f, 0.0f);
	mVertices.push_back(tempVertexInfo);
	mIndices.push_back(2);

	VulkanBuffer::createVertexBuffer(contextInfo, mVertices, vertexBuffer, vertexBufferMemory);
	VulkanBuffer::createIndexBuffer(contextInfo, mIndices, indexBuffer, indexBufferMemory);
}

void Mesh::genGridMesh(const VulkanContextInfo& contextInfo, const uint32_t camIndex, const uint32_t shift) {
	//top left (vulk top left of screen is -1,-1 and uv 0, 0) also the front face is set to ccw
    float x = (camIndex == 0) ? 0.f : 0.5f;
    float y = 0.0;
    float w = 0.5;
    float h = 1.0;

	//ndc offset
    //float XCenterOffset = (camIndex == 1) ? -Distortion_XCenterOffset : Distortion_XCenterOffset;
	//const float shiftVal = (XCenterOffset*(uint32_t)contextInfo.camera.vrmode*shift);
	//const float xStart = -1.f + shiftVal;

	const float stride = 2.f / quadsPerDim;
	//vertices
	mVertices.reserve((quadsPerDim+1)*2);
	const glm::vec3 color(1.0f);
	const glm::vec3 nor(1.0f);
	for (float y = -1.f; y < 1.f+stride*0.5f; y += stride) {
		for (float x = -1.f; x < 1.f+stride*0.5f; x += stride) {
			Vertex v;
			v.pos = glm::vec3(x, y, 0.5f);
			//v.color.r = shiftVal;//if needed in shader
			v.uv = glm::vec2((x+1.f)*0.5f, (y+1.f)*0.5f);
			//std::cout << "\n\nPos:( " << v.pos.x << ", " << v.pos.y << ")";
			//std::cout << "\nUV:( " << v.uv.x << ", " << v.uv.y << ")";
			mVertices.push_back(v);
		}
	}

	//indexing
	mIndices.reserve(quadsPerDim*quadsPerDim*2*3);
	uint32_t first, second, third;
	for (uint32_t y = 0; y < quadsPerDim; ++y) {
		for (uint32_t x = 0; x < quadsPerDim; ++x) {
			first = y*(quadsPerDim+1) + x;
			second = first + 1 + quadsPerDim + 1;
			third = first + 1;

			//std::cout << "\n\nf: " << first << " s: " << second << " t: " << third;
			mIndices.push_back(first);
			mIndices.push_back(second);
			mIndices.push_back(third);

			first = first + quadsPerDim + 1;
			second = first + 1;
			third = second - 1 - quadsPerDim - 1;
			mIndices.push_back(first);
			mIndices.push_back(second);
			mIndices.push_back(third);
			//std::cout << "\nf: " << first << " s: " << second << " t: " << third;
		}
	}
}

void Mesh::createNDCBarrelMesh(const VulkanContextInfo& contextInfo, const uint32_t camIndex) {
	//top left (vulk top left of screen is -1,-1 and uv 0, 0) also the front face is set to ccw
	genGridMesh(contextInfo, camIndex, 1);
	VulkanBuffer::createVertexBuffer(contextInfo, mVertices, vertexBuffer, vertexBufferMemory);
	VulkanBuffer::createIndexBuffer(contextInfo, mIndices, indexBuffer, indexBufferMemory);
}


void Mesh::getSourceUV(const uint32_t camIndex, const glm::vec2& oTexCoord,
	glm::vec2& out_tcRed, glm::vec2& out_tcGreen, glm::vec2& out_tcBlue) {

	//modified code sample from old oculus demo implementation
	// Values that were scattered throughout the Oculus world demo
	const glm::vec4 HmdWarpParam = glm::vec4(1.0f, 0.22f, 0.24f, 0.0f); // For the 7-inch device
	const glm::vec4 ChromAbParam = glm::vec4(0.996f, -0.004f, 1.014f, 0.f);
	const float HMD_HResolution = 1280.0;
	const float HMD_VResolution = 800.0;
	const float HMD_HScreenSize = 0.14976;
	const float HMD_VScreenSize = HMD_HScreenSize / (HMD_HResolution / HMD_VResolution);
	const float HMD_InterpupillaryDistance = 0.064;
	const float HMD_LensSeparationDistance = 0.0635;
	const float HMD_EyeToScreenDistance = 0.041;
	const float lensOffset = HMD_LensSeparationDistance * 0.5;
	const float lensShift = HMD_HScreenSize * 0.25 - lensOffset;
	const float Distortion_XCenterOffset = 4.0 * lensShift / HMD_HScreenSize;
	const float DistortionFitX = -1.0;
	const float DistortionFitY = 0.0;
	const float stereoAspect = 0.5 * HMD_HResolution / HMD_VResolution;
	const float dx = DistortionFitX - Distortion_XCenterOffset;
	const float dy = DistortionFitY / stereoAspect;
	const float fitRadiusSquared = dx * dx + dy * dy;
	const float Distortion_Scale =
		HmdWarpParam.x +
		HmdWarpParam.y * fitRadiusSquared +
		HmdWarpParam.z * fitRadiusSquared * fitRadiusSquared +
		HmdWarpParam.w * fitRadiusSquared * fitRadiusSquared * fitRadiusSquared;
    float x = (camIndex == 0) ? 0.f : 0.5f;
    float y = 0.0;
    float w = 0.5;
    float h = 1.0;

    // Set up values for the shader
    float XCenterOffset = (camIndex == 1) ? -Distortion_XCenterOffset : Distortion_XCenterOffset;
    glm::vec2 LensCenter = glm::vec2(x + (w + XCenterOffset * 0.5) * 0.5, y + h * 0.5);
    glm::vec2 ScreenCenter = glm::vec2(x + w * 0.5, y + h * 0.5);
    float scaleFactor = 1.0 / Distortion_Scale;
    glm::vec2 Scale = glm::vec2(w * 0.5 * scaleFactor, h * 0.5 * scaleFactor * stereoAspect);
    glm::vec2 ScaleIn = glm::vec2(2.0 / w, 2.0 / h / stereoAspect);

    // Compute the warp
    glm::vec2 theta = (oTexCoord - LensCenter) * ScaleIn; // Scales to [-1, 1]
    float rSq = theta.x * theta.x + theta.y * theta.y;
    glm::vec2 theta1 = theta * (
            HmdWarpParam.x +
            HmdWarpParam.y * rSq +
            HmdWarpParam.z * rSq * rSq +
            HmdWarpParam.w * rSq * rSq * rSq);

    // Compute chromatic aberration
    glm::vec2 thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);
    glm::vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);
    out_tcRed = LensCenter + Scale * thetaRed;
    out_tcGreen = LensCenter + Scale * theta1;
    out_tcBlue = LensCenter + Scale * thetaBlue;
}

//return srcRlen
float distort(float destRlen) {
	const float a = 0.24f;
	const float b = 0.22f;
	const float c = 1.f - (a + b);
    return a * pow(destRlen,4) + b * pow(destRlen,2) + c * destRlen;
}

//calculates inverse distortion for an ndc
//distorted = source loc in fragment shader version
//where source is the the part of tex to read from
//undistorted = dest loc in fragment shader version
//where dest is render frag location in which to write
//given a distorted ndc (our mesh pos) get its orignal undistorted ndc
//radius distorted radius from lens center (length input ndc)
//return the undistorted ndc 
glm::vec2 convertRadToUV(const glm::vec2 normalized_ndc, const float radius, const uint32_t camIndex) {
	const int vrMode = 1;
	glm::vec2 ndc = radius * normalized_ndc;
	glm::vec2 uv = (ndc + 1.f)*0.5f;
	uv.x = (uv.x * (1.f - 0.5f*vrMode)) + 0.5f*camIndex;
	return uv;
}

float convertUVToRad(const glm::vec2 uv, const uint32_t camIndex) {
	const glm::vec2 equivNDC = glm::vec2((uv.x - 0.5*camIndex)*4.f - 1.f, uv.y*2.f - 1.f);
	return length(equivNDC);
}

glm::vec3 distortInverse(glm::vec3 ndc, const uint32_t camIndex) {
    // Uses Secant method for finding inverse of function
    //based on eulers method for approx. inverse of function i.e. given y find x
	//modified code sample from webvr
	glm::vec2 tcRed, tcGreen, tcBlue;
	glm::vec2 normalized_ndc = glm::normalize(glm::vec2(ndc.x, ndc.y));
    float radius = glm::length(glm::vec2(ndc.x, ndc.y));

	float distort;

    float r0 = 0.f;
    float r1 = 1.f;
	Mesh::getSourceUV(camIndex, convertRadToUV(normalized_ndc,r0,camIndex), tcRed, tcGreen, tcBlue);
	distort = convertUVToRad(tcGreen, camIndex);
	float dr0 = radius - distort;
    while (abs(r1-r0) > 0.001) {//0.1mm seems theres a numerical instability when doing inverse of common approach
		Mesh::getSourceUV(camIndex, convertRadToUV(normalized_ndc, r1, camIndex), tcRed, tcGreen, tcBlue);
		distort = convertUVToRad(tcGreen, camIndex);
        float dr1 = radius - distort;
        float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
        r0 = r1;
        r1 = r2;
        dr0 = dr1;

    }
    return glm::vec3(normalized_ndc * r1, ndc.z);
}


//PreCalc distorted ndc position and color channel sampling UV's, 
//vertex is just passthrough and fragment samples tex using r g b UV's
//only use these meshes for vr mode
void Mesh::createNDCBarrelMeshPreCalc(const VulkanContextInfo& contextInfo, const uint32_t camIndex) {
	//top left (vulk top left of screen is -1,-1 and uv 0, 0) also the front face is set to ccw
	genGridMesh(contextInfo,camIndex,0);
	const uint32_t vrMode = 1;


	//for (auto& v : mVertices) {
	for (int y = 0; y <= quadsPerDim; ++y) {
		for (int x = 0; x <= quadsPerDim; ++x) {


			if (x == 9 && y == 10) {
				int asdfienv = 1;
			}
			Vertex& v = mVertices[y*(quadsPerDim + 1) + x];
			//glm::vec2 oTexCoord = v.uv;//0-1

			//for vrMode, shrink UV.x by half and shift //to sample one eye of the original full screen texture
			//mapping UV(0-1) to either 0-.5 or .5-1 based on camIndex
			//oTexCoord.x = (oTexCoord.x * (1.f - 0.5f*vrMode)) + 0.5f*camIndex;

			//warp the ndc positions down using secant method for finding inverse of function
			v.pos = distortInverse(v.pos, camIndex);

			//to avoid secant method root finding issues re-convert v.pos to its corresponding undistorted UV for the eye
			glm::vec2 oTexCoord(((v.pos.x + 1.f)*0.25) + 0.5*camIndex, (v.pos.y + 1.f) * 0.5f);


			//passIn uv(that is for left or right eye determined by camIndex
			//get source uv for each channel
			glm::vec2 tcRed, tcGreen, tcBlue;
			getSourceUV(camIndex, oTexCoord, tcRed, tcGreen, tcBlue);
			v.color = glm::vec3(tcRed, 1.f);
			v.uv = tcGreen;
			v.nor = glm::vec3(tcBlue, 1.f);

			//convert this uv to an equivalent ndc, if fails range check set a flag stating that its out of range
			const glm::vec2 tc = tcGreen;
			const glm::vec2 equivNDC = glm::vec2((tc.x - 0.5*camIndex)*4.f - 1.f, tc.y*2.f - 1.f);
			if (any(greaterThan(abs(equivNDC), glm::vec2(1.f)))) {
				//set flag on blue channel of color
				v.color.b = 0;
			}
			//optimization(do after working):
			//then loop through index buffer and remove index triple if all 3 point to positions with this flag
		}
	}

	VulkanBuffer::createVertexBuffer(contextInfo, mVertices, vertexBuffer, vertexBufferMemory);
	VulkanBuffer::createIndexBuffer(contextInfo, mIndices, indexBuffer, indexBufferMemory);
}
