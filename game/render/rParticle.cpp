#include "rParticle.h"
#include <gc/Decal.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <algorithm>

static std::pair<DecalType, const char*> textures[] = {
	{ PARTICLE_FIRE1, "particle_fire" },
	{ PARTICLE_FIRE2, "particle_fire2" },
	{ PARTICLE_FIRE3, "particle_fire3" },
	{ PARTICLE_FIRE4, "particle_fire4" },
	{ PARTICLE_FIRESPARK, "projectile_fire" },
	{ PARTICLE_TYPE1, "particle_1" },
	{ PARTICLE_TYPE2, "particle_2" },
	{ PARTICLE_TYPE3, "particle_3" },
	{ PARTICLE_TRACE1, "particle_trace" },
	{ PARTICLE_TRACE2, "particle_trace2" },
	{ PARTICLE_SMOKE, "particle_smoke" },
	{ PARTICLE_EXPLOSION1, "explosion_o" },
	{ PARTICLE_EXPLOSION2, "explosion_big" },
	{ PARTICLE_EXPLOSION_G, "explosion_g" },
	{ PARTICLE_EXPLOSION_E, "explosion_e" },
	{ PARTICLE_EXPLOSION_S, "explosion_s" },
	{ PARTICLE_EXPLOSION_P, "explosion_plazma" },
	{ PARTICLE_BIGBLAST, "bigblast" },
	{ PARTICLE_SMALLBLAST, "smallblast" },
	{ PARTICLE_GAUSS1, "particle_gauss1" },
	{ PARTICLE_GAUSS2, "particle_gauss2" },
	{ PARTICLE_GAUSS_HIT, "particle_gausshit" },
	{ PARTICLE_GREEN, "particle_green" },
	{ PARTICLE_YELLOW, "particle_yellow" },
	{ PARTICLE_CATTRACK, "cat_track" },
};

R_Particle::R_Particle(TextureManager &tm)
	: _tm(tm)
{
	int maxId = 0;
	for (auto p: textures)
		maxId = std::max(maxId, (int) p.first);
	_ptype2texId.resize(maxId + 1);
	for (auto p: textures)
		_ptype2texId[p.first] = tm.FindSprite(p.second);
}

void R_Particle::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Decal*>(&mo));
	const GC_Decal &decal = static_cast<const GC_Decal&>(mo);
	DecalType ptype = decal.GetDecalType();
	float ptime = world.GetTime() - decal.GetTimeCreated();
	if (ptype < (int) _ptype2texId.size() && ptime < decal.GetLifeTime())
	{
		size_t texId = _ptype2texId[ptype];
		float state = ptime / decal.GetLifeTime();
		auto frame = std::min(_tm.GetFrameCount(texId) - 1, (int) ((float) _tm.GetFrameCount(texId) * state));
		vec2d pos = decal.GetPos();
		vec2d dir = Vec2dAddDirection(decal.GetDirection(), Vec2dDirection(decal.GetRotationSpeed() * ptime));
		SpriteColor color;
		if (decal.GetFade())
		{
			unsigned char op = (unsigned char) int(255.0f * (1.0f - state));
			color.r = op;
			color.g = op;
			color.b = op;
			color.a = op;
		}
		else
		{
			color = 0xffffffff;
		}
		float size = decal.GetSizeOverride();
		if( size < 0 )
			rc.DrawSprite(texId, frame, color, pos, dir);
		else
			rc.DrawSprite(texId, frame, color, pos, size, size, dir);
	}
}
