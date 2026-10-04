# UE5 GAS ARPG Demo

> Unreal Engine 5.8 · C++ · Gameplay Ability System · UMG · MVVM

俯视角动作 RPG 实践项目，围绕 GAS 搭建技能、属性、伤害、成长、UI 与存档流程，并补充客户端目标数据预测和世界状态恢复。

**Demo：待补充**

## 核心内容

### GAS 技能与成长
- 使用 Gameplay Ability、Gameplay Effect 与 Gameplay Tag 管理技能和状态
- 支持主动 / 被动技能、技能升级、槽位装备与换槽
- 使用 Gameplay Tag 关联输入、技能状态和技能槽位

### 网络与伤害
- 鼠标指向技能采用客户端先执行、服务器后权威处理的 TargetData 流程
- 使用 PredictionKey 对齐技能激活与目标数据
- 通过 SetByCaller 与 ExecCalc 统一处理四类伤害、抗性、护甲穿透、格挡、暴击与 Debuff
- 扩展 EffectContext 传递额外战斗结果，并支持网络序列化

### UI
- 使用 WidgetController 解耦 Gameplay 数据与 UMG
- 技能、属性、经验等界面通过事件更新
- 存档界面使用 MVVM / FieldNotify 管理槽位数据

### 存档
- 保存等级、属性、技能等级与技能槽位
- 保存实现 SaveInterface 的场景 Actor 状态
- 支持检查点、出生点、跨地图传送与死亡后恢复

## 技术栈

- Unreal Engine 5.8
- C++
- Gameplay Ability System
- Gameplay Tags
- Enhanced Input
- UMG / MVVM
- SaveGame
- UE Networking

## 代码入口

- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp`
- `Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp`
- `Source/Aura/Private/UI/WidgetController/`
- `Source/Aura/Private/Game/AuraGameModeBase.cpp`

## 环境

- Unreal Engine 5.8
- Windows
- Visual Studio 2022 / Rider

项目入口：`Aura.uproject`
