/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_GUILD_WAR_DATA_H
#define GAME_SERVER_COMPONENT_GUILD_WAR_DATA_H

#define TW_GUILDS_WARS_TABLE "tw_guilds_wars"

class CGS;
class CGuildData;
class CGuildWarHandler;

class CGuildWarData
{
	friend class CGuildWarHandler;
	CGuildWarHandler* m_pWarHandler {};
	CGuildData* m_pGuild {};
	CGuildData* m_pTargetGuild{};
	int m_Score {};

public:
	CGuildWarData(CGuildData* pGuild, CGuildData* pTargetGuild, int Score = 0);

	CGuildWarHandler* GetHandler() const { return m_pWarHandler; }
	CGuildData* GetTargetGuild() const { return m_pTargetGuild; }
	CGuildData* GetGuild() const { return m_pGuild; }
	int GetScore() const { return m_Score; }
	void AddScore(int Score);
};

class CGuildWarHandler : public MultiworldIdentifiableStaticData<std::deque <CGuildWarHandler*>>
{
	friend class CGuildWarData;
	time_t m_TimeUntilEnd {};
	std::pair<CGuildWarData*, CGuildWarData*> m_pWarData;

public:
	~CGuildWarHandler();

	static CGuildWarHandler* CreateElement()
	{
		auto pData = new CGuildWarHandler;
		return m_pData.emplace_back(pData);
	}

	void FormatTimeLeft(char* pBuf, int Size) const;
	void Init(const CGuildWarData& WarData1, const CGuildWarData& WarData2, time_t TimeUntilEnd);
	std::pair<CGuildWarData* , CGuildWarData*> GetWarData() const { return m_pWarData; }

	void Tick();
	void End();
	void Save() const;
};

#endif
