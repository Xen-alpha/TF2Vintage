//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TF_WEAPON_MILKRIFLE_H
#define TF_WEAPON_MILKRIFLE_H
#ifdef _WIN32
#pragma once
#endif

//#include "tf_weaponbase_gun.h"
#include "tf_weapon_sniperrifle.h"
#include "Sprite.h"

#if defined( CLIENT_DLL )
#define CTFMilkRifle C_TFMilkRifle
#endif


//=============================================================================
//
// Sniper Rifle class.
//
class CTFMilkRifle : public CTFSniperRifle
{
public:

	DECLARE_CLASS(CTFMilkRifle, CTFSniperRifle);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFMilkRifle();
	~CTFMilkRifle();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_MILKRIFLE; }

	virtual void Spawn();
	virtual void Precache();

	virtual void ItemPostFrame(void);
	virtual float GetProjectileDamage(void);

	virtual bool CanFireCriticalShot(bool bIsHeadshot = false) { return false; }
#ifdef CLIENT_DLL
	float GetHUDDamagePerc(void);
#endif

	virtual int TranslateViewmodelHandActivity(int iActivity);
	virtual void SetViewModel();
	virtual const char *GetViewModel(int iViewModel = 0) const;
	virtual const char *DetermineViewModelType(const char *vModel) const;


#ifdef CLIENT_DLL
	virtual C_BaseAnimating *GetAppropriateWorldOrViewModel(void);
#endif
};

#endif // TF_WEAPON_SNIPERRIFLE_H
