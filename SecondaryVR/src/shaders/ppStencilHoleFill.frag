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

const float virtualVRRenderTargetScaling = 1.0f;
const float width = 1280 * virtualVRRenderTargetScaling;
const float height = 800 * virtualVRRenderTargetScaling;
const float invWidth = 1.f / width;
const float invHeight = 1.f / height;
const float middleRegionRadius = 0.52;//roughly 0.52
const float NDCcenterOffset = 0.1425;//0.15 ndc centeer UV center offset 0.0375
const float extraRadius = NDCcenterOffset*0.5f;//same as NDCcenterOffset?
const vec2 ndcCenter[2] = { vec2(NDCcenterOffset, 0.f), vec2(-NDCcenterOffset, 0.f) };

void fillCheckerHole(ivec2 pixel);
void reshadeRenderedChecker(ivec2 pixel);

void main() {
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    //if vrMode, shrink UV.x by half and shift to sample one eye
    vec2 fragTexCoord = fragUV;
    fragTexCoord.x = (fragTexCoord.x * (1.f - 0.5*vrMode)) + 0.5f*camIndex;

    //just sample normally if not vrMode
    if(0 == vrMode) { outColor = texture(texSampler, fragTexCoord); return;}

    ivec2 pixel = ivec2(int(gl_FragCoord.x), int(gl_FragCoord.y));
    ivec2 groupPixel = pixel;
    groupPixel.x = ((groupPixel.x & 1) == 1) ? groupPixel.x : groupPixel.x+1;
    groupPixel.y = ((groupPixel.y & 1) == 1) ? groupPixel.y : groupPixel.y+1;

//    if((int(groupPixel.x) & 1) == 1){
//        outColor = vec4(0.f,0.f,0.f,1.f); return;
//    } else {
//        outColor = vec4(1.f,1.f,1.f,1.f); return;
//    }


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
                reshadeRenderedChecker(pixel);
//                outColor = vec4(1.f,0.f,0.f,1.f); 
//                outColor = texture(texSampler, fragTexCoord);
            } else {//black pixel 
                fillCheckerHole(pixel);
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

int determinePixelID2x2(ivec2 pixel) {
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

vec2 pixelCenterToUV(const vec2 pixelCenter) {
	return pixelCenter * vec2(invWidth, invHeight);
}

void fillCheckerHole(ivec2 pixel) {
	const int pixelID = determinePixelID2x2(pixel);//numbered like reading a book(0 is upper left, 3 is lower right)
	const vec2 pixelC = vec2(pixel.x + 0.5f, pixel.y + 0.5f);
	const float w1 = 0.375f;
	const float w2 = 0.375f;
	const float w3 = 0.125f;
	const float w4 = 0.125f;
	if		 (pixelID == 0) {//upper left
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f, -1.f))) );//above
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-1.f,  0.f))) );//left
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 2.f,  0.f))) );//skip right
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  2.f))) );//skip down
		outColor = vec4(sample1 + sample2 + sample3 + sample4, 1.f);
	} else if(pixelID == 1) {//upper right
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f, -1.f))) );//above
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 1.f,  0.f))) );//right
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-2.f,  0.f))) );//skip left
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  2.f))) );//skip down
		outColor = vec4(sample1 + sample2 + sample3 + sample4, 1.f);
	} else if(pixelID == 2) {//lower left
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  1.f))) );//below
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-1.f,  0.f))) );//left
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 2.f,  0.f))) );//skip right
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f, -2.f))) );//skip up
		outColor = vec4(sample1 + sample2 + sample3 + sample4, 1.f);
	} else if(pixelID == 3) {//lower right
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  1.f))) );//below
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 1.f,  0.f))) );//right
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-2.f,  0.f))) );//skip left
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f, -2.f))) );//skip up
		outColor = vec4(sample1 + sample2 + sample3 + sample4, 1.f);
	}
	
}

void reshadeRenderedChecker(ivec2 pixel) {
	const int pixelID = determinePixelID2x2(pixel);//numbered like reading a book(0 is upper left, 3 is lower right)
	const vec2 pixelC = vec2(pixel.x + 0.5f, pixel.y + 0.5f);
	const float w1 = 0.50000f;
	const float w2 = 0.28125f;
	const float w3 = 0.09375f;
	const float w4 = 0.09375f;
	const float w5 = 0.03125f;
	if		 (pixelID == 0) {//upper left
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  0.f))) );//self
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-1.f, -1.f))) );//1Left 1Up
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 2.f, -1.f))) );//2Right 1Up
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-1.f,  2.f))) );//1Left 2Down
		const vec3 sample5 =  w5 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 2.f,  2.f))) );//2Right 2Down
		outColor = vec4(sample1 + sample2 + sample3 + sample4 + sample5, 1.f);
	} else if(pixelID == 1) {//upper right
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  0.f))) );//self
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 1.f, -1.f))) );//1Right 1Up
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-2.f, -1.f))) );//2Left 1Up
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 1.f,  2.f))) );//1Right 2Down
		const vec3 sample5 =  w5 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-2.f,  2.f))) );//2Left 2Down
		outColor = vec4(sample1 + sample2 + sample3 + sample4 + sample5, 1.f);
	} else if(pixelID == 2) {//lower left
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  0.f))) );//self
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-1.f,  1.f))) );//1Left 1Down
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 2.f,  1.f))) );//2Right 1Down
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-1.f, -2.f))) );//1Left 2Up
		const vec3 sample5 =  w5 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 2.f, -2.f))) );//2Right 2Up
		outColor = vec4(sample1 + sample2 + sample3 + sample4 + sample5, 1.f);
	} else if(pixelID == 3) {//lower right
		const vec3 sample1 =  w1 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 0.f,  0.f))) );//self
		const vec3 sample2 =  w2 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 1.f,  1.f))) );//1Right 1Down
		const vec3 sample3 =  w3 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-2.f,  1.f))) );//2Left 1Down
		const vec3 sample4 =  w4 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2( 1.f, -2.f))) );//1Right 2Up
		const vec3 sample5 =  w5 * vec3( texture(texSampler, pixelCenterToUV(pixelC+vec2(-2.f, -2.f))) );//2Left 2Up
		outColor = vec4(sample1 + sample2 + sample3 + sample4 + sample5, 1.f);
	}
}
