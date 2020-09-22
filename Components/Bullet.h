#pragma once
#include "StdAfx.h"

class CBulletComponent : public IEntityComponent
{

public:
	static void ReflectType(Schematyc::CTypeDesc<CBulletComponent>& desc)
	{
		desc.SetGUID("{68B40AFA-8D34-46F6-B441-33A7A2A05107}"_cry_guid);
		desc.SetLabel("Bullet");
		desc.SetEditorCategory("Combat");
		desc.SetDescription("a bullet, that is being propelled into the direction its facing");
	}
	
	virtual void Initialize() override;

	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

	float m_damage;
	IEntity* m_pOrigin;
	float m_lifeTimer;
	const float maxLifeTime = 5.f;
	IEntityTriggerComponent* m_pTrigger;

protected:
	void TravelStep(float frameTime);
	void CheckCollision(Vec3 direction);
	void LogHitInfo(IEntity* hitEnt);
};