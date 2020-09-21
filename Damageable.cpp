#include "Damageable.h"
#include <CryEntitySystem/IEntitySystem.h>


namespace
{
	static void RegisterDamageableComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CDamageable));
		}
	}

	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterDamageableComponent);
}

void CDamageable::Die() {
	m_isAlive = false;
	if (onDeath) {
		onDeath();
	}
}

void CDamageable::TakeDamage(float damagePoints) {
	if (m_isAlive) {
		m_healthPoints -= damagePoints;
		if (m_healthPoints <= 0) {
			Die();
		}
	}
}

//! set a custom function to be called on death
void CDamageable::SetOnDeath(std::function<void()> death) {
	onDeath = death;
}
