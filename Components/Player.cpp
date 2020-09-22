#include "StdAfx.h"
#include "Player.h"
#include "GamePlugin.h"

#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryNetwork/Rmi.h>

#define MOUSE_DELTA_TRESHOLD 0.0001f

namespace
{
	static void RegisterPlayerComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPlayerComponent));
		}
	}

	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterPlayerComponent);
}

#pragma region built-in event system
Cry::Entity::EventFlags CPlayerComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::GameplayStarted;
}

void CPlayerComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case Cry::Entity::EEvent::Update:
	{
		// Don't update the player if he's dead or not spawned yet
		if(m_pDamageableComponent && !m_pDamageableComponent->m_isAlive)
			return;
		
		const float frameTime = event.fParam[0];

		UpdateLookOrientation(frameTime);
		ProcessInputs(frameTime);
		UpdateCamera(frameTime);
		UpdateAnimation(frameTime);
	}
	break;
	case Cry::Entity::EEvent::GameplayStarted: 
	{
		if (!m_pSpawn) {
			FindSpawnPoint();
		}
		Respawn();
	}
	break;
	}
}
#pragma endregion

#pragma region Update methods
void CPlayerComponent::HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type)
{
	switch (type)
	{
	case EInputFlagType::Hold:
	{
		if (activationMode == eAAM_OnRelease)
		{
			m_inputFlags &= ~flags;
		}
		else
		{
			m_inputFlags |= flags;
		}
	}
	break;
	case EInputFlagType::Toggle:
	{
		if (activationMode == eAAM_OnRelease)
		{
			// Toggle the bit(s)
			m_inputFlags ^= flags;
		}
	}
	break;
	}
}

void CPlayerComponent::ProcessInputs(float frameTime) {
	if (!m_pCharacterController->IsOnGround())
		return;

	const float moveSpeed = 27.5f;
	Vec3 velocity = ZERO;

	// Check input to calculate local space velocity
	if (m_inputFlags & EInputFlag::MoveLeft)
	{
		velocity.x -= moveSpeed * frameTime;
	}
	if (m_inputFlags & EInputFlag::MoveRight)
	{
		velocity.x += moveSpeed * frameTime;
	}
	if (m_inputFlags & EInputFlag::MoveForward)
	{
		velocity.y += moveSpeed * frameTime;
	}
	if (m_inputFlags & EInputFlag::MoveBack)
	{
		velocity.y -= moveSpeed * frameTime;
	}

	m_pCharacterController->AddVelocity(GetEntity()->GetWorldRotation() * velocity);	
}

void CPlayerComponent::UpdateLookOrientation(float frameTime) 
{
	if (m_mouseDeltaRotation.IsEquivalent(ZERO, MOUSE_DELTA_TRESHOLD))
		return;

	const float rotationSpeed = 0.002f;
	const float rotationLimitsMinPitch = -0.84f;
	const float rotationLimitsMaxPitch = 1.5f;
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	ypr.x += m_mouseDeltaRotation.x * rotationSpeed;
	ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);
	ypr.z = 0; // Disable roll 

	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	// Reset the mouse delta since we "used" it this frame
	m_mouseDeltaRotation = ZERO;

}

void CPlayerComponent::UpdateCamera(float frameTime) {
	// Update the rotation of both camera and entity
	// Rotate entity only around yaw and camera only around pitch
	Ang3 camYpr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));
	camYpr.z = 0;
	Ang3 entYpr = camYpr;
	camYpr.x = 0;
	entYpr.y = 0;

	// Create a transform to also set position of camera
	Matrix34 transform = IDENTITY;
	transform.SetRotation33(CCamera::CreateOrientationYPR(camYpr));

	// Offset the player along the forward axis (normally back)
	// Also offset upwards
	const float viewOffsetForward = 0.01f;
	const float viewOffsetUp = .26f;

	// Get the position of the head bone of the model
	if (ICharacterInstance* characterModel = m_pAnimationComponent->GetCharacter()) {
		const QuatT &cameraOrientation = characterModel->GetISkeletonPose()->GetAbsJointByID(m_cameraJointId);
		transform.SetTranslation(cameraOrientation.t + Vec3(0.f, viewOffsetForward, viewOffsetUp));
	}

	m_pCameraComponent->SetTransformMatrix(transform);
	m_pAudioListenerComponent->SetOffset(transform.GetTranslation());

	// Update entity rotation as the player turns
	const Quat entityOrientation = Quat(CCamera::CreateOrientationYPR(entYpr));
	m_pEntity->SetPosRotScale(m_pEntity->GetPos(), entityOrientation, Vec3(1.f));
}

void CPlayerComponent::UpdateAnimation(float frameTime) {
	if (m_activeAnimationFragment != m_idleAnimationFragment)
	{
		m_activeAnimationFragment = m_idleAnimationFragment;
		m_pAnimationComponent->QueueFragmentWithId(m_activeAnimationFragment);
	}

}
#pragma endregion


void CPlayerComponent::EquipWeapon(CFirearmComponent* firearm) {
	m_pEquippedWeapon = firearm;
	//TODO: Set model in mannequin
}

#pragma region Initialization methods
void CPlayerComponent::Initialize()
{
	m_pDamageableComponent = m_pEntity->GetOrCreateComponent<CDamageable>();
	auto onDeath = std::bind(&CPlayerComponent::Die, this);
	m_pDamageableComponent->SetOnDeath(onDeath);

	InitializeCharacterController();
	CreateProtagonistComponents();
	EquipDefaultWeapon();
	RegisterInputs();

	//m_pEntity->SetWorldTM(Matrix34::Create(Vec3(1,1,1), IDENTITY, Vec3(30,30,35)));
}

void CPlayerComponent::CreateProtagonistComponents() {
	// Create the camera component, will automatically update the viewport every frame
	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	if (m_pCameraComponent == nullptr) { CryLog("ERR: Camera creation failed"); }
	else {
		// Set the clipping plane small enough so it does not cut weapon or bodyparts
		m_pCameraComponent->SetNearPlane(.01f);
	}
	m_lookOrientation = IDENTITY;
	m_mouseDeltaRotation = ZERO;
	// Create the audio listener component.
	m_pAudioListenerComponent = m_pEntity->GetOrCreateComponent<Cry::Audio::DefaultComponents::CListenerComponent>();
	if (m_pAudioListenerComponent == nullptr) { CryLog("ERR: AudioListener creation failed"); }
}

void CPlayerComponent::InitializeCharacterController() {

	m_pCharacterController = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	// Offset the default character controller up by one unit
	m_pCharacterController->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, 1.f)));
	m_pCharacterController->GetPhysicsParameters().m_height = 2;
	m_pCharacterController->GetPhysicsParameters().m_radius = 1.5;

	m_pAnimationComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();
	m_pAnimationComponent->SetMannequinAnimationDatabaseFile("Animations/Mannequin/ADB/FirstPerson.adb");
	m_pAnimationComponent->SetCharacterFile("objects/player/mixamo_soldier/soldierRotated.cdf");
	m_pAnimationComponent->SetControllerDefinitionFile("Animations/Mannequin/ADB/FirstPersonControllerDefinition.xml");
	m_pAnimationComponent->SetDefaultScopeContextName("FirstPersonCharacter");
	m_pAnimationComponent->LoadFromDisk();

	// Queue the idle fragment to start playing immediately on next update
	m_pAnimationComponent->SetDefaultFragmentName("Idle");
	m_idleAnimationFragment = m_pAnimationComponent->GetFragmentId("Idle");
	m_activeAnimationFragment = FRAGMENT_ID_INVALID;

	if (ICharacterInstance* characterInstance = m_pAnimationComponent->GetCharacter()) {
		m_cameraJointId = characterInstance->GetIDefaultSkeleton().GetJointIDByName("mixamorig:Head");
	}

	// Apply the character to the entity and queue animations
	m_pAnimationComponent->ResetCharacter();
	m_pCharacterController->Physicalize();
}

void CPlayerComponent::EquipDefaultWeapon() {
	CFirearmComponent* defaultRifle = m_pEntity->GetOrCreateComponent<CFirearmComponent>();;
	defaultRifle->SetMockValues();
	EquipWeapon(defaultRifle);

	//	Set the point at which the bullets should spawn
	if (ICharacterInstance* characterInstance = m_pAnimationComponent->GetCharacter()) {
		IAttachment* pBarrelOutAttachment = characterInstance->GetIAttachmentManager()->GetInterfaceByName("barrel_out");

		if (pBarrelOutAttachment != nullptr)
		{
			defaultRifle->m_pBarrelOut = pBarrelOutAttachment;
		}
	}
}

void CPlayerComponent::RegisterInputs() {
	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are triggered
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();

	if (m_pInputComponent != nullptr) {
		// Register an action, and the callback that will be sent when it's triggered
		m_pInputComponent->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveLeft, (EActionActivationMode)activationMode);  });
		// Bind the 'A' key the "moveleft" action
		m_pInputComponent->BindAction("player", "moveleft", eAID_KeyboardMouse, EKeyId::eKI_A);

		m_pInputComponent->RegisterAction("player", "moveright", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveRight, (EActionActivationMode)activationMode);  });
		m_pInputComponent->BindAction("player", "moveright", eAID_KeyboardMouse, EKeyId::eKI_D);

		m_pInputComponent->RegisterAction("player", "moveforward", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveForward, (EActionActivationMode)activationMode);  });
		m_pInputComponent->BindAction("player", "moveforward", eAID_KeyboardMouse, EKeyId::eKI_W);

		m_pInputComponent->RegisterAction("player", "moveback", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveBack, (EActionActivationMode)activationMode);  });
		m_pInputComponent->BindAction("player", "moveback", eAID_KeyboardMouse, EKeyId::eKI_S);

		m_pInputComponent->RegisterAction("player", "mouse_rotateyaw", [this](int activationMode, float value) { 
			m_mouseDeltaRotation.x -= value; });
		m_pInputComponent->BindAction("player", "mouse_rotateyaw", eAID_KeyboardMouse, EKeyId::eKI_MouseX);

		m_pInputComponent->RegisterAction("player", "mouse_rotatepitch", [this](int activationMode, float value) { 
			m_mouseDeltaRotation.y -= value; });
		m_pInputComponent->BindAction("player", "mouse_rotatepitch", eAID_KeyboardMouse, EKeyId::eKI_MouseY);

		m_pInputComponent->RegisterAction("player", "shoot", [this](int activationMode, float value) {
			if (activationMode == eAAM_OnPress) {
				if (m_pEquippedWeapon != nullptr) {
					m_pEquippedWeapon->Shoot();
				}
				else {
					CryLog("no weapon equipped");
				}
			}
		});
		m_pInputComponent->BindAction("player", "shoot", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);

		m_pInputComponent->RegisterAction("player", "reload", [this](int activationMode, float value) {
			if (activationMode == eAAM_OnPress) {
				if (m_pEquippedWeapon != nullptr) {
					//TODO: if correct ammo type available - subtract from carried ammo, if ammo missing
					m_pEquippedWeapon->StartReload();
				}
			}
		});
		m_pInputComponent->BindAction("player", "reload", eAID_KeyboardMouse, EKeyId::eKI_R);
	}
	else {
		CryLog("ERR: InputComponent creation failed");
	}
}
#pragma endregion

bool CPlayerComponent::IsAlive() {
	return !m_pDamageableComponent || m_pDamageableComponent->m_isAlive;
}

void CPlayerComponent::Die() {
	// Enable Ragdoll, change camera, show death screen, etcl.
	// TODO: Respawn
	CryLog("Player dead");
	Respawn();
}

void CPlayerComponent::Respawn() {
	//Reset health
	m_pDamageableComponent->Reset();
	//Reset weapon
	m_pEquippedWeapon->Reset();

	// Reset position
	// Check if spawnpoint changed
	FindSpawnPoint();
	if (m_pSpawn) {
		m_pEntity->SetPos(m_pSpawn->GetEntity()->GetPos());
	}

}

void CPlayerComponent::FindSpawnPoint() {
	IEntityItPtr pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();

	while (IEntity* pEntity = pEntityIterator->Next())
	{
		if (CSpawnpointComponent* spawn = pEntity->GetComponent<CSpawnpointComponent>()) {
			if (spawn->m_isPlayerSpawn) {
				m_pSpawn = spawn;
				return;
			}
		}
	}
	m_pSpawn = nullptr;
}