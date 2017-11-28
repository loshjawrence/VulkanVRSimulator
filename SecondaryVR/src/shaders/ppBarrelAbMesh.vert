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


//could also try this kernel:
//from: https://www.reddit.com/r/gamedev/comments/20xyn4/heres_a_great_chromatic_aberration_glsl_function/ 
//// A re-weighted Gaussian kernel
//const vec3 chromaticAberrationKernel[9] = vec3[9](
//vec3(0.0000000000000000000, 0.04416589065853191, 0.0922903086524308425), vec3(0.10497808951021347), vec3(0.0922903086524308425, 0.04416589065853191, 0.0000000000000000000),
//vec3(0.0112445223775533675, 0.10497808951021347, 0.1987116566428735725), vec3(0.40342407932501833), vec3(0.1987116566428735725, 0.10497808951021347, 0.0112445223775533675),
//vec3(0.0000000000000000000, 0.04416589065853191, 0.0922903086524308425), vec3(0.10497808951021347), vec3(0.0922903086524308425, 0.04416589065853191, 0.0000000000000000000)
//); 

vec2 barrelDistortion(vec2 coord, float amt) {
	vec2 cc = coord - 0.5;
	float dist = dot(cc, cc);
	return coord + cc * dist * amt;
}
float sat( float t ) {
	return clamp( t, 0.0, 1.0 );
}
float linterp( float t ) {
	return sat( 1.0 - abs( 2.0*t - 1.0 ) );
}
float remap( float t, float a, float b ) {
	return sat( (t - a) / (b - a) );
}
vec4 spectrum_offset( float t ) {
	vec4 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	ret = vec4(lo,1.0,hi, 1.) * vec4(1.0-w, w, 1.0-w, 1.);
	return pow( ret, vec4(1.0/2.2) );
}

const float max_distort = 2.2;
const int num_iter = 12;
const float reci_num_iter_f = 1.0 / float(num_iter);

//or: http://jsfiddle.net/s175ozts/4/
//https://www.imgtec.com/blog/speeding-up-gpu-barrel-distortion-correction-in-mobile-vr/
const float a = 0.24f;
const float b = 0.22f;
const float c = 1.f - (a + b);

vec2 getOculusBarrelSourceNDC(vec2 ndc) {
    // Calculate the source location "radius" (distance from the centre of the viewport)
    // ndc - xy position of the current fragment (destination) in NDC space [-1 1]^2
    float destRlen = length(ndc);
    float srcRlen = a * pow(destRlen,4) + b * pow(destRlen,2) + c * destRlen;

    // Calculate the source vector (radial)
    vec2 correctedRadial = normalize(ndc) * srcRlen;

    return correctedRadial;
}

//return srcRlen
float distort(float destRlen) {
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
vec3 distortInverse(vec3 ndc) {
    // Uses Secant method for finding inverse of function
    //based on eulers method for approx. inverse of function i.e. given y find x
    float radius = length(ndc.xy);

    float r0 = 0.f;
    float r1 = 1.f;
    float dr0 = radius - distort(r0);
    while (abs(r1 - r0) > 0.0001) {//0.1mm
        float dr1 = radius - distort(r1);
        float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
        r0 = r1;
        r1 = r2;
        dr0 = dr1;
    }
    return vec3(normalize(ndc.xy) * r1, ndc.z);
}


void main() {	
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    //if vrMode, shrink UV.x by half and shift to order to sample correct region for the eye
    fragUV = inUV;
    fragUV.x = (fragUV.x * (1.f - 0.5*vrMode)) + 0.5f*camIndex;

    if(1 == vrMode) {
        gl_Position = vec4(distortInverse(inPos), 1.f);
    } else {
        gl_Position = vec4(inPos, 1.f);
    }
}


//void main() {
//    gl_Position     = vec4(inPos, 1.0);
//    fragUV    = inUV;
//}
