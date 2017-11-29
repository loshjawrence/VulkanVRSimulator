#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;

layout (push_constant) uniform PerDrawCallInfo {
    int toggleFlags;
} PushConstant;
const int camBit = 1;
const int vrBit = 0;


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNor;
layout(location = 3) in vec3 fragTan;
layout(location = 4) in vec3 fragBiTan;

layout(location = 0) out vec4 outColor;

//void main() {
//    outColor = texture(texSampler, fragUV);
//}


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

// This samples from a single unwarped [0,0]x[1,1] box containing two views
// side-by-side that have been rendered using normal perspective projection.
// The left eye takes up [0,0]x[0.5,1] and the right eye takes up [0.5,0]x[1,1].
vec4 sampleChroma(vec2 point, vec2 ScreenCenter) {
  if (clamp(point, ScreenCenter - vec2(0.25, 0.5), ScreenCenter + vec2(0.25, 0.5)) != point) return vec4(0.0);
  vec2 checkerboard = fract(point * vec2(4.0, 2.0)) - 0.5;
  return vec4(checkerboard.x * checkerboard.y < 0.0 ? 0.25 : 1.0);
}


void main() {	
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;
	//dont do barrel and chroma ab if not vrMode
	if(0 == vrMode ) { outColor = texture(texSampler, fragUV); return;}

    //if vrMode, shrink UV.x by half and shift
    //to sample one eye of the original full screen texture
    //mapping UV(0-1) to either 0-.5 or .5-1 based on camIndex
    vec2 oTexCoord = fragUV;
    oTexCoord.x = (oTexCoord.x * (1.f - 0.5*vrMode)) + 0.5f*camIndex;

    // Compute the viewport size
    bool isRight = oTexCoord.x > 0.5;
    float x = isRight ? 0.5 : 0.0;
    float y = 0.0;
    float w = 0.5;
    float h = 1.0;

    // Set up values for the shader
    float XCenterOffset = isRight ? -Distortion_XCenterOffset : Distortion_XCenterOffset;
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
    vec2 tcRed = LensCenter + Scale * thetaRed;
    vec2 tcGreen = LensCenter + Scale * theta1;
    vec2 tcBlue = LensCenter + Scale * thetaBlue;

    vec2 tc = tcGreen;
    vec2 equivNDC = vec2((tc.x - 0.5*camIndex)*4.f-1.f , tc.y*2.f-1.f); 
    if(any(greaterThan(abs(equivNDC), vec2(1.f)))) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
		outColor = vec4(texture(texSampler, tcRed).r,
						texture(texSampler, tcGreen).g,
						texture(texSampler, tcBlue).b, 1.f);
   }
}





