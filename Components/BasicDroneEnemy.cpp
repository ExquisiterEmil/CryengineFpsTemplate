#include "BasicDroneEnemy.h"
#include "Bullet.h"
#include <CryPhysics/IPhysics.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <math.h>

static void RegisterBasicDroneEnemyComponent(Schematyc::IEnvRegistrar& registrar) {
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CBasicDroneEnemyComponent));
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterBasicDroneEnemyComponent)

void CBasicDroneEnemyComponent::Initialize() {
	m_pDamageableComponent = m_pEntity->GetOrCreateComponent<CDamageable>();
	m_pDamageableComponent->SetOnDeath([this]() {
		gEnv->pEntitySystem->RemoveEntity(GetEntityId());
	});
	
	m_pAnimationComponent = m_pEntity->GetComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();
	m_pDroneGun = m_pEntity->GetOrCreateComponent<CFirearmComponent>();
	if (m_infiniteBullets) {
		m_pDroneGun->InfiniteBullets();
	}

	//	Set the point at which the bullets should spawn
	if (m_pAnimationComponent)
	{
		if (ICharacterInstance* characterInstance = m_pAnimationComponent->GetCharacter()) {
			IAttachment* pBarrelOut = characterInstance->GetIAttachmentManager()->GetInterfaceByName("barrel_out");
			m_pDroneGun->m_pBarrelOut = pBarrelOut;
		}
	}

	FindPlayerEntity();
}

//! returns true if the position has been reached
bool CBasicDroneEnemyComponent::MoveTowardsPosition(Vec3 pos, float accuracy, float frameTime) {
	Vec3 vecToPosition = pos - m_pEntity->GetWorldPos();
	float distance = vecToPosition.GetLength();

	if (distance > accuracy) {
		// INSERT complex pathfinding algorithm here.
		Vec3 movementStep = vecToPosition.Normalize() * m_movementSpeed * frameTime;
		m_pEntity->SetPos(m_pEntity->GetPos() + movementStep);
		return false;
	}
	return true;
}

bool CBasicDroneEnemyComponent::CanSeePlayer() {
	return true;
}

void CBasicDroneEnemyComponent::LookForPlayer() {

}

void CBasicDroneEnemyComponent::FindPlayerEntity() {
	if (!m_pPlayer) 
	{
		IEntityItPtr it = gEnv->pEntitySystem->GetEntityIterator();
		while (IEntity* entity = it->Next())
		{
			if (CPlayerComponent* player = entity->GetComponent<CPlayerComponent>())
			{
				m_pPlayer = player;
			}
		}
	}
}

void CBasicDroneEnemyComponent::ShootAtPlayer(float frameTime) 
{
	RotateTowardsPlayer(frameTime);
	if (!m_pDroneGun->IsLoaded()) {
		m_pDroneGun->StartReload();
		return;
	}
	if (m_rotationAligned && m_pDroneGun) {
		m_pDroneGun->Shoot();
	}
}

void CBasicDroneEnemyComponent::RotateTowardsPlayer(float frameTime)
{
	Quat q = m_pEntity->GetWorldRotation();
	Ang3 ang = Ang3(q);

	Vec3 vecToPlayer = GetPlayerPosition() - m_pEntity->GetPos();
	Vec3 worldFireDirection = localFireDirection.GetRotated(Vec3(0,0,1), ang.z);

	//get normalized horizontal vectors towards the player
	Vec3 vecToPlayerh = Vec3(vecToPlayer.x, vecToPlayer.y, 0).Normalize();
	Vec3 firedirh = Vec3(worldFireDirection.x, worldFireDirection.y, 0).Normalize();

	// calculate angles in rad of each vector (from 0 to pi)
	float vecToPlayerAngle = atan(vecToPlayerh.y / vecToPlayerh.x) + 3.1415f/2.f;
	float fireDirAngle = atan(firedirh.y / firedirh.x) + 3.1415f / 2.f;

	// add pi/2 in case its an obtuse angle
	if (vecToPlayerh.x < 0) {
		vecToPlayerAngle += 3.1415;
	}
	if (firedirh.x < 0) {
		fireDirAngle += 3.1415;
	}

	// is the rotation of the enemy already roughly correct?
	m_rotationAligned = abs(vecToPlayerAngle - fireDirAngle) < 0.1f;

	// if not rotate towards player
	if (!m_rotationAligned) {

		// either vecToPlayerAngle is the bigger and inner distance is smaller than the outer
		// or its smaller and the inner distance is bigger than the outer (crossing 0)
		if(vecToPlayerAngle > fireDirAngle && vecToPlayerAngle - fireDirAngle < 3.1415 ||
			vecToPlayerAngle < fireDirAngle && fireDirAngle - vecToPlayerAngle > 3.1415)
		{
			//rotate left - increase z
			ang.z += rotationSpeed * frameTime;
		}
		else {
			//rotate right
			ang.z -= rotationSpeed * frameTime;
		}
		q.SetRotationXYZ(ang);
		m_pEntity->SetRotation(q);
	}

	//debug

	std::string a = "fire angle: ";
	a.append(std::to_string(fireDirAngle));
	//gEnv->pAuxGeomRenderer->Draw2dLabel(50, 50, 1.5, {1,0,0,1}, false, a.c_str());
	std::string b = "to player Angle: ";
	b.append(std::to_string(vecToPlayerAngle));
	//gEnv->pAuxGeomRenderer->Draw2dLabel(50, 75, 1.5, { 1,0,0,1 }, false, b.c_str());
	std::string c = "diff: ";
	c.append(std::to_string(vecToPlayerAngle - fireDirAngle));
	//gEnv->pAuxGeomRenderer->Draw2dLabel(50, 100, 1.5, { 1,0,0,1 }, false, c.c_str());
	std::string d = "vecToPlayerh: ";
	d.append(std::to_string(vecToPlayerh.x)).append(", ").append(std::to_string(vecToPlayerh.y));
	//gEnv->pAuxGeomRenderer->Draw2dLabel(50, 125, 1.5, { 1,0,0,1 }, false, d.c_str());
	std::string e = "fireDir: ";
	e.append(std::to_string(firedirh.x)).append(", ").append(std::to_string(firedirh.y));
	//gEnv->pAuxGeomRenderer->Draw2dLabel(50, 150, 1.5, { 1,0,0,1 }, false, e.c_str());
}

Vec3 CBasicDroneEnemyComponent::GetPlayerPosition() {
	if (m_pPlayer) {
		return m_pPlayer->GetEntity()->GetPos();
	}
	else {
		return Vec3(NAN, NAN, NAN);
	}
}

Cry::Entity::EventFlags CBasicDroneEnemyComponent::GetEventMask() const {
	return Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::GameplayStarted;
}

void CBasicDroneEnemyComponent::ProcessEvent(const SEntityEvent& event) {
	switch (event.event) {
	case Cry::Entity::EEvent::Update:
	{
		const float frameTime = event.fParam[0];
		if (m_pPlayer && gEnv->IsEditorGameMode())
		{
			if (m_pPlayer->IsAlive()) {
				// decide if move towards player or shootAtPlayer -> resolve behaviour tree
				// for now just shoot him
				ShootAtPlayer(frameTime); 
				//MoveTowardsPosition(m_pPlayer->GetEntity()->GetPos(), 5.f, frameTime);
			}
		}
		
	}
	break;
	case Cry::Entity::EEvent::GameplayStarted:
	{
		if (!m_pPlayer || !m_pDroneGun->m_pBarrelOut || !m_pDamageableComponent) {
			Initialize();
		}
	}
	}
}