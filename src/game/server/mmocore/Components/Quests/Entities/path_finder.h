/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_MMOCORE_COMPONENTS_QUESTS_ENTITIES_PATH_FINDER_H
#define GAME_SERVER_MMOCORE_COMPONENTS_QUESTS_ENTITIES_PATH_FINDER_H

#include <game/server/entity.h>

class CEntityPathFinder : public CEntity
{
public:
	int m_ClientID;
	int m_WorldID;
	bool* m_pComplete;
	class CPlayer* m_pPlayer;
	std::deque < CEntityPathFinder* >* m_apCollection;

	CEntityPathFinder(CGameWorld* pGameWorld, vec2 Pos, int WorldID, int ClientID, bool* pComplete, std::deque < CEntityPathFinder* >* apCollection);
	~CEntityPathFinder() override;

	void Tick() override;
	void Snap(int SnappingClient) override;
	
	int GetClientID() const { return m_ClientID; }
};

#endif