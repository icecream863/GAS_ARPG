// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "AuraEnemy.generated.h"

class AAuraAIController;
class UBehaviorTree;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class AURA_API AAuraEnemy : public AAuraCharacterBase, public IEnemyInterface
{
	GENERATED_BODY()

public:
	AAuraEnemy();
	
	virtual void PossessedBy(AController* NewController) override;
	virtual AActor* GetCombatTarget_Implementation() override;
	virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override;
	
	/** EnemyInterface */
	virtual void HighLightActor() override;
	virtual void UnHighLightActor() override;
	/** End EnemyInterface */
	
	/** CombatInterface */
	virtual int32 GetPlayerLevel_Implementation() override;
	/** End CombatInterface */
	
	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
	virtual void BeginPlay() override;
	
	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	virtual void StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount) override;
	
	virtual void Die(const FVector& DeathImpulse) override;
	
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
