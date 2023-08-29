/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "QuestManager.h"

#include <game/server/gamecontext.h>

#include <game/server/mmocore/Components/Dungeons/DungeonManager.h>
#include <game/server/mmocore/Components/Worlds/WorldManager.h>

CGS* CQuestData::GS() const
{
	return (CGS*)Server()->GameServerPlayer(m_ClientID);
}

CPlayer* CQuestData::GetPlayer() const
{
	if(m_ClientID >= 0 && m_ClientID < MAX_PLAYERS)
	{
		return GS()->m_apPlayers[m_ClientID];
	}
	return nullptr;
}

CQuestDataInfo& CQuestData::Info() const { return CQuestDataInfo::ms_aDataQuests[m_ID]; }
std::string CQuestData::GetJsonFileName() const { return Info().GetJsonFileName(GetPlayer()->Acc().m_UserID); }

void CQuestData::InitSteps()
{
	if(m_State != QuestState::ACCEPT || !GetPlayer())
		return;

	// checking dir
	if(!fs_is_dir("server_data/quest_tmp"))
	{
		fs_makedir("server_data");
		fs_makedir("server_data/quest_tmp");
	}

	// initialized quest steps
	m_Step = 1;
	Info().InitPlayerDefaultSteps(m_aPlayerSteps);

	nlohmann::json JsonQuestData;
	JsonQuestData["current_step"] = m_Step;
	for(auto& pStep : m_aPlayerSteps)
	{
		if(pStep.second.m_Bot.m_HasAction)
		{
			JsonQuestData["steps"].push_back(
			{
				{ "subbotid", pStep.second.m_Bot.m_SubBotID },
				{ "state", pStep.second.m_StepComplete }
			});

			if(!pStep.second.m_Bot.m_RequiredDefeat.empty())
			{
				for(auto& p : pStep.second.m_Bot.m_RequiredDefeat)
				{
					JsonQuestData["steps"].back()["defeat"].push_back(
						{
							{ "id", p.m_BotID },
							{ "count", 0 },
						});

					pStep.second.m_aMobProgress[p.m_BotID] = 0;
				}
			}
		}

		pStep.second.UpdateBot();
		pStep.second.CreateStepArrow(m_ClientID);
	}

	// save file
	IOHANDLE File = io_open(GetJsonFileName().c_str(), IOFLAG_WRITE);
	if(!File)
		return;

	std::string Data = JsonQuestData.dump();
	io_write(File, Data.c_str(), Data.length());
	io_close(File);
}

void CQuestData::LoadSteps()
{
	if(m_State != QuestState::ACCEPT || !GetPlayer())
		return;

	// loading file is not open pereinitilized steps
	IOHANDLE File = io_open(GetJsonFileName().c_str(), IOFLAG_READ);
	if(!File)
	{
		InitSteps();
		return;
	}

	const int FileSize = (int)io_length(File) + 1;
	char* pFileData = (char*)malloc(FileSize);
	mem_zero(pFileData, FileSize);
	io_read(File, pFileData, FileSize);

	// close and clear
	nlohmann::json JsonQuestData = nlohmann::json::parse(pFileData);
	mem_free(pFileData);
	io_close(File);

	// loading steps
	Info().InitPlayerDefaultSteps(m_aPlayerSteps);
	m_Step = JsonQuestData.value("current_step", 1);
	for(auto& pStep : JsonQuestData["steps"])
	{
		const int SubBotID = pStep.value("subbotid", 0);
		m_aPlayerSteps[SubBotID].m_StepComplete = pStep.value("state", false);
		m_aPlayerSteps[SubBotID].m_ClientQuitting = false;

		if(m_aPlayerSteps[SubBotID].m_StepComplete || pStep.find("defeat") == pStep.end())
			continue;

		for(auto& p : pStep["defeat"])
			m_aPlayerSteps[SubBotID].m_aMobProgress[p.value("id", 0)] = p.value("count", 0);
	}

	// update step bot's
	for(auto& pStep : m_aPlayerSteps)
	{
		if(!pStep.second.m_StepComplete)
		{
			pStep.second.UpdateBot();
			pStep.second.CreateStepArrow(m_ClientID);
		}
	}
}

void CQuestData::SaveSteps()
{
	if(m_State != QuestState::ACCEPT)
		return;

	// set json structure
	nlohmann::json JsonQuestData;
	JsonQuestData["current_step"] = m_Step;
	for(auto& pStep : m_aPlayerSteps)
	{
		if(pStep.second.m_Bot.m_HasAction)
		{
			JsonQuestData["steps"].push_back(
				{
					{ "subbotid", pStep.second.m_Bot.m_SubBotID },
					{ "state", pStep.second.m_StepComplete }
				});

			for(auto& p : pStep.second.m_aMobProgress)
			{
				JsonQuestData["steps"].back()["defeat"].push_back(
					{
						{ "id", p.first },
						{ "count", p.second },
					});
			}
		}
	}

	// replace file
	IOHANDLE File = io_open(GetJsonFileName().c_str(), IOFLAG_WRITE);
	if(!File)
		return;

	std::string Data = JsonQuestData.dump();
	io_write(File, Data.c_str(), Data.length());
	io_close(File);
}

void CQuestData::ClearSteps()
{
	for(auto& pStepBot : m_aPlayerSteps)
	{
		pStepBot.second.UpdateBot();
		pStepBot.second.CreateStepArrow(m_ClientID);
	}

	m_aPlayerSteps.clear();
	fs_remove(GetJsonFileName().c_str());
}

bool CQuestData::Accept()
{
	if(m_State != QuestState::NO_ACCEPT)
		return false;

	int ClientID = GetPlayer()->GetCID();
	CGS* pGS = (CGS*)Instance::GetServer()->GameServerPlayer(ClientID);

	// init quest
	m_State = QuestState::ACCEPT;
	Database->Execute<DB::INSERT>("tw_accounts_quests", "(QuestID, UserID, Type) VALUES ('%d', '%d', '%d')", m_ID, GetPlayer()->Acc().m_UserID, m_State);

	// init steps
	InitSteps();

	// information
	const int QuestsSize = Info().GetQuestStorySize();
	const int QuestPosition = Info().GetQuestStoryPosition();
	pGS->Chat(ClientID, "------ Quest story [{STR}] ({INT}/{INT}) ------", Info().GetStory(), QuestPosition, QuestsSize);
	pGS->Chat(ClientID, "Name: \"{STR}\"", Info().GetName());
	pGS->Chat(ClientID, "Reward: \"Gold {VAL}, Experience {INT}\".", Info().m_Gold, Info().m_Exp);
	pGS->CreatePlayerSound(ClientID, SOUND_CTF_CAPTURE);
	return true;
}

void CQuestData::Finish()
{
	if(m_State != QuestState::ACCEPT)
		return;
	
	CGS* pGS = (CGS*)Instance::GetServer()->GameServerPlayer(m_ClientID);

	// finish quest
	m_State = QuestState::FINISHED;
	Database->Execute<DB::UPDATE>("tw_accounts_quests", "Type = '%d' WHERE QuestID = '%d' AND UserID = '%d'", m_State, m_ID, GetPlayer()->Acc().m_UserID);

	// clear steps
	ClearSteps();

	// awards and write about completion
	GetPlayer()->AddMoney(Info().m_Gold);
	GetPlayer()->AddExp(Info().m_Exp);
	pGS->Chat(-1, "{STR} completed the \"{STR} - {STR}\".", pGS->Server()->ClientName(m_ClientID), Info().m_aStoryLine, Info().m_aName);
	pGS->ChatDiscord(DC_SERVER_INFO, pGS->Server()->ClientName(m_ClientID), "Completed ({STR} - {STR})", Info().m_aStoryLine, Info().m_aName);

	// notify whether the after quest has opened something new
	pGS->Mmo()->WorldSwap()->NotifyUnlockedZonesByQuest(GetPlayer(), m_ID);
	pGS->Mmo()->Dungeon()->NotifyUnlockedDungeonsByQuest(GetPlayer(), m_ID);

	// save player stats and accept next story quest
	pGS->Mmo()->SaveAccount(GetPlayer(), SAVE_STATS);
	pGS->Mmo()->Quest()->AcceptNextStoryQuest(GetPlayer(), m_ID);

	// effect's
	pGS->CreateText(nullptr, false, vec2(GetPlayer()->m_ViewPos.x, GetPlayer()->m_ViewPos.y - 70), vec2(0, -0.5), 30, "COMPLECTED");
}

void CQuestData::CheckAvailableNewStep()
{
	// check whether the active steps is complete
	if(std::find_if(m_aPlayerSteps.begin(), m_aPlayerSteps.end(), [this](std::pair <const int, CPlayerQuestStepDataInfo> &p)
	{ return (p.second.m_Bot.m_Step == m_Step && !p.second.m_StepComplete && p.second.m_Bot.m_HasAction); }) != m_aPlayerSteps.end())
		return;

	m_Step++;

	// check if all steps have been completed
	bool FinalStep = true;
	for(auto& pStepBot : m_aPlayerSteps)
	{
		if(!pStepBot.second.m_StepComplete && pStepBot.second.m_Bot.m_HasAction)
			FinalStep = false;

		pStepBot.second.UpdateBot();
		pStepBot.second.CreateStepArrow(m_ClientID);
	}

	// finish the quest or update the step
	if(FinalStep)
	{
		Finish();

		CGS* pGS = (CGS*)Instance::GetServer()->GameServerPlayer(m_ClientID);

		pGS->StrongUpdateVotes(m_ClientID, MENU_JOURNAL_MAIN);
		pGS->StrongUpdateVotes(m_ClientID, MENU_MAIN);
	}
	else
	{
		SaveSteps();
	}
}
