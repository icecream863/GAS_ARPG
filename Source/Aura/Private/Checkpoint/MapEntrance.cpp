#include "Checkpoint/MapEntrance.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"

AMapEntrance::AMapEntrance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 让触发球体跟随 MoveToComponent：蓝图上只需移动 MoveToComponent，
	// 球体和“点击后自动奔跑的到达点”就一起移动。
	Sphere->SetupAttachment(MoveToComponent);
}

void AMapEntrance::HighLightActor_Implementation()
{
	// 地图出入口无论是否被“到达过”都保持可高亮。
	if (CheckpointMesh)
	{
		CheckpointMesh->SetRenderCustomDepth(true);
	}
}

void AMapEntrance::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                   bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->Implements<UPlayerInterface>())
	{
		bReached = true;

		// 用目标地图的出生点标签保存玩家进度（而不是本入口的标签）。
		IPlayerInterface::Execute_SaveProgress(OtherActor, DestinationPlayerStartTag);

		// 保存世界状态，并把“将要前往的地图”信息写进存档（供加载界面/HUD 使用）。
		if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			AuraGameMode->SaveWorldState(GetWorld(), DestinationMap.ToSoftObjectPath().GetAssetName());
		}

		// 存档完成后切换到目标地图。
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, DestinationMap);
	}
}

void AMapEntrance::LoadActor_Implementation()
{
	// 由 AuraGameModeBase::LoadWorldState 里的 ISaveInterface::Execute_LoadActor 派发而来。
	// 地图入口读档时不需要“已点亮”表现，因此明确留空（检查点的同函数会重新发光）。
}
