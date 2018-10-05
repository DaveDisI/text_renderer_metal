#include <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

#include "graphics_math.h"
#include "bitmap_modifier.h"
#include "truetype_parser.h"

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


#include <stdlib.h>
#include <math.h>

#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <CoreFoundation/CFRunLoop.h>

#define NUM_BUFFERS 3
#define BUFFER_SIZE 4096
#define SAMPLE_TYPE short
#define MAX_NUMBER 32767
#define SAMPLE_RATE 44100

unsigned int count;

int main(int argc, char** argv){
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [NSApp sharedApplication];

    NSData* dta = [NSData dataWithContentsOfFile: @"Arial.ttf"];
    unsigned char* fontData = (unsigned char*)[dta bytes];

    int totChar = 127 - 32;
    Bitmap* bmArr = new Bitmap[totChar];
    for(int i = 0; i < totChar; i++){
        unsigned char c = i + 32;
        bmArr[i].bytes = getReducedBitmapFromCharCode(fontData, c, &bmArr[i].width, &bmArr[i].height, 32);
    }
    BitmapAtlas bitmapAtlas = createBitmapAtlas(bmArr, totChar);
    unsigned char* bitmap = bitmapAtlas.bitmapData;
    unsigned int glyphWidth = bitmapAtlas.totalWidth; 
    unsigned int glyphHeight = bitmapAtlas.totalHeight;
    
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
    float triangleVertices[12][2] = {
            {-sz, -sz}, {0, 0},   
            {-sz, sz}, {0, 1}, 
            {sz, sz}, {1, 1},
            {sz, sz}, {1, 1},  
            {sz, -sz}, {1, 0},   
            {-sz, -sz}, {0, 0}
    };

    mat4 mvp = setOrthogonalProjection(-450, 450, -250, 250, -1, 1);

    id<MTLBuffer> vertBuffer = [device newBufferWithBytes: triangleVertices
                                        length: sizeof(triangleVertices)
                                        options: MTLResourceStorageModeShared];
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
                        vertexCount:6];
            [renderEncoder endEncoding];
            [commandBuffer presentDrawable:view.currentDrawable];
        }
        [commandBuffer commit];
        [view draw];
    }

    [pool release];
    return 0;
}