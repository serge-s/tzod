// Camera.cpp

#include "Camera.h"

#include "GlobalListHelper.inl"
#include "Level.h"
#include "Macros.h"
#include "Player.h"
#include "SaveFile.h"
#include "Vehicle.h"
#include "Weapons.h"

#include "config/Config.h"
#include "video/RenderBase.h" // FIXME

// ui
#include <ConsoleBuffer.h>
UI::ConsoleBuffer& GetConsole();


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Camera)
{
	return true;
}

GC_Camera::GC_Camera(GC_Player *player)
  : GC_Actor()
  , _memberOf(this)
  , _rotator(_rotatorAngle)
  , _player(player)
{
	assert(_player);

	_rotator.reset(0.0f, 0.0f,
		g_conf.g_rotcamera_m.GetFloat(),
		g_conf.g_rotcamera_a.GetFloat(),
		std::max(0.001f, g_conf.g_rotcamera_s.GetFloat()));

	MoveTo( vec2d(g_level->_sx / 2, g_level->_sy / 2) );
	if( _player->GetVehicle() )
	{
		_rotatorAngle =  -_player->GetVehicle()->GetDirection().Angle() + PI/2;
		MoveTo( _player->GetVehicle()->GetPos() );
	}
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	_player->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Camera::OnDetach);
	_player->Subscribe(NOTIFY_PLAYER_SETCONTROLLER, this, (NOTIFYPROC) &GC_Camera::OnDetach);

	_target     = GetPos();
	_time_shake = 0;
	_time_seed  = frand(1000);
	_zoom       = 1.0f;

	UpdateLayout();
}

GC_Camera::GC_Camera(FromFile)
  : GC_Actor(FromFile())
  , _memberOf(this)
  , _rotator(_rotatorAngle)
{
}

GC_Camera::~GC_Camera()
{
	UpdateLayout();
}

void GC_Camera::TimeStepFloat(float dt)
{
	float mu = 3;

	_rotator.process_dt(dt);
	if( _player->GetVehicle() )
	{
		_rotator.rotate_to(-_player->GetVehicle()->GetDirection().Angle() - PI/2);

		mu += _player->GetVehicle()->_lv.len() / 100;

		int dx = (int) std::max(.0f, ((float) WIDTH(_viewport) / _zoom  - g_level->_sx) / 2);
		int dy = (int) std::max(.0f, ((float) HEIGHT(_viewport) / _zoom - g_level->_sy) / 2);

		vec2d r = _player->GetVehicle()->GetPos() + _player->GetVehicle()->_lv / mu;

		if( _player->GetVehicle()->GetWeapon() )
		{
			r += _player->GetVehicle()->GetWeapon()->GetDirection() * 130.0f;
		}
		else
		{
			r += _player->GetVehicle()->GetDirection() * 130.0f;
		}

		_target.x = r.x + (float) dx;
		_target.y = r.y + (float) dy;

		_target.x = std::max(_target.x, (float) WIDTH(_viewport) / _zoom * 0.5f + dx);
		_target.x = std::min(_target.x, g_level->_sx - (float) WIDTH(_viewport) / _zoom * 0.5f + dx);
		_target.y = std::max(_target.y, (float) HEIGHT(_viewport) / _zoom * 0.5f + dy);
		_target.y = std::min(_target.y, g_level->_sy - (float) HEIGHT(_viewport) / _zoom * 0.5f + dy);
	}

	if( _time_shake > 0 )
	{
		_time_shake -= dt;
		if( _time_shake < 0 ) _time_shake = 0;
	}

	MoveTo(_target + (GetPos() - _target) * expf(-dt * mu));
}

void GC_Camera::GetWorld(FRECT &outWorld) const
{
	vec2d shake(0, 0);
	if( _time_shake > 0 )
	{
		shake.Set(cos((_time_shake + _time_seed)*70.71068f), sin((_time_shake + _time_seed)*86.60254f));
		shake *= _time_shake * CELL_SIZE * 0.1f;
	}

	outWorld.left   = floor((GetPos().x + shake.x - (float)  WIDTH(_viewport) / _zoom * 0.5f) * _zoom) / _zoom;
	outWorld.top    = floor((GetPos().y + shake.y - (float) HEIGHT(_viewport) / _zoom * 0.5f) * _zoom) / _zoom;
	outWorld.right  = outWorld.left + (float)  WIDTH(_viewport) / _zoom;
	outWorld.bottom = outWorld.top + (float) HEIGHT(_viewport) / _zoom;
}

void GC_Camera::GetScreen(Rect &vp) const
{
	vp = _viewport;
}

void GC_Camera::UpdateLayout()
{
	size_t camCount = 0;

	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
	{
		++camCount;
	}

	Rect viewports[MAX_HUMANS];

	if( g_render->GetWidth() >= int(g_level->_sx) && g_render->GetHeight() >= int(g_level->_sy) )
	{
		viewports[0] = CRect(
			(g_render->GetWidth() - int(g_level->_sx)) / 2,
			(g_render->GetHeight() - int(g_level->_sy)) / 2,
			(g_render->GetWidth() + int(g_level->_sx)) / 2,
			(g_render->GetHeight() + int(g_level->_sy)) / 2
		);
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera)
		{
			pCamera->_viewport = viewports[0];
		}
	}
	else if( camCount )
	{
		int w = g_render->GetWidth();
		int h = g_render->GetHeight();

		switch( camCount )
		{
		case 1:
			viewports[0] = CRect(0, 0, w, h);
			break;
		case 2:
			viewports[0] = CRect(0, 0, w/2 - 1, h);
			viewports[1] = CRect(w/2 + 1, 0, w, h);
			break;
		case 3:
			viewports[0] = CRect(0, 0, w/2 - 1, h/2 - 1);
			viewports[1] = CRect(w/2 + 1, 0, w, h/2 - 1);
			viewports[2] = CRect(w/4, h/2 + 1, w*3/4, h);
			break;
		case 4:
			viewports[0] = CRect(0, 0, w/2 - 1, h/2 - 1);
			viewports[1] = CRect(w/2 + 1, 0, w, h/2 - 1);
			viewports[2] = CRect(0, h/2 + 1, w/2 - 1, h);
			viewports[3] = CRect(w/2 + 1, h/2 + 1, w, h);
			break;
		default:
			assert(false);
		}

		size_t count = 0;
		float  zoom  = camCount > 2 ? 0.5f : 1.0f;
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
		{
			pCamera->_viewport = viewports[count++];
			pCamera->_zoom     = zoom;
		}
	}
}

bool GC_Camera::GetWorldMousePos(const vec2d &screenPos, vec2d &outWorldPos, bool editorMode)
{
	Point ptinscr = { (int) screenPos.x, (int) screenPos.y };

	if( editorMode || g_level->GetList(LIST_cameras).empty() )
	{
		// use default camera
		outWorldPos.x = float(ptinscr.x) / g_level->_defaultCamera.GetZoom() + g_level->_defaultCamera.GetPosX();
		outWorldPos.y = float(ptinscr.y) / g_level->_defaultCamera.GetZoom() + g_level->_defaultCamera.GetPosY();
		return true;
	}
	else
	{
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
		{
			if( PtInRect(pCamera->_viewport, ptinscr) )
			{
				FRECT w;
				pCamera->GetWorld(w);
				outWorldPos.x = w.left + (float) (ptinscr.x - pCamera->_viewport.left) / pCamera->_zoom;
				outWorldPos.y = w.top + (float) (ptinscr.y - pCamera->_viewport.top) / pCamera->_zoom;
				return true;
			}
		}
	}
	return false;
}

void GC_Camera::Shake(float level)
{
	assert(_player);
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = std::min(_time_shake + 0.5f * level, PLAYER_RESPAWN_DELAY / 2);
}

void GC_Camera::Serialize(SaveFile &f)
{
	GC_Actor::Serialize(f);

	f.Serialize(_rotatorAngle);
	f.Serialize(_target);
	f.Serialize(_time_seed);
	f.Serialize(_time_shake);
	f.Serialize(_zoom);
	f.Serialize(_player);

	_rotator.Serialize(f);
	if( f.loading() ) UpdateLayout();
}

void GC_Camera::OnDetach(GC_Object *sender, void *param)
{
	Kill();
}

// end of file
