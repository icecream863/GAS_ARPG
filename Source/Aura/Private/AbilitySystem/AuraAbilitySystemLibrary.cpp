// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffectTypes.h"
#include "Engine/Engine.h"
#include "Game/AuraGameModeBase.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/WidgetController/AuraWidgetController.h"


bool UAuraAbilitySystemLibrary::MakeWidgetControllerParams(const UObject* WorldContextObject,
	FWidgetControllerParams& OutWCParams, AAuraHUD*& OutAuraHUD)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		OutAuraHUD = Cast<AAuraHUD>(PC->GetHUD());
		
		if (OutAuraHUD)
		{
			AAuraPlayerState* PS = PC->GetPlayerState<AAuraPlayerState>();	
			UAttributeSet* AS = PS->GetAttributeSet();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();

			OutWCParams = FWidgetControllerParams(PS, PC, ASC, AS);
			
			return true;
		}
	}
	
	return false;
}

UOverlayWidgetController* UAuraAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WidgetControllerParams;
	AAuraHUD* AuraHUD = nullptr;
	
	if (MakeWidgetControllerParams(WorldContextObject, WidgetControllerParams, AuraHUD))
	{
		return AuraHUD->GetOverlayWidgetController(WidgetControllerParams);
	}
	
	return nullptr;
}

UAttributeMenuWidgetController* UAuraAbilitySystemLibrary::GetAttributeMenuWidgetController(
	const UObject* WorldContextObject)
{
	FWidgetControllerParams WidgetControllerParams;
	AAuraHUD* AuraHUD = nullptr;
	
	if (MakeWidgetControllerParams(WorldContextObject, WidgetControllerParams, AuraHUD))
	{
		return AuraHUD->GetAttributeMenuWidgetController(WidgetControllerParams);
	}
	
	return nullptr;
}

USpellMenuWidgetController* UAuraAbilitySystemLibrary::GetSpellMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WidgetControllerParams;
	AAuraHUD* AuraHUD = nullptr;
	
	if (MakeWidgetControllerParams(WorldContextObject, WidgetControllerParams, AuraHUD))
	{
		return AuraHUD->GetSpellMenuWidgetController(WidgetControllerParams);
	}
	
	return nullptr;
}

void UAuraAbilitySystemLibrary::InitializeDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return;//客户端没有 GameMode,在客户端执行就会返回 空指针
	
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	FCharacterClassDefaultInfo ClassDefaultInfo = CharacterClassInfo->GetCharacterClassDefaultInfo(CharacterClass);
	
	if (ClassDefaultInfo.PrimaryAttribute)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(WorldContextObject);
		
		FGameplayEffectSpecHandle PrimaryAttributesSpecHandle = ASC->MakeOutgoingSpec(ClassDefaultInfo.PrimaryAttribute, Level, EffectContext);
		if (PrimaryAttributesSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributesSpecHandle.Data.Get());
		}
		
		FGameplayEffectSpecHandle SecdaryAttibutesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttribute, Level, EffectContext);
		if (SecdaryAttibutesSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SecdaryAttibutesSpecHandle.Data.Get());
		}
		
		FGameplayEffectSpecHandle VitalAttibutesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttribute, Level, EffectContext);
		if (VitalAttibutesSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*VitalAttibutesSpecHandle.Data.Get());
		}
		
	}
}

void UAuraAbilitySystemLibrary::GiveStartupAbilities(const UObject* WorldContextObject,UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return;//客户端没有 GameMode,在客户端执行就会返回 空指针
	
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return;
	
	
	for (TSubclassOf<UGameplayAbility> AbilityClass: CharacterClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}	
	
	const FCharacterClassDefaultInfo& ClassDefaultInfo = CharacterClassInfo->GetCharacterClassDefaultInfo(CharacterClass);
	for (TSubclassOf<UGameplayAbility> AbilityClass: ClassDefaultInfo.StartupAbilities)
	{
		if (AActor* AvatarActor = ASC->GetAvatarActor(); AvatarActor && AvatarActor->Implements<UCombatInterface>())
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, ICombatInterface::Execute_GetPlayerLevel(AvatarActor));
			ASC->GiveAbility(AbilitySpec);
		}
		
	}
}

int32 UAuraAbilitySystemLibrary::GetRewardForClassAndLevel(const UObject* WorldContextObject,
	const ECharacterClass CharacterClass, const int32 CharacterLevel)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return 0;
	
	const FCharacterClassDefaultInfo& Info = CharacterClassInfo->GetCharacterClassDefaultInfo(CharacterClass);
	
	const float XPReward = Info.XPReward.GetValueAtLevel(CharacterLevel);
	
	return FMath::RoundToInt(XPReward);
}

UCharacterClassInfo* UAuraAbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return nullptr;//客户端没有 GameMode,在客户端执行就会返回 空指针
	
	return AuraGameMode->CharacterClassInfo;
}

UAbilityInfo* UAuraAbilitySystemLibrary::GetAbilityInfo(const UObject* WorldContextObject)
{
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return nullptr;//客户端没有 GameMode,在客户端执行就会返回 空指针
	
	return AuraGameMode->AbilityInfo; // 存在 GameMode 里，所有玩家共享一个 AbilityInfo 数据表
}

bool UAuraAbilitySystemLibrary::IsBlockedHit(const FGameplayEffectContextHandle& EffectContextHandle)
{	
	/**
	 * 这里使用 static_cast 而不是 Cast<>()：
	 * - EffectContextHandle.Get() 返回的是 FGameplayEffectContext*（这是 USTRUCT，不是 UObject）
	 * - UE 的 Cast<>() 只能用于 UObject 体系（依赖 UClass/IsA），对 UStruct 无效
	 *
	 * 因为我们通过 UAuraAbilitySystemGlobals::AllocGameplayEffectContext()
	 * 让项目默认分配的是 FAuraGameplayEffectContext，所以这里把父类指针还原为子类指针来读取自定义字段。
	 *
	 * 注意：static_cast 不做运行时类型检查。
	 * 如果某些 EffectContext 不是 FAuraGameplayEffectContext（例如未正确配置 AbilitySystemGlobalsClassName），
	 * 这里的转换可能导致未定义行为。更稳的做法是先检查：
	 *   EffectContextHandle.Get()->GetScriptStruct() == FAuraGameplayEffectContext::StaticStruct()
	 */
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsBlockedHit();
	}
	
	return false;
}

bool UAuraAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{	
	/**
	 * 同 IsBlockedHit()：EffectContext 是 UStruct（非 UObject），因此不能用 Cast<>。
	 * 使用 static_cast 将 FGameplayEffectContext* 还原为 FAuraGameplayEffectContext* 以读取自定义字段。
	 */
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsCriticalHit();
	}
	
	return false;
}

bool UAuraAbilitySystemLibrary::IsSuccessfulDebuff(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsSuccessfulDebuff();
	}
	return false;
}

float UAuraAbilitySystemLibrary::GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDamage();
	}
	return 0.f;
}

float UAuraAbilitySystemLibrary::GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDuration();
	}
	return 0.f;
}

float UAuraAbilitySystemLibrary::GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffFrequency();
	}
	return 0.f;
}

FGameplayTag UAuraAbilitySystemLibrary::GetDamageType(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		const TSharedPtr<FGameplayTag> DamageType = AuraEffectContext->GetDamageType();
		if (DamageType.IsValid())
		{
			// 对外返回值而不是共享指针，调用方无需关心 Context 内部的指针生命周期。
			return *DamageType;
		}
	}
	return FGameplayTag();
}

FVector UAuraAbilitySystemLibrary::GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDeathImpulse();
	}
	return FVector::ZeroVector;
}

FVector UAuraAbilitySystemLibrary::GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetKnockbackForce();
	}
	return FVector::ZeroVector;
}

void UAuraAbilitySystemLibrary::SetBlockedHit(FGameplayEffectContextHandle& EffectContextHandle, bool bIsBlockedHit)
{
	if ( FAuraGameplayEffectContext* AuraEffectContext = static_cast< FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetBlockedHit(bIsBlockedHit);
	}
}

void UAuraAbilitySystemLibrary::SetCriticalHit(FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetCriticalHit(bInIsCriticalHit);
	}
}

void UAuraAbilitySystemLibrary::SetIsSuccessfulDebuff(
	FGameplayEffectContextHandle& EffectContextHandle, bool bInIsSuccessfulDebuff)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsSuccessfulDebuff(bInIsSuccessfulDebuff);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffDamage(
	FGameplayEffectContextHandle& EffectContextHandle, float InDebuffDamage)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDamage(InDebuffDamage);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffDuration(
	FGameplayEffectContextHandle& EffectContextHandle, float InDebuffDuration)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDuration(InDebuffDuration);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffFrequency(
	FGameplayEffectContextHandle& EffectContextHandle, float InDebuffFrequency)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffFrequency(InDebuffFrequency);
	}
}

void UAuraAbilitySystemLibrary::SetDamageType(
	FGameplayEffectContextHandle& EffectContextHandle, const FGameplayTag& InDamageType)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDamageType(InDamageType);
	}
}

void UAuraAbilitySystemLibrary::SetDeathImpulse(
	FGameplayEffectContextHandle& EffectContextHandle, const FVector& InDeathImpulse)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDeathImpulse(InDeathImpulse);
	}
}

void UAuraAbilitySystemLibrary::SetKnockbackForce(
	FGameplayEffectContextHandle& EffectContextHandle, const FVector& InKnockbackForce)
{
	if (FAuraGameplayEffectContext* AuraEffectContext =
		static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetKnockbackForce(InKnockbackForce);
	}
}

FGameplayEffectContextHandle UAuraAbilitySystemLibrary::ApplyDamageEffect(
	const FDamageEffectParams& DamageEffectParams)
{
	// 没有源/目标 ASC 就无法创建或应用伤害 GE；这属于调用方配置错误。
	check(DamageEffectParams.SourceAbilitySystemComponent);
	check(DamageEffectParams.TargetAbilitySystemComponent);
	check(DamageEffectParams.DamageGameplayEffectClass);

	FGameplayEffectContextHandle EffectContextHandle = DamageEffectParams.SourceAbilitySystemComponent->MakeEffectContext();
	AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();
	EffectContextHandle.AddSourceObject(SourceAvatarActor);
	SetDeathImpulse(EffectContextHandle, DamageEffectParams.DeathImpulse);

	// 只在服务端真正应用伤害时掷一次骰子，避免 Projectile、近战和客户端重复随机。
	// 成功结果写入 Context；零向量表示这一次命中没有触发击退。
	const bool bKnockback = DamageEffectParams.KnockbackChance > 0.f &&
		!DamageEffectParams.KnockbackForce.IsNearlyZero() &&
		FMath::FRandRange(0.f, 100.f) < DamageEffectParams.KnockbackChance;
	if (bKnockback)
	{
		SetKnockbackForce(EffectContextHandle, DamageEffectParams.KnockbackForce);
	}

	// Context 保存这次伤害的来源，后续格挡、暴击和浮动伤害文字都可以读取它。
	const FGameplayEffectSpecHandle SpecHandle =
		DamageEffectParams.SourceAbilitySystemComponent->MakeOutgoingSpec(
			DamageEffectParams.DamageGameplayEffectClass,
			DamageEffectParams.AbilityLevel,
			EffectContextHandle);
	check(SpecHandle.IsValid());

	// Tag 是参数名，Magnitude 是本次技能计算出来的值；ExecCalc_Damage 会用相同 Tag 读取。
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		DamageEffectParams.DamageType,
		DamageEffectParams.BaseDamage);

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	// Debuff 参数也随同一个 Spec 传递，后续 Debuff 逻辑可以按 Tag 读取它们。
	//  先存在 set by caller ，等后面 在 exec calc 确认要触发 debuff 时，再把这些值转存到 EffectContext 里供 AttributeSet 使用。
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, GameplayTags.Debuff_Chance, DamageEffectParams.DebuffChance);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, GameplayTags.Debuff_Damage, DamageEffectParams.DebuffDamage);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, GameplayTags.Debuff_Duration, DamageEffectParams.DebuffDuration);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, GameplayTags.Debuff_Frequency, DamageEffectParams.DebuffFrequency);

	DamageEffectParams.TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return EffectContextHandle;
}

void UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(const UObject* WorldContextObject,
	TArray<AActor*>& OutOverlappingActors,const TArray<AActor*>& ActorsToIgnore, float Radius, const FVector& SphereOrigin)
{
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(ActorsToIgnore);
	
	TArray<FOverlapResult> Overlaps;
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		//它在指定位置以给定半径做球形碰撞查询，只检索动态对象类型，并把忽略列表应用到查询参数里；结果写入 `Overlaps`。
		World->OverlapMultiByObjectType(Overlaps,SphereOrigin , FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(Radius), SphereParams);
		
		for (FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.GetActor()->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsDead(Overlap.GetActor()) )
			{
				OutOverlappingActors.AddUnique(Overlap.GetActor());
			}
		}
	}

}

void UAuraAbilitySystemLibrary::GetClosestTargets(const int32 MaxTargets, const TArray<AActor*>& Actors,
	TArray<AActor*>& OutClosestTargets, const FVector& Origin)
{
	OutClosestTargets.Reset();
	if (MaxTargets <= 0 || Actors.IsEmpty())
	{
		return;
	}

	// 复制后排序，不能改变调用方传入的候选数组；DistSquared 避免无意义的开方运算。
	TArray<AActor*> SortedActors = Actors;
	SortedActors.RemoveAll([](const AActor* Actor) { return !IsValid(Actor); });
	SortedActors.Sort([&Origin](const AActor& Left, const AActor& Right)
	{
		return FVector::DistSquared(Left.GetActorLocation(), Origin) <
			FVector::DistSquared(Right.GetActorLocation(), Origin);
	});

	const int32 TargetCount = FMath::Min(MaxTargets, SortedActors.Num());
	OutClosestTargets.Append(SortedActors.GetData(), TargetCount);
}

bool UAuraAbilitySystemLibrary::IsNotFriend(const AActor* FirstActor, const AActor* SecondActor)
{
	const bool bBothArePlayer = FirstActor->ActorHasTag("Player") && SecondActor->ActorHasTag("Player");
	const bool bBothAreEnemy = FirstActor->ActorHasTag("Enemy") && SecondActor->ActorHasTag("Enemy");
	
	const bool bIsNotFriend = !(bBothArePlayer || bBothAreEnemy);
	return bIsNotFriend;
}

TArray<FRotator> UAuraAbilitySystemLibrary::EvenlySpacedRotators(
	const FVector& Forward, const FVector& Axis, float Spread, int32 NumRotators)
{
	TArray<FRotator> Rotators;
	if (NumRotators <= 0)
	{
		return Rotators;
	}

	if (NumRotators == 1)
	{
		Rotators.Add(Forward.Rotation());
		return Rotators;
	}

	const FVector LeftOfSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const float DeltaSpread = Spread / (NumRotators - 1);
	Rotators.Reserve(NumRotators);

	for (int32 Index = 0; Index < NumRotators; ++Index)
	{
		const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * Index, Axis);
		Rotators.Add(Direction.Rotation());
	}

	return Rotators;
}

TArray<FVector> UAuraAbilitySystemLibrary::EvenlyRotatedVectors(
	const FVector& Forward, const FVector& Axis, float Spread, int32 NumVectors)
{
	TArray<FVector> Vectors;
	if (NumVectors <= 0)
	{
		return Vectors;
	}

	if (NumVectors == 1)
	{
		Vectors.Add(Forward);
		return Vectors;
	}

	const FVector LeftOfSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const float DeltaSpread = Spread / (NumVectors - 1);
	Vectors.Reserve(NumVectors);

	for (int32 Index = 0; Index < NumVectors; ++Index)
	{
		Vectors.Add(LeftOfSpread.RotateAngleAxis(DeltaSpread * Index, Axis));
	}

	return Vectors;
}
