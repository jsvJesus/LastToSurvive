#include "r3dPCH.h"
#include "r3d.h"
#include "StudioGraphicsShell.h"
#include "StudioRuntimeBridge.h"

#include <windowsx.h>

#include <Assets/AssetLoaderRegistry.h>
#include <Assets/AssetManager.h>
#include <Assets/DdsTextureLoader.h>
#include <Assets/FileAssetSource.h>
#include <Assets/MaterialAssetLoader.h>
#include <Assets/MeshAssetLoader.h>
#include <Assets/ShaderAssetLoader.h>
#include <Assets/StaticModelAssetLoader.h>
#include <Graphics/CommandContext.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/SwapChain.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>
#include <GraphicsDX11/D3D11Device.h>
#include <Math/Matrix4.h>
#include <Platform/Clock.h>
#include <Platform/MessagePump.h>
#include <Platform/Window.h>
#include <Renderer/StaticModelRenderer.h>
#include <Runtime/Engine.h>
#include <Runtime/RendererBackend.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

extern void RegisterMsgProc(bool (*proc)(UINT,WPARAM,LPARAM));
extern void UnregisterMsgProc(bool (*proc)(UINT,WPARAM,LPARAM));
extern char __r3dCmdLine[1024];
PCHAR* CommandLineToArgvA(PCHAR commandLine,int* argumentCount);

namespace
{
using engine::graphics::GraphicsResult;using studio::StudioGraphicsShellResult;
bool HasCommandLineSwitch(const char* value)
{int count=0;PCHAR* args=CommandLineToArgvA(__r3dCmdLine,&count);if(!args)return false;bool found=false;for(int i=0;i<count;++i)if(_stricmp(args[i],value)==0){found=true;break;}GlobalFree(args);return found;}
std::string GetCommandLineValue(const char* prefix)
{int count=0;PCHAR* args=CommandLineToArgvA(__r3dCmdLine,&count);if(!args)return{};std::string result;const auto length=std::strlen(prefix);for(int i=0;i<count;++i)if(_strnicmp(args[i],prefix,length)==0){result=args[i]+length;break;}GlobalFree(args);return result;}
unsigned SceneCount()noexcept{const auto text=GetCommandLineValue("-dx11sceneinstances=");if(text.empty())return 3U;char* end=nullptr;const unsigned long value=std::strtoul(text.c_str(),&end,10);return end&&*end=='\0'&&value>=1&&value<=64?static_cast<unsigned>(value):3U;}
engine::renderer::MaterialDebugMode DebugMode()noexcept
{const auto value=GetCommandLineValue("-dx11materialdebug=");constexpr const char* names[]={"lit","basecolor","worldnormal","tangentnormal","roughness","specular","emissive"};for(unsigned i=0;i<7;++i)if(value==names[i])return static_cast<engine::renderer::MaterialDebugMode>(i);return engine::renderer::MaterialDebugMode::Lit;}

class StudioDX11Shell final
{
public:
 bool Initialize(const std::uintptr_t nativeWindow)noexcept
 {
  Shutdown();HWND hwnd=reinterpret_cast<HWND>(nativeWindow);if(!hwnd)return Fail("invalid Studio HWND",GraphicsResult::InvalidArgument);if(!ApplyResizableWindowStyle(hwnd))return Fail("Studio window style",GraphicsResult::BackendFailure);window_=engine::platform::Window(engine::platform::NativeWindowHandle::FromValue(nativeWindow));const auto size=window_.GetClientSize();if(!window_.IsValid()||size.IsEmpty())return Fail("window size",GraphicsResult::InvalidArgument);
  engine::graphics::RenderDeviceDesc dd;dd.backend=HasCommandLineSwitch("-dx11shell-fail")?engine::graphics::GraphicsBackend::None:engine::graphics::GraphicsBackend::D3D11;dd.enableValidation=true;auto gr=device_.Initialize(dd);if(engine::graphics::Failed(gr))return Fail("device",gr);context_=device_.GetImmediateCommandContext();if(!context_||!context_->IsValid())return Fail("context",GraphicsResult::InvalidState);
  engine::graphics::SwapChainDesc sd;sd.window=window_.GetNativeHandle();sd.width=size.width;sd.height=size.height;sd.bufferCount=2;sd.presentMode=engine::graphics::PresentMode::VSync;if(engine::graphics::Failed(gr=device_.CreateSwapChain(sd,swapChain_)))return Fail("swap chain",gr);if(!CreateSizeResources(size.width,size.height))return false;
  const auto rootText=GetCommandLineValue("-dx11assetroot=");const auto modelText=GetCommandLineValue("-dx11model=");if(rootText.empty()||modelText.empty())return Fail("-dx11assetroot/-dx11model",GraphicsResult::InvalidArgument);const auto root=std::filesystem::absolute(std::filesystem::u8path(rootText)).lexically_normal();if(engine::assets::Failed(assetSource_.Initialize(root))||engine::assets::Failed(assetManager_.Initialize(assetSource_))||engine::assets::Failed(assetRegistry_.Register(staticModelLoader_))||engine::assets::Failed(assetRegistry_.Register(meshLoader_))||engine::assets::Failed(assetRegistry_.Register(materialLoader_))||engine::assets::Failed(assetRegistry_.Register(textureLoader_))||engine::assets::Failed(assetRegistry_.Register(shaderLoader_)))return Fail("asset setup",GraphicsResult::InvalidState);
  const auto vsText=GetCommandLineValue("-dx11staticvs=").empty()?"cookedshaders/staticmodelcompatibility.vs.ltsshader":GetCommandLineValue("-dx11staticvs=");const auto psText=GetCommandLineValue("-dx11staticps=").empty()?"cookedshaders/staticmodelcompatibility.ps.ltsshader":GetCommandLineValue("-dx11staticps=");engine::assets::AssetPath vsPath,psPath,modelPath;if(engine::assets::Failed(engine::assets::AssetPath::TryCreate(vsText,vsPath))||engine::assets::Failed(engine::assets::AssetPath::TryCreate(psText,psPath))||engine::assets::Failed(engine::assets::AssetPath::TryCreate(modelText,modelPath)))return Fail("asset paths",GraphicsResult::InvalidArgument);
  std::unique_ptr<engine::assets::LoadedAsset> loaded;auto ar=LoadAsset(vsPath,engine::assets::AssetType::Shader,loaded);if(engine::assets::Failed(ar)){r3dOutToLog("[Graphics][DX11] Static VS load failed: %s\n",engine::assets::ToString(ar));return Fail("compiled vertex shader",GraphicsResult::InvalidState);}auto vs=static_cast<engine::assets::ShaderLoadedAsset*>(loaded.get())->ReleaseShader();ar=LoadAsset(psPath,engine::assets::AssetType::Shader,loaded);if(engine::assets::Failed(ar)){r3dOutToLog("[Graphics][DX11] Static PS load failed: %s\n",engine::assets::ToString(ar));return Fail("compiled pixel shader",GraphicsResult::InvalidState);}auto ps=static_cast<engine::assets::ShaderLoadedAsset*>(loaded.get())->ReleaseShader();if(engine::graphics::Failed(gr=renderer_.Initialize(device_,*context_,vs,ps)))return Fail("StaticModelRenderer",gr);if(engine::assets::Failed(ar=renderer_.CreateModel(assetManager_,assetRegistry_,modelPath,model_))){r3dOutToLog("[Graphics][DX11] Model resource load failed: %s\n",engine::assets::ToString(ar));return Fail("model resource",GraphicsResult::InvalidState);}if(engine::assets::Failed(renderer_.GetModelBounds(model_,bounds_)))return Fail("model bounds",GraphicsResult::InvalidState);
  BuildInstances(SceneCount());debugMode_=DebugMode();width_=size.width;height_=size.height;initialized_=true;minimized_=false;r3dOutToLog("[Graphics][DX11] Compiled shaders loaded: VS=%s PS=%s\n",vsText.c_str(),psText.c_str());r3dOutToLog("[Graphics][DX11] Reusable model resource created once; scene instances=%zu\n",instances_.size());r3dOutToLog("[Graphics][DX11] Selected reusable renderer shell backend\n");return true;
 }
 void OnSize(WPARAM type,std::uint32_t w,std::uint32_t h)noexcept{if(!initialized_)return;if(type==SIZE_MINIMIZED||!w||!h){minimized_=true;return;}pendingWidth_=w;pendingHeight_=h;resizePending_=true;if(minimized_)resetTimer_=true;minimized_=false;}
 void OnMouseButton(bool down,int x,int y)noexcept{if(down&&!manualRotation_){yaw_=elapsed_*.22F;pitch_=elapsed_*.11F;}manualRotation_=true;dragging_=down;lastMouseX_=x;lastMouseY_=y;if(windowHandle_){if(down)SetCapture(windowHandle_);else if(GetCapture()==windowHandle_)ReleaseCapture();}}
 void OnMouseMove(int x,int y)noexcept{if(dragging_){yaw_+=static_cast<float>(x-lastMouseX_)*0.008F;pitch_+=static_cast<float>(y-lastMouseY_)*0.008F;pitch_=(std::max)(-1.45F,(std::min)(1.45F,pitch_));}lastMouseX_=x;lastMouseY_=y;}
 void OnMouseWheel(short delta)noexcept{zoomScale_*=std::exp(-static_cast<float>(delta)/WHEEL_DELTA*.14F);zoomScale_=(std::max)(.35F,(std::min)(4.0F,zoomScale_));}
 void RequestClose()noexcept{closeRequested_=true;}bool IsCloseRequested()const noexcept{return closeRequested_;}bool ShouldWaitForMessage()const noexcept{return minimized_||occluded_;}bool ConsumeTimerReset()noexcept{const bool value=resetTimer_;resetTimer_=false;return value;}StudioGraphicsShellResult GetFailureResult()const noexcept{return failureResult_;}
 bool RenderFrame(const double delta)noexcept
 {
  if(!initialized_)return false;if(resizePending_&&!Resize(pendingWidth_,pendingHeight_))return false;if(minimized_)return true;elapsed_+=static_cast<float>(delta);if(elapsed_>1000)elapsed_=std::fmod(elapsed_,1000.0F);UpdateInstances();engine::graphics::ClearColor clear{0.035F,0.055F,0.085F,1};auto gr=context_->SetSwapChainRenderTarget(*swapChain_,depth_);if(engine::graphics::Failed(gr))return FailFrame("targets",gr);engine::graphics::Viewport viewport;viewport.width=static_cast<float>(width_);viewport.height=static_cast<float>(height_);if(engine::graphics::Failed(gr=context_->SetViewport(viewport)))return FailFrame("viewport",gr);engine::graphics::ScissorRect scissor;scissor.right=static_cast<std::int32_t>(width_);scissor.bottom=static_cast<std::int32_t>(height_);if(engine::graphics::Failed(gr=context_->SetScissorRect(scissor))||engine::graphics::Failed(gr=context_->ClearSwapChainColor(*swapChain_,clear))||engine::graphics::Failed(gr=context_->ClearDepthStencilTarget(depth_,engine::graphics::ClearDepthStencilFlags::Depth|engine::graphics::ClearDepthStencilFlags::Stencil,1,0)))return FailFrame("clear",gr);
  engine::renderer::RenderView view;const float radius=(std::max)(bounds_.sphereRadius,0.001F);const unsigned columns=static_cast<unsigned>(std::ceil(std::sqrt(static_cast<float>(instances_.size()))));const float distance=(std::max)(radius*(3.2F+columns*1.35F)*zoomScale_,radius*1.05F);view.view=engine::math::Matrix4::CreateTranslation({0,0,distance});const float aspect=static_cast<float>(width_)/height_,ys=1/std::tan(.55F),xs=ys/aspect,zn=(std::max)(radius*.01F,.0001F),zf=(std::max)(distance+radius*12,zn+1);view.projection={xs,0,0,0,0,ys,0,0,0,0,zf/(zf-zn),1,0,0,-zn*zf/(zf-zn),0};view.viewProjection=view.view*view.projection;view.cameraPosition={0,0,-distance};view.viewport=viewport;view.elapsedTime=elapsed_;view.debugMode=debugMode_;view.ambientColor={.34F,.35F,.38F};view.lightIntensity=1.2F;engine::renderer::StaticModelRenderStats stats;if(engine::graphics::Failed(gr=renderer_.Render(view,instances_.data(),instances_.size(),stats)))return FailFrame("renderer",gr);if(!firstFrameLogged_){r3dOutToLog("[Renderer] First frame: submitted=%zu accepted=%zu draws=%zu triangles=%zu models=%zu textures=%zu reused=%zu pipelines=%zu materials=%zu meshes=%zu\n",stats.submittedInstances,stats.acceptedInstances,stats.drawCalls,stats.triangles,stats.uniqueModelResources,stats.uniqueGpuTextures,stats.reusedTextureBindings,stats.pipelineChanges,stats.materialChanges,stats.meshChanges);firstFrameLogged_=true;}
  engine::graphics::PresentStatus status;if(engine::graphics::Failed(gr=swapChain_->Present(status))||status==engine::graphics::PresentStatus::DeviceLost||status==engine::graphics::PresentStatus::DeviceRemoved){failureResult_=status==engine::graphics::PresentStatus::DeviceLost?StudioGraphicsShellResult::DeviceLost:status==engine::graphics::PresentStatus::DeviceRemoved?StudioGraphicsShellResult::DeviceRemoved:StudioGraphicsShellResult::FrameFailed;return false;}if(status==engine::graphics::PresentStatus::Occluded){occluded_=true;return true;}if(occluded_){occluded_=false;resetTimer_=true;}return true;
 }
 void Shutdown()noexcept
 {
  if(context_){context_->UnbindRenderTargets();}if(model_.IsValid())(void)renderer_.DestroyModel(model_);model_={};renderer_.Shutdown();if(context_){context_->ClearState();context_->Flush();}DestroyDepth();swapChain_.reset();assetRegistry_.Clear();assetManager_.Shutdown();assetSource_.Shutdown();context_=nullptr;device_.Shutdown();RestoreWindowStyle();instances_.clear();initialized_=minimized_=resizePending_=closeRequested_=occluded_=firstFrameLogged_=false;manualRotation_=dragging_=false;resetTimer_=true;yaw_=pitch_=elapsed_=0;zoomScale_=1;width_=height_=0;failureResult_=StudioGraphicsShellResult::FrameFailed;
 }
 ~StudioDX11Shell()noexcept{Shutdown();}
private:
 engine::assets::AssetResult LoadAsset(const engine::assets::AssetPath& path,engine::assets::AssetType type,std::unique_ptr<engine::assets::LoadedAsset>& out)noexcept{engine::assets::AssetHandle handle;auto result=assetManager_.FindByPath(path,handle);if(result==engine::assets::AssetResult::NotFound){engine::assets::AssetMetadata md;md.path=path;md.id=path.GetId();md.type=type;result=assetManager_.Register(md,handle);}if(engine::assets::Failed(result))return result;if(engine::assets::Failed(result=assetManager_.Load(handle)))return result;const engine::assets::AssetData* data=nullptr;if(engine::assets::Failed(result=assetManager_.GetData(handle,data))||!data)return engine::assets::AssetResult::IoError;engine::assets::AssetMetadata md;if(engine::assets::Failed(result=assetManager_.GetMetadata(handle,md)))return result;return assetRegistry_.Load(md,*data,out);}
 void BuildInstances(unsigned count){instances_.resize(count);const unsigned columns=static_cast<unsigned>(std::ceil(std::sqrt(static_cast<float>(count))));const unsigned rows=(count+columns-1)/columns;const float spacing=(std::max)(bounds_.sphereRadius*2.7F,.05F);const auto center=engine::math::Matrix4::CreateTranslation({-bounds_.sphereCenter[0],-bounds_.sphereCenter[1],-bounds_.sphereCenter[2]});for(unsigned i=0;i<count;++i){const float x=(static_cast<float>(i%columns)-static_cast<float>(columns-1)*.5F)*spacing;const float y=(static_cast<float>(rows-1)*.5F-static_cast<float>(i/columns))*spacing;auto scale=i==count-1&&count>1?engine::math::Matrix4::CreateScale({1.25F,.8F,1.1F}):engine::math::Matrix4::Identity();instances_[i].model=model_;instances_[i].world=center*scale*engine::math::Matrix4::CreateTranslation({x,y,0});instances_[i].objectId=i+1;}}
 void UpdateInstances(){const auto center=engine::math::Matrix4::CreateTranslation({-bounds_.sphereCenter[0],-bounds_.sphereCenter[1],-bounds_.sphereCenter[2]});const unsigned count=static_cast<unsigned>(instances_.size()),columns=static_cast<unsigned>(std::ceil(std::sqrt(static_cast<float>(count)))),rows=(count+columns-1)/columns;const float spacing=(std::max)(bounds_.sphereRadius*2.7F,.05F);for(unsigned i=0;i<count;++i){const float x=(static_cast<float>(i%columns)-static_cast<float>(columns-1)*.5F)*spacing,y=(static_cast<float>(rows-1)*.5F-static_cast<float>(i/columns))*spacing;auto scale=i==count-1&&count>1?engine::math::Matrix4::CreateScale({1.25F,.8F,1.1F}):engine::math::Matrix4::Identity();const float yaw=manualRotation_?yaw_:elapsed_*(.22F+.07F*i),pitch=manualRotation_?pitch_:elapsed_*.11F;instances_[i].world=center*scale*engine::math::Matrix4::CreateRotationY(yaw)*engine::math::Matrix4::CreateRotationX(pitch)*engine::math::Matrix4::CreateTranslation({x,y,0});}}
 bool CreateSizeResources(std::uint32_t w,std::uint32_t h)noexcept{engine::graphics::TextureDesc td;td.width=w;td.height=h;td.format=engine::graphics::Format::D24UNormS8UInt;td.bindFlags=engine::graphics::TextureBindFlags::DepthStencil;const auto result=device_.CreateTexture(td,nullptr,0,depth_);return engine::graphics::Succeeded(result)||Fail("depth",result);}
 bool Resize(std::uint32_t w,std::uint32_t h)noexcept{resizePending_=false;if(!w||!h||(w==width_&&h==height_))return true;context_->UnbindRenderTargets();DestroyDepth();auto result=swapChain_->Resize(w,h);if(engine::graphics::Failed(result))return FailFrame("resize",result);if(!CreateSizeResources(w,h))return false;width_=w;height_=h;return true;}
 void DestroyDepth()noexcept{if(depth_.IsValid()){(void)device_.DestroyTexture(depth_);depth_={};}}
 bool Fail(const char* phase,GraphicsResult result)noexcept{r3dOutToLog("[Graphics][DX11] Initialization failed at %s: %s\n",phase,engine::graphics::ToString(result));Shutdown();return false;}bool FailFrame(const char* phase,GraphicsResult result)noexcept{r3dOutToLog("[Graphics][DX11] Frame failure at %s: %s\n",phase,engine::graphics::ToString(result));failureResult_=result==GraphicsResult::DeviceLost?StudioGraphicsShellResult::DeviceLost:result==GraphicsResult::DeviceRemoved?StudioGraphicsShellResult::DeviceRemoved:StudioGraphicsShellResult::FrameFailed;return false;}
 bool ApplyResizableWindowStyle(HWND hwnd)noexcept{windowHandle_=hwnd;originalWindowStyle_=GetWindowLongPtr(hwnd,GWL_STYLE);const LONG_PTR style=originalWindowStyle_|WS_THICKFRAME|WS_MAXIMIZEBOX;if(style!=originalWindowStyle_){if(SetWindowLongPtr(hwnd,GWL_STYLE,style)==0&&GetLastError()!=ERROR_SUCCESS)return false;styleChanged_=true;(void)SetWindowPos(hwnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);}return true;}
 void RestoreWindowStyle()noexcept{if(styleChanged_&&windowHandle_){(void)SetWindowLongPtr(windowHandle_,GWL_STYLE,originalWindowStyle_);(void)SetWindowPos(windowHandle_,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);}windowHandle_=nullptr;styleChanged_=false;}
 engine::platform::Window window_;engine::graphics::d3d11::D3D11Device device_;engine::graphics::CommandContext* context_=nullptr;std::unique_ptr<engine::graphics::SwapChain> swapChain_;engine::graphics::TextureHandle depth_;engine::assets::FileAssetSource assetSource_;engine::assets::AssetManager assetManager_;engine::assets::AssetLoaderRegistry assetRegistry_;engine::assets::StaticModelAssetLoader staticModelLoader_;engine::assets::MeshAssetLoader meshLoader_;engine::assets::MaterialAssetLoader materialLoader_;engine::assets::DdsTextureLoader textureLoader_;engine::assets::ShaderAssetLoader shaderLoader_;engine::renderer::StaticModelRenderer renderer_;engine::renderer::StaticModelRenderHandle model_;engine::assets::MeshBounds bounds_;std::vector<engine::renderer::StaticModelInstance> instances_;engine::renderer::MaterialDebugMode debugMode_=engine::renderer::MaterialDebugMode::Lit;std::uint32_t width_=0,height_=0,pendingWidth_=0,pendingHeight_=0;bool initialized_=false,minimized_=false,resizePending_=false,closeRequested_=false,occluded_=false,resetTimer_=true,firstFrameLogged_=false;bool manualRotation_=false,dragging_=false;int lastMouseX_=0,lastMouseY_=0;float yaw_=0,pitch_=0,elapsed_=0,zoomScale_=1;HWND windowHandle_=nullptr;LONG_PTR originalWindowStyle_=0;bool styleChanged_=false;StudioGraphicsShellResult failureResult_=StudioGraphicsShellResult::FrameFailed;
};

StudioDX11Shell* g_activeShell=nullptr;
bool StudioDX11ShellMsgProc(UINT message,WPARAM w,LPARAM l){if(!g_activeShell)return false;if(message==WM_SIZE)g_activeShell->OnSize(w,LOWORD(l),HIWORD(l));else if(message==WM_LBUTTONDOWN){g_activeShell->OnMouseButton(true,GET_X_LPARAM(l),GET_Y_LPARAM(l));return true;}else if(message==WM_LBUTTONUP){g_activeShell->OnMouseButton(false,GET_X_LPARAM(l),GET_Y_LPARAM(l));return true;}else if(message==WM_MOUSEMOVE){g_activeShell->OnMouseMove(GET_X_LPARAM(l),GET_Y_LPARAM(l));}else if(message==WM_MOUSEWHEEL){g_activeShell->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(w));return true;}else if(message==WM_CLOSE||message==WM_DESTROY){g_activeShell->RequestClose();return true;}return false;}
}

namespace studio
{
bool WantsDX11Shell()noexcept{return HasCommandLineSwitch("-dx11shell")||HasCommandLineSwitch("/dx11shell")||HasCommandLineSwitch("-dx11shell-fail");}
const char* ToString(StudioGraphicsShellResult result)noexcept{switch(result){case StudioGraphicsShellResult::NotRequested:return"NotRequested";case StudioGraphicsShellResult::Completed:return"Completed";case StudioGraphicsShellResult::InitializationFailed:return"InitializationFailed";case StudioGraphicsShellResult::RuntimeInitializationFailed:return"RuntimeInitializationFailed";case StudioGraphicsShellResult::FrameFailed:return"FrameFailed";case StudioGraphicsShellResult::DeviceLost:return"DeviceLost";case StudioGraphicsShellResult::DeviceRemoved:return"DeviceRemoved";default:return"Unknown";}}
StudioGraphicsShellResult RunDX11Shell(const std::uintptr_t nativeWindow)noexcept
{
 if(!WantsDX11Shell())return StudioGraphicsShellResult::NotRequested;StudioDX11Shell shell;if(!shell.Initialize(nativeWindow))return StudioGraphicsShellResult::InitializationFailed;if(!InitializeStudioRuntimeBridge(engine::runtime::RendererBackend::D3D11)){shell.Shutdown();return StudioGraphicsShellResult::RuntimeInitializationFailed;}g_activeShell=&shell;RegisterMsgProc(StudioDX11ShellMsgProc);ShowWindow(reinterpret_cast<HWND>(nativeWindow),SW_SHOW);UpdateWindow(reinterpret_cast<HWND>(nativeWindow));bool frame=true,quit=false;auto previous=engine::platform::Clock::Now();bool timer=previous!=0;while(!quit&&frame){if(shell.ShouldWaitForMessage())(void)engine::platform::MessagePump::WaitForMessage();const auto messages=engine::platform::MessagePump::ProcessPendingMessages();quit=messages.quitRequested||shell.IsCloseRequested();if(!quit){const auto current=engine::platform::Clock::Now();double delta=0;if(shell.ConsumeTimerReset())timer=false;if(timer&&current)delta=engine::platform::Clock::ElapsedSeconds(previous,current);if(!std::isfinite(delta)||delta<0)delta=0;delta=(std::min)(delta,.25);previous=current;timer=current!=0;auto* runtime=TryGetRuntimeEngine();const bool began=runtime&&runtime->BeginFrame(delta);frame=began&&shell.RenderFrame(delta);if(began&&!runtime->EndFrame())frame=false;}}
 UnregisterMsgProc(StudioDX11ShellMsgProc);g_activeShell=nullptr;ShutdownStudioRuntimeBridge();const auto result=quit?StudioGraphicsShellResult::Completed:shell.GetFailureResult();shell.Shutdown();r3dOutToLog("[Graphics][DX11] Final shell result: %s\n",ToString(result));return result;
}
}
