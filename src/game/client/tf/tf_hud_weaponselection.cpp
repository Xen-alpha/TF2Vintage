//========= Copyright ?1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>

#include "vgui/ILocalize.h"
#include "controls/tf_advbuttonbase.h"

#include <string.h>
#include "baseobject_shared.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "c_tf_weapon_builder.h"

#define SELECTION_TIMEOUT_THRESHOLD		2.5f	// Seconds
#define SELECTION_FADEOUT_TIME			3.0f

#define FASTSWITCH_DISPLAY_TIMEOUT		0.5f
#define FASTSWITCH_FADEOUT_TIME			0.5f

ConVar tf_weapon_select_demo_start_delay( "tf_weapon_select_demo_start_delay", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Delay after spawning to start the weapon bucket demo." );
ConVar tf_weapon_select_demo_time( "tf_weapon_select_demo_time", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Time to pulse each weapon bucket upon spawning as a new class. 0 to turn off." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CItemModelPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CItemModelPanel, vgui::Panel);
public:
	CItemModelPanel(Panel *parent, const char* name);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PerformLayout();
	virtual void SetWeapon(C_BaseCombatWeapon *pWeapon, int iBorderStyle = -1, int ID = -1);
	//virtual void SetWeapon(CEconItemDefinition *pItemDefinition, int iBorderStyle = -1, int ID = -1);
private:
	C_BaseCombatWeapon	*m_pWeapon;
	vgui::Label			*m_pWeaponName;
	vgui::Label			*m_pSlotID;
	vgui::ImagePanel	*m_pWeaponImage;
	vgui::HFont			 m_pDefaultFont;
	vgui::HFont			 m_pSelectedFont;
	vgui::HFont			 m_pNumberDefaultFont;
	vgui::HFont			 m_pNumberSelectedFont;
	vgui::IBorder		*m_pDefaultBorder;
	vgui::IBorder		*m_pSelectedRedBorder;
	vgui::IBorder		*m_pSelectedBlueBorder;
	int					 m_iBorderStyle;
	int					 m_ID;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CItemModelPanel::CItemModelPanel(Panel *parent, const char* name) : EditablePanel(parent, name)
{
	m_pWeapon = NULL;
	m_pWeaponName = new vgui::Label(this, "WeaponName", "text");
	m_pSlotID = new vgui::Label(this, "SlotID", "0");
	m_pWeaponImage = new vgui::ImagePanel(this, "WeaponImage");
	m_iBorderStyle = -1;
	m_ID = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	m_pSlotID->SetFgColor(pScheme->GetColor("TanLight", Color(255, 255, 255, 255)));
	m_pDefaultFont = pScheme->GetFont("ItemFontNameSmallest", true);
	m_pSelectedFont = pScheme->GetFont("ItemFontNameSmall", true);
	m_pNumberDefaultFont = pScheme->GetFont("FontStorePromotion", true);
	m_pNumberSelectedFont = pScheme->GetFont("HudFontSmall", true);
	m_pDefaultBorder = pScheme->GetBorder("TFFatLineBorder");
	m_pSelectedRedBorder = pScheme->GetBorder("TFFatLineBorderRedBG");
	m_pSelectedBlueBorder = pScheme->GetBorder("TFFatLineBorderBlueBG");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::PerformLayout()
{
	if (m_iBorderStyle == -1)
	{
		SetPaintBorderEnabled(false);
	}
	else if (m_iBorderStyle == 0)
	{
		SetPaintBorderEnabled(true);
		IBorder *border = m_pDefaultBorder;
		SetBorder(border);
	}
	else if (m_iBorderStyle == 1)
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		IBorder *border;
		if (!pPlayer)
			return;
		int iTeam = pPlayer->GetTeamNumber();
		if (iTeam == TF_TEAM_RED)
		{
			border = m_pSelectedRedBorder;
		}
		else
		{
			border = m_pSelectedBlueBorder;
		}
		SetBorder(border);
	}
	m_pWeaponImage->SetShouldScaleImage(true);
	m_pWeaponImage->SetZPos(-1);
	m_pWeaponName->SetBounds(XRES(5), GetTall() - YRES(20), GetWide() - XRES(10), YRES(20));
	m_pWeaponName->SetFont(m_iBorderStyle ? m_pSelectedFont : m_pDefaultFont);
	m_pWeaponName->SetContentAlignment(CTFAdvButtonBase::GetAlignment("center"));
	m_pWeaponName->SetCenterWrap(true);
	if (m_pWeapon && !m_pWeapon->CanBeSelected())
	{
		wchar_t *pText = g_pVGuiLocalize->Find("#TF_OUT_OF_AMMO");
		m_pWeaponName->SetText(pText);
		m_pWeaponName->SetFgColor(GETSCHEME()->GetColor("RedSolid", Color(255, 255, 255, 255)));
	}
	else
	{
		m_pWeaponName->SetFgColor(GETSCHEME()->GetColor("TanLight", Color(255, 255, 255, 255)));
	}
	m_pSlotID->SetBounds(0, YRES(5), GetWide() - XRES(5), YRES(10));
	m_pSlotID->SetFont(m_iBorderStyle ? m_pNumberSelectedFont : m_pNumberDefaultFont);
	m_pSlotID->SetContentAlignment(CTFAdvButtonBase::GetAlignment("east"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetWeapon(C_BaseCombatWeapon *pWeapon, int iBorderStyle, int ID)
{
	m_pWeapon = pWeapon;
	m_ID = ID;
	m_iBorderStyle = iBorderStyle;

	int iItemID = m_pWeapon->GetItemID();
	
	wchar_t *pText = NULL;
	pText = g_pVGuiLocalize->Find(m_pWeapon->GetWpnData().szPrintName);
	const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
	if (pTexture)
	{
		char szImage[64];
		Q_snprintf(szImage, sizeof(szImage), "../%s", pTexture->szTextureFile);
		m_pWeaponImage->SetImage(szImage);
	}
	m_pWeaponImage->SetBounds(XRES(4), -1 * (GetTall() / 10.0) + XRES(4), (GetWide() * 1.5) - XRES(8), (GetWide() * 0.75) - XRES(8));
	m_pWeaponName->SetText(pText);
	if (ID != -1)
	{
		char szSlotID[8];
		Q_snprintf( szSlotID, sizeof(szSlotID), "%d", m_ID + 1 );
		m_pSlotID->SetText(szSlotID);
	}
	else
	{
		m_pSlotID->SetText("");
	}
	PerformLayout();
}
/*
void CItemModelPanel::SetWeapon(CEconItemDefinition *pItemDefinition, int iBorderStyle, int ID)
{
	m_pWeapon = NULL;
	m_ID = ID;
	m_iBorderStyle = iBorderStyle;

	wchar_t *pText = NULL;
	if (pItemDefinition)
	{
		pText = g_pVGuiLocalize->Find(pItemDefinition->item_name);
		char szImage[128];
		Q_snprintf(szImage, sizeof(szImage), "../%s_large", pItemDefinition->image_inventory);
		m_pWeaponImage->SetImage(szImage);
		m_pWeaponImage->SetBounds(XRES(4), -1 * (GetTall() / 5.0) + XRES(4), GetWide() - XRES(8), GetWide() - XRES(8));
	}

	m_pWeaponName->SetText(pText);
	if ( ID != -1 )
	{
		char szSlotID[8];
		Q_snprintf( szSlotID, sizeof(szSlotID), "%d", m_ID + 1 );
		m_pSlotID->SetText( szSlotID );
	}
	else
	{
		m_pSlotID->SetText("");
	}
	PerformLayout();
}
*/
//-----------------------------------------------------------------------------
// Purpose: tf weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection(const char *pElementName );

	virtual bool ShouldDraw();
	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos );
	virtual void SelectWeaponSlot( int iSlot );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void );

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void Init();
	virtual void LevelInit();

	virtual void FireGameEvent( IGameEvent *event );

	virtual void Reset(void)
	{
		CBaseHudWeaponSelection::Reset();

		// selection time is a little farther back so we don't show it when we spawn
		m_flSelectionTime = gpGlobals->curtime - ( FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME + 0.1 );
	}

protected:
	virtual void OnThink();
	virtual void PostChildPaint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual bool IsWeaponSelectable()
	{ 
		if (IsInSelectionMode())
			return true;

		return false;
	}

private:
	C_BaseCombatWeapon *FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);
	C_BaseCombatWeapon *FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);

	void FastWeaponSwitch( int iWeaponSlot );
	void PlusTypeFastWeaponSwitch( int iWeaponSlot );
	int GetNumVisibleSlots();
	bool ShouldDrawInternal();

	virtual	void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon ) 
	{ 
		m_hSelectedWeapon = pWeapon;
	}

	virtual	void SetSelectedSlot( int slot ) 
	{ 
		m_iSelectedSlot = slot;
	}

	void DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number);
	void DrawString( wchar_t *text, int xpos, int ypos, Color col, bool bCenter = false );

	void DrawPlusStyleBox(int x, int y, int wide, int tall, bool bSelected, float normalizedAlpha, int number, bool bOutOfAmmo );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudSelectionText" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );

	CPanelAnimationVarAliasType( float, m_flSmallBoxWide, "SmallBoxWide", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSmallBoxTall, "SmallBoxTall", "21", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flPlusStyleBoxWide, "PlusStyleBoxWide", "120", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flPlusStyleBoxTall, "PlusStyleBoxTall", "84", "proportional_float" );
	CPanelAnimationVar( float, m_flPlusStyleExpandPercent, "PlusStyleExpandSelected", "0.3" )

	CPanelAnimationVarAliasType( float, m_flLargeBoxWide, "LargeBoxWide", "108", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxTall, "LargeBoxTall", "72", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "12", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flRightMargin, "RightMargin", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberYPos, "SelectionNumberYPos", "4", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flIconXPos, "IconXPos", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flIconYPos, "IconYPos", "8", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flTextYPos, "TextYPos", "35", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flErrorYPos, "ErrorYPos", "60", "proportional_float" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_EmptyBoxColor, "EmptyBoxColor", "SelectionEmptyBoxBg" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxClor", "SelectionSelectedBoxBg" );

	CPanelAnimationVar( float, m_flWeaponPickupGrowTime, "SelectionGrowTime", "0.1" );

	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1.0" );

	CPanelAnimationVar( int, m_iMaxSlots, "MaxSlots", "6" );
	CPanelAnimationVar( bool, m_bPlaySelectionSounds, "PlaySelectSounds", "1" );

	float m_flDemoStartTime;
	float m_flDemoModeChangeTime;
	int m_iDemoModeSlot;

	// HUDTYPE_PLUS weapon display
	int						m_iSelectedBoxPosition;		// in HUDTYPE_PLUS, the position within a slot
	int						m_iSelectedSlot;			// in HUDTYPE_PLUS, the slot we're currently moving in
	CPanelAnimationVar( float, m_flHorizWeaponSelectOffsetPoint, "WeaponBoxOffset", "0" );

	CUtlVector<CItemModelPanel*>pModelPanels;
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection( pElementName ), EditablePanel( NULL, "HudWeaponSelection" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		char name[32];
		Q_snprintf(name, sizeof(name), "modelpanel%i", i);
		pModelPanels.AddToTail(new CItemModelPanel(this, name));
	}

	SetPostChildPaintEnabled( true );

	m_flDemoStartTime = -1;
	m_flDemoModeChangeTime = 0;
	m_iDemoModeSlot = -1;

	ListenForGameEvent( "localplayer_changeclass" );
}

//-----------------------------------------------------------------------------
// Purpose: sets up display for showing weapon pickup
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnWeaponPickup( C_BaseCombatWeapon *pWeapon )
{
	// add to pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	if ( pHudHR )
	{
		pHudHR->AddToHistory( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink()
{
	float flSelectionTimeout = SELECTION_TIMEOUT_THRESHOLD;
	float flSelectionFadeoutTime = SELECTION_FADEOUT_TIME;
	if ( hud_fastswitch.GetBool() )
	{
		flSelectionTimeout = FASTSWITCH_DISPLAY_TIMEOUT;
		flSelectionFadeoutTime = FASTSWITCH_FADEOUT_TIME;
	}

	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > flSelectionTimeout )
	{
		// close
		if ( gpGlobals->curtime - m_flSelectionTime > flSelectionTimeout + flSelectionFadeoutTime )
		{
			HideSelection();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	bool bShouldDraw = ShouldDrawInternal();

	return bShouldDraw;
}

bool CHudWeaponSelection::ShouldDrawInternal()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
		{
			HideSelection();
		}
		return false;
	}

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return false;

	if ( pPlayer->IsAlive() == false )
		return false;

	// we only show demo mode in hud_fastswitch 0
	if ( hud_fastswitch.GetInt() == 0 && ( m_iDemoModeSlot >= 0 || m_flDemoStartTime > 0 ) )
	{
		return true;
	}

	bool bret = CBaseHudWeaponSelection::ShouldDraw();
	if ( !bret )
		return false;

	// draw weapon selection a little longer if in fastswitch so we can see what we've selected
	if ( hud_fastswitch.GetBool() && ( gpGlobals->curtime - m_flSelectionTime ) < (FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME) )
		return true;

	return ( m_bSelectionVisible ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::Init()
{
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();
	
	m_iMaxSlots = clamp( m_iMaxSlots, 0, MAX_WEAPON_SLOTS );
}

//-------------------------------------------------------------------------
// Purpose: Calculates how many weapons slots need to be displayed
//-------------------------------------------------------------------------
int CHudWeaponSelection::GetNumVisibleSlots()
{
	int nCount = 0;

	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		if ( GetFirstPos( i ) )
		{
			nCount++;
		}
	}

	return nCount;
}

//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::PostChildPaint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( hud_fastswitch.GetInt() == 0 )
	{
		// See if we should start the bucket demo
		if ( m_flDemoStartTime > 0 && m_flDemoStartTime < gpGlobals->curtime )
		{
			float flDemoTime = tf_weapon_select_demo_time.GetFloat();

			if ( flDemoTime > 0 )
			{
				m_iDemoModeSlot = 0;
				m_flDemoModeChangeTime = gpGlobals->curtime + flDemoTime;
			}

			m_flDemoStartTime = -1;
			m_iSelectedSlot = m_iDemoModeSlot;
		}

		// scroll through the slots for demo mode
		if ( m_iDemoModeSlot >= 0 && m_flDemoModeChangeTime < gpGlobals->curtime )
		{
			// Keep iterating until we find a slot that has a weapon in it
			while ( !GetFirstPos( ++m_iDemoModeSlot ) && m_iDemoModeSlot < m_iMaxSlots )
			{
				// blank
			}			
			m_flDemoModeChangeTime = gpGlobals->curtime + tf_weapon_select_demo_time.GetFloat();
		}

		if ( m_iDemoModeSlot >= m_iMaxSlots )
		{
			m_iDemoModeSlot = -1;
		}
	}	

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = NULL;
	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_FASTSWITCH:
		pSelectedWeapon = pPlayer->GetActiveWeapon();
		break;
	default:
		pSelectedWeapon = GetSelectedWeapon();
		break;
	}
	if ( !pSelectedWeapon )
		return;

	if ( hud_fastswitch.GetInt() == 0 )
	{
		if ( m_iDemoModeSlot > -1 )
		{
			pSelectedWeapon = GetWeaponInSlot( m_iDemoModeSlot, 0 );
			m_iSelectedSlot = m_iDemoModeSlot;
			m_iSelectedBoxPosition = 0;
		}
	}

	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_PLUS:
	{
			// calculate where to start drawing
			int nTotalHeight = (nNumSlots - 1) * (m_flSmallBoxTall + m_flBoxGap) + m_flLargeBoxTall;
			int xStartPos = GetWide() - m_flBoxGap - m_flRightMargin;
			int xpos = xStartPos;
			int ypos = (GetTall() - nTotalHeight) / 2;

			// Draw the four buckets
			for ( int i = 0; i < MAX_WEAPON_SLOTS; ++i )
			{
				pModelPanels[i]->SetVisible(false);

				// Find out how many positions to draw - an empty position should still
				// be drawn if there is an active weapon in any slots past it.
				int lastSlotPos = -1;
				int iMaxSlotPositions = 3;	//MAX_WEAPON_POSITIONS	- no need to do this 20 times, we only have 1 weapon usually
				for ( int slotPos = 0; slotPos < iMaxSlotPositions; ++slotPos )
				{
					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, slotPos );
					if ( pWeapon )
					{
						lastSlotPos = slotPos;
					}
				}

				// Draw the weapons in this bucket
				for ( int slotPos = 0; slotPos <= lastSlotPos; ++slotPos )
				{
					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, slotPos );

					bool bSelectedWeapon = ( i == m_iSelectedSlot && slotPos == m_iSelectedBoxPosition );

					if ( pWeapon && pWeapon->VisibleInWeaponSelection() )
					{

						// draw selected weapon
						pModelPanels[i]->SetVisible(true);
						if (bSelectedWeapon)
						{
							xpos = xStartPos - m_flLargeBoxWide;
							pModelPanels[i]->SetBounds(xpos, ypos, m_flLargeBoxWide, m_flLargeBoxTall);
							ypos += (m_flLargeBoxTall + m_flBoxGap);
						}
						else
						{
							xpos = xStartPos - m_flSmallBoxWide;
							pModelPanels[i]->SetBounds(xpos, ypos, m_flSmallBoxWide, m_flSmallBoxTall);
							ypos += (m_flSmallBoxTall + m_flBoxGap);
						}
						pModelPanels[i]->SetWeapon(pWeapon, bSelectedWeapon, i);

					}
				}
			}
		}
	break;

	case HUDTYPE_BUCKETS:
	default:
		{
			// calculate where to start drawing
			int nTotalHeight = ( nNumSlots - 1 ) * ( m_flSmallBoxTall + m_flBoxGap ) + m_flLargeBoxTall;
			int xStartPos = GetWide() - m_flBoxGap - m_flRightMargin;
			int xpos = xStartPos;
			int ypos = ( GetTall() - nTotalHeight ) / 2;

			int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);

			// draw the bucket set
			// iterate over all the weapon slots
			for ( int i = 0; i < m_iMaxSlots; i++ )
			{
				pModelPanels[i]->SetVisible(false);

				if ( i == iActiveSlot )
				{
					xpos = xStartPos - m_flLargeBoxWide;

					bool bFirstItem = true;
					for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
					{

						C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i, slotpos);

						if ( !pWeapon )
							continue;

						if ( !pWeapon->VisibleInWeaponSelection() )
							continue;

						// draw selected weapon
						pModelPanels[i]->SetVisible(true);
						pModelPanels[i]->SetBounds(xpos, ypos, m_flLargeBoxWide, m_flLargeBoxTall);
						pModelPanels[i]->SetWeapon(pWeapon, true, i);

						xpos -= ( m_flLargeBoxWide + m_flBoxGap );
						bFirstItem = false;
					}

					ypos += ( m_flLargeBoxTall + m_flBoxGap );
				}
				else
				{
					xpos = xStartPos - m_flSmallBoxWide;

					// check to see if there is a weapons in this bucket
					if ( GetFirstPos( i ) )
					{

						C_BaseCombatWeapon *pWeapon = GetFirstPos( i );
						if ( !pWeapon )
							continue;

						// draw has weapon in slot
						pModelPanels[i]->SetVisible(true);
						pModelPanels[i]->SetBounds(xpos, ypos, m_flSmallBoxWide, m_flSmallBoxTall);
						pModelPanels[i]->SetWeapon(pWeapon, false, i);
						
						ypos += ( m_flSmallBoxTall + m_flBoxGap );	
					}
				}
			}
		}
		break;
	}	
}

void CHudWeaponSelection::DrawString( wchar_t *text, int xpos, int ypos, Color col, bool bCenter )
{
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextFont( m_hTextFont );

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;
	{
		for (wchar_t *pch = text; *pch != 0; pch++)
		{
			if (*pch == '\n') 
			{
				// newline character, drop to the next line
				if (slen > maxslen)
				{
					maxslen = slen;
				}
				slen = 0;
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				slen += surface()->GetCharacterWidth( m_hTextFont, *pch );
				charCount++;
			}
		}
	}
	if (slen > maxslen)
	{
		maxslen = slen;
	}

	int x = xpos;

	if ( bCenter )
	{
		x = xpos - slen * 0.5;
	}

	surface()->DrawSetTextPos( x, ypos );
	// adjust the charCount by the scan amount
	charCount *= m_flTextScan;
	for (wchar_t *pch = text; charCount > 0; pch++)
	{
		if (*pch == '\n')
		{
			// newline character, move to the next line
			surface()->DrawSetTextPos( x + ((m_flLargeBoxWide - slen) / 2), ypos + (surface()->GetFontTall(m_hTextFont) * 1.1f));
		}
		else if (*pch == '\r')
		{
			// do nothing
		}
		else
		{
			surface()->DrawUnicodeChar(*pch);
			charCount--;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number)
{
	BaseClass::DrawBox( x, y, wide, tall, color, normalizedAlpha / 255.0f );

	// draw the number
	if ( IsPC() && number >= 0)
	{
		Color numberColor = m_NumberColor;
		numberColor[3] *= normalizedAlpha / 255.0f;
		surface()->DrawSetTextColor(numberColor);
		surface()->DrawSetTextFont(m_hNumberFont);
		wchar_t wch = '0' + number;
		surface()->DrawSetTextPos(x + wide - m_flSelectionNumberXPos, y + m_flSelectionNumberYPos);
		surface()->DrawUnicodeChar(wch);
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawPlusStyleBox(int x, int y, int wide, int tall, bool bSelected, float normalizedAlpha, int number, bool bOutOfAmmo )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	/*
	int iMaterial;

	if ( bSelected && !bOutOfAmmo )
	{
		switch (pLocalPlayer->GetTeamNumber())
		{
			case TF_TEAM_RED:
				iMaterial = m_iBGImage_Red;
				break;

			case TF_TEAM_BLUE:
				iMaterial = m_iBGImage_Blue;
				break;

			default:
				iMaterial = m_iBGImage_Blue;
				break;
		}
	}
	else
	{
		iMaterial = m_iBGImage_Inactive;
	}

	vgui::surface()->DrawSetTexture( iMaterial );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );

	if ( bSelected )
	{
		float flExpandWide = m_flPlusStyleBoxWide * m_flPlusStyleExpandPercent * 0.5;
		float flExpandTall = m_flPlusStyleBoxTall * m_flPlusStyleExpandPercent * 0.5;

		vgui::surface()->DrawTexturedRect( x-flExpandWide, y-flExpandTall, x+wide+flExpandWide, y+tall+flExpandTall );
	}
	else
	{
		vgui::surface()->DrawTexturedRect( x, y, x+wide, y+tall );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);

	// load control settings...
	LoadControlSettings( "resource/UI/HudWeaponSelection.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	CBaseHudWeaponSelection::OpenSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("OpenWeaponSelectionMenu");
	m_iSelectedBoxPosition = 0;
	m_iSelectedSlot = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	m_flSelectionTime = 0;
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestNextSlot = MAX_WEAPON_SLOTS;
	int iLowestNextPosition = MAX_WEAPON_POSITIONS;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot > iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition > iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot < iLowestNextSlot || (weaponSlot == iLowestNextSlot && weaponPosition < iLowestNextPosition) )
				{
					iLowestNextSlot = weaponSlot;
					iLowestNextPosition = weaponPosition;
					pNextWeapon = pWeapon;
				}
			}
		}
	}

	return pNextWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the prior available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pPrevWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestPrevSlot = -1;
	int iLowestPrevPosition = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot < iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition < iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot > iLowestPrevSlot || (weaponSlot == iLowestPrevSlot && weaponPosition > iLowestPrevPosition) )
				{
					iLowestPrevSlot = weaponSlot;
					iLowestPrevPosition = weaponPosition;
					pPrevWeapon = pWeapon;
				}
			}
		}
	}

	return pPrevWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindNextWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindNextWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to start
		pNextWeapon = FindNextWeaponInWeaponSelection(-1, -1);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindPrevWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindPrevWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to end of weapon list
		pNextWeapon = FindPrevWeaponInWeaponSelection(MAX_WEAPON_SLOTS, MAX_WEAPON_POSITIONS);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon(i);
		
		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos )
			return pWeapon;
	}

	return NULL;
}

C_BaseCombatWeapon *CHudWeaponSelection::GetSelectedWeapon( void )
{ 
	if ( hud_fastswitch.GetInt() == 0 && m_iDemoModeSlot >= 0 )
	{
		C_BaseCombatWeapon *pWeapon = GetFirstPos( m_iDemoModeSlot );
		return pWeapon;
	}
	else
	{
		return m_hSelectedWeapon;
	}
}

void CHudWeaponSelection::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "localplayer_changeclass") == 0 )
	{
		for (int i = 0; i < m_iMaxSlots; i++)
		{
			pModelPanels[i]->SetVisible(false);
		}
		int nUpdateType = event->GetInt( "updateType" );
		bool bIsCreationUpdate = ( nUpdateType == DATA_UPDATE_CREATED );
		// Don't demo selection in minmode
		ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
		if ( !cl_hud_minmode.IsValid() || cl_hud_minmode.GetBool() == false )
		{
			if ( !bIsCreationUpdate )
			{
				m_flDemoStartTime = gpGlobals->curtime + tf_weapon_select_demo_start_delay.GetFloat();
			}
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::FastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// see where we should start selection
	int iPosition = -1;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	if ( pActiveWeapon && pActiveWeapon->GetSlot() == iWeaponSlot )
	{
		// start after this weapon
		iPosition = pActiveWeapon->GetPosition();
	}

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search for the weapon after the current one
	pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot, iPosition);
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iWeaponSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot, -1);
	}

	// see if we found a weapon that's different from the current and in the selected slot
	if ( pNextWeapon && pNextWeapon != pActiveWeapon && pNextWeapon->GetSlot() == iWeaponSlot )
	{
		// select the new weapon
		::input->MakeWeaponSelection( pNextWeapon );
	}
	else if ( pNextWeapon != pActiveWeapon )
	{
		// error sound
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	// kill any fastswitch display
	m_flSelectionTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::PlusTypeFastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int newSlot = m_iSelectedSlot;

	// Changing slot number does not necessarily mean we need to change the slot - the player could be
	// scrolling through the same slot but in the opposite direction. Slot pairs are 0,2 and 1,3 - so
	// compare the 0 bits to see if we're within a pair. Otherwise, reset the box to the zero position.
	if ( -1 == m_iSelectedSlot || ( ( m_iSelectedSlot ^ iWeaponSlot ) & 1 ) )
	{
		// Changing vertical/horizontal direction. Reset the selected box position to zero.
		m_iSelectedBoxPosition = 0;
		m_iSelectedSlot = iWeaponSlot;
	}
	else
	{
		// Still in the same horizontal/vertical direction. Determine which way we're moving in the slot.
		int increment = 1;
		if ( m_iSelectedSlot != iWeaponSlot )
		{
			// Decrementing within the slot. If we're at the zero position in this slot, 
			// jump to the zero position of the opposite slot. This also counts as our increment.
			increment = -1;
			if ( 0 == m_iSelectedBoxPosition )
			{
				newSlot = ( m_iSelectedSlot + 2 ) % 4;
				increment = 0;
			}
		}

		// Find out of the box position is at the end of the slot
		int lastSlotPos = -1;
		for ( int slotPos = 0; slotPos < MAX_WEAPON_POSITIONS; ++slotPos )
		{
			C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( newSlot, slotPos );
			if ( pWeapon )
			{
				lastSlotPos = slotPos;
			}
		}

		// Increment/Decrement the selected box position
		if ( m_iSelectedBoxPosition + increment <= lastSlotPos )
		{
			m_iSelectedBoxPosition += increment;
			m_iSelectedSlot = newSlot;
		}
		else
		{
			// error sound
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
			return;
		}
	}

	// Select the weapon in this position
	bool bWeaponSelected = false;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_iSelectedSlot, m_iSelectedBoxPosition );
	if ( pWeapon && CanBeSelectedInHUD( pWeapon ) )
	{
		if ( pWeapon != pActiveWeapon )
		{
			// Select the new weapon
			::input->MakeWeaponSelection( pWeapon );
			SetSelectedWeapon( pWeapon );
			bWeaponSelected = true;
		}
	}

	if ( !bWeaponSelected )
	{
		// Still need to set this to make hud display appear
		SetSelectedWeapon( pPlayer->GetActiveWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot > MAX_WEAPON_SLOTS )
		return;
	
	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;

	switch( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_FASTSWITCH:
		{
			FastWeaponSwitch( iSlot );
			return;
		}
		
	case HUDTYPE_PLUS:
		{
			if ( !IsInSelectionMode() )
			{
				// open the weapon selection
				OpenSelection();
			}
				
			PlusTypeFastWeaponSwitch( iSlot );
		}
		break;

	case HUDTYPE_BUCKETS:
		{
			int slotPos = 0;
			C_BaseCombatWeapon *pActiveWeapon = GetSelectedWeapon();

			// start later in the list
			if ( IsInSelectionMode() && pActiveWeapon && pActiveWeapon->GetSlot() == iSlot )
			{
				slotPos = pActiveWeapon->GetPosition() + 1;
			}

			// find the weapon in this slot
			pActiveWeapon = GetNextActivePos( iSlot, slotPos );
			if ( !pActiveWeapon )
			{
				pActiveWeapon = GetNextActivePos( iSlot, 0 );
			}
			
			if ( pActiveWeapon != NULL )
			{
				if ( !IsInSelectionMode() )
				{
					// open the weapon selection
					OpenSelection();
				}

				// Mark the change
				SetSelectedWeapon( pActiveWeapon );
				m_iDemoModeSlot = -1;
				m_flDemoStartTime = -1;
			}
		}
		break;

	default:
		break;
	}

	if( m_bPlaySelectionSounds )
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}
