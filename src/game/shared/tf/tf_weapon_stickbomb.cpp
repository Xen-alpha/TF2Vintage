#include "cbase.h"
#include "tf_weapon_stickbomb.h"
#include "tf_viewmodel.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "c_tf_viewmodeladdon.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFStickBomb, DT_TFWeaponStickBomb );

BEGIN_NETWORK_TABLE( CTFStickBomb, DT_TFWeaponStickBomb )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iDetonated ) )
#else
	SendPropInt( SENDINFO( m_iDetonated ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFStickBomb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_stickbomb, CTFStickBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_stickbomb );


#define MODEL_NORMAL   "models/weapons/c_models/c_caber/c_caber.mdl"
#define MODEL_EXPLODED "models/weapons/c_models/c_caber/c_caber_exploded.mdl"

#define TF_STICKBOMB_NORMAL    0
#define TF_STICKBOMB_DETONATED 1


CTFStickBomb::CTFStickBomb()
{
	m_iDetonated = TF_STICKBOMB_NORMAL;
}

const char *CTFStickBomb::GetWorldModel() const
{
	if ( m_iDetonated == TF_STICKBOMB_DETONATED )
	{
		return MODEL_EXPLODED;
	}
	
	return BaseClass::GetWorldModel();
}

void CTFStickBomb::Precache()
{
	BaseClass::Precache();
	
	PrecacheModel( MODEL_NORMAL );
	PrecacheModel( MODEL_EXPLODED );
}

void CTFStickBomb::Smack()
{
	BaseClass::Smack();
	
	if ( m_iDetonated == TF_STICKBOMB_NORMAL && ConnectedHit() ) 
	{
		m_iDetonated = TF_STICKBOMB_DETONATED;
		//m_bBroken = true;
		
		SwitchBodyGroups();
		
#ifdef GAME_DLL
		CTFPlayer *owner = GetTFPlayerOwner();
		if ( owner ) 
		{
			// TF2 does these things and doesn't use the results:
			// calls owner->EyeAngles() and then AngleVectors()
			// calls this->GetSwingRange()
			
			// my bet: they meant to multiply the fwd vector by the swing range
			// and then use that for the damage force, but they typo'd it and
			// just reused the shoot position instead
			
			Vector where = owner->Weapon_ShootPosition();
			
			CPVSFilter filter( where );
			TE_TFExplosion( filter, 0.0f, where, Vector( 0.0f, 0.0f, 1.0f ),
				TF_WEAPON_GRENADELAUNCHER, ENTINDEX( owner ) );
			
			/* why is the damage force vector set to Weapon_ShootPosition()?
			 * I dunno! */
			CTakeDamageInfo dmginfo( owner, owner, this, where, where, 75.0f,
				DMG_BLAST | DMG_CRITICAL | ( IsCurrentAttackACrit() ? DMG_USEDISTANCEMOD : 0 ),
				TF_DMG_CUSTOM_STICKBOMB_EXPLOSION, &where );
			
			CTFRadiusDamageInfo radius;
			radius.info       = dmginfo;
			radius.m_vecSrc   = where;
			radius.m_flRadius = 100.0f;
			TFGameRules()->RadiusDamage( radius );
		}
#endif
	}
}

void CTFStickBomb::SwitchBodyGroups()
{
	DevMsg( "CTFStickBomb::SwitchBodyGroups\n" );

#ifdef CLIENT_DLL
	C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
	if ( pAttach )
	{
		int bodygroup = pAttach->FindBodygroupByName( "broken" );
		pAttach->SetBodygroup( bodygroup, m_iDetonated );
	}
#endif
}

void CTFStickBomb::WeaponRegenerate()
{
	m_iDetonated = TF_STICKBOMB_NORMAL;
	SetContextThink( &CTFStickBomb::SwitchBodyGroups, gpGlobals->curtime + 0.01f, "SwitchBodyGroups" );
}

void CTFStickBomb::WeaponReset()
{
	m_iDetonated = TF_STICKBOMB_NORMAL;
	BaseClass::WeaponReset();
}

#ifdef CLIENT_DLL
int C_TFStickBomb::GetWorldModelIndex()
{
	if ( !modelinfo ) {
		return BaseClass::GetWorldModelIndex();
	}
	
	int index = modelinfo->GetModelIndex( m_iDetonated == TF_STICKBOMB_DETONATED ? MODEL_EXPLODED : MODEL_NORMAL );
	m_iWorldModelIndex = index;
	return index;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFStickBomb::TranslateViewmodelHandActivity(int iActivity)
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
void CTFStickBomb::SetViewModel()
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

const char *CTFStickBomb::DetermineViewModelType(const char *vModel) const
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
const char *CTFStickBomb::GetViewModel(int iViewModel) const
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
C_BaseAnimating *C_TFStickBomb::GetAppropriateWorldOrViewModel(void)
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