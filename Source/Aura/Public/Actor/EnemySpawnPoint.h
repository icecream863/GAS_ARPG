#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "EnemySpawnPoint.generated.h"

class AAuraEnemy;

/**
 * 敌人出生点：放在关卡里的一个目标点，配置要生成的敌人种类、等级与职业，
 * 由 SpawnVolume 触发后调用 SpawnEnemy 生成。
 */
UCLASS()
class AURA_API AAuraEnemySpawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	/** 要生成的敌人蓝图类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	TSubclassOf<AAuraEnemy> EnemyClass;

	/** 敌人等级，默认 1 级。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	int32 EnemyLevel = 1;

	/** 敌人职业，默认战士。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;

	/** 按本点配置生成一个敌人（可在蓝图里直接调用）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Spawn")
	void SpawnEnemy();
};
