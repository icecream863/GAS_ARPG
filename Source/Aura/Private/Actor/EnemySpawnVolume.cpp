#include "Actor/EnemySpawnVolume.h"

#include "Actor/EnemySpawnPoint.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInterface.h"

AAuraEnemySpawnVolume::AAuraEnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AAuraEnemySpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &AAuraEnemySpawnVolume::OnBoxOverlap);
}

void AAuraEnemySpawnVolume::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                         const FHitResult& SweepResult)
{
	// 只对玩家角色（实现 PlayerInterface）生效，避免其他 Pawn 触发生成。
	if (!OtherActor || !OtherActor->Implements<UPlayerInterface>())
	{
		return;
	}

	bReached = true;

	for (TObjectPtr<AAuraEnemySpawnPoint> Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->SpawnEnemy();
		}
	}

	// 【课程修正】不要 Destroy 自身：Destroy 后本 Actor 在保存世界状态时已失效，
	// bReached 无法写入存档。只禁用 Box 碰撞阻止再次触发；
	// 下次读档时由 LoadActor 根据 bReached 销毁本体积。
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AAuraEnemySpawnVolume::LoadActor_Implementation()
{
	if (bReached)
	{
		// 【修复】不要 Destroy 自身：Destroy 后本 Actor 在下次 SaveWorldState 时
		// 已不在世界中，bReached 记录会随 SavedActors 清空重建而丢失，
		// 导致第三次进图又重新生成敌人。改为隐藏并禁用碰撞：
		// 既不会重复触发，又能保证后续保存时存档记录一直有效。
		SetActorHiddenInGame(true);
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
