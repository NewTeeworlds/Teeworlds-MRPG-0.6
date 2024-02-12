﻿#include "vote_wrapper.h"

#include <game/server/gamecontext.h>

const char* VOTE_LINE_DEF = "\u257E\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u257C";

namespace Border
{
	enum BorderSymbol { Beggin, Middle, MiddleOption, Level, End, Num };
	inline constexpr const char* g_pSimpleBorder[Num] = { "\u256D", "\u2502", "\u251C", "\u2508", "\u2570" };
	inline constexpr const char* g_pDoubleBorder[Num] = { "\u2554", "\u2551", "\u2560", "\u2550", "\u255A" };
	inline constexpr const char* g_pStrictBorder[Num] = { "\u250C", "\u2502", "\u251C", "\u2508", "\u2514" };
	inline constexpr const char* g_pStrictBoldBorder[Num] = { "\u250F", "\u2503", "\u2523", "\u2509", "\u2517" };

	static constexpr const char* get(BorderSymbol Border, int Flags)
	{
		if(Flags & VWFLAG_BSTYLE_SIMPLE) { return g_pSimpleBorder[Border]; }
		if(Flags & VWFLAG_BSTYLE_DOUBLE) { return g_pDoubleBorder[Border]; }
		if(Flags & VWFLAG_BSTYLE_STRICT) { return g_pStrictBorder[Border]; }
		if(Flags & VWFLAG_BSTYLE_STRICT_BOLD) { return g_pStrictBoldBorder[Border]; }
		return "";
	}
}

CVoteGroup::CVoteGroup(int ClientID, int Flags) : m_Flags(Flags), m_ClientID(ClientID)
{
	m_pGS = (CGS*)Instance::GameServerPlayer(ClientID);
	m_GroupID = (int)CVoteWrapper::Data()[m_ClientID].size();
	m_TitleIsSet = false;
	m_CurrentDepth = 0;
	m_GroupSize = 0;
}

// Function to add a vote title implementation with variable arguments
void CVoteGroup::SetVoteTitleImpl(const char* pCmd, int SettingsID1, int SettingsID2, const char* pText, ...)
{
	// Check if the player is valid
	CPlayer* pPlayer = GS()->GetPlayer(m_ClientID);
	if(!pPlayer)
		return;

	// Format the text using the player's language and additional arguments
	va_list VarArgs;
	va_start(VarArgs, pText);
	dynamic_string Buffer;
	Instance::Server()->Localization()->Format_VL(Buffer, pPlayer->GetLanguage(), pText, VarArgs);
	va_end(VarArgs);

	const char* pAppend = "\0";
	if(m_Flags & (VWFLAG_DEFAULT_CLOSE | VWFLAG_DEFAULT_OPEN | VWFLAG_UNIQUE))
	{
		const bool HiddenTab = pPlayer->m_VotesData.EmplaceHidden(m_GroupID, m_Flags)->m_Value;
		pAppend = HiddenTab ? "\u21BA " : "\u27A4 ";
		SettingsID1 = m_GroupID;
		pCmd = "HIDDEN";
	}

	char aBufText[VOTE_DESC_LENGTH];
	str_format(aBufText, sizeof(aBufText), "%s%s", pAppend, Buffer.buffer());
	Buffer.clear();

	// Check if the player's language is "ru" or "uk" and convert aBufText to Latin if true
	if(str_comp(pPlayer->GetLanguage(), "ru") == 0 || str_comp(pPlayer->GetLanguage(), "uk") == 0)
		str_translation_cyrlic_to_latin(aBufText);

	// Create a new VoteOption with the values from aBufText, pCmd, SettingsID1, and SettingsID2
	CVoteOption Vote;
	str_copy(Vote.m_aDescription, aBufText, sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, pCmd, sizeof(Vote.m_aCommand));
	Vote.m_SettingID = SettingsID1;
	Vote.m_SettingID2 = SettingsID2;

	// Add title to front or update the title if it already exists
	if(m_vpVotelist.empty() || !m_TitleIsSet)
	{
		m_TitleIsSet = true;
		m_vpVotelist.push_front(Vote);
	}
	else
	{
		m_vpVotelist.front() = Vote;
	}
}

// Function to add a vote implementation with variable arguments
void CVoteGroup::AddVoteImpl(const char* pCmd, int Settings1, int Settings2, const char* pText, ...)
{
	// Check if the player is valid
	CPlayer* pPlayer = GS()->GetPlayer(m_ClientID);
	if(!pPlayer)
		return;

	// Check if the hidden vote object exists and if its value is true
	CVotePlayerData::VoteGroupHidden* pHidden = pPlayer->m_VotesData.GetHidden(m_GroupID);
	if(pHidden && pHidden->m_Value)
		return;

	// Format the text using the player's language and additional arguments
	va_list VarArgs;
	va_start(VarArgs, pText);
	dynamic_string Buffer;
	Instance::Server()->Localization()->Format_VL(Buffer, pPlayer->GetLanguage(), pText, VarArgs);
	va_end(VarArgs);

	char aBufText[VOTE_DESC_LENGTH];
	str_format(aBufText, sizeof(aBufText), "%s%s", str_comp(pCmd, "null") != 0 ? "\u257E " : "\0", Buffer.buffer());
	Buffer.clear();

	// Check if the player's language is "ru" or "uk" and convert aBufText to Latin if true
	if(str_comp(pPlayer->GetLanguage(), "ru") == 0 || str_comp(pPlayer->GetLanguage(), "uk") == 0)
		str_translation_cyrlic_to_latin(aBufText);

	// Create a new VoteOption with the values from aBufText, pCmd, Settings1, and Settings2
	CVoteOption Vote;
	str_copy(Vote.m_aDescription, aBufText, sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, pCmd, sizeof(Vote.m_aCommand));
	Vote.m_SettingID = Settings1;
	Vote.m_SettingID2 = Settings2;
	Vote.m_Depth = m_CurrentDepth;

	// Add the VoteOption to the player's votes
	m_GroupSize++;
	m_vpVotelist.emplace_back(Vote);
}

// This function is used to add a line
void CVoteGroup::AddLineImpl()
{
	// Create a new VoteOption with the values
	CVoteOption Vote;
	str_copy(Vote.m_aDescription, VOTE_LINE_DEF, sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, "null", sizeof(Vote.m_aCommand));
	Vote.m_Line = true;

	// Add the VoteOption to the player's votes
	m_GroupSize++;
	m_vpVotelist.emplace_back(Vote);
}

// This function adds the implementation for the AddBackpage method in the CVoteGroup class.
void CVoteGroup::AddBackpageImpl()
{
	// Create a new VoteOption with the values
	CVoteOption Vote;
	str_copy(Vote.m_aDescription, "\u21A9 Backpage", sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, "BACK", sizeof(Vote.m_aCommand));

	// Add the VoteOption to the player's votes
	AddLineImpl();
	m_vpVotelist.emplace_back(Vote);
}

// This function adds an empty line
void CVoteGroup::AddEmptylineImpl()
{
	// Check if the last player vote is an line
	CVoteOption Vote;
	str_copy(Vote.m_aDescription, "\0", sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, "null", sizeof(Vote.m_aCommand));

	// Add the VoteOption to the player's votes
	m_vpVotelist.emplace_back(Vote);
}

// Function to add an item value information to the CVoteGroup
void CVoteGroup::AddItemValueImpl(int ItemID)
{
	if(CPlayer* pPlayer = GS()->GetPlayer(m_ClientID))
		AddVoteImpl("null", NOPE, NOPE, "You have {VAL} {STR}", pPlayer->GetItem(ItemID)->GetValue(), GS()->GetItemInfo(ItemID)->GetName());
}

// This function is responsible for rebuilding the votes for a specific client.
void CVoteWrapper::RebuildVotes(int ClientID)
{
	CGS* pGS = (CGS*)Instance::GameServerPlayer(ClientID);
	CPlayer* pPlayer = pGS->GetPlayer(ClientID);
	if(!pPlayer)
		return;

	// If the player has no votes, give a chance to come back
	// Code format: {INT}x{INT} (CurrentMenuID, LastMenuID)
	if(m_pData[ClientID].empty())
	{
		CVotePlayerData* pVotesData = &pPlayer->m_VotesData;
		CVoteWrapper VError(ClientID, VWFLAG_BSTYLE_SIMPLE, "Error");
		VError.Add("The voting list is empty").BeginDepthList().Add("Probably a server error").EndDepthList()
			.Add("Report the error code #{INT}x{INT}", pVotesData->GetCurrentMenuID(), pVotesData->GetLastMenuID());
		pVotesData->SetLastMenuID(MENU_MAIN);
		CVoteWrapper::AddBackpage(ClientID);
	}

	// Rebuild the vote options
	CVoteOption* pLastVoteOption = nullptr;
	for(auto pItem : m_pData[ClientID])
	{
		// Add empty vote for empty hidden group
		if(pItem->IsEmpty() && pItem->m_TitleIsSet)
		{
			pItem->m_Flags = VWFLAG_DISABLED;
			pItem->AddVoteImpl("null", NOPE, NOPE, "The list is empty");
		}

		for(auto iter = pItem->m_vpVotelist.begin(); iter != pItem->m_vpVotelist.end();)
		{
			// There should not be two lines in a row, or if there are three lines, the middle should be empty, aesthetics.
			// Rebuild the vote option to have an empty line aesthetic
			if(pLastVoteOption && pLastVoteOption->m_Line && (*iter).m_Line)
			{
				iter = pItem->m_vpVotelist.erase(iter);
				continue;
			}

			// Rebuild the vote options to aesthetic style
			if(pItem->m_Flags & (VWFLAG_BSTYLE_SIMPLE | VWFLAG_BSTYLE_DOUBLE | VWFLAG_BSTYLE_STRICT | VWFLAG_BSTYLE_STRICT_BOLD))
			{
				// Skip the vote option if the player has hidden the vote option
				CVotePlayerData::VoteGroupHidden* pHidden = pPlayer->m_VotesData.GetHidden(pItem->m_GroupID);
				if(!pHidden || (pHidden && !pHidden->m_Value))
				{
					const int& Flags = pItem->m_Flags;
					auto pFront = &pItem->m_vpVotelist.front();
					auto pBack = &pItem->m_vpVotelist.back();

					// Append border to the vote option (can outside but)
					dynamic_string Buffer;
					if(&(*iter) == pFront)
						Buffer.append(get(Border::Beggin, Flags));
					else if(&(*iter) == pBack)
						Buffer.append(get(Border::End, Flags));
					else if(str_comp((*iter).m_aCommand, "null") == 0 && (*iter).m_Depth <= 0 && !(*iter).m_Line)
						Buffer.append(get(Border::Middle, Flags));
					else
						Buffer.append(get(Border::MiddleOption, Flags));

					// Display the level of the vote option
					// Dissable for line is a line as it is
					if(!(*iter).m_Line && (*iter).m_Depth > 0)
					{
						for(int i = 0; i < (*iter).m_Depth; i++)
							Buffer.append(get(Border::Level, Flags));
					}

					if(str_comp((*iter).m_aDescription, VOTE_LINE_DEF) != 0 && str_comp((*iter).m_aCommand, "null") == 0)
						Buffer.append(" ");

					// Save changes
					char aRebuildBuffer[VOTE_DESC_LENGTH];
					str_copy(aRebuildBuffer, (*iter).m_aDescription, sizeof(aRebuildBuffer));
					str_format((*iter).m_aDescription, sizeof((*iter).m_aDescription), "%s%s", Buffer.buffer(), aRebuildBuffer);
				}
			}

			// Send the vote option to the client
			CNetMsg_Sv_VoteOptionAdd OptionMsg;
			OptionMsg.m_pDescription = (*iter).m_aDescription;
			pGS->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);

			// Set the last vote option to the current vote option
			pLastVoteOption = &(*iter);
			++iter;
		}
	}
}

// This function returns the vote option for a specific action for a given client
CVoteOption* CVoteWrapper::GetOptionVoteByAction(int ClientID, const char* pActionName)
{
	// Get a reference to the data associated with the given ClientID
	auto& vData = CVoteWrapper::Data()[ClientID];

	// Find the vote option with the matching action name
	for(auto& iterGroup : vData)
	{
		auto iter = std::find_if(iterGroup->m_vpVotelist.begin(), iterGroup->m_vpVotelist.end(), [pActionName](const CVoteOption& vote)
		{
			return (str_comp_nocase(pActionName, vote.m_aDescription) == 0);
		});

		if(iter != iterGroup->m_vpVotelist.end())
		{
			return &(*iter);
		}
	}

	// If no matching vote option is found, return nullptr
	return nullptr;
}

// Function to create or update a hidden vote group
CVotePlayerData::VoteGroupHidden* CVotePlayerData::EmplaceHidden(int ID, int Type)
{
	// Check if the vote group already exists and has the same type
	auto& CurrentHidden = m_aHiddenGroup[m_CurrentMenuID];
	if(CurrentHidden.find(ID) != CurrentHidden.end() && CurrentHidden[ID].m_Flag == Type)
		return &CurrentHidden[ID];

	bool Value = (Type & VWFLAG_DEFAULT_CLOSE) || (Type & VWFLAG_UNIQUE);
	CurrentHidden[ID] = { Value, Type };
	return &CurrentHidden[ID];
}

// Function to get a hidden vote group
CVotePlayerData::VoteGroupHidden* CVotePlayerData::GetHidden(int ID)
{
	auto& CurrentHidden = m_aHiddenGroup[m_CurrentMenuID];
	auto it = CurrentHidden.find(ID);
	if(it != CurrentHidden.end())
		return &it->second;

	return nullptr;
}

// Function to reset the hidden vote groups for a menu
void CVotePlayerData::ResetHidden(int MenuID)
{
	// If the hidden group is empty, return
	auto& HiddenGroup = m_aHiddenGroup[MenuID];
	if(HiddenGroup.empty())
		return;

	// Iterate over each hidden vote group
	for(auto& [ID, Hide] : HiddenGroup)
	{
		if(Hide.m_Flag & VWFLAG_UNIQUE)
			Hide.m_Value = true;
	}
}

// This function is responsible for updating the vote data for a player in a separate thread.
void CVotePlayerData::ThreadVoteUpdater(CVotePlayerData* pData)
{
	// Required sleep for to work properly and correct output of texts and data
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	if(!pData || !pData->m_pPlayer)
		return;

	// TODO: Same prepared vote thread update
	pData->m_VoteUpdaterStatus.store(STATE_UPDATER::DONE);
}

// This function applies the vote updater data to the player's vote data
void CVotePlayerData::ApplyVoteUpdaterData()
{
	if(m_VoteUpdaterStatus == STATE_UPDATER::DONE)
	{
		ClearVotes();
		int ClientID = m_pPlayer->GetCID();
		m_pGS->Core()->OnPlayerHandleMainMenu(ClientID, m_CurrentMenuID);
		CVoteWrapper::RebuildVotes(ClientID);
		m_VoteUpdaterStatus.store(STATE_UPDATER::WAITING);
	}
}

void CVotePlayerData::UpdateVotes(int MenuID)
{
	m_CurrentMenuID = MenuID;

	if(m_VoteUpdaterStatus == STATE_UPDATER::WAITING)
	{
		m_VoteUpdaterStatus.store(STATE_UPDATER::RUNNING);
		m_VoteUpdater = std::thread(&CVotePlayerData::ThreadVoteUpdater, this);
		m_VoteUpdater.detach();
	}
}

void CVotePlayerData::UpdateVotesIf(int MenuID)
{
	if(m_CurrentMenuID == MenuID)
		UpdateVotes(MenuID);
}

void CVotePlayerData::ClearVotes() const
{
	int ClientID = m_pPlayer->GetCID();
	CVoteWrapper::Data()[ClientID].clear();

	// send vote options
	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Instance::Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);
}

// Function to parse default system commands
bool CVotePlayerData::ParsingDefaultSystemCommands(const char* CMD, const int VoteID, const int VoteID2, int Get, const char* Text)
{
	// Check if the command is "MENU"
	if(PPSTR(CMD, "MENU") == 0)
	{
		// Set the temporary menu integer to the specified value
		m_TempMenuInteger = VoteID2;

		// Update the votes for the player
		ResetHidden(VoteID);
		UpdateVotes(VoteID);
		return true;
	}

	// Check if the command is "BACK"
	if(PPSTR(CMD, "BACK") == 0)
	{
		// Update the votes for the player
		UpdateVotes(m_LastMenuID);
		return true;
	}

	// Check if the command is "HIDDEN"
	if(PPSTR(CMD, "HIDDEN") == 0)
	{
		// If the hidden vote group does not exist, return true
		if(VoteGroupHidden* pHidden = GetHidden(VoteID))
		{
			// If the hidden vote group has the VWFLAG_UNIQUE flag and its ID is not the specified ID, set its value to true
			if(pHidden->m_Flag & VWFLAG_UNIQUE)
			{
				for(auto& [ID, Hide] : m_aHiddenGroup[m_CurrentMenuID])
				{
					if(Hide.m_Flag & VWFLAG_UNIQUE && ID != VoteID)
						Hide.m_Value = true;
				}
			}

			// Toggle the value of the hidden vote group
			pHidden->m_Value = !pHidden->m_Value;
			UpdateCurrentVotes();
		}
		return true;
	}

	return false;
}