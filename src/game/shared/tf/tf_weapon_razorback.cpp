#include "cbase.h"
#include "tf_weapon_razorback.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CREATE_SIMPLE_WEAPON_TABLE(TFRazorback, tf_weapon_razorback)

#ifdef GAME_DLL

void CTFRazorback::Precache(void){
	BaseClass::Precache();
	PrecacheModel("models/player/items/sniper/knife_shield.mdl");
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRazorback::Equip(CBasePlayer *pPlayer)
{
	BaseClass::Equip(pPlayer);
	UpdateModelToClass();

	// player_bodygroups
	UpdatePlayerBodygroups();
}

//---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRazorback::UpdateModelToClass(void)
{

	SetModel("models/player/items/sniper/knife_shield.mdl");
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFRazorback::SetWeaponVisible(bool visible)
{
	RemoveEffects(EF_NODRAW);

#ifdef CLIENT_DLL
	UpdateVisibility();
#endif

}
