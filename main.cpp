#include "Core/WindowManager.h"
#include "Core/Input.h"
#include "Core/Loader.h"
#include "core/Camera.h"

//TODO: clean up this file so that we don't need to worry so much about including extra vulkan stuff.
#include "VulkanRenderer/VulkanRenderer.h"

//REFACTOR: get the framebuffer,renderpass, and pipeline interop better. Instead of writing to a front buffer all the time write to a back buffer and flush to front
//REFACTOR: find where you are using constnats and stop doing that. properly set things.
//REFACTOR: error handling inside the renderer needs to happen. Right now we are just flying and that is a mega mistake. shit could be dying badly.
//REFACTOR: consider figuring out a better way to do passing stuff in. I'm getting sick and tired of having brain confusion over & and * this is a symptom of the destructors
//TODO: get a code review...
//NEXT: Camera/camera controls, load an index buffer model.
int main()
{
    //TODO: config file
    bool validation = true;

    WindowManager window;
    window.init(480, 320);
    Input::init(&window);z
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
        // pass in camera descriptor set styling here I guess?
        renderer.buildPipeline(pipeline, *renderer.getFrontRenderPass(), shaders);
        VulkanVertexBuffer vertexBuffer;
        renderer.buildModel(vertexBuffer, &loaded, true);
        Camera mainCamera;
        while (window.getWindowClosed())
        {
            // input
            window.pollEvents();
            mainCamera.update();
            //NEXT: GET THE CAMERA INPUT WORKING HERE, THEN HAVE THAT CAMERA DATA SET INTO THE REST
            // OF THE RENDERER AND THE SHADERS. I know you don't wannt really do it but just fucking handle it.

            // rendering
            renderer.beginFrame();
            renderer.beginRecording();
            renderer.beginRenderPass(*renderer.getFrontRenderPass(), *renderer.getFrontBuffer());
            renderer.bindVertexBuffer(vertexBuffer);
            renderer.bindPipeline(pipeline);
            // set camera location, maybe set up a graphics data class and pass that the camera
            renderer.draw();
            renderer.endRenderPass();
            renderer.endRecording();
            renderer.endFrame();
        }
    }

    renderer.cleanup();
    Input::cleanup();
    window.cleanup();
    return 0;
}
