// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraFireBolt.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraProjectile.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interaction/CombatInterface.h"
#include "Engine/World.h"

void UAuraFireBolt::SpawnProjectiles(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag,
	const bool bOverridePitch, const float PitchOverride, AActor* HomingTarget)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() ||
		!AvatarActor->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
	{
		return;
	}

	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(AvatarActor, SocketTag);
	FRotator TargetRotation = (ProjectileTargetLocation - SocketLocation).Rotation();
	if (bOverridePitch)
	{
		TargetRotation.Pitch = PitchOverride;
	}

	const FVector Forward = TargetRotation.Vector();
	const int32 NumProjectilesToSpawn = FMath::Min(NumProjectiles, GetAbilityLevel());
	const TArray<FRotator> Rotations = UAuraAbilitySystemLibrary::EvenlySpacedRotators(
		Forward, FVector::UpVector, ProjectileSpread, NumProjectilesToSpawn);

	for (const FRotator& Rotation : Rotations)
	{
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SocketLocation);
		SpawnTransform.SetRotation(Rotation.Quaternion());

		AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
			ProjectileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			Cast<APawn>(AvatarActor),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Projectile)
		{
			continue;
		}

		Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();

		const bool bHasActorHomingTarget =
			IsValid(HomingTarget) && HomingTarget->Implements<UCombatInterface>();
		const bool bCanHomeToActor = bHasActorHomingTarget &&
			HomingTarget != AvatarActor;

		if (bCanHomeToActor)
		{
			Projectile->ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
			Projectile->SetHomingTarget(HomingTarget);
		}
		else if (!bHasActorHomingTarget)
		{
			// 世界几何的根组件位置通常不是鼠标命中点，因此创建一个位于点击位置的虚拟追踪目标。
			Projectile->HomingTargetSceneComponent = NewObject<USceneComponent>(Projectile);
			Projectile->HomingTargetSceneComponent->SetWorldLocation(ProjectileTargetLocation);
			Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTargetSceneComponent;
		}
		// 【优化】只禁止追踪施法者自己；其他战斗 Actor（包括友军）都允许成为目标。
		// 自身目标保留初始发射方向但不设置 HomingTarget，避免火球持续追进施法者体内。

		Projectile->ProjectileMovement->HomingAccelerationMagnitude =
			FMath::FRandRange(HomingAccelerationMin, HomingAccelerationMax);
		Projectile->ProjectileMovement->bIsHomingProjectile =
			bLaunchHomingProjectiles && (!bHasActorHomingTarget || bCanHomeToActor);
		Projectile->FinishSpawning(SpawnTransform);
	}
}



FString UAuraFireBolt::GetDescription(int32 Level)
{
	const int32 ScaledDamage = FMath::RoundToInt(Damage.GetValueAtLevel(Level));
	const float ManaCost = GetManaCost(Level);
	const float Cooldown = GetCooldown(Level);
	const int32 Projectiles = FMath::Min(Level, NumProjectiles);

	if (Level == 1)
	{
		return FString::Printf(
			TEXT("<Title>FIREBOLT</>\n\n")
			TEXT("<Small>Level: </><Level>%d</>\n")
			TEXT("<Small>ManaCost: </><ManaCost>%.1f</>\n")
			TEXT("<Small>Cooldown: </><Cooldown>%.1f</>\n\n")
			TEXT("<Default>Launches a bolt of fire, exploding on impact and dealing </>")
			TEXT("<Damage>%d</>")
			TEXT("<Default> fire damage with a chance to burn.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage);
	}

	return FString::Printf(
		TEXT("<Title>FIREBOLT</>\n\n")
		TEXT("<Small>Level: </><Level>%d</>\n")
		TEXT("<Small>ManaCost: </><ManaCost>%.1f</>\n")
		TEXT("<Small>Cooldown: </><Cooldown>%.1f</>\n\n")
		TEXT("<Default>Launches %d bolts of fire, exploding on impact and dealing </>")
		TEXT("<Damage>%d</>")
		TEXT("<Default> fire damage with a chance to burn.</>"),
		Level,
		ManaCost,
		Cooldown,
		Projectiles,
		ScaledDamage);
}

FString UAuraFireBolt::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = FMath::RoundToInt(Damage.GetValueAtLevel(Level));
	const float ManaCost = GetManaCost(Level);
	const float Cooldown = GetCooldown(Level);
	const int32 Projectiles = FMath::Min(Level, NumProjectiles);

	return FString::Printf(
		TEXT("<Title>NEXT LEVEL:</>\n\n")
		TEXT("<Small>Level: </><Level>%d</>\n")
		TEXT("<Small>ManaCost: </><ManaCost>%.1f</>\n")
		TEXT("<Small>Cooldown: </><Cooldown>%.1f</>\n\n")
		TEXT("<Default>Launches %d bolts of fire, exploding on impact and dealing </>")
		TEXT("<Damage>%d</>")
		TEXT("<Default> fire damage with a chance to burn.</>"),
		Level,
		ManaCost,
		Cooldown,
		Projectiles,
		ScaledDamage);
}
