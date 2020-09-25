#include "Components/BasicDroneEnemy.h"

#include <CryAiSystem/BehaviorTree/IBehaviorTree.h>
#include <CryAiSystem/BehaviorTree/Action.h>
#include <CrySerialization/ClassFactory.h> // <-- VERY IMPORTANT INCLUSION RIGHT HERE

namespace BehaviorTree {

class MoveToPosition : public BehaviorTree::Action
{
	typedef BehaviorTree::Action BaseClass;
	const float range = 5.0f;

public:

	struct RuntimeData {
		CBasicDroneEnemyComponent* drone;
	};

	virtual void OnInitialize(const BehaviorTree::UpdateContext& context) override {
		GetRuntimeData<RuntimeData>(context).drone = context.entity.GetComponent<CBasicDroneEnemyComponent>();
	}

	virtual BehaviorTree::Status Update(const BehaviorTree::UpdateContext& context) override {
		auto drone = GetRuntimeData<RuntimeData>(context).drone;
		if (!drone->CanSeePlayer()) {
			return BehaviorTree::Failure;
		}
		bool reached = drone->MoveTowardsPosition(drone->m_pPlayer->GetEntity()->GetPos(), range, context.frameDeltaTime);
		return reached ? BehaviorTree::Success : BehaviorTree::Running;
	}

#ifdef USING_BEHAVIOR_TREE_SERIALIZATION
	virtual void Serialize(Serialization::IArchive& archive)
	{
		BaseClass::Serialize(archive);
	}
#endif // USING_BEHAVIOR_TREE_SERIALIZATION
};

class ShootPlayer : public BehaviorTree::Action
{
	typedef BehaviorTree::Action BaseClass;
	// shoot range should be a bit larger than move range
	const float range = 7.0f;

public:
	struct RuntimeData {
		CBasicDroneEnemyComponent* drone;
	};

	virtual void OnInitialize(const BehaviorTree::UpdateContext& context) override {
		GetRuntimeData<RuntimeData>(context).drone = context.entity.GetComponent<CBasicDroneEnemyComponent>(); 
	}

	virtual BehaviorTree::Status Update(const BehaviorTree::UpdateContext& context) override {
		auto drone = GetRuntimeData<RuntimeData>(context).drone;
		if (!drone->CanSeePlayer()) {
			return BehaviorTree::Failure;
		}
		drone->ShootAtPlayer(context.frameDeltaTime);
		return BehaviorTree::Running;
	}

	static void RegisterBehaviorTreeNodes_Custom()
	{
		// DOES NOT WORK FOR SOME REASON...
		IAISystem* ai = gEnv->pAISystem;

		IBehaviorTreeManager& managerRef = *ai->GetIBehaviorTreeManager();
		const char* COLOR_GAME = "ff00ff";

		REGISTER_BEHAVIOR_TREE_NODE_WITH_SERIALIZATION(managerRef, MoveToPosition, "Game\\Enemy\\MoveToPosition", COLOR_GAME);
		REGISTER_BEHAVIOR_TREE_NODE_WITH_SERIALIZATION(managerRef, ShootPlayer, "Game\\Enemy\\ShootPlayer", COLOR_GAME);

	}
};
}
