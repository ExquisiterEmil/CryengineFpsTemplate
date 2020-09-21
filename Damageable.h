#pragma once
#include "StdAfx.h"

class CDamageable : public IEntityComponent 
{
public:
	static void ReflectType(Schematyc::CTypeDesc<CDamageable>& desc)
	{
		desc.SetGUID("{4894A4E8-2EEB-47CD-A56D-9545E3918020}"_cry_guid);
		desc.SetLabel("Damageable");
		desc.SetEditorCategory("Combat");
		desc.SetDescription("A component for something that can be destroyed");
		desc.AddMember(&CDamageable::m_startHealthPoints, 'hlth', "StartHealth", "Starting health points", "The amount of health the entity starts with", 100.0f);
	}

public:
	CDamageable() {
		m_healthPoints = m_startHealthPoints;
		m_isAlive = true;
	}

	bool m_isAlive = false;
protected:
	float m_healthPoints;
	float m_startHealthPoints = 100.f;

	void Die();
	
	std::function<void()> onDeath;

public:
	void TakeDamage(float damagePoints);
	void Reset() {
		m_healthPoints = m_startHealthPoints;
		m_isAlive = true;
	}

	//! set a custom function to be called on death
	void SetOnDeath(std::function<void()> death);
};