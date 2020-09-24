#pragma once

#include "Firearm.h"
#include "Damageable.h"
#include "Spawnpoint.h"
#include <CryEntitySystem/IEntityComponent.h>
#include <CryMath/Cry_Camera.h>

#include <ICryMannequin.h>
#include <CrySchematyc/Utils/EnumFlags.h>

#include <DefaultComponents/Cameras/CameraComponent.h>
#include <DefaultComponents/Input/InputComponent.h>
#include <DefaultComponents/Audio/ListenerComponent.h>
#include <DefaultComponents/Physics/CharacterControllerComponent.h>
#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>


////////////////////////////////////////////////////////
// Represents a player participating in gameplay
////////////////////////////////////////////////////////
class CPlayerComponent final : public IEntityComponent
{
	enum class EInputFlagType
	{
		Hold = 0,
		Toggle
	};

	enum class EInputFlag : uint8
	{
		MoveLeft = 1 << 0,
		MoveRight = 1 << 1,
		MoveForward = 1 << 2,
		MoveBack = 1 << 3
	};

	static constexpr EEntityAspects InputAspect = eEA_GameClientD;
	
public:
	CPlayerComponent() = default;
	virtual ~CPlayerComponent() = default;

	// IEntityComponent
	virtual void Initialize() override;

	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;


	// ~IEntityComponent

	// Reflect type to set a unique identifier for this component
	static void ReflectType(Schematyc::CTypeDesc<CPlayerComponent>& desc)
	{
		desc.SetGUID("{63F4C0C6-32AF-4ACB-8FB0-57D45DD14725}"_cry_guid);
	};

	bool IsAlive();
	
protected:
	void Die();
	void HandleInputFlagChange(CEnumFlags<EInputFlag> flags, CEnumFlags<EActionActivationMode> activationMode, EInputFlagType type = EInputFlagType::Hold);

	// Called when this entity becomes the local player, to create client specific setup such as the Camera
	void InitializeCharacterController();
	void RegisterInputs();
	void CreateProtagonistComponents();
	void EquipDefaultWeapon();
	void ProcessInputs(float frameTime);
	void UpdateCamera(float frameTime);
	void UpdateAnimation(float frameTime);
	void UpdateLookOrientation(float frameTime);
	void EquipWeapon(CFirearmComponent* firearm);
	void Respawn();
	void FindSpawnPoint();
	
protected:
	Quat m_lookOrientation;

	Cry::DefaultComponents::CCameraComponent* m_pCameraComponent = nullptr;
	Cry::DefaultComponents::CInputComponent* m_pInputComponent = nullptr;
	Cry::DefaultComponents::CCharacterControllerComponent* m_pCharacterController = nullptr;
	Cry::DefaultComponents::CAdvancedAnimationComponent* m_pAnimationComponent = nullptr;
	Cry::Audio::DefaultComponents::CListenerComponent* m_pAudioListenerComponent = nullptr;

	CEnumFlags<EInputFlag> m_inputFlags;
	Vec2 m_mouseDeltaRotation;
	int m_cameraJointId = -1;

	CSpawnpointComponent* m_pSpawn;
	FragmentID m_idleAnimationFragment;
	FragmentID m_activeAnimationFragment;
	FragmentID m_desiredAnimationFragment;
	FragmentID m_shootAnimationFragment;
	FragmentID m_reloadAnimationFragment;
	CFirearmComponent* m_pEquippedWeapon = nullptr;
	CDamageable* m_pDamageableComponent;

};
