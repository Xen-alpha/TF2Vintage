//====== Copyright ?1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: A remake of Heavy's Sandvich from live TF2
//
//=============================================================================
#ifndef TF_WEAPON_LUNCHBOX_H
#define TF_WEAPON_LUNCHBOX_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#define CTFLunchBox C_TFLunchBox
#endif

class CTFLunchBox : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFLunchBox, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int 		GetWeaponID() const 						{ return TF_WEAPON_LUNCHBOX; }

	virtual bool		ShouldBlockPrimaryFire( void ) 				{ return true; }

	virtual void		PrimaryAttack( void );
	virtual void		SecondaryAttack( void );

	virtual void		DepleteAmmo( void );


	virtual bool		HasChargeBar( void )						{ return true; }
	virtual float		InternalGetEffectBarRechargeTime( void )	{ return 30.0f; }
	virtual const char	*GetEffectLabelText( void )					{ return "#TF_Sandwich"; }
	virtual void		SwitchBodyGroups( void );

	virtual int TranslateViewmodelHandActivity(int iActivity);
	virtual void SetViewModel();
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual const char *DetermineViewModelType(const char *vModel) const;
	

#ifdef CLIENT_DLL
	virtual C_BaseAnimating *GetAppropriateWorldOrViewModel(void);
#endif

#ifdef GAME_DLL
	virtual void		Precache( void );
	virtual void		ApplyBiteEffects( bool bHurt );
#endif

private:
#ifdef GAME_DLL
	EHANDLE m_hDroppedLunch;
#endif

	bool	m_bBitten;
};

#endif // TF_WEAPON_LUNCHBOX_H
