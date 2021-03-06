#pragma once
#include "VehicleStateReader.h"
#include <gc/WorldEvents.h>
#include <vector>

class ShellConfig;

namespace Plat
{
	class ConsoleBuffer;
}

class InputManager
{
public:
	InputManager(ShellConfig &conf, Plat::ConsoleBuffer &logger);
	~InputManager();
	VehicleStateReader* GetVehicleStateReader(unsigned int playerIndex);
	const VehicleStateReader* GetVehicleStateReader(unsigned int playerIndex) const;

	void Step(float dt);

private:
	ShellConfig &_conf;
	Plat::ConsoleBuffer &_logger;
	std::vector<VehicleStateReader> _controllers;
	void OnProfilesChange();

	InputManager(const InputManager&) = delete;
	void operator=(const InputManager&) = delete;
};

