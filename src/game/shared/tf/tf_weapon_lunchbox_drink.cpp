//====== Copyright ?1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_lunchbox_drink.h"
#include "tf_viewmodel.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFLunchBox_Drink, tf_weapon_lunchbox_drink )

//-----------------------------------------------------------------------------
// Purpose: Show the drink splash when we deploy
//-----------------------------------------------------------------------------
bool CTFLunchBox_Drink::Deploy( void )
{
#ifdef CLIENT_DLL
	ParticleProp()->Create( "energydrink_splash", PATTACH_ABSORIGIN_FOLLOW );
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox_Drink::PrimaryAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
	{
		return;
	}

#ifdef GAME_DLL
	pOwner->Taunt();
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox_Drink::DepleteAmmo( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
	{
		return;
	}

	// Switch away from it immediately, don't want it to stick around.
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	pOwner->SwitchToNextBestWeapon( this );

	StartEffectBarRegen();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox_Drink::Precache( void )
{
	PrecacheParticleSystem( "energydrink_splash" );
	BaseClass::Precache();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFLunchBox_Drink::TranslateViewmodelHandActivity(int iActivity)
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
void CTFLunchBox_Drink::SetViewModel()
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

const char *CTFLunchBox_Drink::DetermineViewModelType(const char *vModel) const
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
const char *CTFLunchBox_Drink::GetViewModel(int iViewModel) const
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
C_BaseAnimating *C_TFLunchBox_Drink::GetAppropriateWorldOrViewModel(void)
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
