//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_NAIL_H
#define TF_PROJECTILE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_projectile_base.h"

//-----------------------------------------------------------------------------
// Purpose: The base Nail projectile
//-----------------------------------------------------------------------------
class CTFProjectile_Nail : public CTFBaseProjectile
{
	DECLARE_CLASS(CTFProjectile_Nail, CTFBaseProjectile);

public:

	CTFProjectile_Nail();
	~CTFProjectile_Nail();

	// Creation.
	static CTFProjectile_Nail *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, bool bCritical = false);

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return 1500.0; }
};

//-----------------------------------------------------------------------------
// Purpose: Identical to a nail except for model used
//-----------------------------------------------------------------------------
class CTFProjectile_Syringe : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Syringe, CTFBaseProjectile );

public:

	CTFProjectile_Syringe();
	~CTFProjectile_Syringe();

	// Creation.
	static CTFProjectile_Syringe *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, bool bCritical = false );	

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void );

	static float	GetInitialVelocity( void ) { return 1500.0; }
};
#endif	//TF_PROJECTILE_NAIL_H