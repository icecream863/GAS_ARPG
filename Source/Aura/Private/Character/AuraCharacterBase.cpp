// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "Aura/Aura.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AAuraCharacterBase::AAuraCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BurnDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("BurnDebuffComponent"));
	BurnDebuffComponent->SetupAttachment(GetRootComponent());
	BurnDebuffComponent->DebuffTag = FAuraGameplayTags::Get().Debuff_Burn;

	StunDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("StunDebuffComponent"));
	StunDebuffComponent->SetupAttachment(GetRootComponent());
	StunDebuffComponent->DebuffTag = FAuraGameplayTags::Get().Debuff_Stun;
	StunDebuffComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> StunSystem(
		TEXT("/Game/Assets/Effects/Stun/NS_Stars.NS_Stars"));
	if (StunSystem.Succeeded())
	{
		// 提供稳定的通用默认值；体型特殊的敌人仍可在角色蓝图中覆写位置或系统。
		StunDebuffComponent->SetAsset(StunSystem.Object);
	}

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(FName("Weapon") );
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// Projectile 自定义通道的项目默认响应是 Ignore；必须让角色 Capsule 明确接收重叠，
	// 否则追踪火球可能穿过 Capsule，并停留在目标根组件附近而无法触发伤害和销毁。
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	/** 
	* ECC = `ECollisionChannel`（碰撞通道，*Collision Channel*）
	用来表示“你在和哪一类对象/用途进行碰撞查询或响应”。
	例如：ECC_Camera 表示相机通道（相机的碰撞/遮挡检测常用这个通道）。
	ECR = `ECollisionResponse`（碰撞响应，*Collision Response*）
	用来表示“对这个通道要怎么响应”。常见有：
	*/
}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void AAuraCharacterBase::BeginPlay()
{ 
	Super::BeginPlay();
	
}

void AAuraCharacterBase::Die(const FVector& DeathImpulse)
{	
	//将武器从角色身上分离，并保持其在世界空间中的位置、旋转和缩放不变
	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	MulticastHandleDeath(DeathImpulse);
}

void AAuraCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAuraCharacterBase, bIsStunned);
	DOREPLIFETIME(AAuraCharacterBase, bIsBurned);
	DOREPLIFETIME(AAuraCharacterBase, bIsBeingShocked);
}

void AAuraCharacterBase::MulticastHandleDeath_Implementation(const FVector& DeathImpulse)
{
	
	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	//变成布娃娃形态
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	// Velocity Change ignores mass, keeping death knockback consistent across character meshes.
	Weapon->AddImpulse(DeathImpulse * 0.1f, NAME_None, true);
	GetMesh()->AddImpulse(DeathImpulse, NAME_None, true);
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bDead = true;
	BurnDebuffComponent->Deactivate();
	StunDebuffComponent->Deactivate();
	OnDeath.Broadcast(this);

}


FVector AAuraCharacterBase::GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag)
{
	
	if (MontageTag == FAuraGameplayTags::Get().CombatSocket_Weapon && Weapon)
	{
		return Weapon->GetSocketLocation(WeaponTipSocketName);
	}
	
	if (MontageTag == FAuraGameplayTags::Get().CombatSocket_LeftHand)
	{
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}

	if (MontageTag == FAuraGameplayTags::Get().CombatSocket_RightHand)
	{
		return GetMesh()->GetSocketLocation(RightHandSocketName);
	}
	
	if (MontageTag == FAuraGameplayTags::Get().CombatSocket_Trail)
	{
		return GetMesh()->GetSocketLocation(TailSocketName);
	}
	
	return FVector::ZeroVector;
	
}

USkeletalMeshComponent* AAuraCharacterBase::GetWeapon_Implementation()
{
	// Weapon 是角色拥有的骨骼网格组件；Cue 可将 Niagara 系统附着到它的 TipSocket。
	return Weapon;
}

bool AAuraCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

AActor* AAuraCharacterBase::GetAvatar_Implementation() 
{
	return this;
}

TArray<FTaggedMontage> AAuraCharacterBase::GetAttackMontages_Implementation()
{
	return AttackMontages;
}

UNiagaraSystem* AAuraCharacterBase::GetBloodEffect_Implementation()
{
	return BloodEffect;
}

FTaggedMontage AAuraCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag)
{
	for (FTaggedMontage& TaggedMontage : AttackMontages)
	{
		if (TaggedMontage.MontageTag == MontageTag)
		{
			return TaggedMontage;
		}
	}
	return FTaggedMontage();
}

int32 AAuraCharacterBase::GetMinionCount_Implementation()
{
	return MinionCount;
}

void AAuraCharacterBase::IncrementMinionCount_Implementation(int32 Amount)
{
	MinionCount += Amount;
}

ECharacterClass AAuraCharacterBase::GetCharacterClass_Implementation()
{
	return CharacterClass;
}

FOnExternalGameplayModifierDependencyChange* AAuraCharacterBase::GetExternalGameplayModifierDependencyMulticast()
{
	// 返回角色级外部依赖委托，供 MaxHealth/MaxMana 这类 MMC 注册刷新回调。
	return &ExternalGameplayModifierDependencyMulticast;
}

FOnASCRegistered& AAuraCharacterBase::GetOnASCRegisteredDelegate()
{
	return OnASCRegistered;
}

FOnDeath& AAuraCharacterBase::GetOnDeathDelegate()
{
	return OnDeath;
}

UAnimMontage* AAuraCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

void AAuraCharacterBase::InitAbilityActorInfo()
{
}

bool AAuraCharacterBase::IsBeingShocked_Implementation() const
{
	return bIsBeingShocked;
}

void AAuraCharacterBase::SetIsBeingShocked_Implementation(bool bInShock)
{
	bIsBeingShocked = bInShock;
}

void AAuraCharacterBase::RegisterDebuffTagEvents()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	const FGameplayTag StunTag = FAuraGameplayTags::Get().Debuff_Stun;
	FOnGameplayEffectTagCountChanged& StunTagDelegate =
		AbilitySystemComponent->RegisterGameplayTagEvent(StunTag, EGameplayTagEventType::NewOrRemoved);
	StunTagDelegate.RemoveAll(this);
	StunTagDelegate.AddUObject(this, &AAuraCharacterBase::StunTagChanged);

	// ASC 初始化时可能已经存在 Stun 标签，注册后立即同步一次当前状态。
	StunTagChanged(StunTag, AbilitySystemComponent->GetTagCount(StunTag));

	const FGameplayTag BurnTag = FAuraGameplayTags::Get().Debuff_Burn;
	FOnGameplayEffectTagCountChanged& BurnTagDelegate =
		AbilitySystemComponent->RegisterGameplayTagEvent(BurnTag, EGameplayTagEventType::NewOrRemoved);
	BurnTagDelegate.RemoveAll(this);
	BurnTagDelegate.AddUObject(this, &AAuraCharacterBase::BurnTagChanged);
	BurnTagChanged(BurnTag, AbilitySystemComponent->GetTagCount(BurnTag));
}

void AAuraCharacterBase::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bIsStunned = NewCount > 0;
	ApplyStunMovementState();
}

void AAuraCharacterBase::OnRep_Stunned()
{
	ApplyStunMovementState();
	if (bIsStunned)
	{
		StunDebuffComponent->Activate();
	}
	else
	{
		StunDebuffComponent->Deactivate();
	}
}

void AAuraCharacterBase::BurnTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bIsBurned = NewCount > 0;
}

void AAuraCharacterBase::OnRep_Burned()
{
	if (bIsBurned)
	{
		BurnDebuffComponent->Activate();
	}
	else
	{
		BurnDebuffComponent->Deactivate();
	}
}

void AAuraCharacterBase::ApplyStunMovementState()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = bIsStunned ? 0.f : BaseWalkSpeed;
		if (bIsStunned)
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

void AAuraCharacterBase::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(GetAbilitySystemComponent());
	check(GameplayEffectClass);
	
	FGameplayEffectContextHandle EffectContextHandle =  GetAbilitySystemComponent()->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, 1, EffectContextHandle);
	
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
}

void AAuraCharacterBase::InitialDefaultAttributes() const
{
	// 先初始化主属性，后续次属性可能会依赖主属性的数值（例如 MaxHealth 可能依赖 Vigor），所以先初始化主属性比较合理。当然具体顺序也要看你的设计需求。
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);//instant
	ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);//infinite
	ApplyEffectToSelf(DefaultVitalAttributes, 1.f);//初始化为最大值，instant就行
	//顺序很重要
}

void AAuraCharacterBase::AddCharacterAbilities() const
{
	if (!HasAuthority()) return;
	// 只有服务器才会添加能力，客户端不需要添加能力，
	// 因为能力的执行和效果的应用都是由服务器控制的，客户端只需要接收服务器的状态更新即可。
	
	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (AuraASC)
	{
		AuraASC->AddCharacterAbility(StartupAbilities);
		AuraASC->AddPassiveCharacterAbility(StartupPassiveAbilities);
	}
	
}


