//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_nailgun.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Syringe Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFNailGun, DT_WeaponNailGun )

BEGIN_NETWORK_TABLE( CTFNailGun, DT_WeaponNailGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFNailGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_nailgun, CTFNailGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_nailgun );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFNailGun )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon NailGun functions.
//
void CTFNailGun::Precache()
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheTeamParticles("nailtrails_medic_%s");
	PrecacheTeamParticles("nailtrails_medic_%s_crit");
#endif
}
