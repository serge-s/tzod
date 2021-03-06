#pragma once

class AppConfig;
class World;
struct Gameplay;

struct GameContextBase
{
	virtual ~GameContextBase() {}
	virtual World& GetWorld() = 0;
	virtual Gameplay* GetGameplay() const = 0;
	virtual void Step(float dt, AppConfig &appConfig, bool *outConfigChanged) = 0;
	virtual bool IsWorldActive() const = 0;
};
