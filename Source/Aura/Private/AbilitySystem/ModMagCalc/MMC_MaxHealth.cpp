// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxHealth.h"

#include "AbilitySystem/AuraAttributeSet.h"
#include "Interaction/CombatInterface.h"

UMMC_MaxHealth::UMMC_MaxHealth()
{
	VigorDef.AttributeToCapture = UAuraAttributeSet::GetVigorAttribute();
	VigorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VigorDef.bSnapshot = false;
}

float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	
	// Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	//~EvaluationParameters 结构体是 GAS里一个用来传递额外信息的结构体，在计算MMC/Execution时会被传入，
	//~里面有个成员：TagsToConsider，可以把我们想要的标签传进去，GAS就会在计算时考虑这些标签对属性的影响（比如加成/减成）。
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	float Vigor = 0.f;
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);
	//从Spec里抓取Vigor属性的数值，考虑标签的影响，并把结果存到Vigor变量里
	Vigor = FMath::Max(Vigor, 0.f);//确保Vigor不会是负数
	
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	check(CombatInterface);
	const int32 PlayerLevel = ICombatInterface::Execute_GetPlayerLevel(Spec.GetContext().GetSourceObject());
	
	return 80.f + 2.5f * Vigor + 10.f * PlayerLevel;//~ 这个公式是我随便写的，实际游戏里可以根据需要调整
	//这就是自定义计算公式的地方，我们可以根据Vigor属性和玩家等级来计算最大生命值的基数。
}

FOnExternalGameplayModifierDependencyChange* UMMC_MaxHealth::GetExternalModifierDependencyMulticast(
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
