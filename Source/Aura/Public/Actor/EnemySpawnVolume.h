#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SaveInterface.h"
#include "EnemySpawnVolume.generated.h"

class UBoxComponent;
class AAuraEnemySpawnPoint;

/**
 * 敌人生成体积：玩家进入 Box 后按 SpawnPoints 生成敌人。
 * 实现 ISaveInterface，触发状态（bReached）随世界状态保存，
 * 读档时若已触发过则直接销毁自身，不再重复生成。
 */
UCLASS()
class AURA_API AAuraEnemySpawnVolume : public AActor, public ISaveInterface
{
	GENERATED_BODY()

public:
	AAuraEnemySpawnVolume();

	/** 是否已触发过；SaveGame 使其随世界状态序列化。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Enemy Spawn")
	bool bReached = false;

	/** 触发后要生成敌人的出生点列表（在关卡实例里配置）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawn")
	TArray<TObjectPtr<AAuraEnemySpawnPoint>> SpawnPoints;

	// ---- SaveInterface ----
	/** 读档后若已触发过，销毁本体积（敌人不会再次生成）。 */
	virtual void LoadActor_Implementation() override;
	// ---- End SaveInterface ----

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                          const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Box;
};
