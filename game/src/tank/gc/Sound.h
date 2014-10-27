// Sound.h

#pragma once

#include "Actor.h"

#include <SoundTemplates.h>

/////////////////////////////////////////////////////////////

enum enumSoundMode
{
    SMODE_UNKNOWN = 0,
    SMODE_PLAY,  // the GC_Soung object will be destroyed at the end
    SMODE_LOOP,
    SMODE_STOP,  // pause with the resource releasing
    SMODE_WAIT,  // forced pause
};

class GC_Sound : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Sound);
    DECLARE_LIST_MEMBER();
    typedef GC_Actor base;

	enumSoundTemplate   _soundTemplate;
#ifndef NOSOUND
    unsigned int _source;
#endif
    
protected:
	bool          _freezed;
	enumSoundMode _mode;
    float _speed;

public:
	float _volume;  // 0 - min;  1 - max

public:
	GC_Sound(vec2d pos, enumSoundTemplate sound);
	explicit GC_Sound(FromFile);
	virtual ~GC_Sound();
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);

	void KillWhenFinished(World &world);
	virtual void MoveTo(World &world, const vec2d &pos) override;

	void SetMode(World &world, enumSoundMode mode);
	void Pause(World &world, bool pause);
	void Freeze(bool freeze);

	void SetSpeed(float speed);
	void SetVolume(float vol);
	void UpdateVolume();  // should be called each time the conf.s_volume changes

public:
	static int _countMax;
	static int _countActive;
	static int _countWaiting;
};

/////////////////////////////////////////////////////////////
// got destroyed together with the target object
class GC_Sound_link : public GC_Sound
{
	DECLARE_SELF_REGISTRATION(GC_Sound_link);
    DECLARE_LIST_MEMBER();
    typedef GC_Sound base;

protected:
	ObjPtr<GC_Actor> _object;

public:
	GC_Sound_link(enumSoundTemplate sound, GC_Actor *object);
	GC_Sound_link(FromFile);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

public:
	bool CheckObject(const GC_Object *object) const
	{
		return _object == object;
	}
};

// end of file
