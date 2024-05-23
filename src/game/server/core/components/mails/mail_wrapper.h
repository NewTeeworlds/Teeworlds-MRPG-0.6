/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_MAIL_DATA_H
#define GAME_SERVER_CORE_COMPONENTS_MAIL_DATA_H

#include <engine/server.h>

#include <game/server/core/components/Inventory/ItemData.h>

class MailWrapper
{
	int m_AccountID {};
	std::string m_Title {};
	std::string m_Sender {};
	std::vector<std::string> m_vDescriptionLines {};
	CItemsContainer m_vAttachedItems {};

public:
	template <typename ... Ts>
	MailWrapper(const char* pFrom, int AccountID, const char* pTitle, Ts&&... args)
	{
		m_Sender = pFrom;
		m_AccountID = AccountID;
		m_Title = Instance::Server()->Localization()->Format("en", pTitle, std::forward<Ts>(args) ...);
	}

	template <typename ... Ts>
	MailWrapper& AddDescLine(const char* pDescline, Ts&&... args)
	{
		m_vDescriptionLines.push_back(Instance::Server()->Localization()->Format("en", pDescline, std::forward<Ts>(args) ...));
		return *this;
	}

	MailWrapper& AttachItem(CItem Item)
	{
		m_vAttachedItems.push_back(Item);
		return *this;
	}

	void Send();
};

#endif