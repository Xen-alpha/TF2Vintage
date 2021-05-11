//=========== Copyright © 2018, LFE-Team, Not All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_buff_item.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "effect_dispatch_data.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#include "tf_wearable.h"
#include "tf_projectile_arrow.h"
// Server specific.
#else
#include "tf_player.h"
#include "ai_basenpc.h"
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "tf_gamestats.h"
#endif

const char *g_BannerModels[] =
{
	"models/weapons/c_models/c_buffbanner/c_buffbanner.mdl", 
	"models/workshop/weapons/c_models/c_battalion_buffbanner/c_battalion_buffbanner.mdl",
	"models/workshop_partner/weapons/c_models/c_shogun_warbanner/c_shogun_warbanner.mdl",
	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_parachute.mdl",
};

//=============================================================================
//
// Weapon BUFF item tables.
//


IMPLEMENT_NETWORKCLASS_ALIASED( TFBuffItem, DT_WeaponBuffItem )

BEGIN_NETWORK_TABLE( CTFBuffItem, DT_WeaponBuffItem )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bBuffUsed ) ),
#else
	SendPropBool( SENDINFO( m_bBuffUsed ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBuffItem )
#ifdef CLIENT_DLL
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_buff_item, CTFBuffItem );
PRECACHE_WEAPON_REGISTER( tf_weapon_buff_item );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFBuffItem )
END_DATADESC()
#endif

CTFBuffItem::CTFBuffItem()
{
	m_flEffectBarRegenTime = 0.0f;
	UseClientSideAnimation();

#ifdef CLIENT_DLL
	ListenForGameEvent( "deploy_buff_banner" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::Precache( void )
{
	PrecacheTeamParticles( "soldierbuff_%s_buffed" );
	PrecacheParticleSystem( "soldierbuff_mvm" );

	for ( int i = 0; i < ARRAYSIZE( g_BannerModels ); i++ )
	{
		PrecacheModel( g_BannerModels[i] );
	}
	PrecacheModel( "models/weapons/c_models/c_buffpack/c_buffpack.mdl" );
	PrecacheModel( "models/workshop/weapons/c_models/c_battalion_buffpack/c_battalion_buffpack.mdl" );
	PrecacheModel( "models/workshop_partner/weapons/c_models/c_shogun_warpack/c_shogun_warpack.mdl" );
	PrecacheModel( "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack_open.mdl" );
	PrecacheModel( "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack.mdl" );

	PrecacheScriptSound( "Weapon_BuffBanner.HornRed" );
	PrecacheScriptSound( "Weapon_BuffBanner.HornBlue" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornRed" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornBlue" );
	PrecacheScriptSound( "Samurai.Conch" );
	PrecacheScriptSound( "Weapon_BuffBanner.Flag" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Only holster when the buff isn't used
//-----------------------------------------------------------------------------
bool CTFBuffItem::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( !m_bBuffUsed )
	{
		return BaseClass::Holster( pSwitchingTo );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge meter 
//-----------------------------------------------------------------------------
void CTFBuffItem::WeaponReset( void )
{
	m_bBuffUsed = false;

	BaseClass::WeaponReset();
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::PrimaryAttack( void )
{
	// Rage not ready
	//if ( !IsFull() )
		//return;

	if ( !CanAttack() )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( !pOwner )
		return;

	if ( !m_bBuffUsed && pOwner->m_Shared.GetRageProgress() >= 100.0f )
	{
		if ( GetTeamNumber() == TF_TEAM_RED )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		}
		else
		{
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
		}

		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );

#ifdef GAME_DLL
		SetContextThink( &CTFBuffItem::BlowHorn, gpGlobals->curtime + 0.22f, "BlowHorn" );
#endif
	}
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::BlowHorn( void )
{
	if ( !CanAttack() )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( !pOwner || !pOwner->IsAlive() )
		return;

	int iBuffType = GetBuffType();
	const char *iszSound = "\0";
	switch( iBuffType )
	{
		case TF_BUFF_OFFENSE:
			if ( pOwner->GetTeamNumber() == TF_TEAM_RED )
				iszSound = "Weapon_BuffBanner.HornRed";
			else
				iszSound = "Weapon_BuffBanner.HornBlue";
			break;
		case TF_BUFF_DEFENSE:
			if ( pOwner->GetTeamNumber() == TF_TEAM_RED )
				iszSound = "Weapon_BattalionsBackup.HornRed";
			else
				iszSound = "Weapon_BattalionsBackup.HornBlue";
			break;
		case TF_BUFF_REGENONDAMAGE:
			iszSound = "Samurai.Conch";
			break;
	};
	pOwner->EmitSound( iszSound );
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::RaiseFlag( void )
{
	if ( !CanAttack() )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( !pOwner || !pOwner->IsAlive() )
		return;

	int iBuffType = GetBuffType();

#ifdef GAME_DLL
	m_bBuffUsed = false;
	IGameEvent *event = gameeventmanager->CreateEvent( "deploy_buff_banner" );
	if ( event )
	{
		event->SetInt( "buff_type", iBuffType );
		event->SetInt( "buff_owner", pOwner->entindex() );
 		gameeventmanager->FireEvent( event );
	}

	pOwner->EmitSound( "Weapon_BuffBanner.Flag" );
	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
#endif

	pOwner->m_Shared.ActivateRageBuff( this, iBuffType );
	pOwner->SwitchToNextBestWeapon( this );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CTFBuffItem::GetBuffType( void )
{
	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );
	return iBuffType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFBuffItem::GetEffectBarProgress( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner)
	{
		return pOwner->m_Shared.GetRageProgress() / 100.0f;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFBuffItem::EffectMeterShouldFlash( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	// Rage meter should flash while draining
	if ( pOwner && pOwner->m_Shared.IsRageActive() && pOwner->m_Shared.GetRageProgress() < 100.0f )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Make sure the weapon uses the correct animations 
//-----------------------------------------------------------------------------
bool CTFBuffItem::SendWeaponAnim( int iActivity )
{	
	int iNewActivity = iActivity;

	switch ( iActivity )
	{
	case ACT_VM_DRAW:
		iNewActivity =ACT_ITEM1_VM_DRAW;
		break;
	case ACT_VM_HOLSTER:
		iNewActivity = ACT_ITEM1_VM_HOLSTER;
		break;
	case ACT_VM_IDLE:
		iNewActivity = ACT_ITEM1_VM_IDLE;

		// Do the flag raise animation 
		if ( m_bBuffUsed )
		{
			RaiseFlag();
			iNewActivity = ACT_RESET;
		}
		else

		break;
	case ACT_VM_PULLBACK:
		iNewActivity = ACT_ITEM1_VM_PULLBACK;
		break;
	case ACT_VM_PRIMARYATTACK:
		iNewActivity = ACT_ITEM1_VM_PRIMARYATTACK;
		m_bBuffUsed = true;
		break;
	case ACT_VM_SECONDARYATTACK:
		iNewActivity = ACT_ITEM1_VM_SECONDARYATTACK;
		m_bBuffUsed = true;
		break;
	case ACT_VM_IDLE_TO_LOWERED:
		iNewActivity = ACT_ITEM1_VM_IDLE_TO_LOWERED;
		break;
	case ACT_VM_IDLE_LOWERED:
		iNewActivity = ACT_ITEM1_VM_IDLE_LOWERED;
		break;
	case ACT_VM_LOWERED_TO_IDLE:
		iNewActivity = ACT_ITEM1_VM_LOWERED_TO_IDLE;
		break;
	};
	
	return BaseClass::SendWeaponAnim( iNewActivity );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Create the banner addon for the backpack
//-----------------------------------------------------------------------------
void C_TFBuffItem::CreateBanner( int iBuffType )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();

	if ( !pOwner || !pOwner->IsAlive() )
		return;

	C_EconWearable *pWearable = pOwner->GetWearableForLoadoutSlot( TF_LOADOUT_SLOT_SECONDARY );

	// if we don't have a backpack for some reason don't make a banner
	if ( !pWearable )
		return;

	// yes I really am using the arrow class as a base
	// create the flag
	C_TFProjectile_Arrow *pBanner = new C_TFProjectile_Arrow();
	if ( pBanner )
	{
		pBanner->InitializeAsClientEntity( g_BannerModels[iBuffType - 1], RENDER_GROUP_OPAQUE_ENTITY );

		// Attach the flag to the backpack
		int bone = pWearable->LookupBone( "bip_spine_3" );
		if ( bone != -1 )
		{
			pBanner->SetDieTime( gpGlobals->curtime + 10.0f );
			pBanner->AttachEntityToBone( pWearable, bone );
		}
	}
}

/*//-----------------------------------------------------------------------------
// Purpose: Destroy the banner addon for the backpack
//-----------------------------------------------------------------------------
void C_TFBuffItem::DestroyBanner( void )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();

	if ( !pOwner )
		return;

	C_EconWearable *pWearable = pOwner->GetWearableForLoadoutSlot( TF_LOADOUT_SLOT_SECONDARY );

	if ( !pWearable )
		return;

	pWearable->DestroyBoneAttachments();
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFBuffItem::FireGameEvent(IGameEvent *event)
{
	if ( V_strcmp( event->GetName(), "deploy_buff_banner" ) == 0 )
	{
		int entindex = event->GetInt( "buff_owner" );

		// Make sure the person who deployed owns this weapon
		if ( UTIL_PlayerByIndex( entindex ) == GetPlayerOwner() )
		{
			// Create the banner addon
			CreateBanner( event->GetInt( "buff_type" ) );
		}
	}
}
#endif