#include <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

#include "graphics_math.h"
#include "font_atlas.cpp"
#include "truetype_parser.h"

#include <stdlib.h>
#include <math.h>

struct UniformData{
    mat4 mvp;
};

NSString *shaders = @"\
#include <metal_stdlib>\n\
using namespace metal;\n\
typedef struct{\n\
    float4 pos[[position]];\n\
    float2 textureCoordinate;\n\
} VertOutData;\n\
\
vertex VertOutData vertexShader(uint vertexID [[vertex_id]], constant float2 *vertices[[buffer(0)]], constant float4x4 *mvp[[buffer(1)]]){\n\
    VertOutData out;\n\
    out.pos = mvp[0] * float4(vertices[vertexID * 2], 0, 1);\n\
    out.textureCoordinate = vertices[(vertexID * 2) + 1];\n\
    return out;\n\
}\n\
\
fragment float4 fragmentShader(VertOutData in [[stage_in]], texture2d<half> colorTexture[[texture(0)]]){\n\
    constexpr sampler textureSampler (mag_filter::nearest, min_filter::nearest);\n\
    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);\n\
    return float4(1 - colorSample.r, 1 - colorSample.r, 1 - colorSample.r, colorSample.r);\n\
}\
";

int vertexCount = 0;

void renderText(float* vecPtr, FontAtlas* fa, const char* text, int x, int y, float scale){
    int ctr = 0;
    int xMarker = x;
    while(*text != '\0'){
        char c = *text;
        for(int i = 0; i < fa->totalCharacters; i++){
            if(c == fa->characterCodes[i]){
                if(c == ' '){
                    xMarker += (fa->xShifts[i] * scale);
                    break;
                }

                float left = xMarker;
                float right = (float)xMarker + ((float)fa->widths[i] * scale);
                float bottom = y + (fa->yShifts[i] * scale);
                float top = y + ((fa->heights[i] + fa->yShifts[i]) * scale);
                float tleft = (float)fa->xOffsets[i] / (float)fa->totalBitmapWidth;
                float tright = (float)(fa->xOffsets[i] + fa->widths[i]) / (float)fa->totalBitmapWidth;
                float tbottom = (float)fa->yOffsets[i] / (float)fa->totalBitmapHeight;
                float ttop = (float)(fa->yOffsets[i] + fa->heights[i]) / (float)fa->totalBitmapHeight;

                vecPtr[ctr++] = left; vecPtr[ctr++] = bottom;
                vecPtr[ctr++] = tleft; vecPtr[ctr++] = tbottom;
                vertexCount++;

                vecPtr[ctr++] = left; vecPtr[ctr++] = top;
                vecPtr[ctr++] = tleft; vecPtr[ctr++] = ttop;
                vertexCount++;

                vecPtr[ctr++] = right; vecPtr[ctr++] = top;
                vecPtr[ctr++] = tright; vecPtr[ctr++] = ttop;
                vertexCount++;

                vecPtr[ctr++] = right; vecPtr[ctr++] = top;
                vecPtr[ctr++] = tright; vecPtr[ctr++] = ttop;
                vertexCount++;

                vecPtr[ctr++] = right; vecPtr[ctr++] = bottom;
                vecPtr[ctr++] = tright; vecPtr[ctr++] = tbottom;
                vertexCount++;

                vecPtr[ctr++] = left; vecPtr[ctr++] = bottom;
                vecPtr[ctr++] = tleft; vecPtr[ctr++] = tbottom;
                vertexCount++;

                xMarker += (fa->xShifts[i] * scale);
                break;
            }
        }
        text ++;
    }
}

int main(int argc, char** argv){
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [NSApp sharedApplication];

    NSData* dta = [NSData dataWithContentsOfFile: @"Times New Roman.ttf"];
    unsigned char* fontData = (unsigned char*)[dta bytes];

    unsigned short* charCodes = new unsigned short[95];
    int numChars = 95;
    for(int i = 0; i < numChars; i++){
        charCodes[i] = (unsigned short)(i + 32);
    }
    FontAtlas fa;
    buildFontAtlas(&fa, fontData, numChars, charCodes);

    unsigned char* bitmap = fa.bitmap;
    unsigned int glyphWidth = fa.totalBitmapWidth; 
    unsigned int glyphHeight = fa.totalBitmapHeight;
    
    NSUInteger windowStyle = NSWindowStyleMaskTitled        | 
                             NSWindowStyleMaskClosable      | 
                             NSWindowStyleMaskResizable     | 
                             NSWindowStyleMaskMiniaturizable;

	NSRect screenRect = [[NSScreen mainScreen] frame];
	NSRect viewRect = NSMakeRect(0, 0, 900, 500); 
	NSRect windowRect = NSMakeRect(NSMidX(screenRect) - NSMidX(viewRect),
								 NSMidY(screenRect) - NSMidY(viewRect),
								 viewRect.size.width, 
								 viewRect.size.height);

	NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect 
						styleMask:windowStyle 
						backing:NSBackingStoreBuffered 
						defer:NO]; 
	[window autorelease]; 
 
	NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window]; 
	[windowController autorelease];
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    id<MTLRenderPipelineState> pipelineState;
    id<MTLCommandQueue> commandQueue;
    unsigned int width, height;

    MTKView * view = [[MTKView alloc] initWithFrame: viewRect
                                             device: device];
    view.clearColor = MTLClearColorMake(0.2, 0.4, 0.7, 1);
    view.paused = true;
    view.enableSetNeedsDisplay = false;
    if(view){
        device = view.device;
        NSError* err;
        // Load all the shader files with a .metal file extension in the project
        //id<MTLLibrary> defaultLibrary = [device newLibraryWithFile:@"shaders.metallib" 
        //                                                     error:&err];
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        id<MTLLibrary> defaultLibrary = [device newLibraryWithSource:shaders
                                                             options: options 
                                                             error:&err];

        // Load the vertex function from the library
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
        
        // Load the fragment function from the library
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

        // Configure a pipeline descriptor that is used to create a pipeline state
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.label = @"Simple Pipeline";
        pipelineStateDescriptor.vertexFunction = vertexFunction;
        pipelineStateDescriptor.fragmentFunction = fragmentFunction;
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

        pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
                                                                 error:&err];
        if (!pipelineState)
        {
            // Pipeline State creation could fail if we haven't properly set up our pipeline descriptor.
            //  If the Metal API validation is enabled, we can find out more information about what
            //  went wrong.  (Metal API validation is enabled by default when a debug build is run
            //  from Xcode)
            NSLog(@"Failed to created pipeline state, error %@", err);
            return 1;
        }

		commandQueue = [device newCommandQueue];
    }
    float sz = 200;
    float triangleVertices[24] = {
            -sz, -sz, 0, 0,   
            -sz, sz, 0, 1, 
            sz, sz, 1, 1,
            sz, sz, 1, 1,  
            sz, -sz, 1, 0,   
            -sz, -sz, 0, 0
    };

    mat4 mvp = setOrthogonalProjection(0, 900, 0, 500, -1, 1);

    id<MTLBuffer> vertBuffer = [device newBufferWithLength:64000
                                        options: MTLResourceStorageModeShared];
    float* vpvp = (float*)vertBuffer.contents;

    renderText(vpvp, &fa, "T3$t to icuL@r", 10, 100, 1);

    id<MTLBuffer> uniBuffer = [device newBufferWithBytes: &mvp.m[0][0]
                                        length: sizeof(float) * 16
                                        options: MTLResourceStorageModeShared];

    MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.width = glyphWidth;
    textureDescriptor.height = glyphHeight;
    textureDescriptor.pixelFormat = MTLPixelFormatR8Unorm;
    id<MTLTexture> texture = [device newTextureWithDescriptor: textureDescriptor];
    MTLRegion region = {
        {0, 0, 0},
        {glyphWidth, glyphHeight, 1}
    };
    [texture replaceRegion:region
               mipmapLevel:0
               withBytes:bitmap
               bytesPerRow:glyphWidth];


    width = view.bounds.size.width;
    height = view.bounds.size.height;
    
    [window setContentView:view];
    [window setTitle:[[NSProcessInfo processInfo] processName]];
    [window setAcceptsMouseMovedEvents:YES];
    [window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
	[window orderFrontRegardless];  

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    // Obtain a render pass descriptor, generated from the view's drawable
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;

    // If you've successfully obtained a render pass descriptor, you can render to
    // the drawable; otherwise you skip any rendering this frame because you have no
    // drawable to draw to
    id<MTLRenderCommandEncoder> renderEncoder;
    
    mat4 modelMat = genIdentityMatrix();
    NSEvent* ev;  
    while(true){
        do {
            ev = [NSApp nextEventMatchingMask: NSEventMaskAny
                                    untilDate: nil
                                       inMode: NSDefaultRunLoopMode
                                      dequeue: YES];
            if (ev) {
                if([ev type] == NSEventTypeKeyDown){
                   switch([ev keyCode]){
                       case 53:{
                           [NSApp terminate:NSApp];
                           break;
                       }
                       case 0:{
                           NSLog(@"A\n");
                           break;
                       }
                   }
                }else{
                    [NSApp sendEvent: ev];
                }
            }
        } while (ev);

        // // Finalize rendering here and submit the command buffer to the GPU
        commandBuffer = [commandQueue commandBuffer];
        MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
        if(renderPassDescriptor != nil){
            renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            [renderEncoder setRenderPipelineState:pipelineState];

            mat4* mvpp = (mat4*)uniBuffer.contents;

            *mvpp = multiply(mvp, modelMat);

            [renderEncoder setVertexBuffer:vertBuffer
                                offset:0
                                atIndex:0];

            [renderEncoder setVertexBuffer:uniBuffer
                                offset:0
                                atIndex:1];        

            [renderEncoder setFragmentTexture:texture
                                atIndex:0];

            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:vertexCount];
            [renderEncoder endEncoding];
            [commandBuffer presentDrawable:view.currentDrawable];
        }
        [commandBuffer commit];
        [view draw];
    }

    [pool release];
    return 0;
}