//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_wrench.h"
#include "decals.h"
#include "baseobject_shared.h"
#include "tf_viewmodel.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
// Server specific.
#else
	#include "tf_player.h"
	#include "variant_t.h"
#endif

//=============================================================================
//
// Weapon Wrench tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWrench, DT_TFWeaponWrench )

BEGIN_NETWORK_TABLE( CTFWrench, DT_TFWeaponWrench )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWrench )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_wrench, CTFWrench );
PRECACHE_WEAPON_REGISTER( tf_weapon_wrench );

//=============================================================================
//
// Weapon Wrench functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWrench::CTFWrench()
{
}

#ifdef GAME_DLL
void CTFWrench::OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector vecHitPos )
{
	// Did this object hit do any work? repair or upgrade?
	bool bUsefulHit = pObject->InputWrenchHit( pPlayer, this, vecHitPos );

	CDisablePredictionFiltering disabler;

	if ( bUsefulHit )
	{
		// play success sound
		WeaponSound( SPECIAL1 );
	}
	else
	{
		// play failure sound
		WeaponSound( SPECIAL2 );
	}
}
#endif

void CTFWrench::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// only trace against objects

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsBaseObject() &&
		 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
#ifdef GAME_DLL
		OnFriendlyBuildingHit( dynamic_cast< CBaseObject * >( trace.m_pEnt ), pPlayer, trace.endpos );
#endif
	}
	else
	{
		// if we cannot, Smack as usual for player hits
		BaseClass::Smack();
	}
}

//=============================================================================
//
// Weapon Robot Arm tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotArm, DT_TFWeaponRobotArm )

BEGIN_NETWORK_TABLE( CTFRobotArm, DT_TFWeaponRobotArm )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRobotArm )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_robot_arm, CTFRobotArm );
PRECACHE_WEAPON_REGISTER( tf_weapon_robot_arm );

//=============================================================================
//
// Weapon Robot Arm functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::Smack( void )
{
	//TODO: check if this needs lag compensation

	trace_t tr;

	// Did we hit an enemy player?
	if ( DoSwingTrace( tr ) && tr.DidHitNonWorldEntity() && tr.m_pEnt && tr.m_pEnt->IsPlayer() && tr.m_pEnt->GetTeamNumber() != GetTeamNumber() )
	{
		m_iConsecutivePunches++;
		m_flComboDecayTime = gpGlobals->curtime;

		if ( m_iConsecutivePunches > 2 )
			m_bComboKill = true;
	}
	else
	{
		m_bComboKill = false;
		m_iConsecutivePunches = 0;
	}

	BaseClass::Smack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::PrimaryAttack( void )
{
	// reset the combo if we've already hit 3 times or exceeded the decay time
	if ( gpGlobals->curtime - m_flComboDecayTime > 1.0f || m_iConsecutivePunches > 2  )
	{
		m_iConsecutivePunches = 0;
		m_bComboKill = false;
	}

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::WeaponIdle( void )
{
	if ( m_bComboKill )
	{
		SendWeaponAnim( ACT_ITEM2_VM_IDLE_2 );
		m_bComboKill = false;
	}

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFRobotArm::GetCustomDamageType( void ) const
{
	if ( m_iConsecutivePunches == 3 )
		return TF_DMG_CUSTOM_COMBO_PUNCH;

	return TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRobotArm::CalcIsAttackCriticalHelper( void )
{
	// punch after 2 consecutive hits always crits
	return ( m_iConsecutivePunches == 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::DoViewModelAnimation( void )
{
	if ( m_iConsecutivePunches == 2 )
		SendWeaponAnim( ACT_ITEM2_VM_SWINGHARD );
	else
		SendWeaponAnim( ACT_ITEM2_VM_HITCENTER );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRobotArm::GetForceScale( void )
{
	if ( m_iConsecutivePunches == 3 )
	{
		return 500.0f;
	}
	
	return BaseClass::GetForceScale();
}
#endif
/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFRobotArm::TranslateViewmodelHandActivity(int iActivity)
{
	CTFPlayer *pTFPlayer = ToTFPlayer(GetOwner());
	if (pTFPlayer == NULL)
	{
		Assert(false); // This shouldn't be possible
		return iActivity;
	}

	CTFViewModel *vm = dynamic_cast<CTFViewModel*>(pTFPlayer->GetViewModel(m_nViewModelIndex, false));
	if (vm == NULL)
	{
		return iActivity;
	}

	int iWeaponRole = GetTFWpnData().m_iWeaponType;
	for (int i = 0; i <= 160; i++)
	{
		const viewmodel_acttable_t& act = s_viewmodelacttable[i];
		if (iActivity == act.actBaseAct && iWeaponRole == act.iWeaponRole)
		{
			return act.actTargetAct;
		}
	}

	return iActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::SetViewModel()
{
	CTFPlayer *pTFPlayer = ToTFPlayer(GetOwner());
	if (pTFPlayer == NULL)
		return;

	CTFViewModel *vm = dynamic_cast< CTFViewModel* >(pTFPlayer->GetViewModel(m_nViewModelIndex, false));
	if (vm == NULL)
		return;

	Assert(vm->ViewModelIndex() == m_nViewModelIndex);

	vm->SetViewModelType(VMTYPE_NONE);

	const char *pszModelName = GetViewModel(m_nViewModelIndex);

	m_iViewModelIndex = modelinfo->GetModelIndex(pszModelName);

	vm->SetWeaponModel(pszModelName, this);

#ifdef CLIENT_DLL
	UpdateViewModel();
#endif
}

const char *CTFRobotArm::DetermineViewModelType(const char *vModel) const
{
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return vModel;


	int iType = 1;

	CTFViewModel *vm = dynamic_cast<CTFViewModel *>(pPlayer->GetViewModel(m_nViewModelIndex));
	if (vm)
		vm->SetViewModelType(iType);

	if (iType == VMTYPE_TF2)
	{
		int iGunslinger = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pPlayer, iGunslinger, wrench_builds_minisentry);

		return iGunslinger ? pPlayer->GetPlayerClass()->GetHandModelName(true) : pPlayer->GetPlayerClass()->GetHandModelName(false);
	}

	return vModel;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFRobotArm::GetViewModel(int iViewModel) const
{
	const char *pszModelName = NULL;
	CTFPlayer *pOwner = GetTFPlayerOwner();


	pszModelName = BaseClass::GetViewModel(iViewModel);


	return DetermineViewModelType(pszModelName);
}
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Find the appropriate weapon model to update bodygroups on
//-----------------------------------------------------------------------------
C_BaseAnimating *C_TFRobotArm::GetAppropriateWorldOrViewModel(void)
{
	C_TFPlayer *pPlayer = GetTFPlayerOwner();
	if (pPlayer && UsingViewModel())
	{

		C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
		if (pAttach)
			return pAttach;

	}

	// this too
	return this;
}
#endif
*/