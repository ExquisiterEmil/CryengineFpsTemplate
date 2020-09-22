#pragma once

#include <CryEntitySystem/IEntityComponent.h>

#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>

//////////////////////////////////////////////////
// Basic functionality for semi-automatic weapons
//////////////////////////////////////////////////
class CFirearmComponent : public IEntityComponent {

public:
	CFirearmComponent() = default;
	virtual ~CFirearmComponent() {}

	static void ReflectType(Schematyc::CTypeDesc<CFirearmComponent>& desc) {
		desc.SetGUID("{40029B27-A77B-49B3-BDC2-E6515533E23F}"_cry_guid);
		desc.SetLabel("Firearm");
		desc.SetEditorCategory("Combat");
		desc.SetDescription("Component for rifles, guns, etc.");

		desc.AddMember(&CFirearmComponent::m_fireRate, 'rate', "Firerate", "Fire rate", "shots per second", 5.f);
		desc.AddMember(&CFirearmComponent::m_reloadTime, 'rldt', "ReloadTime", "Reload time", "seconds needed for reloading", 1.5f);
		desc.AddMember(&CFirearmComponent::m_magazineCapacity, 'mgc', "MagazineCapacity", "Magazine capacity", "how many bullets one can hold", 16);
		desc.AddMember(&CFirearmComponent::m_loadedBulletsCount, 'bcnt', "LoadedBullets", "Loaded bullets", "amount of bullets in the arm at the moment. Must be smaller than mag capacity", 0);
		desc.AddMember(&CFirearmComponent::m_ammoId, 'amid', "AmmoId", "AmmoID", "identifier for the ammunition the gun takes", 0);
		desc.AddMember(&CFirearmComponent::damagePerBullet, 'dmg', "Damage", "Damage per bullet", "the amount of damage a single bullet does when it hits an enemy", 10.0f);
		desc.AddMember(&CFirearmComponent::inaccuracy, 'inac', "Inaccuracy", "Inaccuracy", "How much the bullets spread", .2f);
	}

	virtual void Initialize();

	void SetMockValues() {
		 m_fireRate= 5.f;
		 m_reloadTime = 1.5f;
		 m_magazineCapacity = 30;
		 m_loadedBulletsCount = 0;
		 m_ammoId = 0;
		 damagePerBullet = 10.f;
		 inaccuracy = .02f;
	}

	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;
	float StartReload();
	void CancelReload();
	void Shoot();
	bool IsLoaded() { return m_loadedBulletsCount > 0; }
	void InfiniteBullets() {
		// not actually infinite but more than is going to be needed.
		m_magazineCapacity = std::numeric_limits<int>::max();
		m_loadedBulletsCount = std::numeric_limits<int>::max();
	}
	void Reset() {
		m_loadedBulletsCount = m_magazineCapacity;
		m_fireRateTimer = 0.0f;
		m_reloadTimer = 0.0f;
	}
	IAttachment* m_pBarrelOut;

protected:

	void FinishReload();

	float m_fireRate;
	float m_reloadTime;
	float m_reloadTimer;
	float m_fireRateTimer;
	int m_magazineCapacity;
	int m_loadedBulletsCount;
	int m_ammoId;
	float damagePerBullet;
	float inaccuracy;
	// TriggerComponent (abstraction for different types of shots 
	// - different bullets, burst, shotgun, etc.)
};