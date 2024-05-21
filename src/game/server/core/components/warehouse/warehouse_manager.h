/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_WAREHOUSE_CORE_H
#define GAME_SERVER_COMPONENT_WAREHOUSE_CORE_H
#include <game/server/core/mmo_component.h>

#include "warehouse_data.h"

class CWarehouseManager : public MmoComponent
{
	~CWarehouseManager() override
	{
		for(auto pData : CWarehouse::Data())
			delete pData;
		CWarehouse::Data().clear();
	}

	void OnInit() override;
	void OnTick() override;
	bool OnHandleTile(CCharacter* pChr) override;
	bool OnHandleMenulist(CPlayer* pPlayer, int Menulist) override;
	bool OnHandleVoteCommands(CPlayer* pPlayer, const char* CMD, int VoteID, int VoteID2, int Get, const char* GetText) override;

	// vote list's menus
	void ShowWarehouseList(CPlayer* pPlayer, CWarehouse* pWarehouse) const;
	void ShowTradeList(CWarehouse* pWarehouse, CPlayer* pPlayer, const char* TypeName, ItemType Type) const;
	void ShowTrade(CPlayer* pPlayer, CWarehouse* pWarehouse, const TradeIdentifier& TradeID) const;

	// buy item for player from warehouse
	bool BuyItem(CPlayer* pPlayer, CWarehouse* pWarehouse, TradeIdentifier ID) const;

	// sell item for warehouse from player
	bool SellItem(CPlayer* pPlayer, CWarehouse* pWarehouse, TradeIdentifier ID, int Value) const;

public:
	// finding a warehouse by position
	CWarehouse* GetWarehouse(vec2 Pos) const;

	// finding a warehouse by ID
	CWarehouse* GetWarehouse(WarehouseIdentifier ID) const;
};

#endif