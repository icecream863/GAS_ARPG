// Fill out your copyright notice in the Description page of Project Settings.
#include "Actor/AuraEffectActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ActiveGameplayEffectHandle.h"


// Sets default values
AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SetRootComponent( CreateDefaultSubobject<USceneComponent>("SceneRoot") );
}


void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();

}

void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	/** 1. 用IAbilitySystemInterface接口
	
	// IAbilitySystemInterface* AbilitySystemInterface =  Cast<IAbilitySystemInterface>(Target);
	// if (AbilitySystemInterface)
	// {
	// 	UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
	// }
	
	// 2. 用gas的函数库，即便没有继承 IAbilitySystemInterface接口，蓝图中也可以使用
	*/
	
	if (!bApplyEffectToEnemy && TargetActor->ActorHasTag(FName("Enemy"))) return;// 如果这个效果不作用于敌人，并且目标是敌人，就直接返回，不施加效果
	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC == nullptr) return;
	check(GameplayEffectClass);
	
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);
	const FActiveGameplayEffectHandle ActiveGameplayEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	
	const bool bIsInfinite =  EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	
	if (bIsInfinite && InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		ActiveEffectHandles.Add(ActiveGameplayEffectHandle, TargetASC); // 存了 这个GE,施加给了谁 ,谁施加，谁移除
		//~ 同一 GE, 可能有不同ActiveGameplayEffectHandle， 对应 一个TargetASC，
		//~ TargetASC可能对应不同ActiveGameplayEffectHandle
	}
	
	if (!bIsInfinite)
	{
		Destroy();
	}
	
}

void AAuraEffectActor::OnOverlap(AActor* TargetActor)
{
	if (!bApplyEffectToEnemy && TargetActor->ActorHasTag(FName("Enemy"))) return;// 如果这个效果不作用于敌人，并且目标是敌人，就直接返回，不施加效果

	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AAuraEffectActor::OnEndOverlap(AActor* TargetActor)
{
	if (!bApplyEffectToEnemy && TargetActor->ActorHasTag(FName("Enemy"))) return;// 如果这个效果不作用于敌人，并且目标是敌人，就直接返回，不施加效果

	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
	
	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC) return;
		
		for (auto it = ActiveEffectHandles.CreateIterator(); it; ++it)
		{
			if (it.Value() == TargetASC)
			{
				TargetASC->RemoveActiveGameplayEffect(it.Key(), 1);
				it.RemoveCurrent();//也可以 先遍历存起来，再删，
				//遍历是因为 1.多个targetActor， 2.一个targetActor可能对应多个key
			}
		}
		
		
	}
	
	
}
