// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Aura/Aura.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AAuraProjectile::AAuraProjectile()
{
	//SetReplicates(true);
	//上面更好,但是要 初始化完成后才用
 	bReplicates = true;
	//发射火球只在服务端执行，客户端通过火球开启复制来出现火球
	PrimaryActorTick.bCanEverTick = false;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);
	
	Sphere->SetCollisionObjectType(ECC_Projectile);
	
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	
}

void AAuraProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SetLifeSpan(LifeSpan);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	
	 LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent());
}

/**
 * 服务器销毁复制的投射物时，客户端不一定已经触发了本地的重叠回调。
 * 如果当前客户端还没播放过命中反馈，就在 Actor 销毁前补播一次。
 * bHit 是每个网络实例各自维护的本地防重标记，不是复制变量。
 */
void AAuraProjectile::Destroyed()
{
	// 仅为尚未处理命中反馈的客户端补播；服务器的命中逻辑已在 OnSphereOverlap 中执行。
	if (!bHit && !HasAuthority())
	{
		OnHit();
	}
	
	Super::Destroyed();
}

void AAuraProjectile::OnHit()
{
	// 只处理当前机器上的命中表现；伤害始终由服务器在 OnSphereOverlap 中结算。
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
	if (LoopingSoundComponent) LoopingSoundComponent->Stop();
	// 防止本地重叠回调与 Destroyed 兜底重复播放命中反馈。
	bHit = true;
}

void AAuraProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent ?
	DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	if (!SourceAvatarActor || SourceAvatarActor == OtherActor)
	{
		return; // 没有有效的伤害来源，或投射物碰到了施法者自己。
	}
	
	if (!UAuraAbilitySystemLibrary::IsNotFriend(SourceAvatarActor, OtherActor))
	{
		return; // 友方目标不触发命中。
	}
	
	// 每个网络实例只播放一次本地命中反馈。
	if (!bHit)
	{
		OnHit();
	}
	
	
	if (HasAuthority())
	{	
		// 目标只有在命中时才能确定；只有服务器可以设置目标 ASC 并结算伤害。
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			DamageEffectParams.TargetAbilitySystemComponent = TargetASC;

			// 不直接使用带俯仰角的火球前向量，否则向上/向下瞄准会把布娃娃打向天空或地面。
			// 保留水平命中方向，再叠加固定的小上抬量，使死亡表现可预测且便于调参。
			FVector DeathImpulseDirection = GetActorForwardVector().GetSafeNormal2D();
			if (DeathImpulseDirection.IsNearlyZero())
			{
				DeathImpulseDirection =
					(OtherActor->GetActorLocation() - SourceAvatarActor->GetActorLocation()).GetSafeNormal2D();
			}
			DeathImpulseDirection = (DeathImpulseDirection + FVector::UpVector * 0.25f).GetSafeNormal();
			DamageEffectParams.DeathImpulse = DeathImpulseDirection * DamageEffectParams.DeathImpulseMagnitude;
			FRotator KnockbackRotation = GetActorRotation();
			KnockbackRotation.Pitch = 45.f;
			DamageEffectParams.KnockbackForce =
				KnockbackRotation.Vector() * DamageEffectParams.KnockbackForceMagnitude;
			UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
		}
		
		// 服务器销毁复制 Actor，客户端随后会收到销毁通知。
		Destroy();
	}
	else // 客户端
	{
		// 客户端不结算伤害、不主动销毁 Actor，只记录本地已处理命中反馈。
		bHit = true;
	}
}



