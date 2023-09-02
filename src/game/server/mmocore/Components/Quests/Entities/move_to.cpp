/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "move_to.h"

#include <game/server/gamecontext.h>

#include "game/server/mmocore/Components/Quests/QuestManager.h"

CEntityMoveTo::CEntityMoveTo(CGameWorld* pGameWorld, vec2 Pos, int ClientID, int QuestID, bool* pComplete)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_MOVE_TO, Pos)
{
	m_Pos = Pos;
	m_ClientID = ClientID;
	m_QuestID = QuestID;
	m_pComplete = pComplete;
	m_pPlayer = GS()->GetPlayer(m_ClientID, true, true);
	GameWorld()->InsertEntity(this);

	GS()->CreateLaserOrbite(&m_Pos, 4, EntLaserOrbiteType::DEFAULT, 0.0f, 80.0f);
}

void CEntityMoveTo::Reset()
{
	if(m_pPlayer && m_pPlayer->GetCharacter())
	{
		GS()->Mmo()->Quest()->UpdateSteps(m_pPlayer);
	}
	GS()->m_World.DestroyEntity(this);
}


void CEntityMoveTo::Tick()
{
	if(!m_pPlayer || !m_pPlayer->GetCharacter() || !total_size_vec2(m_PosTo) || !m_pComplete || (*m_pComplete == true))
	{
		Reset();
		return;
	}

	if(distance(m_pPlayer->m_ViewPos, m_Pos) < 60.0f)
	{
		(*m_pComplete) = true;
		m_pPlayer->GetQuest(m_QuestID)->SaveSteps();
		GS()->CreateDeath(m_Pos, m_ClientID);
		Reset();
	}
}

void CEntityMoveTo::Snap(int SnappingClient)
{
	if(m_ClientID != SnappingClient)
		return;

	CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(pPickup)
	{
		pPickup->m_X = (int)m_Pos.x;
		pPickup->m_Y = (int)m_Pos.y;
		pPickup->m_Type = POWERUP_ARMOR;
		pPickup->m_Subtype = 0;
	}
}