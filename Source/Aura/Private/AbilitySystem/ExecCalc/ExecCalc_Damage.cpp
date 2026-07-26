// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "Interaction/CombatInterface.h"

/** 
 * \brief 定义一个结构体，用于集中管理我们要在此 Execution Calculation 中捕获的属性（CaptureDef）。
 * 这种写法是 GAS 官方推荐的设计模式，将属性的宏声明和定义包裹在结构体中，以便于配合单例模式使用。
* 	FGameplayTag Attributes_Secondary_CriticalHitChance;
	FGameplayTag Attributes_Secondary_CriticalHitDamage;
	FGameplayTag Attributes_Secondary_CriticalHitResistance;
 */
struct AuraDamageStatics
{
	/** 
	 * 宏声明：这会在后台自动声明两个变量：
	 * 1. ArmorProperty (FGameplayAttribute) -> 指向属性本身
	 * 2. ArmorDef (FGameplayEffectAttributeCaptureDefinition) -> 属性抓取规则
	 */
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(BlockChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitResistance);
	
	DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArcaneResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalResistance);
	
	AuraDamageStatics()
	{
		/** 
		 * 宏定义/初始化：配置我们该如何抓取 Armor 属性，参数依次为：
		 * - 属性所在的类：UAuraAttributeSet
		 * - 要抓取的属性名：Armor
		 * - 抓取的对象：Target (这里指我们要获取受击方/目标的护甲值)
		 * - 是否使用快照(Snapshot)：false (代表不在 GE 生成时抓取，而是要在 GE 应用结算时的瞬间实时抓取)
		 */
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, Armor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, BlockChance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, ArmorPenetration, Source, false);//护甲穿透看的是 攻击者的属性，所以 AttributeSource 是 Source
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, CriticalHitChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, CriticalHitDamage, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, CriticalHitResistance, Target, false);
		
		// 抗性是“受击者/目标(Target)”的属性，因此从 Target 抓取
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, FireResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, LightningResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, ArcaneResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, PhysicalResistance, Target, false);
	}
};

/** 
 * \brief 全局静态获取函数（单例模式）
 * 使用静态局部变量 DStatics，确保 AuraDamageStatics 结构体有且仅被初始化一次，
 * 这样既节省性能，也避免了因为多次初始化导致的捕获定义不一致。
 */
static const AuraDamageStatics& DamageStatics()
{
	static AuraDamageStatics DStatics;
	return DStatics;
}

// 根据 Spec 中实际存在的伤害类型，判断对应 Debuff 是否触发。
static void DetermineDebuff(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	const FGameplayEffectSpec& Spec,
	const FAggregatorEvaluateParameters& EvaluationParameters,
	const TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition>& TagsToCaptureDefs)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	for (const TPair<FGameplayTag, FGameplayTag>& Pair : GameplayTags.DamageTypesToDebuffs)
	{
		const FGameplayTag& DamageType = Pair.Key;
		const FGameplayTag& DebuffType = Pair.Value;

		// 未被当前 Spec 设置的伤害类型返回 -1；只有实际造成的伤害类型才参与判定。
		const float TypeDamage = Spec.GetSetByCallerMagnitude(DamageType, false, -1.f);
		if (TypeDamage <= -1.f)
		{
			continue;
		}

		const float SourceDebuffChance =
			Spec.GetSetByCallerMagnitude(GameplayTags.Debuff_Chance, false, -1.f);
		if (SourceDebuffChance <= 0.f)
		{
			continue;
		}

		const FGameplayTag& ResistanceTag = GameplayTags.DamageTypesToResistances.FindChecked(DamageType);
		const FGameplayEffectAttributeCaptureDefinition* CaptureDef = TagsToCaptureDefs.Find(ResistanceTag);
		checkf(CaptureDef, TEXT("Resistance tag %s has no matching capture definition while determining debuff %s"),
			*ResistanceTag.ToString(), *DebuffType.ToString());

		float TargetDebuffResistance = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			*CaptureDef, EvaluationParameters, TargetDebuffResistance);
		TargetDebuffResistance = FMath::Max(TargetDebuffResistance, 0.f);

		// 每 1 点对应抗性降低 1% 的 Debuff 触发概率。
		const float EffectiveDebuffChance = FMath::Clamp(
			SourceDebuffChance * (100.f - TargetDebuffResistance) / 100.f,
			0.f,
			100.f);
		// FRand 返回 [0, 1)，因此零抗性下配置 100% 时一定成功，同时保留小数概率精度。
		const bool bDebuff = FMath::FRand() * 100.f < EffectiveDebuffChance;
		if (bDebuff)
		{
			FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
			UAuraAbilitySystemLibrary::SetIsSuccessfulDebuff(ContextHandle, true);
			UAuraAbilitySystemLibrary::SetDamageType(ContextHandle, DamageType);

			// 这些值已经由 ApplyDamageEffect 写入 Spec，此处转存到 Context 供 AttributeSet 使用。
			const float DebuffDamage =
				Spec.GetSetByCallerMagnitude(GameplayTags.Debuff_Damage, false, -1.f);
			const float DebuffDuration =
				Spec.GetSetByCallerMagnitude(GameplayTags.Debuff_Duration, false, -1.f);
			const float DebuffFrequency =
				Spec.GetSetByCallerMagnitude(GameplayTags.Debuff_Frequency, false, -1.f);
			UAuraAbilitySystemLibrary::SetDebuffDamage(ContextHandle, DebuffDamage);
			UAuraAbilitySystemLibrary::SetDebuffDuration(ContextHandle, DebuffDuration);
			UAuraAbilitySystemLibrary::SetDebuffFrequency(ContextHandle, DebuffFrequency);
			return;
		}
	}
}

UExecCalc_Damage::UExecCalc_Damage()
{
	/** 
	 * 1. 在结构体中声明捕获定义
	 * 2. 在构造函数中定义并注册捕获规则
	 * 3. 在 Execute_Implementation 中读取捕获到的属性值
	 *
	 * 在 Execution 类的构造函数中，把你定义好的捕获规则添加到 RelevantAttributesToCapture 数组。
	 * 这里告诉 GAS 系统："在真正执行计算逻辑前，请帮我把目标(Target)的护甲(Armor)数据准备好！"
	 */
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().BlockChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitResistanceDef);
	
	RelevantAttributesToCapture.Add(DamageStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().LightningResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArcaneResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalResistanceDef);
	
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	
	AActor* SourceAvatar = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
	check(SourceAvatar && SourceAvatar->Implements<UCombatInterface>());
	check(TargetAvatar && TargetAvatar->Implements<UCombatInterface>());
	
	const FGameplayEffectSpec Spec = ExecutionParams.GetOwningSpec();
	
	// Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	/**~EvaluationParameters 结构体是 GAS里一个用来传递额外信息的结构体，在计算MMC/Execution时会被传入，
	~里面有个成员：TagsToConsider，可以把我们想要的标签传进去，GAS就会在计算时考虑这些标签对属性的影响（比如加成/减成）。*/
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	// 局部建立映射，避免静态 ExecCalc 对象早于 Native GameplayTags 初始化。
	TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> TagsToCaptureDefs;
	TagsToCaptureDefs.Add(GameplayTags.Damage_Resistance_Fire, DamageStatics().FireResistanceDef);
	TagsToCaptureDefs.Add(GameplayTags.Damage_Resistance_Lightning, DamageStatics().LightningResistanceDef);
	TagsToCaptureDefs.Add(GameplayTags.Damage_Resistance_Arcane, DamageStatics().ArcaneResistanceDef);
	TagsToCaptureDefs.Add(GameplayTags.Damage_Resistance_Physical, DamageStatics().PhysicalResistanceDef);
	
	FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();

	// 成功时 DetermineDebuff 会把结果写进当前 Spec 的 EffectContext。
	DetermineDebuff(ExecutionParams, Spec, EvaluationParameters, TagsToCaptureDefs);
	// 从 EffectContextHandle 里读取是否格挡/暴击的字段，这些字段是我们在自定义 EffectContext 里添加的，并通过 NetSerialize 同步到客户端的。
	
	
	
	//Get Damage by Caller Magnitude
	//中文注释：从 GE 的 SetByCaller 里获取伤害值。我们在技能蓝图里配置这个 GE 时，会通过 SetByCaller 的方式把伤害值传进来，这样就能让伤害值具有更好的灵活性和可调节性。
	
	float Damage = 0.f;
	
	for (const TPair<FGameplayTag, FGameplayTag>& Pair : FAuraGameplayTags::Get().DamageTypesToResistances)
	{
		FGameplayTag DamageTag = Pair.Key;
		FGameplayTag ResistanceTag = Pair.Value;
		
		checkf(TagsToCaptureDefs.Contains(ResistanceTag),
			TEXT("DamageType %s does not have a corresponding CaptureDef for resistance tag %s. (TagsToCaptureDefs Num=%d)"),
			*DamageTag.ToString(), *ResistanceTag.ToString(), TagsToCaptureDefs.Num());
		
		// 用 FindChecked：1) const 安全；2) 缺失时会直接 assert，定位更清晰
		const FGameplayEffectAttributeCaptureDefinition& CaptureDef = TagsToCaptureDefs.FindChecked(ResistanceTag);
		
		float DamageTypeValue =  Spec.GetSetByCallerMagnitude(Pair.Key, false);//从 GE 的 SetByCaller 里获取这个伤害类型的伤害值,没有返回 0
		
		float Resistance = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, Resistance);
		Resistance = FMath::Clamp(Resistance, 0.f, 100.f);
		
		DamageTypeValue *= (100.f - Resistance) / 100.f;//根据抗性计算这个伤害类型的实际伤害值
			
		Damage += DamageTypeValue;
	}
	
	
	
	/** 捕获值模板 */
	float TargetBlockChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BlockChanceDef, EvaluationParameters, TargetBlockChance);
	TargetBlockChance = FMath::Max<float>(TargetBlockChance, 0.f);
	
	float TargetArmor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluationParameters, TargetArmor);
	TargetArmor = FMath::Max<float>(TargetArmor, 0.f);
	
	float SourceArmorPenetration = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationDef, EvaluationParameters, SourceArmorPenetration);
	SourceArmorPenetration = FMath::Max<float>(SourceArmorPenetration, 0.f);
	
	float SourceCriticalHitChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef, EvaluationParameters, SourceCriticalHitChance);
	SourceCriticalHitChance = FMath::Max<float>(SourceCriticalHitChance, 0.f);
	
	float SourceCriticalHitDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitDamageDef, EvaluationParameters, SourceCriticalHitDamage);
	SourceCriticalHitDamage = FMath::Max<float>(SourceCriticalHitDamage, 0.f);
	
	float TargetCriticalHitResistance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitResistanceDef, EvaluationParameters, TargetCriticalHitResistance);
	TargetCriticalHitResistance = FMath::Max<float>(TargetCriticalHitResistance, 0.f);
	
	
	//获取伤害计算系数
	UCharacterClassInfo* CharacterClassInfo = UAuraAbilitySystemLibrary::GetCharacterClassInfo(SourceAvatar);
	const FRealCurve* ArmorPenetrationCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("ArmorPenetration"), FString());
	const FRealCurve* EffectiveArmorCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("EffectiveArmor"), FString());
	const FRealCurve* CriticalHitResistanceCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("CriticalHitResistance"), FString());
	
	const float ArmorPenetrationCoefficient = ArmorPenetrationCurve->Eval(ICombatInterface::Execute_GetPlayerLevel(SourceAvatar));
	const float EffectiveArmorCoefficient = EffectiveArmorCurve->Eval(ICombatInterface::Execute_GetPlayerLevel(TargetAvatar));
	const float CriticalHitResistanceCoefficient = CriticalHitResistanceCurve->Eval(ICombatInterface::Execute_GetPlayerLevel(TargetAvatar));
	
	//格挡伤害减半
	const bool bBlock = FMath::RandRange(1, 100) < TargetBlockChance;
	Damage = bBlock ? Damage/2.f : Damage;
	
	UAuraAbilitySystemLibrary::SetBlockedHit(EffectContextHandle, bBlock);
	//在GE Context里设置是否格挡的字段，这样客户端就能通过 EffectContext 读取到这个信息来显示格挡文本/特效等。
	
	//计算护甲穿透 和 护甲减伤
	float EffectiveArmor = TargetArmor * (100.f - SourceArmorPenetration * ArmorPenetrationCoefficient) / 100.f;
	Damage *= (100.f - EffectiveArmor * EffectiveArmorCoefficient) / 100.f;
	
	//计算 暴击率，暴击伤害，暴击抗性
	const float EffectiveCriticalHitChance = SourceCriticalHitChance - TargetCriticalHitResistance * CriticalHitResistanceCoefficient;
	const bool bCriticalHit = FMath::RandRange(1, 100) < EffectiveCriticalHitChance;
	Damage = bCriticalHit ? 2 * Damage + SourceCriticalHitDamage : Damage;
	
	const FGameplayEffectContext* Ctx = EffectContextHandle.Get();
	UE_LOG(LogTemp, Warning, TEXT("CtxStruct=%s"), *GetNameSafe(Ctx ? Ctx->GetScriptStruct() : nullptr));
	
	UAuraAbilitySystemLibrary::SetCriticalHit(EffectContextHandle, bCriticalHit);
	//在GE Context里设置是否暴击的字段，这样客户端就能通过 EffectContext 读取到这个信息来显示暴击文本/特效等。
	
	
	const FGameplayModifierEvaluatedData EvaluationData(UAuraAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluationData);
	//把计算得到的Armor值封装成一个FGameplayModifierEvaluatedData结构体，并添加到OutExecutionOutput里，这样GAS系统就会知道要把这个值应用到目标的Armor属性上，应用方式是Additive（加成）。
}
