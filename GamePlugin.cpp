#include "StdAfx.h"
#include "GamePlugin.h"

#include "Components/Player.h"
#include "BehaviorTreeNodes/BasicDroneNodes.cpp"

#include <IGameObjectSystem.h>
#include <IGameObject.h>

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <CryAISystem/IAISystem.h>
#include <CryAiSystem/BehaviorTree/IBehaviorTree.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

CGamePlugin::~CGamePlugin()
{

	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (gEnv->pSchematyc)
	{
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(CGamePlugin::GetCID());
	}
}
bool CGamePlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	// Register for engine system events, in our case we need ESYSTEM_EVENT_GAME_POST_INIT to load the map
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CGamePlugin");
	return true;
}

void CGamePlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
		// Called when the game framework has initialized and we are ready for game logic to start
		case ESYSTEM_EVENT_GAME_POST_INIT:
		{
			BehaviorTree::ShootPlayer::RegisterBehaviorTreeNodes_Custom();
			// Don't need to load the map in editor
			if (!gEnv->IsEditor())
			{
				// Load the example map in client server mode
				gEnv->pConsole->ExecuteString("map example s", false, true);
			}
		}
		break;

		case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			InitializePlayer();
		}
		break;

		case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
		{
			// Register all components that belong to this plug-in
			auto staticAutoRegisterLambda = [](Schematyc::IEnvRegistrar& registrar)
			{
				// Call all static callback registered with the CRY_STATIC_AUTO_REGISTER_WITH_PARAM
				Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
			};

			if (gEnv->pSchematyc)
			{
				gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
					stl::make_unique<Schematyc::CEnvPackage>(
						CGamePlugin::GetCID(),
						"EntityComponents",
						"Crytek GmbH",
						"Components",
						staticAutoRegisterLambda
						)
				);
			}
		}
		break;
		
		case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			m_playerInited = false;
		}
		break;
	}
}

bool CGamePlugin::InitializePlayer()
{
	// Connection received from a client, create a player entity and component
	SEntitySpawnParams spawnParams;
	spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
	
	spawnParams.sName = "Player";
	
	// Set local player details
	if (m_playerInited == false && !gEnv->IsDedicated())
	{
		spawnParams.id = LOCAL_PLAYER_ENTITY_ID;
		spawnParams.nFlags |= ENTITY_FLAG_LOCAL_PLAYER;
	}

	// Spawn the player entity
	if (IEntity* pPlayerEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
	{
		// Create the player component instance
		m_pPlayer = pPlayerEntity->GetOrCreateComponentClass<CPlayerComponent>();
		CryLog("Player created");

		if (m_pPlayer != nullptr)
		{
			m_player = (pPlayerEntity->GetId());
			return true;

		}
	}
	return false;
}

CRYREGISTER_SINGLETON_CLASS(CGamePlugin)