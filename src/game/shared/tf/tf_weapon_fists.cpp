//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_fists.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Fists tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFists, DT_TFWeaponFists )

BEGIN_NETWORK_TABLE( CTFFists, DT_TFWeaponFists )

#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bSwapPunch)),
RecvPropBool(RECVINFO(m_bHook)),
// Server specific.
#else
SendPropBool(SENDINFO(m_bSwapPunch)),
SendPropBool(SENDINFO(m_bHook)),
#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFists )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_fists, CTFFists );
PRECACHE_WEAPON_REGISTER( tf_weapon_fists );

//=============================================================================
//
// Weapon Fists functions.
//


// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	// reversed for 360 because the primary attack is on the right side of the controller
	if ( m_bSwapPunch )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}
	else
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	//Punch
	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::SecondaryAttack()
{
	if ( !CanAttack() )
		return;

	// Hook
	m_bHook = true;
	Swing(GetTFPlayerOwner());
	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	m_bHook = false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::Punch( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Swing the weapon.
	Swing( pPlayer );

	m_flNextSecondaryAttack = m_flNextPrimaryAttack;

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
	m_bSwapPunch = !m_bSwapPunch;
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFFists::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( m_bHook )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFists::DoViewModelAnimation( void )
{
	Activity act;

	if (m_bHook)
	{
		act = ACT_VM_SWINGHARD;
	}
	else
	{
		act = m_bSwapPunch ? ACT_VM_HITLEFT : ACT_VM_HITRIGHT;
	}

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFFists::GetMeleeDamage(CBaseEntity *pTarget, int &iCustomDamage)
{
	float flDamage = (float)m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_nDamage;

	if (m_bHook) {
		flDamage *= 1.2f;
	}

	return flDamage;
}