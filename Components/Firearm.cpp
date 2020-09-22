#include "StdAfx.h"
#include "Firearm.h"
#include "Bullet.h"

static void RegisterFirearmComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CFirearmComponent));
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterFirearmComponent)

void CFirearmComponent::Initialize() {
	m_reloadTimer = 0;
	m_fireRateTimer = 0;
}


Cry::Entity::EventFlags CFirearmComponent::GetEventMask() const {
	return Cry::Entity::EEvent::Update;
}
void CFirearmComponent::ProcessEvent(const SEntityEvent& event) {
	switch (event.event)
	{
	case Cry::Entity::EEvent::Update:
	{
		float frameTime = event.fParam[0];
		
		if (m_fireRateTimer > 0) {
			m_fireRateTimer -= frameTime;
		}

		if (m_reloadTimer > 0) {
			m_reloadTimer -= frameTime;
			if (m_reloadTimer <= 0)
			{
				FinishReload();
			}
		}
	}
	break;
	}
}

// pull the trigger
void CFirearmComponent::Shoot() {
	if (m_fireRateTimer <= 0 && m_loadedBulletsCount > 0) {
		m_fireRateTimer = 1.0f / m_fireRate;
		m_loadedBulletsCount--;

		// TODO spawn a bullet, play animation, particles, sound
		if (m_pBarrelOut) {
			QuatTS bulletOrigin = m_pBarrelOut->GetAttWorldAbsolute();
			SEntitySpawnParams spawnParams;
			spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("schematyc::bullet");
			spawnParams.vPosition = bulletOrigin.t;

			///// Bullet Spread
			float pi = g_PI;
			// random values between -90 and 90 depending on inaccuracy
			float rnd1 = 1.0f*std::rand() / RAND_MAX;
			float rnd2 = 1.0f*std::rand() / RAND_MAX;
			Ang3 inacc = Ang3((rnd1*pi-pi/2.f)*inaccuracy, 0, (rnd2 * pi- pi / 2.f)*inaccuracy);
			Quat q = Quat(inacc);
			spawnParams.qRotation = q * bulletOrigin.q;
			
			IEntity* bullet = gEnv->pEntitySystem->SpawnEntity(spawnParams);
			if (CBulletComponent* bulletC = bullet->GetComponent<CBulletComponent>()) {
				bulletC->m_pOrigin = m_pEntity;
				bulletC->m_damage = damagePerBullet;
			}
		}
		else {
			CryLog("ERR: no barrel_out attachent set");
		}
	}
	else if (m_loadedBulletsCount == 0) {
		CryLog("Need to reload");
	}
}

// iniciate the reloading process
//! returns the time the reloading process is going to require or 0 when it is already running
float CFirearmComponent::StartReload() {
	if (m_reloadTimer == 0) {
		m_reloadTimer = m_reloadTime;
		return m_reloadTime;
	}
	return 0;
}
// cancel the reloading process, call this when dropping or changing weapon
void CFirearmComponent::CancelReload() {
	m_reloadTimer = 0;
}
void CFirearmComponent::FinishReload() {
	m_loadedBulletsCount = m_magazineCapacity;
	m_reloadTimer = 0;
	CryLog("reloaded");
}