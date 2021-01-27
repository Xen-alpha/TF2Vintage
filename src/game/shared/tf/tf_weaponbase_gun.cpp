//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_gun.h"
#include "tf_fx_shared.h"
#include "effect_dispatch_data.h"
#include "takedamageinfo.h"
#include "tf_projectile_nail.h"
#include "tf_projectile_arrow.h"
#include "tf_projectile_jar.h"
#include "tf_shareddefs.h"

#if !defined( CLIENT_DLL )	// Server specific.

	#include "tf_gamestats.h"
	#include "tf_player.h"
	#include "tf_fx.h"
	#include "te_effect_dispatch.h"

	#include "tf_projectile_rocket.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "tf_projectile_flare.h"
	#include "tf_projectile_dragons_fury.h"
	#include "tf_weapon_mechanical_arm.h"
	#include "tf_projectile_energyball.h"
	#include "tf_projectile_energy_ring.h"
	#include "tf_weapon_flaregun.h"
	#include "te.h"

	#include "tf_gamerules.h"
	#include "soundent.h"

#else	// Client specific.

	#include "c_tf_player.h"
	#include "c_te_effect_dispatch.h"

#endif

//=============================================================================
//
// TFWeaponBase Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFWeaponBaseGun, DT_TFWeaponBaseGun)

BEGIN_NETWORK_TABLE(CTFWeaponBaseGun, DT_TFWeaponBaseGun)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFWeaponBaseGun)
END_PREDICTION_DATA()

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponBaseGun )
DEFINE_THINKFUNC( ZoomOutIn ),
DEFINE_THINKFUNC( ZoomOut ),
DEFINE_THINKFUNC( ZoomIn ),
END_DATADESC()
#endif

//=============================================================================
//
// TFWeaponBase Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGun::CTFWeaponBaseGun()
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PrimaryAttack( void )
{
	if ( AutoFiresFullClip() && !m_bFiringWholeClip )
	{
		UpdateAutoFire();
		return;
	}

	// Check for ammunition.
	if ( ( m_iClip1 < GetAmmoPerShot() ) && UsesClipsForAmmo1() )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;
	
	// Check for ammunition for single shot/attributes.
	if ( !UsesClipsForAmmo1() && ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) )
	{
		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) < GetAmmoPerShot() )
			return;
	}
	else if ( !UsesClipsForAmmo2() && ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE ) )
	{
		if ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) < GetAmmoPerShot() )
			return;
	}
	
	if ( !CanAttack() )
		return;

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	pPlayer->RemoveInvisibility();

	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN )
	{
		pPlayer->SpeakWeaponFire();
	}
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireProjectile( pPlayer );

	// Set next attack times.
	float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flFireDelay, hwn_mult_postfiredelay );
	
	if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, rocketjump_attackrate_bonus );
		
	float flHealthModFireBonus = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flHealthModFireBonus, mult_postfiredelay_with_reduced_health );
	if (flHealthModFireBonus != 1.0f)
	{
		flFireDelay *= RemapValClamped( pPlayer->GetHealth() / pPlayer->GetMaxHealth(), 0.2, 0.9, flHealthModFireBonus, 1.0 );
	}

	if ( !UsesClipsForAmmo1() )
	{
		const float flBaseFireDelay = flFireDelay;
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, fast_reload );

		const float flPlaybackRate = flBaseFireDelay / (flFireDelay + FLT_EPSILON);
		if ( pPlayer->GetViewModel( 0 ) )
		{
			pPlayer->GetViewModel( 0 )->SetPlaybackRate( flPlaybackRate );
		}
		if ( pPlayer->GetViewModel( 1 ) )
		{
			pPlayer->GetViewModel( 1 )->SetPlaybackRate( flPlaybackRate );
		}
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	AbortReload();

#ifndef CLIENT_DLL
	int nKeepDisguise = 0;
	CALL_ATTRIB_HOOK_INT( nKeepDisguise, keep_disguise_on_attack );
	if( nKeepDisguise == 0 )
	{
		pPlayer->RemoveDisguise();
	}
#endif
	m_flLastFireTime = gpGlobals->curtime;
}	

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::SecondaryAttack( void )
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

CBaseEntity *CTFWeaponBaseGun::FireProjectile( CTFPlayer *pPlayer )
{
	int iProjectile = TF_PROJECTILE_NONE;

	CALL_ATTRIB_HOOK_INT( iProjectile, override_projectile_type );

	if ( iProjectile == TF_PROJECTILE_NONE )
		iProjectile = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iProjectile;

	CBaseEntity *pProjectile = NULL;

	switch( iProjectile )
	{
		case TF_PROJECTILE_BULLET:
			FireBullet( pPlayer );
			break;

		case TF_PROJECTILE_ROCKET:
			pProjectile = FireRocket( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;
			
		case TF_PROJECTILE_ENERGY_BALL:
			pProjectile = FireEnergyBall( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;
			
		case TF_PROJECTILE_ENERGY_RING:
			pProjectile = FireEnergyRing( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;
			
		case TF_PROJECTILE_BALLOFFIRE:
			pProjectile = FireFireBall( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;	
			
		case TF_PROJECTILE_ENERGYORB:
			pProjectile = FireEnergyOrb( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;	
		
		case TF_PROJECTILE_SYRINGE:
		case TF_PROJECTILE_NAIL:
		case TF_PROJECTILE_DART:
			pProjectile = FireNail( pPlayer, iProjectile );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_PIPEBOMB:
			pProjectile = FirePipeBomb( pPlayer, 0 );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_PIPEBOMB_REMOTE:
		case TF_PROJECTILE_PIPEBOMB_REMOTE_PRACTICE:
			pProjectile = FirePipeBomb( pPlayer, 1 );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_WEAPON_GRENADE_PIPEBOMB_PROJECTILE:
			pProjectile = FirePipeBomb( pPlayer, 3 );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;
			
		case TF_PROJECTILE_CANNONBALL:
			pProjectile = FirePipeBomb( pPlayer, 4 );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_FLARE:
			pProjectile = FireFlare( pPlayer );
			pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
			break;

		case TF_PROJECTILE_JAR:
		case TF_PROJECTILE_JAR_MILK:
		case TF_PROJECTILE_FESTIVE_URINE:
		case TF_PROJECTILE_BREADMONSTER_JARATE:
		case TF_PROJECTILE_BREADMONSTER_MADMILK:
		case TF_PROJECTILE_CLEAVER:
		case TF_PROJECTILE_JAR_GAS:
			pProjectile = FireJar( pPlayer, iProjectile );
			pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
			break;

		case TF_PROJECTILE_ARROW:
		case TF_PROJECTILE_HEALING_BOLT:
		case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
		case TF_PROJECTILE_FESTIVE_ARROW:
		case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
		case TF_PROJECTILE_GRAPPLINGHOOK:
			pProjectile = FireArrow( pPlayer, iProjectile );
			pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
			break;

		case TF_PROJECTILE_THROWABLE:
		case TF_PROJECTILE_NONE:
		default:
			// do nothing!
			DevMsg( "Weapon does not have a projectile type set\n" );
			break;
	}

#if defined( GAME_DLL )
	if ( pProjectile )
	{
		if ( GetProjectileModelOverride() )
		{
			pProjectile->SetModel( GetProjectileModelOverride() );
		}
	}
#endif

	DoFireEffects();

	RemoveAmmo( pPlayer );

	UpdatePunchAngles( pPlayer );

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;

	if ( flPunchAngle > 0 )
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
		pPlayer->SetPunchAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire a bullet!
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::FireBullet( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		GetWeaponID(),
		m_iWeaponMode,
		CBaseEntity::GetPredictionRandomSeed() & 255,
		GetWeaponSpread(),
		GetProjectileDamage(),
		IsCurrentAttackACrit() );
}

//-----------------------------------------------------------------------------
// Purpose: Return angles for a projectile reflected by airblast
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetProjectileReflectSetup( CTFPlayer *pPlayer, const Vector &vecPos, Vector *vecDeflect, bool bHitTeammates /* = true */, bool bUseHitboxes /* = false */ )
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;
	int nMask = bUseHitboxes ? MASK_SOLID | CONTENTS_HITBOX : MASK_SOLID;

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}

	// vecPos is projectile's current position. Use that to find angles.

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	//if ( tr.fraction > 0.1 || bUseHitboxes )
	if ( tr.fraction > 0.1 )
	{
		*vecDeflect = tr.endpos - vecPos;
	}
	else
	{
		*vecDeflect = endPos - vecPos;
	}

	VectorNormalize( *vecDeflect );
}

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /* = true */, bool bUseHitboxes /* = false */ )
{
	int nCenterFireProjectile = 0;
	CALL_ATTRIB_HOOK_INT( nCenterFireProjectile, centerfire_projectile );
	if ( nCenterFireProjectile == 1 )
	{
		vecOffset.y = 0;
	}

	Vector vecForward, vecRight, vecUp;
	AngleVectors( GetSpreadAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;
	int nMask = bUseHitboxes ? MASK_SOLID | CONTENTS_HITBOX : MASK_SOLID;

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}

#ifndef CLIENT_DLL
	// If viewmodel is flipped fire from the other side.
	if ( IsViewModelFlipped() )
	{
		vecOffset.y *= -1.0f;
	}

	// Offset actual start point
	*vecSrc = vecShootPos + (vecForward * vecOffset.x) + (vecRight * vecOffset.y) + (vecUp * vecOffset.z);
#else
	// If we're seeing another player shooting the projectile, move their start point to the weapon origin
	if ( pPlayer )
	{
		if ( !UsingViewModel() )
		{
			GetAttachment( "muzzle", *vecSrc );
		}
		else
		{
			C_BaseEntity *pViewModel = pPlayer->GetViewModel();

			if ( pViewModel )
			{
				QAngle vecAngles;
				int iMuzzleFlashAttachment = pViewModel->LookupAttachment( "muzzle" );
				pViewModel->GetAttachment( iMuzzleFlashAttachment, *vecSrc, vecAngles );

				Vector vForward;
				AngleVectors( vecAngles, &vForward );

				trace_t trace;	
				UTIL_TraceLine( *vecSrc + vForward * -50, *vecSrc, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );

				*vecSrc = trace.endpos;
			}
		}
	}
#endif

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	//if ( tr.fraction > 0.1 || bUseHitboxes )
	if ( tr.fraction > 0.1 )
	{
		VectorAngles( tr.endpos - *vecSrc, *angForward );
	}
	else
	{
		VectorAngles( endPos - *vecSrc, *angForward );
	}
	
}

QAngle CTFWeaponBaseGun::GetSpreadAngles( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	QAngle angSpread = pOwner->EyeAngles();

	float flSpreadAngle = 0.0f; 
	CALL_ATTRIB_HOOK_FLOAT( flSpreadAngle, projectile_spread_angle );
	if ( flSpreadAngle )
	{
		angSpread.x += RandomFloat( -flSpreadAngle, flSpreadAngle );
		angSpread.y += RandomFloat( -flSpreadAngle, flSpreadAngle );
	}

	return angSpread;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireRocket( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	Vector vecSrc;
	Vector vecOffset(23.5f, 12.0f, -3.0f);
	QAngle angForward;

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create(this, vecSrc, angForward, pPlayer, pPlayer);
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fires an energy ball.
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireEnergyBall( CTFPlayer *pPlayer, bool bCharged /* ==false */ )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	Vector vecSrc;
	Vector vecOffset(23.5f, -8.0f, -3.0f);
	QAngle angForward;

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_EnergyBall *pProjectile = CTFProjectile_EnergyBall::Create( this, vecSrc, angForward, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetIsCharged( bCharged );
		pProjectile->SetColor( 1, GetEnergyWeaponColor( false ) );
		pProjectile->SetColor( 2, GetEnergyWeaponColor( true ) );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire an energy beam.
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireEnergyRing( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	Vector vecSrc;
	Vector vecOffset(23.5f, -8.0f, -3.0f);
	QAngle angForward;

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	// Add attribute spread.
	float flSpread = 0;
	CALL_ATTRIB_HOOK_FLOAT( flSpread, projectile_spread_angle );
	if ( flSpread != 0)
	{
		angForward.x += RandomFloat(-flSpread, flSpread);
		angForward.y += RandomFloat(-flSpread, flSpread);
	}

	CTFProjectile_EnergyRing *pBeam = CTFProjectile_EnergyRing::Create(this, vecSrc, angForward, pPlayer, pPlayer);
	if ( pBeam )
	{
		pBeam->SetCritical( IsCurrentAttackACrit() );
		pBeam->SetDamage( GetProjectileDamage() );
	}
	return pBeam;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fires a fire ball.
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireFireBall( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	Vector vecSrc;
	Vector vecOffset(23.5f, 12.0f, -3.0f);
	QAngle angForward;

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_BallOfFire *pProjectile = CTFProjectile_BallOfFire::Create( this, vecSrc, angForward, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fires an energy orb.
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireEnergyOrb(CTFPlayer *pPlayer)
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	Vector vecSrc;
	Vector vecOffset(23.5f, 12.0f, -3.0f);
	QAngle angForward;

	if (pPlayer->GetFlags() & FL_DUCKING)
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup(pPlayer, vecOffset, &vecSrc, &angForward, false);

	CTFProjectile_MechanicalArmOrb *pOrb = CTFProjectile_MechanicalArmOrb::Create(this, vecSrc, angForward, pPlayer, pPlayer);
	if (pOrb)
	{
		pOrb->SetDamage(GetProjectileDamage());
	}
	return pOrb;

#endif

	return NULL;
}



//-----------------------------------------------------------------------------
// Purpose: Fire a projectile nail
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireNail( CTFPlayer *pPlayer, int iSpecificNail )
{
	PlayWeaponShootSound();

	Vector vecSrc;
	QAngle angForward;
	GetProjectileFireSetup( pPlayer, Vector(16,6,-8), &vecSrc, &angForward );

	// Add some extra spread to our nails.
	const float flSpread = 1.5 + GetProjectileSpread();

	CTFBaseProjectile *pProjectile = NULL;
	switch( iSpecificNail )
	{
	case TF_PROJECTILE_SYRINGE:
		angForward.x += RandomFloat(-flSpread, flSpread);
		angForward.y += RandomFloat(-flSpread, flSpread);
		pProjectile = CTFProjectile_Syringe::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), this );
		break;

	case TF_PROJECTILE_NAIL:
		pProjectile = CTFProjectile_Nail::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), this );
		break;

	case TF_PROJECTILE_DART:
		pProjectile = CTFProjectile_Dart::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), this );
		break;

	default:
		Assert(0);
	}

	if ( pProjectile )
	{
		pProjectile->SetWeaponID( GetWeaponID() );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
#ifdef GAME_DLL
		pProjectile->SetLauncher( this );
		pProjectile->SetDamage( GetProjectileDamage() );
#endif
	}
	
	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a  pipe bomb
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FirePipeBomb( CTFPlayer *pPlayer, int iRemoteDetonate )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL

	int iMode = TF_GL_MODE_REGULAR, iNoSpin = 0;
	CALL_ATTRIB_HOOK_INT( iMode, set_detonate_mode );

	if ( iRemoteDetonate == 1 )
	{
		iMode = TF_GL_MODE_REMOTE_DETONATE;
	}
	else if ( iRemoteDetonate == 3 )
	{
		iMode = TF_GL_MODE_BETA_DETONATE;
	}
	else if ( iRemoteDetonate == 4 )
	{
		iMode = TF_GL_MODE_CANNONBALL;
	}

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	vecSrc +=  vecForward * 16.0f + vecRight * 8.0f + vecUp * -6.0f;
	
	Vector vecVelocity = ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );

	float flDamageMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flDamageMult, mult_dmg );
	
	// Special damage bonus case for stickybombs.
	if ( iMode == TF_GL_MODE_REMOTE_DETONATE )
	{
		float flDamageChargeMult = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDamageChargeMult, stickybomb_charge_damage_increase );
		if ( flDamageChargeMult != 1.0f )
		{
			// If we're a stickybomb with this attribute, we need to calculate out the charge level.
			// Since we know GetProjectileSpeed(), we can use maths to get the charge level percent.
			float flSpeedMult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT( flSpeedMult, mult_projectile_range );
			float flProjectileSpeedDiffCurrent = GetProjectileSpeed() - ( TF_PIPEBOMB_MIN_CHARGE_VEL * flSpeedMult );
			float flProjectileSpeedDiffMax = ( TF_PIPEBOMB_MAX_CHARGE_VEL - TF_PIPEBOMB_MIN_CHARGE_VEL ) * flSpeedMult;
			float flChargePercent = flProjectileSpeedDiffCurrent / flProjectileSpeedDiffMax;
			
			// Calculate out our additional damage bonus from charging our stickybomb.
			flDamageMult *= ( ( flDamageChargeMult - 1 ) * flChargePercent ) + 1;
			
		}
	}

	CALL_ATTRIB_HOOK_INT ( iNoSpin, grenade_no_spin );

	AngularImpulse spin( 600, 0, 0 );

	if ( iNoSpin == 0 )
	{
		spin = AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 );
	}

	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( vecSrc, pPlayer->EyeAngles(), vecVelocity, 
																spin, pPlayer, GetTFWpnData(), iMode, flDamageMult, this );


	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a flare
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireFlare( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Flare *pProjectile = CTFProjectile_Flare::Create( this, vecSrc, angForward, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );

		CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun *>( this );
		if ( pFlareGun && pFlareGun->GetFlareGunMode() == TF_FLARE_MODE_DETONATE )
		{
			pFlareGun->AddFlare( pProjectile );
		}
	}
	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a throwable
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireJar( CTFPlayer *pPlayer, int iType )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL
	AngularImpulse spin = AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	// Set the starting position a bit behind the player so the projectile
	// launches out of the players view
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	vecSrc +=  vecForward * -64.0f + vecRight * 8.0f + vecUp * -6.0f;
	
	Vector vecVelocity = ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );

	//GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, false );

	CTFWeaponBaseGrenadeProj *pProjectile = NULL;

	switch ( iType )
	{
		case TF_PROJECTILE_JAR:
		case TF_PROJECTILE_FESTIVE_URINE:
		case TF_PROJECTILE_BREADMONSTER_JARATE:
			pProjectile = CTFProjectile_Jar::Create( this, vecSrc, pPlayer->EyeAngles(), vecVelocity, pPlayer, pPlayer, spin, GetTFWpnData() );
			break;
		case TF_PROJECTILE_JAR_MILK:
		case TF_PROJECTILE_BREADMONSTER_MADMILK:
			pProjectile = CTFProjectile_JarMilk::Create( this, vecSrc, pPlayer->EyeAngles(), vecVelocity, pPlayer, pPlayer, spin, GetTFWpnData() );
			break;
		case TF_PROJECTILE_CLEAVER:
			pProjectile = CTFProjectile_Cleaver::Create( this, vecSrc, pPlayer->EyeAngles(), vecVelocity, pPlayer, pPlayer, spin, GetTFWpnData() );
			break;
	}
	
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}
	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire an Arrow
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireArrow( CTFPlayer *pPlayer, int iType )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	/*if ( IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
	{
		// Valve were apparently too lazy to fix the viewmodel and just flipped it through the code.
		vecOffset.y *= -1.0f;
	}*/
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, true );

	CTFProjectile_Arrow *pProjectile = CTFProjectile_Arrow::Create( this, vecSrc, angForward, GetProjectileSpeed(), GetProjectileGravity(), IsFlameArrow(), pPlayer, pPlayer, iType );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetCollisionGroup( TFCOLLISION_GROUP_ARROWS );

		int nCanPenetrate = 0;
		CALL_ATTRIB_HOOK_INT( nCanPenetrate, projectile_penetration );
		if ( nCanPenetrate == 1 )
		{
			pProjectile->SetCanPenetrate( true );
			pProjectile->SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
		}
	}
	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Use this for any old grenades: MIRV, Frag, etc
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireGrenade( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	vecSrc += vecForward * 16.0f + vecRight * 8.0f + vecUp * -6.0f;

	Vector vecVelocity = ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f) * vecRight ) +
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );

	float flDamageMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flDamageMult, mult_dmg );

	char szEntName[256];
	V_snprintf( szEntName, sizeof( szEntName ), "%s_projectile", WeaponIdToClassname( GetWeaponID() ) );

	CTFWeaponBaseGrenadeProj *pProjectile = CTFWeaponBaseGrenadeProj::Create( szEntName, vecSrc, pPlayer->EyeAngles(), vecVelocity,
		AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ),
		pPlayer, GetTFWpnData(), 3.0f, flDamageMult );


	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetLauncher( this );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PlayWeaponShootSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseGun::GetAmmoPerShot( void ) const
{
	int nCustomAmmoPerShot = 0;
	CALL_ATTRIB_HOOK_INT( nCustomAmmoPerShot, mod_ammo_per_shot );
	if (nCustomAmmoPerShot)
		return nCustomAmmoPerShot;

	int iAmmoPerShot = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_iAmmoPerShot;
	return iAmmoPerShot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::RemoveAmmo( CTFPlayer *pPlayer )
{
#ifndef CLIENT_DLL
	if (IsEnergyWeapon())
	{
		Energy_DrainEnergy();
	}
	else if (UsesClipsForAmmo1())
	{
		m_iClip1 -= GetAmmoPerShot();
	}
	else
	{
		if (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE)
		{
			if (!IsEnergyWeapon())
				pPlayer->RemoveAmmo( GetAmmoPerShot(), m_iPrimaryAmmoType );
			if (0 < m_iRefundedAmmo)
				pPlayer->GiveAmmo( m_iRefundedAmmo, m_iPrimaryAmmoType );
		}
		else
		{
			if (!IsEnergyWeapon())
				pPlayer->RemoveAmmo( GetAmmoPerShot(), m_iSecondaryAmmoType );
			if (0 < m_iRefundedAmmo)
				pPlayer->GiveAmmo( m_iRefundedAmmo, m_iSecondaryAmmoType );
		}

		m_iRefundedAmmo = 0;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for huntsman flame arrow
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::IsFlameArrow(void)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetWeaponSpread( void )
{
	float flSpread = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSpread;
	CALL_ATTRIB_HOOK_FLOAT( flSpread, mult_spread_scale );
	
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		float flHealthModSpread = 1;
		CALL_ATTRIB_HOOK_FLOAT( flHealthModSpread, panic_attack_negative );
		if (flHealthModSpread != 1)
		{
			flSpread *= RemapValClamped( pPlayer->GetHealth() / pPlayer->GetMaxHealth(), 0.2, 0.9, flHealthModSpread, 1.0 );
		}	
	}
	
	return flSpread;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileDamage( void )
{
	float flDamage = (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg_disguised );	

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#if !defined( CLIENT_DLL )

	// Make sure to zoom out before we holster the weapon.
	ZoomOut();
	SetContextThink( NULL, 0, ZOOM_CONTEXT );

#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: Should this be put into fire gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::DoFireEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Muzzle flash on weapon.
	bool bMuzzleFlash = true;

	// We no longer need this
	/*
	if ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		//CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
		//if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
		if (pPlayer->IsActiveTFWeapon(TF_WEAPON_MINIGUN))
		{
			bMuzzleFlash = false;
		}
	}*/

	if ( pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		if ( pPlayer->IsActiveTFWeapon( TF_WEAPON_COMPOUND_BOW ) )
		{
			bMuzzleFlash = false;
		}
	}

	if ( bMuzzleFlash )
	{
		pPlayer->DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ToggleZoom( void )
{
	// Toggle the zoom.
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer )
	{
		if( pPlayer->GetFOV() >= 75 )
		{
			ZoomIn();
		}
		else
		{
			ZoomOut();
		}
	}

	// Get the zoom animation time.
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.2;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomIn( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Set the weapon zoom.
	// TODO: The weapon fov should be gotten from the script file.
	pPlayer->SetFOV( pPlayer, TF_WEAPON_ZOOM_FOV, 0.1f );
	pPlayer->m_Shared.AddCond( TF_COND_ZOOMED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOut( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// Set the FOV to 0 set the default FOV.
		pPlayer->SetFOV( pPlayer, 0, 0.1f );
		pPlayer->m_Shared.RemoveCond( TF_COND_ZOOMED );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOutIn( void )
{
	//Zoom out, set think to zoom back in.
	ZoomOut();
	SetContextThink( &CTFWeaponBaseGun::ZoomIn, gpGlobals->curtime + ZOOM_REZOOM_TIME, ZOOM_CONTEXT );
}


#define TF_DOUBLE_DONK_TIMELIMIT 0.5f
//-----------------------------------------------------------------------------
// Purpose: Adds the donked target to our table to check later.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::AddDoubleDonk(CBaseEntity* pVictim )
{
	// Clean up expired donks.
	FOR_EACH_VEC_BACK( hDonkedPlayers, i )
	{
		if( hDonkedTimeLimit[i] <= gpGlobals->curtime )
		{
			hDonkedPlayers.Remove( i );
			hDonkedTimeLimit.Remove( i );
		}
	}
	
	hDonkedPlayers.AddToTail(pVictim);
	hDonkedTimeLimit.AddToTail(gpGlobals->curtime + TF_DOUBLE_DONK_TIMELIMIT);
}

//-----------------------------------------------------------------------------
// Purpose: Checks if this is a Double Donk attack.
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::IsDoubleDonk(CBaseEntity* pVictim )
{
	// Check the players hit by donks.
	FOR_EACH_VEC( hDonkedPlayers, i )
	{
		// Not our victim, skip.
		if (hDonkedPlayers[i] != pVictim)
			continue;
		
		// Check if it's within the time limit.
		if( hDonkedTimeLimit[i] <= gpGlobals->curtime )
			return true;
	}
	
	return false;
}