#include "Checkpoint/Checkpoint.h"

#include "Components/SceneComponent.h"
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

	// 点击检查点时玩家自动奔跑的目标位置；默认在根组件（出生点），可在 BP 视口里移动。
	MoveToComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MoveToComponent"));
	MoveToComponent->SetupAttachment(GetRootComponent());

	// 高亮模板值在构造时设置一次（BP 可通过 CustomDepthStencilOverride 覆写颜色）。
	CheckpointMesh->SetCustomDepthStencilValue(CustomDepthStencilOverride);
	CheckpointMesh->MarkRenderStateDirty();
}

void ACheckpoint::BeginPlay()
{
	Super::BeginPlay();

	// 信标等子类可在蓝图里关闭该绑定，改用蓝图自己的 Overlap 事件（只发光不存档）。
	if (bBindOverlapCallback)
	{
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnSphereOverlap);
	}

	// 【优化】BP 资产的 CheckpointMesh 碰撞被覆盖成了全通道 Ignore，
	// 导致光标射线（ECC_Visibility）打不中检查点、无法高亮也无法点击移动。
	// 这里在运行时强制恢复 Visibility 通道的 Block，其它通道保持蓝图配置不变。
	CheckpointMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
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
			// 【课程】保存世界状态时同时写入当前地图资产名，
			// 这样死亡重生/加载界面都能知道玩家所在/要回的地图。
			UWorld* World = GetWorld();
			FString MapName = World->GetMapName();
			MapName.RemoveFromStart(World->StreamingLevelsPrefix);
			AuraGameMode->SaveWorldState(World, MapName);
		}
	}
}

void ACheckpoint::HandleGlowEffects()
{
	// 标记已触发，随世界状态保存；下次进入该地图时根据 bReached 恢复发光状态。
	bReached = true;

	// 只从基础材质创建一次动态实例；反复触发时复用同一个 MID，
	// 避免从“已点亮的动态实例”再创建子实例，导致参数继承终值、二次发光看不出变化。
	if (!CheckpointDynamicMaterial)
	{
		CheckpointBaseMaterial = CheckpointMesh->GetMaterial(0);
		CheckpointDynamicMaterial = UMaterialInstanceDynamic::Create(CheckpointBaseMaterial, this);
		CheckpointMesh->SetMaterial(0, CheckpointDynamicMaterial);
	}

	// 先把 Glow 参数复位为 0，保证每次触发都能看到“熄灭→发光”的动画反馈。
	CheckpointDynamicMaterial->SetScalarParameterValue(TEXT("Glow Factor"), 0.f);

	// 信标等“只亮一次”的对象在到达后禁用触发球体；检查点保持可反复触发。
	if (bDisableSphereOnReach)
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 交给蓝图处理发光效果（Timeline 渐变等）。
	CheckpointReached(CheckpointDynamicMaterial);
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
		// 读档后恢复发光表现；检查点保持可重叠，玩家可随时回来重新存档。
		HandleGlowEffects();
	}
}

void ACheckpoint::SetMoveToLocation_Implementation(FVector& OutDestination)
{
	if (MoveToComponent)
	{
		OutDestination = MoveToComponent->GetComponentLocation();
	}
}

void ACheckpoint::HighLightActor_Implementation()
{
	// 课程行为：已到达（bReached）的对象不再高亮（信标点亮后不再显示描边）。
	if (CheckpointMesh && !bReached)
	{
		CheckpointMesh->SetRenderCustomDepth(true);
	}
}

void ACheckpoint::UnHighLightActor_Implementation()
{
	if (CheckpointMesh)
	{
		CheckpointMesh->SetRenderCustomDepth(false);
	}
}
