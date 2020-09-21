#include "Bullet.h"
#include "Damageable.h"
#include <CryPhysics/IPhysics.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CryPhysics/physinterface.h>
#include <CryPhysics/IPhysics.h>
#include <CryPhysics/RayCastQueue.h>

static void RegisterBulletComponent(Schematyc::IEnvRegistrar& registrar) {
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CBulletComponent));
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterBulletComponent)

void CBulletComponent::Initialize() {
	m_lifeTimer = maxLifeTime;
	m_pTrigger = m_pEntity->GetOrCreateComponent<IEntityTriggerComponent>();
	Vec3 triggerSize = Vec3(0.07f, 0.07f, 0.07f);
	m_pTrigger->SetTriggerBounds(AABB(triggerSize * -0.5f, triggerSize * 0.5f));
}
/*
	const float mass = 0.03f;
	m_pEntity->SetViewDistRatio(255);
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_RIGID;
	physParams.mass = mass;
	m_pEntity->Physicalize(physParams);

	if (auto *pPhysics = GetEntity()->GetPhysics())
	{
		pe_action_impulse impulseAction;

		const float initialVelocity = 100.f;

		// Set the actual impulse, in this cause the value of the initial velocity CVar in bullet's forward direction
		impulseAction.impulse = m_pEntity->GetWorldRotation().GetColumn1() * initialVelocity / mass;

		// Send to the physical entity
		pPhysics->Action(&impulseAction);
	}
}*/

Cry::Entity::EventFlags CBulletComponent::GetEventMask() const {
	return Cry::Entity::EEvent::Update | Cry::Entity::EEvent::EntityEnteredThisArea;
}

void CBulletComponent::ProcessEvent(const SEntityEvent& event) {
	switch (event.event) {
	case Cry::Entity::EEvent::Update:
	{
		const float frameTime = event.fParam[0];
		m_lifeTimer -= frameTime;

		if (m_lifeTimer <= 0) {
			gEnv->pEntitySystem->RemoveEntity(GetEntityId());
		}

		TravelStep(frameTime);
		// Collider draw
		//gEnv->pAuxGeomRenderer->DrawAABB(AABB(m_pEntity->GetPos() + Vec3(0.07f) * -0.5f, m_pEntity->GetPos() + Vec3(0.07f)* 0.5f), true, { 1,0,0,1 }, EBoundingBoxDrawStyle::eBBD_Faceted);
	}
	break;
	case Cry::Entity::EEvent::EntityEnteredThisArea:
	{
		// TODO: Apply damage on enemy
	}
	}
}

void CBulletComponent::TravelStep(float frameTime) 
{
	const float velocity = 50.f;
	Vec3 direction = m_pEntity->GetWorldRotation().GetColumn1() * velocity * frameTime;
	// Check if we are going to hit something
	CheckCollision(direction);
	// if we are still alive, then move
	m_pEntity->SetPos(m_pEntity->GetPos() + direction);
}

void CBulletComponent::CheckCollision(Vec3 direction) 
{	
	//Ray moveRay = Ray(m_pEntity->GetPos(), velocity * frameTime);
	ray_hit hit;
	auto pWorld = gEnv->pPhysicalWorld;
	auto num = pWorld->RayWorldIntersection(m_pEntity->GetPos(), direction, ent_all, rwi_colltype_any | rwi_stop_at_pierceable, &hit, 1);
		
	if (num > 0) {
		IEntity* hitEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
		LogHitInfo(hitEntity);

		if (hitEntity)
		{
			if (CDamageable* hitDamageable = hitEntity->GetComponent<CDamageable>()) {
				hitDamageable->TakeDamage(m_damage);
			}
		}
		gEnv->pEntitySystem->RemoveEntity(GetEntityId());
	}
}

void CBulletComponent::LogHitInfo(IEntity* hitEntity) {
	std::string hitInfo = "";
	if (hitEntity) {
		hitInfo.append(hitEntity->GetName()).append(" was hit");
	}
	CryLog(hitInfo.c_str());
}
