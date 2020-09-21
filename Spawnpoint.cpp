#include "Spawnpoint.h"
#include "Components/Player.h"
#include "BasicDroneEnemy.h"
#include <CryEntitySystem/IEntitySystem.h>


namespace
{
	static void RegisterSpawnpointComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CSpawnpointComponent));
		}
	}

	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterSpawnpointComponent);
}

Cry::Entity::EventFlags CSpawnpointComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::GameplayStarted;
}

void CSpawnpointComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case Cry::Entity::EEvent::GameplayStarted:
	{
		if (!m_isPlayerSpawn) {
			SEntitySpawnParams spawnParams;
			spawnParams.vPosition = m_pEntity->GetPos() + Vec3(0.f, 0.f, m_groundOffset);
			spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("schematyc::enemy");
			spawnParams.sName = "enemy";
			CBasicDroneEnemyComponent* enemy = gEnv->pEntitySystem->SpawnEntity(spawnParams)->GetComponent<CBasicDroneEnemyComponent>();
			m_pSpawnedEntity = enemy;

		}
	}
	break;
	}
}