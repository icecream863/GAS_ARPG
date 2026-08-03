// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerController.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NavigationPath.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "EnhancedInputSubsystems.h" 
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"
#include "Input/AuraEnhancedInputComponent.h"

#include "Interaction/EnemyInterface.h"
#include "Interaction/HighlightInterface.h"
#include "UI/Widget/DamageTextComponent.h"


AAuraPlayerController::AAuraPlayerController()
{
	bReplicates = true;
	
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	check(AuraContext);
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	if (Subsystem)
	{
		Subsystem->AddMappingContext(AuraContext, 0);
	}
	
	
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);
	
	
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UAuraEnhancedInputComponent* AuraEnhancedInputComponent = CastChecked<UAuraEnhancedInputComponent>(InputComponent);
	
	AuraEnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
	AuraEnhancedInputComponent->BindAction(ShiftAction, ETriggerEvent::Started, this, &AAuraPlayerController::ShiftPressed);
	AuraEnhancedInputComponent->BindAction(ShiftAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShiftReleased);
	
	AuraEnhancedInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
	//加&是因为函数指针需要传递函数地址，而不是调用函数。
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
	
	AutoRun();
}

void AAuraPlayerController::ShowDamageNumber_Implementation(const float DamageAmount, ACharacter* TargetCharacter, bool bIsBlockedHit, bool bIsCriticalHit)
{
	if (DamageTextComponentClass && TargetCharacter && IsLocalController())
	{
		if (UDamageTextComponent* DamageTextComponent = NewObject<UDamageTextComponent>(TargetCharacter, DamageTextComponentClass))
		{
			DamageTextComponent->RegisterComponent();//CreateDefalutSubObject会自动注册
			DamageTextComponent->SetDamageText(DamageAmount, bIsBlockedHit, bIsCriticalHit);
			DamageTextComponent->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			DamageTextComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			
		}
	}
}

void AAuraPlayerController::AutoRun()
{
	
	if (APawn* ControlledPawn = GetPawn())
	{
		if (!bAutoRunning) return;
		
		FVector SplineLocation = Spline->FindLocationClosestToWorldLocation(ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		FVector WorldDirection = Spline->FindDirectionClosestToWorldLocation(ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		ControlledPawn->AddMovementInput(WorldDirection);
		
		const float DistanceToDestination = FVector::Dist(SplineLocation, CachedDestination);
		if (DistanceToDestination <= AutoRunAcceptanceRadius)
		{
			bAutoRunning = false;
		}
	}
}

void AAuraPlayerController::HighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_HighLightActor(InActor);
	}
}

void AAuraPlayerController::UnHighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_UnHighLightActor(InActor);
	}
}

void AAuraPlayerController::CursorTrace()
{
	if (const UAuraAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_CursorTrace))
	{
		// 技能激活前可能已有敌人处于高亮状态；这里先清除旧表现，
		// 再跳过后续射线检测，避免施法期间保留过时的鼠标指向反馈。
		UnHighlightActor(LastActor);
		UnHighlightActor(ThisActor);
		LastActor = nullptr;
		ThisActor = nullptr;
		return;
	}

	GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	if (!CursorHit.bBlockingHit) return ;
	
	LastActor = ThisActor;
	// 只有实现了 IHighlightInterface 的 Actor 才记录为可高亮对象；否则显式置空。
	if (AActor* HitActor = CursorHit.GetActor();
		IsValid(HitActor) && HitActor->Implements<UHighlightInterface>())
	{
		ThisActor = HitActor;
	}
	else
	{
		ThisActor = nullptr;
	}
	
	// 每帧执行，实时更新：目标变了才切换高亮状态。
	if (LastActor != ThisActor)
	{
		UnHighlightActor(LastActor);
		HighlightActor(ThisActor);
	}
}

//Pressed更多是\“开始记录状态\”，Held/Released才是\“根据最终意图执行\”。这样才能同时兼容点击移动、按住跟随、以及对目标释放技能这几种行为而不冲突。
void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const UAuraAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed)) return;

	if (GetASC()) GetASC()->AbilityInputTagPressed(InputTag);

	if (InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (IsValid(ThisActor) && ThisActor->Implements<UEnemyInterface>())
		{
			TargetingStatus = ETargetingStatus::TargetingEnemy;
		}
		else if (IsValid(ThisActor))
		{
			// 高亮但不是敌人（如以后的地图出入口）：不当作施法目标。
			TargetingStatus = ETargetingStatus::TargetingNonEnemy;
		}
		else
		{
			TargetingStatus = ETargetingStatus::NotTargeting;
		}
		bAutoRunning = false;
	}
}

void AAuraPlayerController::AbilityInputTagReleased( FGameplayTag InputTag)
	{
	if (const UAuraAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputReleased)) return;

	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagReleased(InputTag);
		}
		return;
	}
	
	//只要不是点击移动，其他技能的释放都直接通知 GAS 就好，只有点击移动才区分短按长按。
	if (GetASC())	GetASC()->AbilityInputTagReleased(InputTag);
	
	if (TargetingStatus != ETargetingStatus::TargetingEnemy && !bShiftKeyDown)
	{
		if (FollowTime < ShortPressThreshold)
		{
			// 高亮对象（检查点/地图出入口）可先把移动目的地覆盖为指定到达点；
			// 导航路径必须按覆盖后的目的地计算，否则角色会沿着旧路径一直走。
			const bool bOverwriteDestination = IsValid(ThisActor) && ThisActor->Implements<UHighlightInterface>();
			if (bOverwriteDestination)
			{
				IHighlightInterface::Execute_SetMoveToLocation(ThisActor, CachedDestination);
			}

			APawn* ControlledPawn = GetPawn();
			UNavigationPath* NavPath = ControlledPawn
				? UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), ControlledPawn->GetActorLocation(), CachedDestination)
				: nullptr;
			if (NavPath)
			{
				Spline->ClearSplinePoints();

				for (const FVector& PointLoc : NavPath->PathPoints)
				{
					Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
				}
			}

			if (NavPath && NavPath->PathPoints.Num() > 0)
			{
				CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];

				bAutoRunning = true;

				// 只有普通地面点击（目的地未被高亮对象覆盖）才播点击粒子。
				if (!bOverwriteDestination)
				{
					// 短按点击特效也必须遵守 Pressed 屏蔽；否则引导技能期间
					// 玩家仍会看到与实际操作不一致的地面点击反馈。
					if (const UAuraAbilitySystemComponent* ASC = GetASC(); !ASC || !ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ClickNiagaraSystem, CachedDestination);
					}
				}
			}

		}
		FollowTime = 0.f;
		TargetingStatus = ETargetingStatus::NotTargeting;
	}
}

	

// 真正释放技能的地方，
void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	if (const UAuraAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputHeld)) return;

	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagHeld(InputTag);
		}
		return;
	}
	
	if (TargetingStatus == ETargetingStatus::TargetingEnemy || bShiftKeyDown)
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagHeld(InputTag);
		}
	}
	else //导航
	{
		FollowTime += GetWorld()->GetDeltaSeconds();
		
		
		if (CursorHit.bBlockingHit)
		{
			CachedDestination = CursorHit.Location;
		}
		APawn* ControlledPawn = GetPawn();
		FVector WorldDirection =(CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection);
	}
}

UAuraAbilitySystemComponent* AAuraPlayerController::GetASC()
{
	if (AuraAbilitySystemComponent == nullptr)
	{
		AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return AuraAbilitySystemComponent;
}



void AAuraPlayerController::Move(const struct FInputActionValue& InputActinValue)
	{
	// WASD 同样由 PlayerController 回调处理，因此也要使用 Pressed 屏蔽标签，
	// 防止引导施法期间仍能通过键盘移动角色。
	if (const UAuraAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed)) return;

	const FVector2D InputAxiVector2D = InputActinValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRatation = FRotator(0.f, Rotation.Yaw, 0.f);
	
	
	const FVector ForwardDirection = FRotationMatrix(YawRatation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRatation).GetUnitAxis(EAxis::Y);
	
	if (APawn* ControlPawn = GetPawn())
	{
		ControlPawn->AddMovementInput(ForwardDirection, InputAxiVector2D.Y);
		ControlPawn->AddMovementInput(RightDirection, InputAxiVector2D.X);
	}
}
