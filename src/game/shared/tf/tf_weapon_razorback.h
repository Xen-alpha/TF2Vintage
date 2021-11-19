//=============================================================================
//
// Purpose: Stub class for compatibility with item schema
//
//=============================================================================
#ifndef TF_WEAPON_RAZORBACK_H
#define TF_WEAPON_RAZORBACK_H

#ifdef _WIN32
#pragma once
#endif

//#include "econ_wearable.h"
#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#define CTFRazorback C_TFRazorback
#endif

class CTFRazorback: public CTFWeaponBase
{
public:
	DECLARE_CLASS(CTFRazorback, CTFWeaponBase);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	virtual int 		GetWeaponID() const 						{ return TF_WEAPON_RAZORBACK; }
	virtual bool		IsWearable() { return true; }
	
#ifdef GAME_DLL
	virtual void	Precache();
	virtual void	Equip(CBasePlayer *pPlayer);
	void			UpdateModelToClass(void);
#endif

	virtual void SetWeaponVisible(bool visible);

	virtual bool	CanDeploy() { return false; }
	virtual bool	CanAttack() { return false; }
	virtual bool	CanHolster() { return false; }
};

#endif // TF_WEARABLE_H
