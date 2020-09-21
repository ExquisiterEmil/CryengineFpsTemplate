#pragma once
#include "StdAfx.h"
#include <CryEntitySystem/IEntitySystem.h>

class CSpawnpointComponent : public IEntityComponent
{
public:
	static void ReflectType(Schematyc::CTypeDesc<CSpawnpointComponent>& desc)
	{
		desc.SetGUID("{7D38A2FB-52AD-4057-AB92-67865B02189E}"_cry_guid);
		desc.SetLabel("Spawnpoint");
		desc.SetDescription("Spawns either the player or the enemies");
		desc.AddMember(&CSpawnpointComponent::m_isPlayerSpawn, 'pls', "IsPlayerSpawn", "Is player spawn", "Marks this spawnpoint as the one the player spawns in (first one will be used, if multiple exist)", false);
		desc.AddMember(&CSpawnpointComponent::m_groundOffset, 'off', "GroundOffset", "Ground offset", "How much the entity will be offset from the ground", 1.0f);
	}

	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;
	bool m_isPlayerSpawn;

protected:
	void FindPlayer();
	float m_groundOffset;
	IEntityComponent* m_pSpawnedEntity;
};