// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraAbilityTypes.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraDamageGameplayAbility.generated.h"


/**
 * 带伤害类型的技能，比如火焰伤害、冰霜伤害等。它的核心作用是：在执行技能时，
 * 能把伤害类型信息传递给 GameplayEffect，从而让 GameplayEffect 能根据伤害类型做出不同的效果
 * （比如火焰伤害会附带 DOT 效果，冰霜伤害会降低目标移动速度等）。还有抗性
 * 本质就是标签
 */

struct FTaggedMontage;

UCLASS()
class AURA_API UAuraDamageGameplayAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void CauseDamage(AActor* TargetActor); 
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	
	// 基础伤害技能默认只使用一种伤害类型；特殊的多元素技能可在子类中单独扩展。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FScalableFloat Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float DebuffChance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float DebuffDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float DebuffDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float DebuffFrequency = 0.f;

	// 技能只定义冲量强度；实际方向要等 Projectile 命中时才能确定。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float DeathImpulseMagnitude = 60.f;

	// 非致死命中时由 CharacterMovement 使用的发射速度，而不是布娃娃冲量。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float KnockbackForceMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float KnockbackChance = 0.f;

	UFUNCTION(BlueprintPure, Category = "Damage")
	FDamageEffectParams MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor = nullptr) const;
	
	UFUNCTION(BlueprintPure)
	FTaggedMontage GetRandomTaggedMontageFromArray(const TArray<FTaggedMontage>& TaggedMontages);
};
