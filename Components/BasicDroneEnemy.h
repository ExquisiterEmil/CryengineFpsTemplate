#pragma once
#include "StdAfx.h"
#include "Firearm.h"
#include "Damageable.h"
#include "Components/Player.h"
#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>

class CBasicDroneEnemyComponent : public IEntityComponent
{
public:
	static void ReflectType(Schematyc::CTypeDesc<CBasicDroneEnemyComponent>& desc)
	{
		desc.SetGUID("{38F37A2E-41D7-49E8-8403-4D328BA2E4E3}"_cry_guid);
		desc.SetLabel("Basic Drone Enemy");
		desc.SetEditorCategory("AI");
		desc.SetDescription("Basic AI for an Enemy that does not have any animations and just flies towards the player");
		desc.AddMember(&CBasicDroneEnemyComponent::m_infiniteBullets, 'inf', "InfiniteBullets", "Infinite Bullets", "If true, the enemy never has to reload", true);
	}

	virtual void Initialize() override;

	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;
	CPlayerComponent* m_pPlayer;

protected:
	bool MoveTowardsPosition(Vec3 pos, float accuracy, float frameTime);
	void ShootAtPlayer(float frameTime);
	void RotateTowardsPlayer(float frameTime);
	void FindPlayerEntity();
	bool CanSeePlayer();
	void LookForPlayer();
	Vec3 GetPlayerPosition();
	IEntity* RaycastEntityPos(Vec3 target);

	
	const Vec3 localFireDirection = Vec3(0.2f, 1.f, 0.f);
	float rotationSpeed = 3.f;
	float m_movementSpeed = 3.0f;

	bool m_rotationAligned = false;
	bool m_infiniteBullets;

	Cry::DefaultComponents::CAdvancedAnimationComponent* m_pAnimationComponent;
	CFirearmComponent* m_pDroneGun;
	CDamageable* m_pDamageableComponent;
};