//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_inventory.h"
#include "econ_item_system.h"

static CTFInventory g_TFInventory;

CTFInventory *GetTFInventory()
{
	return &g_TFInventory;
}

CTFInventory::CTFInventory() : CAutoGameSystemPerFrame( "CTFInventory" )
{
#ifdef CLIENT_DLL
	m_pInventory = NULL;
#endif

	// Generate dummy base items.
	for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++ )
		{
			m_Items[iClass][iSlot].AddToTail( NULL );
		}
	}
	for (int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++)
	{
		for (int iSlot = 0; iSlot < MAX_WEAPON_SLOTS; iSlot++)
		{
			Weapons_Custom[iClass][iSlot].AddToTail(Weapons[iClass][iSlot]);
		}
	}
};

CTFInventory::~CTFInventory()
{
#if defined( CLIENT_DLL )
	m_pInventory->deleteThis();
#endif
}

bool CTFInventory::Init( void )
{
	Weapons_Custom[TF_CLASS_SCOUT][1].AddToTail(TF_WEAPON_NAILGUN);
	Weapons_Custom[TF_CLASS_SCOUT][2].AddToTail(TF_WEAPON_BAT_WOOD);
	Weapons_Custom[TF_CLASS_SOLDIER][1].AddToTail(TF_WEAPON_BUFF_ITEM);
	Weapons_Custom[TF_CLASS_SNIPER][0].AddToTail(TF_WEAPON_COMPOUND_BOW);
	Weapons_Custom[TF_CLASS_SNIPER][1].AddToTail(TF_WEAPON_JAR);
	Weapons_Custom[TF_CLASS_ENGINEER][1].AddToTail(TF_WEAPON_LASER_POINTER);
	Weapons_Custom[TF_CLASS_DEMOMAN][2].AddToTail(TF_WEAPON_STICKBOMB);
	Weapons_Custom[TF_CLASS_SNIPER][0].AddToTail(TF_WEAPON_MILKRIFLE);


#if defined( CLIENT_DLL )
	LoadInventory();
#endif

	return true;
}

void CTFInventory::LevelInitPreEntity( void )
{
	GetItemSchema()->Precache();
}

int CTFInventory::GetNumPresets(int iClass, int iSlot)
{
	return Weapons_Custom[iClass][iSlot].Count();
};

int CTFInventory::GetWeapon(int iClass, int iSlot, int iNum)
{
	return Weapons_Custom[iClass][iSlot][iNum];
};

CEconItemView *CTFInventory::GetItem( int iClass, int iSlot, int iNum )
{
	if ( CheckValidWeapon( iClass, iSlot, iNum ) == false )
		return NULL;

	return m_Items[iClass][iSlot][iNum];
};

bool CTFInventory::CheckValidSlot(int iClass, int iSlot, bool bHudCheck /*= false*/)
{
	if (iClass < TF_CLASS_UNDEFINED || iClass > TF_CLASS_COUNT)
		return false;

	int iCount = (bHudCheck ? INVENTORY_ROWNUM : TF_LOADOUT_SLOT_COUNT);

	// Array bounds check.
	if ( iSlot >= iCount || iSlot < 0 )
		return false;

	// Slot must contain a base item.
	if (Weapons_Custom[iClass][iSlot][0] == NULL)
		return false;

	return true;
};

bool CTFInventory::CheckValidWeapon(int iClass, int iSlot, int iWeapon, bool bHudCheck /*= false*/)
{
	if (iClass < TF_CLASS_UNDEFINED || iClass > TF_CLASS_COUNT)
		return false;

	int iCount = ( bHudCheck ? INVENTORY_COLNUM : Weapons_Custom[iClass][iSlot].Count() );

	// Array bounds check.
	if ( iWeapon >= iCount || iWeapon < 0 )
		return false;

	// Don't allow switching if this class has no weapon at this position.
	if (Weapons_Custom[iClass][iSlot][iWeapon] == NULL)
		return false;

	return true;
};

#if defined( CLIENT_DLL )
void CTFInventory::LoadInventory()
{
	bool bExist = filesystem->FileExists("scripts/tf_inventory.txt", "MOD");
	if (bExist)
	{
		if (!m_pInventory)
		{
			m_pInventory = new KeyValues("Inventory");
		}
		m_pInventory->LoadFromFile(filesystem, "scripts/tf_inventory.txt");
	}
	else
	{
		ResetInventory();
	}
};

void CTFInventory::SaveInventory()
{
	m_pInventory->SaveToFile(filesystem, "scripts/tf_inventory.txt");
};

void CTFInventory::ResetInventory()
{
	if (m_pInventory)
	{
		m_pInventory->deleteThis();
	}

	m_pInventory = new KeyValues("Inventory");

	for (int i = TF_CLASS_UNDEFINED; i < TF_CLASS_COUNT_ALL; i++)
	{
		KeyValues *pClassInv = new KeyValues(g_aPlayerClassNames_NonLocalized[i]);
		for (int j = 0; j < TF_LOADOUT_SLOT_COUNT; j++)
		{
			pClassInv->SetInt( g_LoadoutSlots[j], 0 );
		}
		m_pInventory->AddSubKey(pClassInv);
	}

	SaveInventory();
}

int CTFInventory::GetWeaponPreset(int iClass, int iSlot)
{
	KeyValues *pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	if (!pClass)	//cannot find class node
	{	
		ResetInventory();
		return 0;
	}
	int iPreset = pClass->GetInt(g_LoadoutSlots[iSlot], -1);
	if (iPreset == -1)	//cannot find slot node
	{
		ResetInventory();
		return 0;
	}

	if ( CheckValidWeapon( iClass, iSlot, iPreset ) == false )
		return 0;

	return iPreset;
};

void CTFInventory::SetWeaponPreset(int iClass, int iSlot, int iPreset)
{
	KeyValues* pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	if (!pClass)	//cannot find class node
	{
		ResetInventory();
		pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	}
	pClass->SetInt(GetSlotName(iSlot), iPreset);
	SaveInventory();
}

const char* CTFInventory::GetSlotName(int iSlot)
{
	return g_LoadoutSlots[iSlot];
};

#endif

// Legacy array, used when we're forced to use old method of giving out weapons.
const int CTFInventory::Weapons[TF_CLASS_COUNT_ALL][TF_PLAYER_WEAPON_COUNT] =
{
	{

	},
	{
		TF_WEAPON_SCATTERGUN,
		TF_WEAPON_PISTOL_SCOUT,
		TF_WEAPON_BAT,
		TF_WEAPON_LUNCHBOX_DRINK
	},
	{
		TF_WEAPON_SNIPERRIFLE,
		TF_WEAPON_SMG,
		TF_WEAPON_CLUB,
		TF_WEAPON_NONE
	},
	{
		TF_WEAPON_ROCKETLAUNCHER,
		TF_WEAPON_SHOTGUN_SOLDIER,
		TF_WEAPON_SHOVEL,
		TF_WEAPON_GRENADE_NORMAL
	},
	{
		TF_WEAPON_GRENADELAUNCHER,
		TF_WEAPON_PIPEBOMBLAUNCHER,
		TF_WEAPON_BOTTLE,
		TF_WEAPON_GRENADE_MIRV
	},
	{
		TF_WEAPON_SYRINGEGUN_MEDIC,
		TF_WEAPON_MEDIGUN,
		TF_WEAPON_BONESAW
	},
	{
		TF_WEAPON_MINIGUN,
		TF_WEAPON_SHOTGUN_HWG,
		TF_WEAPON_FISTS,
		TF_WEAPON_LUNCHBOX
	},
	{
		TF_WEAPON_FLAMETHROWER,
		TF_WEAPON_SHOTGUN_PYRO,
		TF_WEAPON_FIREAXE,
		TF_WEAPON_FLAREGUN
	},
	{
		TF_WEAPON_REVOLVER,
		TF_WEAPON_NONE,
		TF_WEAPON_KNIFE,
		TF_WEAPON_PDA_SPY,
		TF_WEAPON_INVIS
	},
	{
		TF_WEAPON_SHOTGUN_PRIMARY,
		TF_WEAPON_PISTOL,
		TF_WEAPON_WRENCH,
		TF_WEAPON_PDA_ENGINEER_BUILD,
		TF_WEAPON_PDA_ENGINEER_DESTROY
	},
};
