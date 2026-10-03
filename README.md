# GAS_ARPG

基于 Unreal Engine 5.8 的个人 ARPG 实践项目，使用 C++ 与蓝图实现战斗、角色成长和存档流程。

## 技术要点

- **GAS 战斗**：使用 Gameplay Ability、Gameplay Effect 和 Gameplay Tag 管理技能与状态；实现投射物、连锁法术及护甲、抗性、格挡、暴击、Debuff 伤害结算。
- **敌人与交互**：通过 Enhanced Input 触发技能；敌人使用行为树与黑板控制战斗行为，并支持掉落物。
- **成长与存档**：支持等级、属性和技能升级、技能装备；通过检查点保存角色进度及地图状态，加载界面使用 UMG 与 MVVM。

项目文件：`Aura.uproject`（UE 5.8）。
