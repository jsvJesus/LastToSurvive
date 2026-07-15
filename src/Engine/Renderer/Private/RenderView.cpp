#include "Renderer/RenderView.h"
#include <cmath>
namespace engine::renderer
{
namespace{bool MatrixFinite(const engine::math::Matrix4& m)noexcept{for(const auto& row:m.m)for(const float v:row)if(!std::isfinite(v))return false;return true;}bool VectorFinite(const engine::math::Vector3& v)noexcept{return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}}
bool RenderView::IsValid()const noexcept{return MatrixFinite(view)&&MatrixFinite(projection)&&MatrixFinite(viewProjection)&&VectorFinite(cameraPosition)&&viewport.IsValid()&&VectorFinite(lightDirection)&&lightDirection.LengthSquared()>0.000001F&&VectorFinite(lightColor)&&lightColor.x>=0&&lightColor.y>=0&&lightColor.z>=0&&std::isfinite(lightIntensity)&&lightIntensity>=0&&VectorFinite(ambientColor)&&ambientColor.x>=0&&ambientColor.y>=0&&ambientColor.z>=0&&std::isfinite(elapsedTime)&&static_cast<unsigned>(debugMode)<static_cast<unsigned>(MaterialDebugMode::Count);}
}
