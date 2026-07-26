// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Interaction/CombatInterface.h"

FDamageEffectParams UAuraDamageGameplayAbility::MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor) const
{
	FDamageEffectParams Params;
	// 目标在 Projectile 生成时可能还未知，所以 TargetActor 设计成可选参数。
	Params.WorldContextObject = GetAvatarActorFromActorInfo();
	Params.DamageGameplayEffectClass = DamageEffectClass;
	Params.SourceAbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	Params.TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	// 技能类默认值保存的是曲线，真正传给 GE 的是当前等级计算后的数值。
	Params.BaseDamage = Damage.GetValueAtLevel(GetAbilityLevel());
	Params.AbilityLevel = GetAbilityLevel();
	Params.DamageType = DamageType;
	Params.DebuffChance = DebuffChance;
	Params.DebuffDamage = DebuffDamage;
	Params.DebuffDuration = DebuffDuration;
	Params.DebuffFrequency = DebuffFrequency;
	Params.DeathImpulseMagnitude = DeathImpulseMagnitude;
	Params.KnockbackForceMagnitude = KnockbackForceMagnitude;
	Params.KnockbackChance = KnockbackChance;

	if (IsValid(TargetActor))
	{
		if (const AActor* SourceAvatarActor = GetAvatarActorFromActorInfo())
		{
			// 直接伤害没有 Projectile 的飞行方向，因此用“施法者 -> 目标”生成默认方向，
			// 再固定抬高 45 度；投射物命中时会用真实命中方向覆盖它。
			FRotator KnockbackRotation = (TargetActor->GetActorLocation() - SourceAvatarActor->GetActorLocation()).Rotation();
			KnockbackRotation.Pitch = 45.f;
			const FVector KnockbackDirection = KnockbackRotation.Vector();
			Params.DeathImpulse = KnockbackDirection * DeathImpulseMagnitude;
			Params.KnockbackForce = KnockbackDirection * KnockbackForceMagnitude;
		}
	}
	return Params;
}

void UAuraDamageGameplayAbility::CauseDamage(AActor* TargetActor)
{
	UAuraAbilitySystemLibrary::ApplyDamageEffect(MakeDamageEffectParamsFromClassDefaults(TargetActor));
}

float UAuraDamageGameplayAbility::GetDamageAtLevel() const
{
	return Damage.GetValueAtLevel(GetAbilityLevel());
}

void UAuraDamageGameplayAbility::CauseDamageWithoutDebuff(AActor* TargetActor)
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(SourceASC) || !IsValid(TargetASC) || !DamageEffectClass || !DamageType.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		DamageEffectClass,
		GetAbilityLevel(),
		EffectContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 持续电击只传入伤害类型和当前等级的伤害值；不设置 Debuff 的 SetByCaller 数据。
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		DamageType,
		GetDamageAtLevel());
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

FTaggedMontage UAuraDamageGameplayAbility::GetRandomTaggedMontageFromArray(
	const TArray<FTaggedMontage>& TaggedMontages)
{
	if (TaggedMontages.Num() == 0)
	{
		return FTaggedMontage();
	}
	
	const int32 RandomIndex = FMath::RandRange(0, TaggedMontages.Num() - 1);
	return TaggedMontages[RandomIndex];
}
