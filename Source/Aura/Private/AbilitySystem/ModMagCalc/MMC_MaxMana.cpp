// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxMana.h"

#include "AbilitySystem/AuraAttributeSet.h"
#include "Interaction/CombatInterface.h"



UMMC_MaxMana::UMMC_MaxMana()
{
	IntelligenceDef.AttributeToCapture = UAuraAttributeSet::GetIntelligenceAttribute();
	IntelligenceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	IntelligenceDef.bSnapshot = false;
}

float UMMC_MaxMana::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	float Intelligence = 0.f;
	GetCapturedAttributeMagnitude(IntelligenceDef, Spec, EvaluationParameters, Intelligence);
	Intelligence = FMath::Max(Intelligence, 0.f);
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	check(CombatInterface);
	const int32 PlayerLevel = ICombatInterface::Execute_GetPlayerLevel(Spec.GetContext().GetSourceObject());
	
	return 50.f + 2.5f * Intelligence + 15.f * PlayerLevel;// 计算公式
}

FOnExternalGameplayModifierDependencyChange* UMMC_MaxMana::GetExternalModifierDependencyMulticast(
	const FGameplayEffectSpec& Spec, UWorld* World) const
{
	// PlayerLevel 不在 AttributeSet 里，GAS 不会自动感知它变化；通过外部委托手动触发重算。
	// 兜底委托：卸载/对象销毁阶段拿不到 SourceObject 时返回它，
	// 保证引擎的依赖清理（RemoveCustomMagnitudeExternalDependencies）能走完，
	// 避免 FActiveGameplayEffectsContainer::Uninitialize 的 ensure 失败。
	static FOnExternalGameplayModifierDependencyChange FallbackDelegate;

	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject()))
	{
		if (FOnExternalGameplayModifierDependencyChange* ExternalDelegate =
			CombatInterface->GetExternalGameplayModifierDependencyMulticast())
		{
			return ExternalDelegate;
		}
	}
	return &FallbackDelegate;
}
