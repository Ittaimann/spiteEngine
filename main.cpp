#include "Core/WindowManager.h"
#include "Core/Loader.h"
//TODO: clean up this file so that we don't need to worry so much about including extra vulkan stuff.
#include "VulkanRenderer/VulkanRenderer.h"

//NEXT: REFACTOR, get the framebuffer,renderpass, and pipeline interop better. Instead of writing to a front buffer all the time write to a back buffer and flush to front
//NEXT: find where you are using constnats and stop doing that. properly set things. 
//NEXT: error handling inside the renderer needs to happen. Right now we are just flying and that is a mega mistake. shit could be dying badly.

//TODO: get a code review...
int main()
{
    //TODO: config file
    bool validation = true;

    WindowManager window;
    window.init(480, 320);

    // TODO: Figure out a standard way to deal with paths to the cache.
    Loader loader;
    ModelLoad loaded = loader.loadModel("../../Assets/glTF-Sample-Models/2.0/TriangleWithoutIndices/glTF/TriangleWithoutIndices.gltf");
    ShaderLoad vert = loader.loadShader("../../AssetCache/vert.spv");
    ShaderLoad frag = loader.loadShader("../../AssetCache/frag.spv");

    VulkanRenderer renderer;
    renderer.init(validation, &window);

    //TODO: REMOVE VULKAN BLEEDING OUT HERE, might need to alter this once I get more generic classes. return "renderpass", "frame buffer", and "image"
    // and pass those into the renderer as like a "build the driver version", then return a pointer to the default?
    // Maybe have it so that its a struct of data we pass into the apis and  then have a driver pointer attach
    {

        VulkanGraphicsPipeline pipeline;
        std::vector<VulkanShader> shaders(2);
        renderer.buildShader(shaders[0], &vert);
        renderer.buildShader(shaders[1], &frag);
        renderer.buildPipeline(pipeline, *renderer.getFrontRenderPass(), shaders);
        VulkanVertexBuffer vertexBuffer;
        renderer.buildModel(vertexBuffer, &loaded, true);
        //TODO: render loop and exit from glfw input
        while (window.getWindowClosed())
        {
            // input
            window.pollEvents();

            // rendering
            renderer.beginFrame();
            renderer.beginRecording();


            renderer.beginRenderPass(*renderer.getFrontRenderPass(),*renderer.getFrontBuffer());

            renderer.bindVertexBuffer(vertexBuffer);
            renderer.bindPipeline(pipeline);
            renderer.draw();

            renderer.endRenderPass();
            renderer.endRecording();
            renderer.endFrame();

         }
    }   
    renderer.cleanup();

    window.cleanup();
    return 0;
}
