**University of Pennsylvania, CIS 565: GPU Programming and Architecture,
Final Project - Vulkan VR Rendering**

# Barrel Filter and Aberration Methods
* Based on Brown-Conrady Distortion model, but must get constants from HMD vendor
* Option 1. Do it all in frag shader (each fragment is doing the math)
* Option 2. Warp a mesh down in vertex shader and do chormatic aberration in fragment shader(or vertex shader and let the hardware interopolate, Brown-Conrady isn't linear but if the mesh is dense enough it won't matter)
* Option 3. Pre-warp the mesh and pre-calculate all chromatic aberration values up front and pack them into vertex attributes
* All of these were about the same performance (this stage didn't add anything, almost the same as passthrough, about 0.1ms more on a gtx960m)
* A forth option avoids a post process by warping the NDC positions of the drawn meshes in the vertex shader and tesselating them. I belive this requires an art team to make sure this works out for every mesh. 
* see: https://www.imgtec.com/blog/speeding-up-gpu-barrel-distortion-correction-in-mobile-vr/
for reasons why the mesh needs to be dense enough (texture sampling gets funky because of Distortion model's nonlinearity. But if you subdivide enough, things approach linearity)
![](SecondaryVR/img/nonstencil.png)
![](SecondaryVR/img/aberration.png)


# Issues with finding inverse Brown-Conrady Distortion
* I relied on the Secant Method for finding the inverse of non-invertable function in order to reverse warp the mesh in optios 2 and 3 above. 
* However, you can run into root jumping issues, which I did but got around by letting the mesh position fall where it may recalculating its UV and finding the source UV from the normal Brown-conrady distortion. It's all pre-baked anyway so it doesnt matter it we take this extra step, just so long as the end result is correct.
![](SecondaryVR/img/secant.png)
![](SecondaryVR/img/secantmethod.png)
![](SecondaryVR/img/rootjumping.png)
![](SecondaryVR/img/precalcmesh.png)

# Radial Density Masking
* see https://www.youtube.com/watch?v=DdL3WC_oBO4
* Due to barrel distortion (needed to counteract the pincushioning as the 2D image refracts though the lenses) and hits our eyes, we are over rendering at the edges of the image, by as much as 2x. Barrel distortion squeezes down a 1.4x virtual render target to the device's screen size, pixels in the center get blown out and pixels at the edge get pulled in. See below.
* Radial Density masking uses the stencil to cull 2x2 pixel quads in early-z to avoid rendering them in the fragment shader. 
* The Mask is made by hand in code and uploaded to the stencil once and is used by the forward render pass (VR renderers need forward rendering because MSAA is such a huge win for image fidelity)
* Masking is huge savings, about 20-25% off the top, the issue however is hole filling. Which can put you back where you started, it did for me.
**Vlachos GDC 2016 VR talk on radial density masking**<br />
![](SecondaryVR/img/radialdensitymask.png)
**Construct 8bit array of stencil values before rendering**<br />
![](SecondaryVR/img/radialStencilMask.bmp)
**Draw scene**<br />
![](SecondaryVR/img/stencilMask1to1.png)
**Fill holes then warp**<br />
![](SecondaryVR/img/holefill.png)
![](SecondaryVR/img/all.png)
**Warping without hole filling**<br />
![](SecondaryVR/img/stencilmask.png)
**Debug view hole filling**<br />
![](SecondaryVR/img/debugHoleFill.png)
**Sony's version of radial density masking**<br />
![](SecondaryVR/img/radialDensityMaskingWithTAA.png)

# Optimizing Stencil Hole Fill
* Remove all tex fetches from branches(needed for determining uv coords of fetches based on: 1. was it a ignored quad or not, 2: location withing quad), prefetch at the top of shader
* If you're not seeing benefits you'll see additional savings the more expensive your pixel shading is

# Is The Barrel Sampling that Dense? 
* Yes, I thought it would be possible to just precalculate a stencil that masked out the places where the barrel filter wouldn't be sampling
* The motivation for this was the expensive hole filling in radial density masking. 
** All color channels single pixel sample<br />
![](SecondaryVR/img/preCalcBarrelSamplingMaskActualPixelsThatWillBeSampled.bmp)
** All color channels sample pixel quad <br />
![](SecondaryVR/img/preCalcBarrelSamplingMaskActualPixelsThatWIllBeSampled_TheirQuads.bmp)


# Adaptive Quality Filtering
* see https://www.youtube.com/watch?v=DdL3WC_oBO4
* Async time warp and space warp are unpleasent experiences for the user, should really only be last resort. If you're using it to maintain frame rate you're creating a really uncomfortable VR experience.
* Use Adaptive quality filtering to detect when user is turning head towards an expensive view. If the last frame time starts go above some target threshold then begin to turn down settings (MSAA and virtual render target scaling (the render target size pre-barrel distortion))
* Can probably avoid async time and space warp altogether.
![](SecondaryVR/img/adaptiveQuality.bmp)
![](SecondaryVR/img/adaptiveQualitySettings.png)
**Cycling back and forth through settings**<br />
![](SecondaryVR/img/AQ.gif)
**Resolution scale 1.5**<br />
![](SecondaryVR/img/adaptiveQuality1.5.png)
**Resolution scale 0.5**<br />
![](SecondaryVR/img/adaptiveQuality0.5.png)

# Asynchronous Time Warp (ATW)
* In another thread, prepare last frame's final render target, depth buffer, and view matrix. If we are going to miss vsync with our current render task, prempt the gpu and warp old fragments into the new screen space using the updated viewproj and the old viewproj.
* To avoid disocclusion artifacts, use last frames camera position for the current camera position. If you're ok with disocclusion, you can use the updated camera position as well. 
* Vert shader: either using a dense grid mesh or a rect of points for every pixel, sample the old depth buffer and turn the sample into an ndc value.Transform to world space using the viewproj inverse from last frame then transform to current ndc space using the current viewproj. 
* Frag Shader: use the fragments original uv value to sample from the previous render target to pull that colored fragment over to its updated position in the current screen space. 

** Time Warp Simulation: <br />
* Starts out normally with vr and radial density mask
* Then enters time warp simulation mode where rendering is frozen. We take note of tripple buffer ID of last frame rendered as well as the camera state. 
* Perform warping the previous fragments into the new screen space as described above
![](SecondaryVR/img/timewarp.gif)



# Vulkan Performance Things
* To limit context switches (changing shaders, mesh info, etc):
* Sift shader calls into secondary command buffers and combine into one primary so there's only one shader switch for each shader that is needed in the render pass
* use push constants for per draw call dynamic things (model matrix, bit field flags)
* have the UBO use size 2 array for the viewproj of each eye, set the viewport state to dynamic, use push constants to view port switching commands to render in one render pass. (single scene traversal still 2 draw calls)
* multithread your command building (use secondary command buffers), assemble into primary, submit.
* Reduce command buffer count, number of render passes done to build a frame
* Use subpasses when you can, requires 1 to 1 frag sampling from the output of one subpass to the input of another. Good use case would be deferred rendering.

# Data
**Performance of various Barrel/Chromatic Aberration Techniques and Radial Density Mask**<br />
![](SecondaryVR/img/BarrelAberrationStencil.png)

**Push Constant vs UBO updates**<br />
![](SecondaryVR/img/pushconstant.png)


**GPU Device Properties**<br />
https://devblogs.nvidia.com/parallelforall/5-things-you-should-know-about-new-maxwell-gpu-architecture/<br />
cuda cores 640<br />
mem bandwidth 86.4 GB/s<br />
L2 cache size 2MB<br />
num banks in shared memory 32<br />
number of multiprocessor 5<br />
max blocks per multiprocessor 32<br />
total shared mem per block 49152 bytes<br />
total shared mem per MP 65536 bytes<br />
total regs per block and MP 65536<br />
max threads per block 1024<br />
max threads per mp 2048<br />
total const memory 65536<br />
max reg per thread 255<br />
max concurrent warps 64<br />
total global mem 2G<br />
<br />
max dims for block 1024 1024 64<br />
max dims for a grid 2,147,483,647 65536 65536<br />
clock rate 1,097,5000<br />
texture alignment 512<br />
concurrent copy and execution yes<br />
major.minor 5.0<br />
