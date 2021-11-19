//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_milkrifle.h"
#include "in_buttons.h"
#include "tf_viewmodel.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"

// For TFGameRules() and Player resources
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#endif

#define TF_WEAPON_MILKRIFLE_CHARGE_PER_SEC	40.0
#define TF_WEAPON_MILKRIFLE_UNCHARGE_PER_SEC	60.0
#define	TF_WEAPON_MILKRIFLE_DAMAGE_MIN		40
#define TF_WEAPON_MILKRIFLE_DAMAGE_MAX		120
#define TF_WEAPON_MILKRIFLE_RELOAD_TIME		1.0f
#define TF_WEAPON_MILKRIFLE_ZOOM_TIME			0.3f

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFMilkRifle, DT_TFMilkRifle)

BEGIN_NETWORK_TABLE_NOBASE(CTFMilkRifle, DT_MilkRifleLocalData)
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CTFMilkRifle, DT_TFMilkRifle)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFMilkRifle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_milkrifle, CTFMilkRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_milkrifle );

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFMilkRifle::CTFMilkRifle()
{
// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFMilkRifle::~CTFMilkRifle()
{
// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMilkRifle::Spawn()
{
	m_iAltFireHint = HINT_ALTFIRE_SNIPERRIFLE;
	BaseClass::Spawn();

	ResetTimers();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMilkRifle::Precache()
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFMilkRifle::GetProjectileDamage(void)
{
	// Uncharged? Min damage.
	return max(m_flChargedDamage, TF_WEAPON_MILKRIFLE_DAMAGE_MIN);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMilkRifle::ItemPostFrame(void)
{
	// If we're lowered, we're not allowed to fire
	if (m_bLowered)
		return;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer)
		return;

	if (!CanAttack())
	{
		if (IsZoomed())
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if (m_hSniperDot)
	{
		UpdateSniperDot();
	}
#endif

	// Start charging when we're zoomed in, and allowed to fire
	if (pPlayer->m_Shared.IsJumping())
	{
		// Unzoom if we're jumping
		if (IsZoomed())
		{
			ToggleZoom();
		}

		m_flChargedDamage = 0.0f;
		m_bRezoomAfterShot = false;
	}
	else if (m_flNextSecondaryAttack <= gpGlobals->curtime)
	{
		// Don't start charging in the time just after a shot before we unzoom to play rack anim.
		if (pPlayer->m_Shared.InCond(TF_COND_AIMING) && !m_bRezoomAfterShot)
		{
			m_flChargedDamage = min(m_flChargedDamage + gpGlobals->frametime * TF_WEAPON_MILKRIFLE_CHARGE_PER_SEC, TF_WEAPON_MILKRIFLE_DAMAGE_MAX);
		}
		else
		{
			m_flChargedDamage = max(0, m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_MILKRIFLE_UNCHARGE_PER_SEC);
		}
	}

	// Fire.
	if (pPlayer->m_nButtons & IN_ATTACK)
	{
		Fire(pPlayer);
	}

	// Idle.
	if (!((pPlayer->m_nButtons & IN_ATTACK) || (pPlayer->m_nButtons & IN_ATTACK2)))
	{
		// No fire buttons down or reloading
		if (!ReloadOrSwitchWeapons() && (m_bInReload == false))
		{
			WeaponIdle();
		}
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMilkRifle::GetHUDDamagePerc(void)
{
	return (m_flChargedDamage / TF_WEAPON_MILKRIFLE_DAMAGE_MAX);
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFMilkRifle::TranslateViewmodelHandActivity(int iActivity)
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
void CTFMilkRifle::SetViewModel()
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

const char *CTFMilkRifle::DetermineViewModelType(const char *vModel) const
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
const char *CTFMilkRifle::GetViewModel(int iViewModel) const
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
C_BaseAnimating *C_TFMilkRifle::GetAppropriateWorldOrViewModel(void)
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