> Unreal Engine 5.8 · C++ · Gameplay Ability System · UMG · MVVM

基于 Unreal Engine 5.8 开发的俯视角 Action RPG 实践项目。  
项目围绕 Gameplay Ability System 搭建技能、属性、伤害与角色成长体系，并进一步实现客户端目标数据预测、UI 数据解耦以及角色 / 世界状态存档。

> 🎬 Demo：待补充

---

## Features

### Gameplay Ability System

使用 GAS 统一管理主动技能、被动技能、属性、状态和技能装备流程。

- 使用 Gameplay Tag 关联输入、技能状态与技能槽位
- 支持技能解锁、升级、装备、换槽和被动技能激活
- 通过 AbilitySpec 保存技能等级、状态与输入槽
- Enhanced Input 与 GAS 输入逻辑解耦

主要代码：

- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Private/AbilitySystem/Abilities/`

---

### Client Prediction & Target Data

针对鼠标指向类技能实现自定义 AbilityTask。

本地玩家采集鼠标命中结果后，将 TargetData 提交至服务器，同时客户端继续本地技能逻辑，避免所有表现都等待一次网络往返。

PredictionKey 用于将客户端提交的目标数据与对应的 Ability 激活关联，服务端收到数据后继续权威执行。

```text
Client
  └─ Collect TargetData
       ├─ Continue local predicted logic
       └─ Send TargetData
                ↓
             Server
                └─ Authoritative ability execution
