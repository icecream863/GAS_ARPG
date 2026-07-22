// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraProjectileSpell.h"
#include "Actor/AuraProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/KismetMathLibrary.h"

void UAuraProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		
}

void UAuraProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;
	
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
	{
		// BlueprintNativeEvent 必须通过 Execute_XXX 调用，不能直接调用接口事件函数。
		const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(AvatarActor, SocketTag);
		
		FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SocketLocation, ProjectileTargetLocation);
		
		
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SocketLocation);
		SpawnTransform.SetRotation(SpawnRotation.Quaternion());
		
		AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(ProjectileClass,
			SpawnTransform, 
			GetOwningActorFromActorInfo(), 
			Cast<APawn>(GetAvatarActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		
		// 生成时还不知道目标是谁，目标 ASC 会在 Projectile 命中时补上。
		Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		
		Projectile->FinishSpawning(SpawnTransform);// 完成生成过程，激活Actor
	
	}
}
