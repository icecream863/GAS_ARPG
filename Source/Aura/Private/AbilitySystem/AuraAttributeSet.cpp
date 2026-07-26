// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/AuraAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/PlayerInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerController.h"
#include "UObject/Package.h"

UAuraAttributeSet::UAuraAttributeSet()
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	
	//~ Primary Attributes
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Primary_Strength, GetStrengthAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Primary_Intelligence, GetIntelligenceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Primary_Resilience, GetResilienceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Primary_Vigor, GetVigorAttribute);
	
	//~ Secondary Attributes
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_Armor, GetArmorAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_ArmorPenetration, GetArmorPenetrationAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_BlockChance, GetBlockChanceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_CriticalHitDamage, GetCriticalHitDamageAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_CriticalHitResistance, GetCriticalHitResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_HealthRegeneration, GetHealthRegenerationAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_ManaRegeneration, GetManaRegenerationAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_MaxHealth, GetMaxHealthAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Attributes_Secondary_MaxMana, GetMaxManaAttribute);
	
	//~ Resistance Attributes
	TagsToAttribute.Add(GameplayTags.Get().Damage_Resistance_Fire, GetFireResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Damage_Resistance_Lightning, GetLightningResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Damage_Resistance_Arcane, GetArcaneResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Get().Damage_Resistance_Physical, GetPhysicalResistanceAttribute);
	
}

void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 属性复制：告诉 Unreal Engine 哪些属性需要在网络中同步，以及如何同步。 
	
	// DOREPLIFETIME_CONDITION_NOTIFY：
	// - COND_None：无条件复制（只要该 Actor/组件参与网络复制，就会同步）
	// - REPNOTIFY_Always：即使新旧值相同，也强制触发 OnRep（UI 刷新更稳定）
	// Vital Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Mana, COND_None, REPNOTIFY_Always);

	// Primary Attributes：同样走复制 + OnRep，用于客户端 UI/表现更新。
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Resilience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Vigor, COND_None, REPNOTIFY_Always);
	
	// Secondary Attributes：同样走复制 + OnRep，用于客户端 UI/表现更新。
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, BlockChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ManaRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	
	//Resistance
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, LightningResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArcaneResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
	
}



void UAuraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);// NewValue 是即将被写入的属性新值，Attribute 是即将被修改的属性（Health/Mana/Strength 等）。
	
	// 在属性写入前做约束：Health/Mana 不能小于 0，也不能超过各自的 Max。
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
		/** 它不会修改原始的修饰器（Modifier）数据，只是在聚合计算最终值时临时调整了结果。
		* 例如：一个 GE 可能对 Health 做了 -150 的修改，
		* 如果 MaxHealth 是 100，PreAttributeChange 会把 NewValue 从 -150 调整到 -100（Health 最多只能扣到 0），
		* 但 GE 的 Modifier 数据仍然是 -150，不会被永久修改。
		*只改变了GetHealth()的结果，但不改变 GE Modifier 的原始数据，这样就不会影响其他可能基于 Modifier 数据的逻辑（例如后续的 GE 结算、UI 显示等）。
		*/
	}

	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	
	// *** 这里限制的是current value 
}

void UAuraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetMaxHealthAttribute() && bTopOffHealth)
	{
		SetHealth(GetMaxHealth());
		bTopOffHealth = false;
	}
	
	if (Attribute == GetMaxManaAttribute() && bTopOffMana)
	{
		SetMana(GetMaxMana());
		bTopOffMana = false;
	}
}

void UAuraAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	// GE 结算后：这里通常用来处理“伤害 -> 扣血”、“吸血”、“触发受击表现”等。
	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	// Periodic debuffs may tick after the fatal hit. Once the target is dead, no further
	// attribute processing, hit react, floating text, XP, or chained debuff should run.
	if (Props.TargetAvatarActor &&
		Props.TargetAvatarActor->Implements<UCombatInterface>() &&
		ICombatInterface::Execute_IsDead(Props.TargetAvatarActor))
	{
		return;
	}
	
	// 
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Props);
	}
	
	if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		HandleIncomingXP(Props);
	}

	 /* 这里限制 base value
		duration和 infinite都会修改 current value, instant 和 period( + duration or  +infinite) 会修改 base value
		gethealth = base value + duration/infinite modifier + instant/period modifier
	*/
}

void UAuraAttributeSet::HandleIncomingDamage(const FEffectProperties& Props)
{
	const float LocalIncomingDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);
	if (LocalIncomingDamage < 0.f)
	{
		return;
	}

	const float NewHealth = GetHealth() - LocalIncomingDamage;
	SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
	const bool bFatal = NewHealth <= 0.f;

	if (bFatal)
	{
		if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor))
		{
			const FVector DeathImpulse = UAuraAbilitySystemLibrary::GetDeathImpulse(Props.EffectContextHandle);
			CombatInterface->Die(DeathImpulse);
		}
		SendXPEvent(Props);
	}
	else
	{
		const FVector KnockbackForce = UAuraAbilitySystemLibrary::GetKnockbackForce(Props.EffectContextHandle);
		if (Props.TargetCharacter && !KnockbackForce.IsNearlyZero(1.f))
		{
			// 非致死目标仍由 CharacterMovement 驱动，不能像死亡一样切换为布娃娃。
			// Context 为零向量时代表击退概率失败，因此不会进入这里。
			Props.TargetCharacter->LaunchCharacter(KnockbackForce, true, true);
		}

		const bool bIsBeingShocked = Props.TargetAvatarActor &&
			Props.TargetAvatarActor->Implements<UCombatInterface>() &&
			ICombatInterface::Execute_IsBeingShocked(Props.TargetAvatarActor);
		if (!bIsBeingShocked)
		{
			// 持续电击每秒结算十次；ShockLoop 已提供受击表现，不再重复触发 HitReact。
			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FAuraGameplayTags::Get().Effect_HitReact);
			Props.TargetASC->TryActivateAbilitiesByTag(TagContainer);
		}
	}

	const bool bIsBlockedHit = UAuraAbilitySystemLibrary::IsBlockedHit(Props.EffectContextHandle);
	const bool bIsCriticalHit = UAuraAbilitySystemLibrary::IsCriticalHit(Props.EffectContextHandle);
	ShowFloatingText(Props, LocalIncomingDamage, bIsBlockedHit, bIsCriticalHit);

	// ExecCalc 已把成功判定和所有参数写进同一个 EffectContext。
	if (UAuraAbilitySystemLibrary::IsSuccessfulDebuff(Props.EffectContextHandle))
	{
		Debuff(Props);
	}
}

void UAuraAttributeSet::HandleIncomingXP(const FEffectProperties& Props)
{
	const float LocalIncomingXP = GetIncomingXP();
	SetIncomingXP(0.f);

	// XP GameplayEffect 由玩家应用给自己，因此 SourceCharacter 就是获得经验的角色。
	if (!Props.SourceCharacter ||
		!Props.SourceCharacter->Implements<UPlayerInterface>() ||
		!Props.SourceCharacter->Implements<UCombatInterface>())
	{
		return;
	}

	const int32 CurrentXP = IPlayerInterface::Execute_GetXP(Props.SourceCharacter);
	const int32 CurrentLevel = ICombatInterface::Execute_GetPlayerLevel(Props.SourceCharacter);
	const int32 NewLevel = IPlayerInterface::Execute_FindLevelForXP(Props.SourceCharacter, CurrentXP + LocalIncomingXP);
	const int32 NumLevelUps = NewLevel - CurrentLevel;

	if (NumLevelUps > 0)
	{
		int32 AttributePointsReward = 0;
		int32 SpellPointsReward = 0;
		for (int32 Level = CurrentLevel; Level < NewLevel; ++Level)
		{
			AttributePointsReward +=
				IPlayerInterface::Execute_GetAttributePointsReward(Props.SourceCharacter, Level);
			SpellPointsReward +=
				IPlayerInterface::Execute_GetSpellPointsReward(Props.SourceCharacter, Level);
		}

		IPlayerInterface::Execute_AddToPlayerLevel(Props.SourceCharacter, NumLevelUps);
		IPlayerInterface::Execute_AddToAttributePoints(Props.SourceCharacter, AttributePointsReward);
		IPlayerInterface::Execute_AddToSpellPoints(Props.SourceCharacter, SpellPointsReward);

		bTopOffHealth = true;
		bTopOffMana = true;
		if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.SourceCharacter))
		{
			if (FOnExternalGameplayModifierDependencyChange* ExternalDelegate =
				CombatInterface->GetExternalGameplayModifierDependencyMulticast())
			{
				ExternalDelegate->Broadcast();
			}
		}

		if (bTopOffHealth)
		{
			SetHealth(GetMaxHealth());
			bTopOffHealth = false;
		}
		if (bTopOffMana)
		{
			SetMana(GetMaxMana());
			bTopOffMana = false;
		}
		IPlayerInterface::Execute_LevelUp(Props.SourceCharacter);
	}

	IPlayerInterface::Execute_AddToXP(Props.SourceCharacter, LocalIncomingXP);
}

void UAuraAttributeSet::Debuff(const FEffectProperties& Props)
{
	if (!Props.SourceASC || !Props.TargetASC)
	{
		return;
	}

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	const FGameplayTag DamageType = UAuraAbilitySystemLibrary::GetDamageType(Props.EffectContextHandle);
	const FGameplayTag* DebuffTag = GameplayTags.DamageTypesToDebuffs.Find(DamageType);
	if (!DamageType.IsValid() || !DebuffTag || !DebuffTag->IsValid())
	{
		return;
	}

	const float DebuffDamage = UAuraAbilitySystemLibrary::GetDebuffDamage(Props.EffectContextHandle);
	const float DebuffDuration = UAuraAbilitySystemLibrary::GetDebuffDuration(Props.EffectContextHandle);
	const float DebuffFrequency = UAuraAbilitySystemLibrary::GetDebuffFrequency(Props.EffectContextHandle);
	if (DebuffDuration <= 0.f)
	{
		return;
	}
	if (!FMath::IsNearlyZero(DebuffDamage) && DebuffFrequency <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = Props.SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(Props.SourceAvatarActor);
	UAuraAbilitySystemLibrary::SetDamageType(EffectContext, DamageType);

	const FString DebuffName = FString::Printf(TEXT("DynamicDebuff_%s"), *DamageType.ToString());
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(*DebuffName));
	Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Effect->DurationMagnitude = FScalableFloat(DebuffDuration);
	
	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(*DebuffTag);
	if (DebuffTag->MatchesTagExact(GameplayTags.Debuff_Stun))
	{
		// 服务端的动态 Stun GE 直接阻断全部玩家输入；客户端通过 bIsStunned RepNotify 镜像这些标签。
		// 阻挡的实现 写在 PlayerController::InputComponent 里，所有输入事件都会先检查这些标签。
		// 动态ge只在服务端，不会复制到ge，因次要在 客户端的bIsStunned RepNotify里镜像这些标签，保证客户端的输入也被阻断。
		GrantedTags.AddTag(GameplayTags.Player_Block_CursorTrace);
		GrantedTags.AddTag(GameplayTags.Player_Block_InputPressed);
		GrantedTags.AddTag(GameplayTags.Player_Block_InputHeld);
		GrantedTags.AddTag(GameplayTags.Player_Block_InputReleased);
	}
	Effect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(GrantedTags);

	// 在 UE 5.8 里，SetStackingType 这个接口被 WITH_EDITOR 限制了，
	// 运行时动态创建 GameplayEffect 时不能正常调用，所以代码只能临时直接写旧字段：
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Effect->StackingType = EGameplayEffectStackingType::AggregateBySource;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	Effect->StackLimitCount = 1;

	if (!FMath::IsNearlyZero(DebuffDamage))
	{
		Effect->Period = DebuffFrequency;
		FGameplayModifierInfo& ModifierInfo = Effect->Modifiers.AddDefaulted_GetRef();
		ModifierInfo.ModifierMagnitude = FScalableFloat(DebuffDamage);
		ModifierInfo.ModifierOp = EGameplayModOp::Additive;
		ModifierInfo.Attribute = GetIncomingDamageAttribute();
	}


	// 这个接口会把 spec 拷贝进目标 ASC，所以用局部变量就够了，不用 new 一个长期对象。
	const FGameplayEffectSpec DebuffSpec(Effect, EffectContext, 1.f);
	Props.TargetASC->ApplyGameplayEffectSpecToSelf(DebuffSpec);
}


void UAuraAttributeSet::ShowFloatingText(const FEffectProperties& Props, float Damage, bool bIsBlockedHit, bool bIsCriticalHit)
{
	if (Props.TargetCharacter != Props.SourceCharacter)
	{
		/**
		 * 中文注释：PostGameplayEffectExecute 在非预测效果的权威（服务器）上运行。我们必须在*施加者的拥有控制器*上调用客户端 RPC，
		 * 这样伤害数字才会在正确的客户端执行（不总是服务器上的玩家索引 0）。UGameplayStatics::GetPlayerController(GetWorld(), 0) 可能不适用于多人游戏或 AI 场景。
		 * 在服务端用 index=0 取到的通常是“服务端本机的第一个PC（Listen Server 的主机PC）”
		 */
		
		if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.SourceController))//自己
		{
			PC->ShowDamageNumber(Damage, Props.TargetCharacter, bIsBlockedHit, bIsCriticalHit);
			return;
		}
		if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.TargetController))//敌人
		{
			PC->ShowDamageNumber(Damage, Props.TargetCharacter, bIsBlockedHit, bIsCriticalHit);
		}
		
	}
}

void UAuraAttributeSet::SendXPEvent(const FEffectProperties& Props)
{
	if (Props.TargetCharacter && Props.TargetCharacter->Implements<UCombatInterface>())
	{
		const int32 TargetLevel = ICombatInterface::Execute_GetPlayerLevel(Props.TargetCharacter);
		const ECharacterClass TargetClass = ICombatInterface::Execute_GetCharacterClass(Props.TargetCharacter);
		// blueprint native event 不能直接调用，需要用 Execute_ 前缀来调用接口函数
		const int32 XPReward = UAuraAbilitySystemLibrary::GetRewardForClassAndLevel(Props.TargetCharacter,TargetClass, TargetLevel);		
		
		const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
		
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags.Attributes_Meta_IncomingXP;
		Payload.EventMagnitude = XPReward; 
		
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Props.SourceCharacter, GameplayTags.Attributes_Meta_IncomingXP, Payload);
		
		
	}
	
}

void UAuraAttributeSet::SetEffectProperties(const struct FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	// 目标：把这次 GE 结算的 Source/Target 重要对象一次性解析出来，
	// 后续在 PostGameplayEffectExecute 里直接用 Props，少写重复代码。
	
	// EffectContext：能拿到 Instigator/Causer/HitResult 等（是“句柄”，不是每次都创建新对象）
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	
	// SourceASC：通常是“施法者/攻击者”的 ASC（Instigator）
	Props.SourceASC = Props.EffectContextHandle.GetInstigatorAbilitySystemComponent();
	
	if (Props.SourceASC && Props.SourceASC->AbilityActorInfo && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		
		// 有些情况下 PlayerController 取不到（例如 AI 或某些服务器场景），就从 Pawn 兜底。
		if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		
		if (Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}
	
	// Target：承受效果的一方（Data.Target 就是“属性被改的那个 ASC”）
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		
		// TargetASC：如果你需要在执行后给目标发 GameplayEvent / 触发能力，这里会很常用。
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
		
		if (Props.TargetController == nullptr && Props.TargetAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Props.TargetAvatarActor))
			{
				Props.TargetController = Pawn->GetController();
			}
		}
	}
}



void UAuraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	// GAMEPLAYATTRIBUTE_REPNOTIFY：
	// 把 GAS 内部的“属性变化通知”补齐（触发 ASC 的委托/蓝图监听），并携带 oldValue。
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Health, OldHealth);
}

void UAuraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxHealth, OldHealth);
}

void UAuraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Mana, OldHealth);
}

void UAuraAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxMana, OldHealth);
}

void UAuraAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Strength, OldStrength);
}

void UAuraAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Intelligence, OldIntelligence);
}

void UAuraAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldResilience) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Resilience, OldResilience);
}

void UAuraAttributeSet::OnRep_Vigor(const FGameplayAttributeData& OldVigor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Vigor, OldVigor);
}

void UAuraAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Armor, OldArmor);
}

void UAuraAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ArmorPenetration, OldArmorPenetration);
}

void UAuraAttributeSet::OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, BlockChance, OldBlockChance);
}

void UAuraAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitChance, OldCriticalHitChance);
}

void UAuraAttributeSet::OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitDamage, OldCriticalHitDamage);
}

void UAuraAttributeSet::OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitResistance, OldCriticalHitResistance);
}

void UAuraAttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, HealthRegeneration, OldHealthRegeneration);
}

void UAuraAttributeSet::OnRep_ManaRegeneration(const FGameplayAttributeData& OldManaRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ManaRegeneration, OldManaRegeneration);
}

void UAuraAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldFireResistance) const
{	
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, FireResistance, OldFireResistance);
}

void UAuraAttributeSet::OnRep_LightningResistance(const FGameplayAttributeData& OldLightningResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, LightningResistance, OldLightningResistance);
}

void UAuraAttributeSet::OnRep_ArcaneResistance(const FGameplayAttributeData& OldArcaneResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ArcaneResistance, OldArcaneResistance);
	
}

void UAuraAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, PhysicalResistance, OldPhysicalResistance);
}
