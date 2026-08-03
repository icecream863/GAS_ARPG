// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraEnemy.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AI/AuraAIController.h"
#include "Aura/Aura.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/Widget/AuraUserWidget.h"

AAuraEnemy::AAuraEnemy()
{
	BaseWalkSpeed = 250.f;
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 高亮模板值在构造时设置一次即可，悬停时只切换 RenderCustomDepth。
	if (GetMesh())
	{
		GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
		GetMesh()->MarkRenderStateDirty();
	}
	if (Weapon)
	{
		Weapon->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
		Weapon->MarkRenderStateDirty();
	}
	
	AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>(FName("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	
	AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>("AttributeSet");
	
	HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
	HealthBar->SetupAttachment(GetRootComponent());
}

void AAuraEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	/**
	*你要的是**“是否允许执行/是否打断切换”** → 用 Decorator
	你要的是**“分支运行期间持续刷新信息/写黑板”** → 用 Service
	*/
	
	if (!HasAuthority()) return; // 只在服务端执行，服务端才有AI Controller
	AuraAIController = Cast<AAuraAIController>(NewController);
	
	// 【防崩溃保护】：如果蓝图里忘记配置BehaviorTree，或者Controller不是AAuraAIController，直接调用会引起崩溃。
	if (AuraAIController && AuraAIController->GetBlackboardComponent() && BehaviorTree)
	{
		AuraAIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
		//把黑板资源“装载”到 UBlackboardComponent 里
		AuraAIController->RunBehaviorTree(BehaviorTree);
		
		AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), false);
		AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("IsRangedAttacker"), CharacterClass != ECharacterClass::Warrior);
	}
}

AActor* AAuraEnemy::GetCombatTarget_Implementation()
{
	return CombatTarget;
}

void AAuraEnemy::SetCombatTarget_Implementation(AActor* InCombatTarget)
{
	CombatTarget = InCombatTarget;
}

void AAuraEnemy::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	InitAbilityActorInfo();
	
	if (HasAuthority())
	{
		UAuraAbilitySystemLibrary::GiveStartupAbilities(this, AbilitySystemComponent, CharacterClass);
		//客户端没有 GameMode,在客户端执行就会返回 空指针，所以要在服务端执行。
	}
	
	if (UAuraUserWidget* AuraUserWidget = Cast<UAuraUserWidget>(HealthBar->GetUserWidgetObject() ))
	{
		AuraUserWidget->SetWidgetController(this);
	}
	
	if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet) )
	{
		
		// 将事件绑定到属性变化委托上，当属性变化时广播事件，界面可以绑定这些事件来更新显示
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAS->GetHealthAttribute()).AddLambda(
			[this] (const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.NewValue);
			});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAS->GetMaxHealthAttribute()).AddLambda(
			[this] (const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			});
		
		// 玩家攻击 会 添加 Effect_HitReact 标签，移除时会移除标签，所以监听这个标签的添加和移除事件来切换受击状态
		FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
		AbilitySystemComponent->RegisterGameplayTagEvent(GameplayTags.Effect_HitReact, EGameplayTagEventType::NewOrRemoved).
		AddUObject(this, &AAuraEnemy::HitReactTagChanged);
		
		OnHealthChanged.Broadcast(AuraAS->GetHealth());
		OnMaxHealthChanged.Broadcast(AuraAS->GetMaxHealth());
	}
	
	
}

void AAuraEnemy::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bHitReact = NewCount > 0 ? true : false;
	
	GetCharacterMovement()->MaxWalkSpeed = (bHitReact || bIsStunned) ? 0.f : BaseWalkSpeed;

	// 仅把 MaxWalkSpeed 设为 0 有时不足以立刻停下（AI MoveTo 可能还在驱动 PathFollowing），
	// 所以这里显式停止当前移动。
	if (bHitReact)
	{
		GetCharacterMovement()->StopMovementImmediately();
		if (AuraAIController)
		{
			// 清除当前的移动请求，防止受击动画结束后继续跑向原目标或在受击期间滑动
			AuraAIController->StopMovement();
		}
	}

	if (AuraAIController && AuraAIController->GetBlackboardComponent())
	{
		AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), bHitReact);
	}
	
}

void AAuraEnemy::Die(const FVector& DeathImpulse)
{
	SetLifeSpan(LifeSpan);
	/**
	* 倒计时开始：从调用这一行开始，经历 LifeSpan 秒的时间。
	* 自动销毁：时间一到，引擎会自动安全地对这个 Actor（即这个敌人）调用 Destroy()，将其从游戏世界中移除并回收内存。
	 */
	if (AuraAIController && AuraAIController->GetBlackboardComponent())
	{
		AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
	}
	Super::Die(DeathImpulse);
}

void AAuraEnemy::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	Super::StunTagChanged(CallbackTag, NewCount);

	// HitReact 与 Stun 可以重叠；任一状态仍存在时都不能恢复移动速度。
	GetCharacterMovement()->MaxWalkSpeed = (bHitReact || bIsStunned) ? 0.f : BaseWalkSpeed;
	if (bIsStunned && AuraAIController)
	{
		AuraAIController->StopMovement();
	}

	if (AuraAIController && AuraAIController->GetBlackboardComponent())
	{
		AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Stunned"), bIsStunned);
	}
}

void AAuraEnemy::HighLightActor_Implementation()
{
	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(true);
	}

	if (Weapon)
	{
		Weapon->SetRenderCustomDepth(true);
	}
}

void AAuraEnemy::UnHighLightActor_Implementation()
{
	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(false);
	}
	
	if (Weapon)
	{
		Weapon->SetRenderCustomDepth(false);
	}
}

void AAuraEnemy::SetMoveToLocation_Implementation(FVector& OutDestination)
{
	// 敌人不希望覆盖点击移动目的地：明确留空。
}

int32 AAuraEnemy::GetPlayerLevel_Implementation()
{
	return Level;
}

void AAuraEnemy::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
	{
		AuraASC->AbilityActorInfoSet();
	}
	OnASCRegistered.Broadcast(AbilitySystemComponent);
	RegisterDebuffTagEvents();
	
	if (HasAuthority()) //GameMode 只存在于服务器，所以这个函数在客户端执行时会返回 空指针，因此要在服务端执行。
	{
		InitialDefaultAttributes();//初始化默认属性
	}


}

void AAuraEnemy::InitialDefaultAttributes() const
{
	UAuraAbilitySystemLibrary::InitializeDefaultAttributes(this, CharacterClass, Level, AbilitySystemComponent);
}
