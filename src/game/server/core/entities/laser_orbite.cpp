/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "laser_orbite.h"

#include <game/server/gamecontext.h>

CLaserOrbite::CLaserOrbite(CGameWorld* pGameWorld, vec2 Position, int Amount, EntLaserOrbiteType Type, float Speed, float Radius, int LaserType, int64_t Mask)
	: CEntity(pGameWorld, CGameWorld::ENTYPE_LASER_ORBITE, Position, Radius)
{
	m_Type = Type;
	m_ClientID = -1;
	m_pEntParent = nullptr;
	m_MoveSpeed = Speed;
	m_Mask = Mask;
	m_LaserType = LaserType;
	GameWorld()->InsertEntity(this);

	m_IDs.set_size(Amount);
	for(int i = 0; i < m_IDs.size(); i++)
		m_IDs[i] = Server()->SnapNewID();
}

CLaserOrbite::CLaserOrbite(CGameWorld* pGameWorld, int ClientID, CEntity* pEntParent, int Amount, EntLaserOrbiteType Type, float Speed, float Radius, int LaserType, int64_t Mask)
	: CEntity(pGameWorld, CGameWorld::ENTYPE_LASER_ORBITE, vec2(0.f, 0.f), (int)Radius)
{
	m_Type = Type;
	m_ClientID = ClientID;
	m_pEntParent = pEntParent;
	m_MoveSpeed = Speed;
	m_Mask = Mask;
	m_LaserType = LaserType;

	if(m_pEntParent)
	{
		m_AppendPos = vec2(centrelized_frandom(0.f, GetProximityRadius() / 1.5f), centrelized_frandom(0.f, GetProximityRadius() / 1.5f));
		m_Pos = pEntParent->GetPos() + (Type == EntLaserOrbiteType::INSIDE_ORBITE_RANDOM ? m_AppendPos : vec2(0.f, 0.f));
	}
	GameWorld()->InsertEntity(this);

	m_IDs.set_size(Amount);
	for(int i = 0; i < m_IDs.size(); i++)
		m_IDs[i] = Server()->SnapNewID();
}

CLaserOrbite::~CLaserOrbite()
{
	for(int i = 0; i < m_IDs.size(); i++)
		Server()->SnapFreeID(m_IDs[i]);
	m_IDs.clear();
}

void CLaserOrbite::Tick()
{
	CPlayer* pPlayer = GS()->GetPlayer(m_ClientID, false, true);
	if((m_ClientID >= 0 && !pPlayer) || (m_pEntParent && m_pEntParent->IsMarkedForDestroy()))
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	if(m_pEntParent)
	{
		if(m_Type == EntLaserOrbiteType::INSIDE_ORBITE_RANDOM)
			m_Pos = m_pEntParent->GetPos() + m_AppendPos;
		else
			m_Pos = m_pEntParent->GetPos();
	}
	else if(m_ClientID >= 0 && pPlayer)
	{
		m_Pos = pPlayer->GetCharacter()->m_Core.m_Pos;
	}
}

vec2 CLaserOrbite::UtilityOrbitePos(int PosID) const
{
	float AngleStart = 2.0f * pi;
	float AngleStep = 2.0f * pi / (float)m_IDs.size();
	if(m_Type == EntLaserOrbiteType::MOVE_LEFT)
		AngleStart = -(AngleStart * (float)Server()->Tick() / (float)Server()->TickSpeed()) * m_MoveSpeed;
	else if(m_Type == EntLaserOrbiteType::MOVE_RIGHT)
		AngleStart = (AngleStart * (float)Server()->Tick() / (float)Server()->TickSpeed()) * m_MoveSpeed;

	float X = GetProximityRadius() * cos(AngleStart + AngleStep * (float)PosID);
	float Y = GetProximityRadius() * sin(AngleStart + AngleStep * (float)PosID);
	return { X, Y };
}

void CLaserOrbite::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos, GetProximityRadius()) || !CmaskIsSet(m_Mask, SnappingClient))
		return;

	vec2 LastPosition = m_Pos + UtilityOrbitePos(m_IDs.size() - 1);
	for(int i = 0; i < m_IDs.size(); i++)
	{
		vec2 PosStart = m_Pos + UtilityOrbitePos(i);

		if(GS()->GetClientVersion(SnappingClient) >= VERSION_DDNET_MULTI_LASER)
		{
			CNetObj_DDNetLaser* pObj = static_cast<CNetObj_DDNetLaser*>(Server()->SnapNewItem(NETOBJTYPE_DDNETLASER, m_IDs[i], sizeof(CNetObj_DDNetLaser)));
			if(!pObj)
				return;

			pObj->m_FromX = (int)PosStart.x;
			pObj->m_FromY = (int)PosStart.y;
			pObj->m_ToX = (int)LastPosition.x;
			pObj->m_ToY = (int)LastPosition.y;
			pObj->m_StartTick = Server()->Tick() - 4;
			pObj->m_Owner = m_ClientID;
			pObj->m_Type = m_LaserType;
		}
		else
		{
			CNetObj_Laser* pObj = static_cast<CNetObj_Laser*>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
			if(!pObj)
				return;

			pObj->m_FromX = (int)PosStart.x;
			pObj->m_FromY = (int)PosStart.y;
			pObj->m_X = (int)LastPosition.x;
			pObj->m_Y = (int)LastPosition.y;
			pObj->m_StartTick = Server()->Tick() - 4;
		}

		LastPosition = PosStart;
	}
}

void CLaserOrbite::AddClientMask(int ClientID)
{
		m_Mask |= CmaskOne(ClientID);
}

void CLaserOrbite::RemoveClientMask(int ClientID)
{
		m_Mask &= ~CmaskOne(ClientID);
}


