/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_GUILDS_GUILD_MANAGER_H
#define GAME_SERVER_CORE_COMPONENTS_GUILDS_GUILD_MANAGER_H
#include <game/server/core/mmo_component.h>

#include "guild_data.h"

class CGuildManager : public MmoComponent
{
	~CGuildManager() override
	{
		for(const auto pHouse : CGuildHouse::Data())
			delete pHouse;
		for(const auto pGuild : CGuild::Data())
			delete pGuild;
		for(const auto pWar : CGuildWarHandler::Data())
			delete pWar;

		CGuild::Data().clear();
		CGuildHouse::Data().clear();
		CGuildWarHandler::Data().clear();
		CGuild::Data().shrink_to_fit();
		CGuildHouse::Data().shrink_to_fit();
		CGuildWarHandler::Data().shrink_to_fit();
	};

	void OnInit() override;
	void OnInitWorld(const char* pWhereLocalWorld) override;
	void OnTick() override;
	bool OnHandleTile(CCharacter* pChr) override;
	bool OnHandleVoteCommands(CPlayer* pPlayer, const char* CMD, int VoteID, int VoteID2, int Get, const char* GetText) override;
	bool OnHandleMenulist(CPlayer* pPlayer, int Menulist) override;
	void OnHandleTimePeriod(TIME_PERIOD Period) override;

	void InitWars() const;

public:
	void Create(CPlayer *pPlayer, const char *pGuildName) const;
	void Disband(GuildIdentifier GuildID) const;

private:
	void ShowMenu(int ClientID) const;
	void ShowUpgrades(CPlayer* pPlayer) const;
	void ShowDisband(CPlayer* pPlayer) const;
	void ShowHouseSell(CPlayer* pPlayer) const;
	void ShowRanksList(CPlayer* pPlayer) const;
	void ShowRankEdit(CPlayer* pPlayer, GuildRankIdentifier ID) const;
	void ShowFinder(CPlayer* pPlayer) const;
	void ShowFinderDetail(CPlayer* pPlayer, GuildIdentifier ID) const;
	void ShowLogs(CPlayer* pPlayer) const;
	void ShowMembershipList(CPlayer* pPlayer) const;
	void ShowMembershipEdit(CPlayer* pPlayer, int AccountID) const;
	void ShowRequests(CPlayer* pPlayer) const;
	void ShowBuyHouse(CPlayer* pPlayer, CGuildHouse* pHouse) const;
	void ShowDeclareWar(int ClientID) const;
	void ShowDoorsControl(CPlayer* pPlayer) const;
	void ShowFarmzonesControl(CPlayer* pPlayer) const;
	void ShowFarmzoneEdit(CPlayer* pPlayer, int FarmzoneID) const;

public:
	CGuildHouse* GetHouseByID(const GuildHouseIdentifier& ID) const;
	CGuildHouse* GetHouseByPos(vec2 Pos) const;
	CGuild* GetGuildByID(GuildIdentifier ID) const;
	CGuild* GetGuildByName(const char* pGuildname) const;
	CGuildHouse::CFarmzone* GetHouseFarmzoneByPos(vec2 Pos) const;
};

#endif