//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_nail.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "input.h"
#include "c_tf_player.h"
#include "cliententitylist.h"
#endif

#define NAIL_MODEL				"models/weapons/w_models/w_nail.mdl"
#define NAIL_DISPATCH_EFFECT		"ClientProjectile_Syringe"
#define NAIL_GRAVITY	0.2f

LINK_ENTITY_TO_CLASS(tf_projectile_nail, CTFProjectile_Nail);
PRECACHE_REGISTER(tf_projectile_nail);

short g_sModelIndexNail;
void PrecacheNail(void *pUser)
{
	g_sModelIndexNail = modelinfo->GetModelIndex(NAIL_MODEL);
}

PRECACHE_REGISTER_FN(PrecacheNail);


CTFProjectile_Nail::CTFProjectile_Nail()
{
}

CTFProjectile_Nail::~CTFProjectile_Nail()
{
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Nail *CTFProjectile_Nail::Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, bool bCritical)
{
	return static_cast<CTFProjectile_Nail*>(CTFBaseProjectile::Create("tf_projectile_nail", vecOrigin, vecAngles, pOwner, CTFProjectile_Nail::GetInitialVelocity(), g_sModelIndexNail, NAIL_DISPATCH_EFFECT, pScorer, bCritical));
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_Nail::GetProjectileModelName(void)
{
	return NAIL_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_Nail::GetGravity(void)
{
	return NAIL_GRAVITY;
}


//=============================================================================
//
// TF Syringe Projectile functions (Server specific).
//
#define SYRINGE_MODEL				"models/weapons/w_models/w_syringe_proj.mdl"
#define SYRINGE_DISPATCH_EFFECT		"ClientProjectile_Syringe"
#define SYRINGE_GRAVITY	0.3f

LINK_ENTITY_TO_CLASS( tf_projectile_syringe, CTFProjectile_Syringe );
PRECACHE_REGISTER( tf_projectile_syringe );

short g_sModelIndexSyringe;
void PrecacheSyringe(void *pUser)
{
	g_sModelIndexSyringe = modelinfo->GetModelIndex( SYRINGE_MODEL );
}

PRECACHE_REGISTER_FN(PrecacheSyringe);

CTFProjectile_Syringe::CTFProjectile_Syringe()
{
}

CTFProjectile_Syringe::~CTFProjectile_Syringe()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Syringe *CTFProjectile_Syringe::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, bool bCritical )
{
	return static_cast<CTFProjectile_Syringe*>( CTFBaseProjectile::Create( "tf_projectile_syringe", vecOrigin, vecAngles, pOwner, CTFProjectile_Syringe::GetInitialVelocity(), g_sModelIndexSyringe, SYRINGE_DISPATCH_EFFECT, pScorer, bCritical ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_Syringe::GetProjectileModelName( void )
{
	return SYRINGE_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_Syringe::GetGravity( void )
{
	return SYRINGE_GRAVITY;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *GetSyringeTrailParticleName( int iTeamNumber, bool bCritical )
{
	const char *pszFormat = bCritical ? "nailtrails_medic_%s_crit" : "nailtrails_medic_%s";

	return ConstructTeamParticle( pszFormat, iTeamNumber, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileSyringeCallback( const CEffectData &data )
{
	// Get the syringe and add it to the client entity list, so we can attach a particle system to it.
	C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer*>( ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity ) );
	if ( pPlayer )
	{
		C_LocalTempEntity *pSyringe = ClientsideProjectileCallback( data, SYRINGE_GRAVITY );
		if ( pSyringe )
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TF_TEAM_RED:
				pSyringe->m_nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				pSyringe->m_nSkin = 1;
				break;
			}
			bool bCritical = ( ( data.m_nDamageType & DMG_CRITICAL ) != 0 );
			pSyringe->AddParticleEffect(GetSyringeTrailParticleName(pPlayer->GetTeamNumber(), bCritical));
			pSyringe->AddEffects( EF_NOSHADOW );
			pSyringe->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT( SYRINGE_DISPATCH_EFFECT, ClientsideProjectileSyringeCallback );

#endif