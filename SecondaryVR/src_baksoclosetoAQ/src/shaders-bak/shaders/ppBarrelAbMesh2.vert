#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform PerDrawCallInfo {
    int toggleFlags;
} PushConstant;
const int camBit = 1;
const int vrBit = 0;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNor;
layout(location = 4) in vec3 inTan;
layout(location = 5) in vec3 inBiTan;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNor;
layout(location = 3) out vec3 fragTan;
layout(location = 4) out vec3 fragBiTan;

out gl_PerVertex {
    vec4 gl_Position;
};



vec2 getSourceUV(const int camIndex, const vec2 oTexCoord) {
	//code from old oculus demo implementation
	// Values that were scattered throughout the Oculus world demo
	const vec4 HmdWarpParam = vec4(1.0, 0.22, 0.24, 0.0); // For the 7-inch device
	const vec4 ChromAbParam = vec4(0.996, -0.004, 1.014, 0.0);
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
    vec2 LensCenter = vec2(x + (w + XCenterOffset * 0.5) * 0.5, y + h * 0.5);
    vec2 ScreenCenter = vec2(x + w * 0.5, y + h * 0.5);
    float scaleFactor = 1.0 / Distortion_Scale;
    vec2 Scale = vec2(w * 0.5 * scaleFactor, h * 0.5 * scaleFactor * stereoAspect);
    vec2 ScaleIn = vec2(2.0 / w, 2.0 / h / stereoAspect);

    // Compute the warp
    vec2 theta = (oTexCoord - LensCenter) * ScaleIn; // Scales to [-1, 1]
    float rSq = theta.x * theta.x + theta.y * theta.y;
    vec2 theta1 = theta * (
            HmdWarpParam.x +
            HmdWarpParam.y * rSq +
            HmdWarpParam.z * rSq * rSq +
            HmdWarpParam.w * rSq * rSq * rSq);

    // Compute chromatic aberration
    vec2 thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);
    vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);
    vec2 out_tcRed = LensCenter + Scale * thetaRed;
    vec2 out_tcGreen = LensCenter + Scale * theta1;
    vec2 out_tcBlue = LensCenter + Scale * thetaBlue;
    return out_tcGreen;
}


//calculates inverse distortion for an ndc
//distorted = source loc in fragment shader version
//where source is the the part of tex to read from
//undistorted = dest loc in fragment shader version
//where dest is render frag location in which to write
//given a distorted ndc (our mesh pos) get its orignal undistorted ndc
//radius distorted radius from lens center (length input ndc)
//return the undistorted ndc 
vec2 convertRadToUV(const vec2 normalized_ndc, const float radius, const int camIndex) {
	const int vrMode = 1;
	vec2 ndc = radius * normalized_ndc;
	vec2 uv = (ndc + 1.f)*0.5f;
	uv.x = (uv.x * (1.f - 0.5f*vrMode)) + 0.5f*camIndex;
	return uv;
}

float convertUVToRad(const vec2 uv, const int camIndex) {
	const vec2 equivNDC = vec2((uv.x - 0.5*camIndex)*4.f - 1.f, uv.y*2.f - 1.f);
	return length(equivNDC);
}

//this uses secant method for finding inverse of brown-conrady distortion
//(going towards the center), modifed code sample from webvr
vec3 distortInverse(vec3 ndc, const int camIndex) {
    // Uses Secant method for finding inverse of function
    //based on eulers method for approx. inverse of function i.e. given y find x
	vec2 tcRed, tcGreen, tcBlue;
	vec2 normalized_ndc = normalize(vec2(ndc.x, ndc.y));
    float radius = length(vec2(ndc.x, ndc.y));

	float distort;

    float r0 = 0.f;
    float r1 = 1.f;
	tcGreen = getSourceUV(camIndex, convertRadToUV(normalized_ndc, r0, camIndex));
	distort = convertUVToRad(tcGreen, camIndex);
	float dr0 = radius - distort;
    while (abs(r1 - r0) > 0.0001) {//0.1mm
		tcGreen = getSourceUV(camIndex, convertRadToUV(normalized_ndc, r1, camIndex));
		distort = convertUVToRad(tcGreen, camIndex);
        float dr1 = radius - distort;
        float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
        r0 = r1;
        r1 = r2;
        dr0 = dr1;
    }
    return vec3(normalized_ndc * r1, ndc.z);


}

void main() {	
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    //if vrMode, shrink UV.x by half and shift to order to sample correct region for the eye
    fragUV = inUV;
    fragUV.x = (fragUV.x * (1.f - 0.5*vrMode)) + 0.5f*camIndex;

    if(1 == vrMode) {
        gl_Position = vec4(distortInverse(inPos, camIndex), 1.f);
    } else {
        gl_Position = vec4(inPos, 1.f);
    }
}
