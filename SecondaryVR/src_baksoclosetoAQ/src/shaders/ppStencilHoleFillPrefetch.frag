#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;

layout (push_constant) uniform PerDrawCallInfo {
    int toggleFlags;
    int virtualWidth;
    int virtualHeight;
} PushConstant;
const int camBit = 1;
const int vrBit = 0;


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNor;
layout(location = 3) in vec3 fragTan;
layout(location = 4) in vec3 fragBiTan;

layout(location = 0) out vec4 outColor;

const float middleRegionRadius = 0.52;//roughly 0.52
const float NDCcenterOffset = 0.1425;//0.15 ndc centeer UV center offset 0.0375
const float extraRadius = NDCcenterOffset*0.5f;//same as NDCcenterOffset?
const vec2 ndcCenter[2] = { vec2(NDCcenterOffset, 0.f), vec2(-NDCcenterOffset, 0.f) };

void fillCheckerHole(const ivec2 pixel, const vec2 invWandH, const vec3 prefetch[16]);
void reshadeRenderedChecker(const ivec2 pixel, const vec2 invWandH, const vec3 prefetch[16]);

void main() {
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    //if vrMode, shrink UV.x by half and shift to sample one eye
    vec2 fragTexCoord = fragUV;
    fragTexCoord.x = (fragTexCoord.x * (1.f - 0.5*vrMode)) + 0.5f*camIndex;

    //just sample normally if not vrMode
    if(0 == vrMode) { outColor = texture(texSampler, fragTexCoord); return;}

    //else fill holes
    //POSSIBLE SPIR-V COMPILER BUG: when width and height aren't const bad things happen for the odd cases
    const ivec2 pixel = ivec2(gl_FragCoord.x, gl_FragCoord.y);
    const ivec2 groupPixel = ivec2( ((pixel.x & 1) == 1) ? pixel.x : pixel.x+1,
                                    ((pixel.y & 1) == 1) ? pixel.y : pixel.y+1);

    const vec2 gp = vec2(groupPixel.x+0.5f, groupPixel.y+0.5f);
    const vec3 prefetch[16] = {vec3(texture(texSampler, gp+vec2(-2.f,-2.f))), vec3(texture(texSampler, gp+vec2(-1.f,-2.f))), vec3(texture(texSampler, gp+vec2( 0.f,-2.f))), vec3(texture(texSampler, gp+vec2( 1.f,-2.f))),
                               vec3(texture(texSampler, gp+vec2(-2.f,-1.f))), vec3(texture(texSampler, gp+vec2(-1.f,-1.f))), vec3(texture(texSampler, gp+vec2( 0.f,-1.f))), vec3(texture(texSampler, gp+vec2( 1.f,-1.f))),
                               vec3(texture(texSampler, gp+vec2(-2.f, 0.f))), vec3(texture(texSampler, gp+vec2(-1.f, 0.f))), vec3(texture(texSampler, gp+vec2( 0.f, 0.f))), vec3(texture(texSampler, gp+vec2( 1.f, 0.f))),
                               vec3(texture(texSampler, gp+vec2(-2.f, 1.f))), vec3(texture(texSampler, gp+vec2(-1.f, 1.f))), vec3(texture(texSampler, gp+vec2( 0.f, 1.f))), vec3(texture(texSampler, gp+vec2( 1.f, 1.f)))
                              };

    const int width = PushConstant.virtualWidth;
    const int height = PushConstant.virtualHeight;

    const float invWidth = 1.f / width;
    const float invHeight = 1.f / height;
    const vec2 invWandH = vec2(invWidth, invHeight);



    //UV for the 2x2 pixel quad in which it resides
    const vec2 groupUV = vec2(groupPixel.x*invWidth, groupPixel.y*invHeight);
    vec2 equivNDC = vec2((groupUV.x - 0.5*camIndex)*4.f - 1.f, groupUV.y*2.f - 1.f);//convert to eye ndc

    //normalize y ndc against half width (eye viewport size) so we get circles and not tall/short vertical ellipses
    //if Y is greater/less than vr eye viewport x (rendertarget width/2)
    equivNDC *= vec2(1.f , height/(width*0.5f)); 
    const float radius = length(equivNDC - ndcCenter[camIndex]);


    if (radius < (1.f + extraRadius)) {
        if (radius > middleRegionRadius) {//middle region checkerboard 2x2
            if ( (((groupPixel.x - 1) & 0x3) == 0) && (((groupPixel.y - 1) & 0x3) == 0) //both divis by 4
              || (((groupPixel.x - 3) & 0x3) == 0) && (((groupPixel.y - 3) & 0x3) == 0)  //shift above pattern to get checker
            ) {//rendered pixel 
                reshadeRenderedChecker(pixel, invWandH, prefetch);
//                outColor = vec4(1.f,0.f,0.f,1.f); 
//                outColor = texture(texSampler, fragTexCoord);
            } else {//black pixel 
                fillCheckerHole(pixel, invWandH, prefetch);
//                outColor = vec4(0.f,1.f,0.f,1.f); 
//                outColor = texture(texSampler, fragTexCoord);
            }
        } else {//inner region
            outColor = texture(texSampler, fragTexCoord);
        }
    } else {//outside lens range
        outColor = vec4(0.f,0.f,0.f,1.f); //no need to fetch, it's black
    }
}

int determinePixelID2x2(const ivec2 pixel) {
	bool xOdd = (pixel.x & 1) == 1;
	bool yOdd = (pixel.y & 1) == 1;
	if(xOdd) {//right in group
		if(yOdd) {//down in group (vulkan upper left is 0,0)
			return 3;
		} else { //up in group
			return 1;
		}
	} else { //left in group
		if(yOdd) {//down in group (vulkan upper left is 0,0)
			return 2;
		} else { //up in group
			return 0;
		}
	}
}

vec2 pixelCenterToUV(const vec2 pixelCenter, const vec2 invWandH) {
	return pixelCenter * invWandH;
}

void fillCheckerHole(const ivec2 pixel, const vec2 invWandH, const vec3 prefetch[16]) {
	const int pixelID = determinePixelID2x2(pixel);//numbered like reading a book(0 is upper left, 3 is lower right)
	const vec2 pixelC = vec2(pixel.x + 0.5f, pixel.y + 0.5f);
	const float w1 = 0.375f;
	const float w2 = 0.375f;
	const float w3 = 0.125f;
	const float w4 = 0.125f;
    int offset1;
    int offset2;
    int offset3;
    int offset4;
    const int prefetchWidth = 4;
    const int groupIndex = 2*prefetchWidth+2;

	if		 (pixelID == 0) {//upper left
        const int pixelIndex = groupIndex - 1 - (1*prefetchWidth);
		offset1 = pixelIndex+0 + (-1*prefetchWidth);//above
		offset2 = pixelIndex-1 + ( 0*prefetchWidth);//left
		offset3 = pixelIndex+2 + ( 0*prefetchWidth);//skip right
		offset4 = pixelIndex+0 + ( 2*prefetchWidth);//skip down
	} else if(pixelID == 1) {//upper right
        const int pixelIndex = groupIndex - 0 - (1*prefetchWidth);
		offset1 = pixelIndex+0 + (-1*prefetchWidth);//above
		offset2 = pixelIndex+1 + ( 0*prefetchWidth);//right
		offset3 = pixelIndex-2 + ( 0*prefetchWidth);//skip left
		offset4 = pixelIndex+0 + ( 2*prefetchWidth);//skip down
	} else if(pixelID == 2) {//lower left
        const int pixelIndex = groupIndex - 1 + (0*prefetchWidth);
		offset1 = pixelIndex+0 + ( 1*prefetchWidth);//below
		offset2 = pixelIndex-1 + ( 0*prefetchWidth);//left
		offset3 = pixelIndex+2 + ( 0*prefetchWidth);//skip right
		offset4 = pixelIndex+0 + (-2*prefetchWidth);//skip up
	} else if(pixelID == 3) {//lower right
        const int pixelIndex = groupIndex - 0 - (0*prefetchWidth);
		offset1 = pixelIndex+0 + ( 1*prefetchWidth);//below
		offset2 = pixelIndex+1 + ( 0*prefetchWidth);//right
		offset3 = pixelIndex-2 + ( 0*prefetchWidth);//skip left
		offset4 = pixelIndex+0 + (-2*prefetchWidth);//skip up
	}
    const vec3 sample1 =  w1 * prefetch[offset1];
    const vec3 sample2 =  w2 * prefetch[offset2];
    const vec3 sample3 =  w3 * prefetch[offset3];
    const vec3 sample4 =  w4 * prefetch[offset4];
    outColor = vec4(sample1 + sample2 + sample3 + sample4, 1.f);
}

void reshadeRenderedChecker(const ivec2 pixel, const vec2 invWandH, const vec3 prefetch[16]) {
	const int pixelID = determinePixelID2x2(pixel);//numbered like reading a book(0 is upper left, 3 is lower right)
	const vec2 pixelC = vec2(pixel.x + 0.5f, pixel.y + 0.5f);
	const float w1 = 0.50000f;
	const float w2 = 0.28125f;
	const float w3 = 0.09375f;
	const float w4 = 0.09375f;
	const float w5 = 0.03125f;
    int offset1;
    int offset2;
    int offset3;
    int offset4;
    int offset5;
    const int prefetchWidth = 4;
    const int groupIndex = 2*prefetchWidth+2;
	if		 (pixelID == 0) {//upper left
        const int pixelIndex = groupIndex - 1 - (1*prefetchWidth);
		offset1 = pixelIndex+0 + ( 0*prefetchWidth);//self
		offset2 = pixelIndex-1 + (-1*prefetchWidth);//1Left 1Up
		offset3 = pixelIndex+2 + (-1*prefetchWidth);//2Right 1Up
		offset4 = pixelIndex-1 + ( 2*prefetchWidth);//1Left 2Down
		offset5 = pixelIndex+2 + ( 2*prefetchWidth);//2Right 2Down
	} else if(pixelID == 1) {//upper right
        const int pixelIndex = groupIndex - 0 - (1*prefetchWidth);
		offset1 = pixelIndex+0 + ( 0*prefetchWidth);//self
		offset2 = pixelIndex+1 + (-1*prefetchWidth);//1Right 1Up
		offset3 = pixelIndex-2 + (-1*prefetchWidth);//2Left 1Up
		offset4 = pixelIndex+1 + ( 2*prefetchWidth);//1Right 2Down
		offset5 = pixelIndex-2 + ( 2*prefetchWidth);//2Left 2Down
	} else if(pixelID == 2) {//lower left
        const int pixelIndex = groupIndex - 1 + (0*prefetchWidth);
		offset1 = pixelIndex+0 + ( 0*prefetchWidth);//self
		offset2 = pixelIndex-1 + ( 1*prefetchWidth);//1Left 1Down
		offset3 = pixelIndex+2 + ( 1*prefetchWidth);//2Right 1Down
		offset4 = pixelIndex-1 + (-2*prefetchWidth);//1Left 2Up
		offset5 = pixelIndex+2 + (-2*prefetchWidth);//2Right 2Up
	} else if(pixelID == 3) {//lower right
        const int pixelIndex = groupIndex - 0 - (0*prefetchWidth);
		offset1 = pixelIndex+0 + ( 0*prefetchWidth);//self
		offset2 = pixelIndex+1 + ( 1*prefetchWidth);//1Right 1Down
		offset3 = pixelIndex-2 + ( 1*prefetchWidth);//2Left 1Down
		offset4 = pixelIndex+1 + (-2*prefetchWidth);//1Right 2Up
		offset5 = pixelIndex-2 + (-2*prefetchWidth);//2Left 2Up
	}
    const vec3 sample1 =  w1 * prefetch[offset1];
    const vec3 sample2 =  w2 * prefetch[offset2];
    const vec3 sample3 =  w3 * prefetch[offset3];
    const vec3 sample4 =  w4 * prefetch[offset4];
    const vec3 sample5 =  w5 * prefetch[offset5];
    outColor = vec4(sample1 + sample2 + sample3 + sample4 + sample5, 1.f);
}
