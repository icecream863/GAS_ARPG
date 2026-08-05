// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "Interaction/HighlightInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "AbilitySystem/Data/LootTiers.h"
#include "AuraEnemy.generated.h"

class AAuraAIController;
class UBehaviorTree;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class AURA_API AAuraEnemy : public AAuraCharacterBase, public IEnemyInterface, public IHighlightInterface
{
	GENERATED_BODY()

public:
	AAuraEnemy();
	
	virtual void PossessedBy(AController* NewController) override;
	virtual AActor* GetCombatTarget_Implementation() override;
	virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override;
	
	/** HighlightInterface */
	virtual void HighLightActor_Implementation() override;
	virtual void UnHighLightActor_Implementation() override;
	/** 敌人不覆盖点击移动目的地：空实现，明确意图。 */
	virtual void SetMoveToLocation_Implementation(FVector& OutDestination) override;
	/** End HighlightInterface */
	
	/** CombatInterface */
	virtual int32 GetPlayerLevel_Implementation() override;
	/** End CombatInterface */

	/** 由生成点等系统在 SpawnActorDeferred 后、FinishSpawning 前设置敌人等级。 */
	UFUNCTION(BlueprintCallable, Category = "Character Class Default")
	void SetLevel(int32 InLevel) { Level = InLevel; }
	
	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
	virtual void BeginPlay() override;
	
	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	virtual void StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount) override;
	
	virtual void Die(const FVector& DeathImpulse) override;

	/** 敌人死亡时结算并生成掉落物；C++ 提供默认实现，蓝图仍可覆写定制。 */
	UFUNCTION(BlueprintNativeEvent)
	void SpawnLoot();

protected:
	/** 掉落物依次生成的间隔（秒）。 */
	UPROPERTY(EditAnywhere, Category = "Loot")
	float LootSpawnInterval = 0.1f;

	/** 掉落物离敌人的最小/最大散布距离（随机）。 */
	UPROPERTY(EditAnywhere, Category = "Loot")
	float MinSpawnDistance = 25.f;

	UPROPERTY(EditAnywhere, Category = "Loot")
	float MaxSpawnDistance = 150.f;

	/** 依次生成一个掉落物；由定时器驱动。 */
	void SpawnNextLootItem();

	FTimerHandle LootTimer;
	TArray<FLootItem> LootItems;
	TArray<FRotator> LootRotations;
	int32 SpawnLoopCount = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bHitReact = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float LifeSpan = 5.f;
	
protected:
	virtual void InitAbilityActorInfo() override;
	virtual void InitialDefaultAttributes() const override;
	
	
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Character Class Default")
    int32 Level = 1;// 
	
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy | Widget")
	TObjectPtr<UWidgetComponent> HealthBar;
	//这个是 控件组件，需要自己在ue里设置 widgetClass

	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
	
	UPROPERTY()
	TObjectPtr<AAuraAIController> AuraAIController;
	//这是 实例，蓝图里需要 设置一个 类
	
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	TObjectPtr<AActor> CombatTarget;
};
