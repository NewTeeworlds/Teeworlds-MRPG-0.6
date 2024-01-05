/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_GUILD_BANK_MANAGER_H
#define GAME_SERVER_COMPONENT_GUILD_BANK_MANAGER_H

// Forward declaration and alias
class CGS;
class CPlayer;
class CGuildData;

// This class represents a controller for a guild bank
class CGuildBankManager
{
	// Returns a pointer to the game server
	CGS* GS() const;

	CGuildData* m_pGuild {};
	int m_Bank {};

public:
	// Constructor
	CGuildBankManager(int Bank, CGuildData* pGuild) : m_pGuild(pGuild), m_Bank(Bank) {}

	// Get the current amount of currency in the bank
	void Set(int Value);
	int Get() const { return m_Bank; }
	bool Spend(int Value);
};

#endif