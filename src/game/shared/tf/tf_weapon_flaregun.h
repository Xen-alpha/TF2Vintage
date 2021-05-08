//====== Copyright ?1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: A remake of Pyro's flaregun from live TF2s
//
//=============================================================================
#ifndef TF_WEAPON_FLAREGUN_H
#define TF_WEAPON_FLAREGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_grenadeproj.h"

#ifdef CLIENT_DLL
#define CTFFlareGun C_TFFlareGun
#endif

class CTFFlareGun : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS(CTFFlareGun, CTFWeaponBaseGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFFlareGun();

	virtual void	Spawn( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }

	virtual int TranslateViewmodelHandActivity(int iActivity);
	virtual void SetViewModel();
	virtual const char *GetViewModel(int iViewModel = 0) const;
	virtual const char *DetermineViewModelType(const char *vModel) const;


#ifdef CLIENT_DLL
	virtual C_BaseAnimating *GetAppropriateWorldOrViewModel(void);
#endif
};

#endif // TF_WEAPON_FLAREGUN_H