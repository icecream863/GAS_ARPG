// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"

#include "Aura/Aura.h"
#include "AbilitySystemComponent.h"

UTargetDataUnderMouse* UTargetDataUnderMouse::CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility)
{
	UTargetDataUnderMouse* MyObj = NewAbilityTask<UTargetDataUnderMouse>(OwningAbility);
	// 创建一个新的能力任务对象，并将 OwningAbility 作为参数传递给它。这个函数是一个静态函数，允许你在蓝图中直接调用它来创建这个任务。
	return MyObj;//
}

void UTargetDataUnderMouse::Activate()
{
	Super::Activate();
	
	const bool bIsLocalControlled = Ability-> GetCurrentActorInfo()->IsLocallyControlled();
	if (bIsLocalControlled)//客户端就发送 数据
	{
		SendMouseCursorData();
	}
	else//服务端接收
	{
		//AbilityTargetDataSetDelegate 返回一个委托
		AbilitySystemComponent->AbilityTargetDataSetDelegate(GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &UTargetDataUnderMouse::OnTargetDataReplicatedCallBack);
		const bool bCalledDelegate = AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(GetAbilitySpecHandle(), GetActivationPredictionKey());//尝试触发上面的委托
		
		if (!bCalledDelegate)
		{
			SetWaitingOnRemotePlayerData();//如果没有触发到委托，说明数据还没有到达，任务进入等待状态，直到后续网络事件触发委托。
		}
		
		/** 尝试立即触发已到达的复制事件委托（如 TargetData 已经从客户端复制到服务器）。
		* 如果对应的复制事件尚未到达，这里不会触发回调，任务会继续等待后续网络复制事件来触发。
		* 注意：该函数通常需要传入 \`SpecHandle\` 和 \`PredictionKey\` 等参数，并且语句末尾需要分号。
		*/
	}
	
	
	
}

void UTargetDataUnderMouse::SendMouseCursorData()
{
	/**
	 * 打开一个临时预测窗口，让接下来这段代码处于 GAS 的“客户端预测”上下文中。
	 * 这样 ASC 会为这段操作准备/设置当前的 ScopedPredictionKey，后续发出的 RPC
	 * 就会被服务器识别为一次可预测操作，而不是普通的无预测网络调用。
	 * 需要手动开窗（因为 Task 回调通常脱离了原始的激活作用域）。
	 */
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get());
	
	
	FGameplayAbilityTargetData_SingleTargetHit* CachedData = new FGameplayAbilityTargetData_SingleTargetHit();
	FHitResult CursorResult;
	APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	// 技能取点与鼠标悬停使用不同通道，避免 UI/可见性碰撞规则意外截断技能目标射线。
	PC->GetHitResultUnderCursor(ECC_Target, false, CursorResult);
	
	CachedData->HitResult = CursorResult;
	FGameplayAbilityTargetDataHandle DataHandle;
	DataHandle.Add(CachedData);
	
	/**
	 * ServerSetReplicatedTargetData 这里会同时带两个预测键，它们职责不同：
	 * 1. GetActivationPredictionKey()
	 *    表示“这份 TargetData 属于哪一次 Ability 激活”。
	 *    它是这次技能释放的根预测键，用来把 TargetData 归到正确的激活链路上。
	 * 2. AbilitySystemComponent->ScopedPredictionKey
	 *    表示“这份 TargetData 是在当前这个预测窗口里发出去的哪一次具体预测操作”。
	 *    它由上面的 FScopedPredictionWindow 提供，用于服务器确认、去重以及预测回滚对齐。
	 *
	 * 可以把两者理解成：
	 * - ActivationPredictionKey：这次技能释放的总单号
	 * - ScopedPredictionKey：这次技能释放里某一步预测操作的小单号
	 *
	 * 只传 ActivationPredictionKey 还不够，因为服务器还需要知道这次 RPC 自身
	 * 是不是处在一个合法的预测窗口里，以及它对应的是当前哪一次具体预测提交。
	 */
	AbilitySystemComponent->ServerSetReplicatedTargetData(GetAbilitySpecHandle(), 
		GetActivationPredictionKey(), 
		DataHandle, 
		FGameplayTag(), 
		AbilitySystemComponent->ScopedPredictionKey);
	//RPC 客户端调用 ServerSetReplicatedTargetData，把目标数据和预测键发送给服务器。服务器会根据这些信息来处理技能效果的应用和预测结果的确认。
	
	// 仅当任务仍处于可广播状态时才触发委托，避免在任务结束/Ability 取消后仍广播回调
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(DataHandle);
	}
}

void UTargetDataUnderMouse::OnTargetDataReplicatedCallBack(const FGameplayAbilityTargetDataHandle& DataHandle,
	FGameplayTag ActivatedTag)
{
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());
	//清除 AbilityTargetDataMap中的数据，避免重复处理同一份 TargetData。这个函数通常在服务器接收到客户端发送的 TargetData 后调用，以确保数据只被处理一次。
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(DataHandle);
	}
}
/**
*为什么这里客户端和服务端都要广播一次


• 因为这个 AbilityTask 是“同一个任务在两端各自完成各自的工作”。

这里其实有两条独立执行路径：

客户端这条：

- Activate() 里 IsLocallyControlled() 为真
- 调 SendMouseCursorData()
- 采鼠标命中
- 发 TargetData 给服务器
- 本地立刻 ValidData.Broadcast(DataHandle)

服务端这条：

- 同一个 ability 在服务器也会有一份执行
- Activate() 里走 else
- 绑定 AbilityTargetDataSetDelegate(...)
- 等客户端传来的 TargetData
- 收到后进 OnTargetDataReplicatedCallBack(...)
- 再 ValidData.Broadcast(DataHandle)

所以“都广播一次”不是重复发网络消息，而是：

- 客户端广播：让本地预测逻辑先继续跑
- 服务端广播：让权威逻辑在真正收到数据后继续跑

如果只在服务器广播：

- 客户端就得傻等服务器返回
- 本地不会立即有预测表现

如果只在客户端广播：

- 服务器拿到数据后没有后续逻辑
- 权威执行就断了

你可以把它理解成：

- 客户端广播是“我先本地用这份数据做预测”
- 服务端广播是“我确认收到后，再用同一份数据做正式处理”

所以广播两次是正常的，前提是它们发生在不同机器、不同执行实例里，不是同一个实例重复跑两遍。

你这段代码里就是这个设计：
Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp:20
客户端分支广播

Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp:98
服务端收到复制数据后再广播
*/
