#include "Checkpoint/Checkpoint.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

ACheckpoint::ACheckpoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// mesh 作为检查点外观，同时是发光参数的目标组件；与字幕一致用 QueryAndPhysics+BlockAll。
	// 注意：出生点（根胶囊体）上方不要被 mesh 覆盖，否则角色生成时碰撞失败。
	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMesh"));
	CheckpointMesh->SetupAttachment(GetRootComponent());
	CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CheckpointMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Sphere 附着在 mesh 上，随 mesh 移动；仅对 Pawn 做重叠查询，用于触发检查点。
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(CheckpointMesh);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ACheckpoint::BeginPlay()
{
	Super::BeginPlay();
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnSphereOverlap);
}

void ACheckpoint::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                  const FHitResult& SweepResult)
{
	// PlayerInterface 只由人类控制角色实现；通过接口触发保存，避免依赖蓝图 Actor Tag。
	if (OtherActor && OtherActor->Implements<UPlayerInterface>())
	{
		HandleGlowEffects();
		IPlayerInterface::Execute_SaveProgress(OtherActor, PlayerStartTag);

		// 玩家进度（属性/能力）与世界状态（检查点是否已触发）一起落盘。
		if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			AuraGameMode->SaveWorldState(GetWorld());
		}
	}
}

void ACheckpoint::HandleGlowEffects()
{
	// 标记已触发，随世界状态保存；下次进入该地图时根据 bReached 恢复发光状态。
	bReached = true;

	// 先禁碰撞保证只触发一次。
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 用 mesh 当前材质创建动态实例并替换，之后蓝图可对 Glow 参数做动画。
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(
		CheckpointMesh->GetMaterial(0), this);
	CheckpointMesh->SetMaterial(0, DynamicMaterialInstance);

	// 交给蓝图处理发光效果（Timeline 渐变等）。
	CheckpointReached(DynamicMaterialInstance);
}

bool ACheckpoint::ShouldLoadTransform_Implementation() const
{
	// 检查点是固定的 PlayerStart，读档时不需要移动它。
	return false;
}

void ACheckpoint::LoadActor_Implementation()
{
	if (bReached)
	{
		// 复用触发逻辑：禁碰撞 + 动态材质发光。
		HandleGlowEffects();
	}
}
