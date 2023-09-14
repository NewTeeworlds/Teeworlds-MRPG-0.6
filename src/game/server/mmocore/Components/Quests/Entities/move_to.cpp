/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "game/server/mmocore/Components/Bots/BotData.h"
#include "move_to.h"

#include <game/server/gamecontext.h>

#include "game/server/mmocore/Components/Quests/QuestManager.h"

constexpr float s_Radius = 32.0f;
constexpr unsigned int s_Particles = 4;

CEntityMoveTo::CEntityMoveTo(CGameWorld* pGameWorld, const QuestBotInfo::TaskRequiredMoveTo* pTaskMoveTo, int ClientID, int QuestID, bool* pComplete, std::deque < CEntityMoveTo* >* apCollection)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_MOVE_TO, pTaskMoveTo->m_Position), m_QuestID(QuestID), m_ClientID(ClientID), m_pTaskMoveTo(pTaskMoveTo)
{
	m_pComplete = pComplete;
	m_apCollection = apCollection;
	m_pPlayer = GS()->GetPlayer(m_ClientID, true, true);
	GameWorld()->InsertEntity(this);

	m_IDs.set_size(s_Particles);
	for(int i = 0; i < m_IDs.size(); i++)
		m_IDs[i] = Server()->SnapNewID();
}

CEntityMoveTo::~CEntityMoveTo()
{
	if(m_pPlayer && m_pPlayer->GetCharacter())
		GS()->Mmo()->Quest()->UpdateSteps(m_pPlayer);

	if(m_apCollection && !m_apCollection->empty())
	{
		for(auto it = m_apCollection->begin(); it != m_apCollection->end(); ++it)
		{
			if(mem_comp((*it), this, sizeof(CEntityMoveTo)) == 0)
			{
				m_apCollection->erase(it);
				break;
			}
		}
	}

	for(int i = 0; i < m_IDs.size(); i++)
		Server()->SnapFreeID(m_IDs[i]);
}

bool CEntityMoveTo::PressedFire() const
{
	// only for press fire type
	if(m_pTaskMoveTo->m_Type == QuestBotInfo::TaskRequiredMoveTo::Types::PRESS_FIRE)
	{
		if(m_pPlayer->GetCharacter()->m_ReloadTimer)
		{
			m_pPlayer->GetCharacter()->m_ReloadTimer = 0;
			return true;
		}
	}

	return false;
}

void CEntityMoveTo::Tick()
{
	if(!m_pTaskMoveTo || !m_pPlayer || !m_pPlayer->GetCharacter() || !m_pComplete || (*m_pComplete == true))
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	// check distance to check complete
	if(distance(m_pPlayer->m_ViewPos, m_Pos) > s_Radius)
		return;

	// function by success
	auto FuncSuccess = [this]()
	{
		if(!m_pTaskMoveTo->m_aTextChat.empty())
		{
			GS()->Chat(m_ClientID, m_pTaskMoveTo->m_aTextChat.c_str());
		}

		(*m_pComplete) = true;
		m_pPlayer->GetQuest(m_QuestID)->SaveSteps();
		GS()->CreateDeath(m_Pos, m_ClientID);
		GameWorld()->DestroyEntity(this);
	};

	// interact by text or press fire
	QuestBotInfo::TaskRequiredMoveTo::Types Type = m_pTaskMoveTo->m_Type;
	if((Type == QuestBotInfo::TaskRequiredMoveTo::Types::PRESS_FIRE && PressedFire())
		|| (Type == QuestBotInfo::TaskRequiredMoveTo::Types::USE_CHAT_MODE && m_pTaskMoveTo->m_aTextUseInChat == m_pPlayer->m_aLastMsg))
	{
		// clear last msg for correct check required item TODO: FIX (don't clear last msg)
		m_pPlayer->m_aLastMsg[0] = '\0';

		// first required item
		if(m_pTaskMoveTo->m_RequiredItem.IsValid())
		{
			ItemIdentifier ItemID = m_pTaskMoveTo->m_RequiredItem.GetID();
			int RequiredValue = m_pTaskMoveTo->m_RequiredItem.GetValue();

			// check required value
			if(!m_pPlayer->SpendCurrency(RequiredValue, ItemID))
				return;

			// remove item
			CPlayerItem* pPlayerItem = m_pPlayer->GetItem(ItemID);
			GS()->Chat(m_ClientID, "You've used on the point {STR}x{INT}", pPlayerItem->Info()->GetName(), RequiredValue);
		}

		// secound pickup item
		if(m_pTaskMoveTo->m_PickupItem.IsValid())
		{
			ItemIdentifier ItemID = m_pTaskMoveTo->m_PickupItem.GetID();
			int PickupValue = m_pTaskMoveTo->m_PickupItem.GetValue();
			CPlayerItem* pPlayerItem = m_pPlayer->GetItem(ItemID);

			GS()->Chat(m_ClientID, "You've picked up {STR}x{INT}.", pPlayerItem->Info()->GetName(), PickupValue);
			pPlayerItem->Add(PickupValue);
		}

		// finish success
		FuncSuccess();
		return;
	}

	// only move it
	if(Type == QuestBotInfo::TaskRequiredMoveTo::Types::MOVE_ONLY)
	{
		// finish success
		FuncSuccess();
		return;
	}

	// handle broadcast
	HandleBroadcastInformation();
}

void CEntityMoveTo::HandleBroadcastInformation() const
{
	// var
	auto& pPickupItem = m_pTaskMoveTo->m_PickupItem;
	auto& pRequireItem = m_pTaskMoveTo->m_RequiredItem;
	const auto Type = m_pTaskMoveTo->m_Type;

	// text information
	dynamic_string Buffer;
	if(pRequireItem.IsValid())
	{
		const char* pLang = m_pPlayer->GetLanguage();
		CPlayerItem* pPlayerItem = m_pPlayer->GetItem(pRequireItem.GetID());

		GS()->Server()->Localization()->Format(Buffer, pLang, "- Required [{STR}x{VAL}({VAL})]", pPlayerItem->Info()->GetName(), pRequireItem.GetValue(), pPlayerItem->GetValue());
		Buffer.append("\n");
	}
	if(pPickupItem.IsValid())
	{
		const char* pLang = m_pPlayer->GetLanguage();
		CPlayerItem* pPlayerItem = m_pPlayer->GetItem(pPickupItem.GetID());

		GS()->Server()->Localization()->Format(Buffer, pLang, "- Pick up [{STR}x{VAL}({VAL})]", pPlayerItem->Info()->GetName(), pPickupItem.GetValue(), pPlayerItem->GetValue());
		Buffer.append("\n");
	}

	// select by type
	if(Type == QuestBotInfo::TaskRequiredMoveTo::Types::USE_CHAT_MODE)
	{
		GS()->Broadcast(m_ClientID, BroadcastPriority::MAIN_INFORMATION, 10, "Open the chat and enter the text\n(without the brackets)\n'{STR}'\n{STR}", m_pTaskMoveTo->m_aTextUseInChat.c_str(), Buffer.buffer());
	}
	else if(Type == QuestBotInfo::TaskRequiredMoveTo::Types::PRESS_FIRE)
	{
		GS()->Broadcast(m_ClientID, BroadcastPriority::MAIN_INFORMATION, 10, "Press 'Fire', to interact.\n{STR}", Buffer.buffer());
	}
}

void CEntityMoveTo::Snap(int SnappingClient)
{
	if(m_ClientID != SnappingClient)
		return;

	for(int i = 0; i < m_IDs.size(); i++)
	{
		CNetObj_Projectile* pObj = static_cast<CNetObj_Projectile*>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDs[i], sizeof(CNetObj_Projectile)));
		if(!pObj)
			return;

		pObj->m_Type = WEAPON_HAMMER;
		pObj->m_X = m_Pos.x + frandom_num(-s_Radius/1.5f, s_Radius/1.5f);
		pObj->m_Y = m_Pos.y + frandom_num(-s_Radius/1.5f, s_Radius/1.5f);
		pObj->m_StartTick = Server()->Tick() - 3;
	}
}