#pragma once

#include "GameContext.h"
#include <math/MyMath.h>
#include <memory>

class World;
namespace FS
{
	struct Stream;
}

class EditorContext : public GameContextBase
{
public:
	EditorContext(int width, int height, FS::Stream *stream = nullptr);
	virtual ~EditorContext();

	FRECT GetOriginalBounds() const { return _originalBounds; }

	// GameContextBase
	World& GetWorld() override { return *_world; }
	Gameplay* GetGameplay() const override { return nullptr; }
	void Step(float dt, AppConfig &appConfig, bool *outConfigChanged) override;
	bool IsWorldActive() const override { return false; }

private:
	std::unique_ptr<World> _world;
	FRECT _originalBounds = {};
};