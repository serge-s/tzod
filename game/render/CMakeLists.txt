add_library(render
	inc/render/ObjectView.h
	inc/render/ObjectViewsSelector.h
	inc/render/RenderScheme.h
	inc/render/Terrain.h
	inc/render/WorldView.h

	rAnimatedSprite.h
	rBrickFragment.h
	rBooster.h
	rDecoration.h
	rFireSpark.h
	rIndicator.h
	rLight.h
	rMinigun.h
	rParticle.h
	rPredicate.h
	rShock.h
	rSprite.h
	rText.h
	rTile.h
	rTurret.h
	rUserObject.h
	rVehicle.h
	rWall.h
	rWeapon.h
	rWeaponBase.h

	WorldView.cpp
	ObjectViewsSelector.cpp
	RenderScheme.cpp
	rAnimatedSprite.cpp
	rBrickFragment.cpp
	rBooster.cpp
	rDecoration.cpp
	rFireSpark.cpp
	rIndicator.cpp
	rLight.cpp
	rMinigun.cpp
	rParticle.cpp
	rShock.cpp
	rSprite.cpp
	rText.cpp
	rTile.cpp
	rTurret.cpp
	rUserObject.cpp
	rVehicle.cpp
	rWall.cpp
	rWeapon.cpp
	rWeaponBase.cpp
	Terrain.cpp
)

target_link_libraries(render
	PRIVATE ai ctx video
	PUBLIC gc
)

target_include_directories(render INTERFACE inc)
set_target_properties(render PROPERTIES FOLDER game)