// Weapons.h

#pragma once

#include "Pickup.h"


///////////////////////////////////////////////////////////////////////////////

struct AIWEAPSETTINGS; // defined in ai.h

class GC_Weapon : public GC_PickUp
{
protected:
	bool _advanced; // �������� ������
	SafePtr<GC_UserSprite> _fireEffect;
	SafePtr<GC_Light> _fireLight;
	vec2d _fePos;
	float _feTime;
	float _feOrient;

	virtual void UpdateView();

public:
	virtual void SetAdvanced(bool advanced) { _advanced = advanced; };
	inline  bool GetAdvanced() { return _advanced; };

public:
	float    _time;
	float    _time_reload;

	float    _angle;          // ���� �������� ������������ ���������
	Rotator  _rotator;

	SafePtr<GC_Vehicle>   _proprietor;
	SafePtr<GC_Crosshair> _crosshair;

	SafePtr<GC_Sound> _rotateSound;

	bool IsAdvanced() const { return _advanced; }

public:
	GC_Weapon(float x, float y);
	GC_Weapon(FromFile);
	virtual ~GC_Weapon();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 6.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;

	virtual void GiveIt(GC_Vehicle *pVehicle);
	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();
	virtual void ProcessRotate(float dt);

	virtual void SetCrosshair();

	virtual void Fire() = 0;

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

private:
	void OnProprietorMove(GC_Vehicle *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_RocketLauncher : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_RocketLauncher);

public:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
	bool _reloaded;

	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_RocketLauncher(float x, float y);
	GC_Weap_RocketLauncher(FromFile);

	virtual GC_PickUp* SetRespawn();
	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	virtual void SetAdvanced(bool advanced);

public:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;

	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_AutoCannon(float x, float y);
	GC_Weap_AutoCannon(FromFile);
	virtual ~GC_Weap_AutoCannon();
	virtual void Kill();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

private:
	float _time_smoke;
	float _time_smoke_dt;

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Cannon(float x, float y);
	GC_Weap_Cannon(FromFile);
	virtual ~GC_Weap_Cannon();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Plazma : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Plazma);

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Plazma(float x, float y);
	GC_Weap_Plazma(FromFile) : GC_Weapon(FromFile()) {}

	virtual GC_PickUp* SetRespawn();
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Gauss(float x, float y);
	GC_Weap_Gauss(FromFile);
	virtual ~GC_Weap_Gauss();

	virtual GC_PickUp* SetRespawn();

	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ram : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ram);

private:
	SafePtr<GC_Sound> _engineSound;
	SafePtr<GC_Light> _engineLight;

protected:
	virtual void UpdateView();

public:
	float _fuel;
	float _fuel_max;
	float _fuel_rate;  // ������ ������� � ���.
	float _fuel_rep;   // �������������� ������� � ���.
	bool _bFire;
	bool _bReady;

public:
	virtual void SetAdvanced(bool advanced);

public:
	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_Ram(float x, float y);
	GC_Weap_Ram(FromFile);
	virtual ~GC_Weap_Ram();
	virtual void Kill();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_BFG : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_BFG);

private:
	float _time_ready;

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_BFG(float x, float y);
	GC_Weap_BFG(FromFile);
	virtual ~GC_Weap_BFG();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Ripper(float x, float y);
	GC_Weap_Ripper(FromFile);
	virtual ~GC_Weap_Ripper();

	virtual GC_PickUp* SetRespawn();
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Minigun : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Minigun);

private:
	SafePtr<GC_Sound> _sound;
	float _time_rotate; // ��� �������� �������� �������
	float _time_fire;   // ����� ����������� ����
	float _time_shot;
	bool _bFire;

	SafePtr<GC_Crosshair> _crosshair_left;

public:
	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_Minigun(float x, float y);
	GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();
	virtual void Kill();

	virtual void SetCrosshair();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file