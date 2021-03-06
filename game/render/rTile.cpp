#include "rTile.h"
#include <gc/MovingObject.h>
#include <gc/NeighborAware.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

static const float dx[8]   = { 32, 32,  0,-32,-32,-32,  0, 32 };
static const float dy[8]   = {  0, 32, 32, 32,  0,-32,-32,-32 };
static const int edgeFrames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };
static const int centerFrames[4] = { 4,  9, 10, 11 };

R_Tile::R_Tile(TextureManager &tm, const char *tex, SpriteColor color, vec2d offset, bool anyLOD)
	: _texId(tm.FindSprite(tex))
	, _color(color)
	, _offset(offset)
	, _anyLOD(anyLOD)
	, _animated(12 == tm.GetFrameCount(_texId))
{
	assert(9 == tm.GetFrameCount(_texId) || _animated);
}

void R_Tile::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GI_NeighborAware*>(&mo));

	vec2d pos = mo.GetPos() + _offset;
	vec2d dir = mo.GetDirection();

	if (rc.GetScale() > 0.25 )
	{
		auto &na = dynamic_cast<const GI_NeighborAware&>(mo);
		int tile = na.GetNeighbors(world);
		for (int i = 0; i < 8; ++i)
		{
			if (0 == (tile & (1 << i)))
			{
				rc.DrawSprite(_texId, edgeFrames[i], _color, pos + vec2d{ dx[i], dy[i] }, dir);
			}
		}
	}
	if (_anyLOD || rc.GetScale() > 0.25)
	{
		int n = _animated ? static_cast<int>(world.GetTime() * 4) : 0;
		rc.DrawSprite(_texId, centerFrames[n % 4], _color, pos, dir);
	}
}
