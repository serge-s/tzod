#include "rBrickFragment.h"
#include "RenderCfg.h"
#include <gc/MovingObject.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>


R_BrickFragment::R_BrickFragment(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("particle_brick"))
{
}

void R_BrickFragment::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	auto idAsSeed = mo.GetId();
	uint32_t seed = reinterpret_cast<const uint32_t&>(idAsSeed);
	uint32_t rand = ((uint64_t)seed * 279470273UL) % 4294967291UL;

	vec2d pos = mo.GetPos();
	vec2d dir = Vec2dDirection((float) (int(rand%2000) - 1000) + world.GetTime()*(float)(int(rand % 100) - 50) / 5.f);
	unsigned int frame = (rand + 0*static_cast<unsigned int>(world.GetTime() * ANIMATION_FPS)) % _tm.GetFrameCount(_texId);
	rc.DrawSprite(_texId, frame, 0xffffffff, pos, dir);
}
