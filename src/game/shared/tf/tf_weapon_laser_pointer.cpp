#include "cbase.h"
#include "tf_weapon_laser_pointer.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "tf_viewmodel.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_baseobject.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#ifdef GAME_DLL
ConVar tf_debug_wrangler( "tf_wrangler_debug", "0", FCVAR_CHEAT );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFLaserPointer, DT_WeaponLaserPointer )

BEGIN_NETWORK_TABLE( CTFLaserPointer, DT_WeaponLaserPointer )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLaserPointer )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_laser_pointer, CTFLaserPointer );
PRECACHE_WEAPON_REGISTER( tf_weapon_laser_pointer );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFLaserPointer )
END_DATADESC()
#endif

CTFLaserPointer::CTFLaserPointer()
{
#ifdef GAME_DLL
	m_hGun = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFLaserPointer::Deploy( void )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner )
	{
		for ( int i = 0; i < pOwner->GetObjectCount(); i++ )
		{

			CBaseObject *pObject = pOwner->GetObject( i );


			if ( pObject->GetType() == OBJ_SENTRYGUN )
			{
				m_hGun = dynamic_cast< CObjectSentrygun * > ( pObject );
			}
		}
	}
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFLaserPointer::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	if ( m_hGun )
	{

		if ( m_hGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			m_hGun->OnStopWrangling();
			m_hGun->SetShouldFire( false );
		}
	}
	m_hGun = NULL;
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaserPointer::WeaponReset( void )
{
	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	if ( !m_hGun )
		return;

	if ( m_flNextPrimaryAttack < gpGlobals->curtime && m_hGun->GetState() == SENTRY_STATE_WRANGLED )
	{
		m_hGun->SetShouldFire( true );

		// input buffer
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.05f;
	}
#endif
	SendWeaponAnim( ACT_ITEM3_VM_PRIMARYATTACK );
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaserPointer::SecondaryAttack( void )
{
	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	if ( !m_hGun )
		return;

	if ( m_flNextSecondaryAttack <= gpGlobals->curtime && m_hGun->GetState() == SENTRY_STATE_WRANGLED )
	{
		int iUpgradeLevel = m_hGun->GetUpgradeLevel();

		if ( iUpgradeLevel == 3 )
		{
			m_hGun->FireRockets();

			// Rockets fire slightly faster wrangled
			m_flNextSecondaryAttack = gpGlobals->curtime + 2.5;
		}
	}
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaserPointer::ItemPostFrame( void )
{
#ifdef GAME_DLL
	if ( m_hGun )
	{
		//TODO: Find a better way to determine if we can wrangle
		if ( !m_hGun->IsRedeploying() && !m_hGun->IsBuilding() && !m_hGun->IsUpgrading() && !m_hGun->HasSapper() )
		{
			m_hGun->SetState( SENTRY_STATE_WRANGLED );
		}

		if ( m_hGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			UpdateLaserDot();
		}
	}
#endif

	BaseClass::ItemPostFrame();
}

#ifdef GAME_DLL
void CTFLaserPointer::UpdateLaserDot( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	m_hGun->StudioFrameAdvance( );

	if ( !pOwner || !pOwner->IsAlive() )
	{
		m_hGun->OnStopWrangling();
		m_hGun->SetShouldFire( false );
		return;
	}

	trace_t tr;
	Vector vecStart, vecEnd, vecForward;
	pOwner->EyeVectors( &vecForward );

	vecStart = pOwner->EyePosition();
	vecEnd = vecStart + ( vecForward * MAX_TRACE_LENGTH);

	CTraceFilterIgnoreTeammatesAndTeamObjects *pFilter = new CTraceFilterIgnoreTeammatesAndTeamObjects( this, COLLISION_GROUP_NONE, GetTeamNumber() );

	// First pass to find where we are looking
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pFilter, &tr );

	vecStart = m_hGun->EyePosition();

	// If we're looking at a player fix our position to the centermass
	if ( tr.DidHitNonWorldEntity() && tr.m_pEnt && tr.m_pEnt->IsPlayer() )
	{
		vecEnd = m_hGun->GetEnemyAimPosition( tr.m_pEnt );

		// Second pass to make sure the sentry can actually see the person we're targeting 
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pFilter, &tr );

		if ( tr.DidHitNonWorldEntity() && tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			m_hGun->SetEnemy( tr.m_pEnt );
		}
		else
		{
			m_hGun->SetEnemy( NULL );
			vecEnd = tr.endpos;
		}
	}
	else
	{
		// We're not locked on to a player so make sure the laser doesn't clip through walls
		m_hGun->SetEnemy( NULL );

		// Second pass
		UTIL_TraceLine( vecStart, tr.endpos, MASK_SOLID, pFilter, &tr );
		vecEnd = tr.endpos;
	}

	m_hGun->SetEndVector( vecEnd );

	// Adjust sentry angles 
	vecForward = vecEnd - vecStart;
	m_hGun->UpdateSentryAngles( vecForward ); 

	if ( tf_debug_wrangler.GetBool() ) 
	{
		NDebugOverlay::Line( vecStart, vecEnd, 0, 255, 0, true, 0.25f );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFLaserPointer::TranslateViewmodelHandActivity(int iActivity)
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
void CTFLaserPointer::SetViewModel()
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

const char *CTFLaserPointer::DetermineViewModelType(const char *vModel) const
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
const char *CTFLaserPointer::GetViewModel(int iViewModel) const
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
C_BaseAnimating *C_TFLaserPointer::GetAppropriateWorldOrViewModel(void)
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
