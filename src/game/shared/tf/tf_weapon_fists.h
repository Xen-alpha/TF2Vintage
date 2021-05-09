//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FISTS_H
#define TF_WEAPON_FISTS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFFists C_TFFists
#endif

//=============================================================================
//
// Fists weapon class.
//
class CTFFists : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFFists, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFists() { m_bHook = false; m_bSwapPunch = false; }
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_FISTS; }

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();

	virtual void SendPlayerAnimEvent( CTFPlayer *pPlayer );

	virtual void DoViewModelAnimation( void );

	void Punch( void );

	virtual float GetMeleeDamage(CBaseEntity *pTarget, int &iCustomDamage);

private:

	CNetworkVar( bool, m_bSwapPunch);
	CNetworkVar(bool, m_bHook);

	CTFFists( const CTFFists & ) {}
};

#endif // TF_WEAPON_FISTS_H
