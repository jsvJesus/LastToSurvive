#include "Renderer/RenderView.h"
#include "Renderer/StaticModelRenderer.h"
#include <cmath>
#include <cstdio>
#include <limits>
namespace{bool Check(bool value,const char* text){if(value)return true;std::fprintf(stderr,"FAILED: %s\n",text);return false;}}
int main()
{
    engine::renderer::RenderView view;view.viewport.width=1280;view.viewport.height=720;if(!Check(view.IsValid(),"valid RenderView"))return 1;view.lightIntensity=-1;if(!Check(!view.IsValid(),"negative light rejected"))return 1;view.lightIntensity=1;view.view.m[0][0]=(std::numeric_limits<float>::infinity)();if(!Check(!view.IsValid(),"non-finite matrix rejected"))return 1;
    engine::renderer::StaticModelInstance instance;if(!Check(!instance.IsValid(),"invalid model handle rejected"))return 1;instance.model={3,7};if(!Check(instance.IsValid(),"generation-safe instance contract"))return 1;instance.world.m[2][1]=(std::numeric_limits<float>::quiet_NaN)();if(!Check(!instance.IsValid(),"non-finite instance transform rejected"))return 1;
    engine::renderer::StaticModelRenderer renderer;if(!Check(!renderer.IsInitialized(),"renderer starts shutdown"))return 1;renderer.Shutdown();renderer.Shutdown();if(!Check(!renderer.IsInitialized(),"renderer repeated shutdown"))return 1;
    std::puts("LTS.Renderer tests passed");return 0;
}
