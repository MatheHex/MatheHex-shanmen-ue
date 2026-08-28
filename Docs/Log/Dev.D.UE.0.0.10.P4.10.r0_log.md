# Dev.D.UE.0.0.10.P4.10.r0 Development Log

## 目标

把M01 authored `StandardBruiser`与`EliteBulwark`的合法扇形重击接入0.0.10 canonical combat。保留既有world geometry／LOS／faction／telegraph／recovery语义，使用windup前预留的Run-local sequence、冻结action spec、deterministic identity、玩家defense snapshot与exactly-once vitality ledger。M01产品失败必须关闭且不得fallback；非M01 heavy与M01 Boss继续兼容。

## 基线

- 基线分支：`agent/0.0.10-p4-9-ranged-projectile-product`；
- 基线提交：`bf5021cd99dbd0629adac584bcf440ca3531c921`；
- 当前分支：`agent/0.0.10-p4-10-heavy-attack-product`；
- 基线coordinator定向：10/10；
- 基线0.0.10全量：106/106；
- P4.9已完成ordinary melee、melee dash与ranged projectile；Heavy Actor合法扇形接触仍直接调用`UGameplayStatics::ApplyDamage`。

## 审计与设计决定

### 1. 一个HeavySector family覆盖现有三名heavy

M01配置有2个`StandardBruiser`与1个`EliteBulwark`。三者使用同一个Heavy Actor、同一sector授权流程，差异来自authored tuning，因此冻结一个`HeavySector` family。binding必须同时满足空SkillProfile与Heavy Actor类型；同样空profile但使用独立Actor的Boss不会被接入。

### 2. sequence在windup开始时预留

`ResolveCount`是既有兼容诊断，类型为`int32`且只描述实际resolve次数，不适合作为canonical identity。新增独立`uint64` next／active sequence：方向有效后、状态进入windup前预留；取消会消费该action序号，resolve延迟不会改变身份。zero／max值失败关闭，配置与新Run注册重置到1。

### 3. Actor保留世界授权，coordinator拥有结算

Heavy Actor继续负责锁定方向、扇形几何、垂直容差、WorldStatic LOS、阵营过滤与时序。只有授权后的接触进入GameMode/coordinator；coordinator负责Run／Registry／source vitality／binding复核、action与candidate、defense capture、纯函数resolve及唯一vitality commit。

### 4. M01失败关闭，非M01兼容

GameMode的M01 gate声明产品所有权。gate为真时，无论canonical结果如何，Actor都不调用legacy damage，并按既有流程进入recovery；gate为假时保留原`ApplyDamage`。这避免双写，也不改变旧地图。

## 实现过程

1. 审计Heavy Actor的AI、windup、sector resolve、M01定义、GameMode gate与coordinator binding。
2. 冻结`HeavySector` action／detector／formula／content spec。
3. 新增`ExecuteM01EnemyHeavySectorAttack`，在构造identity前拒绝zero与耗尽sequence。
4. 扩展通用enemy attack执行器，仅允许空profile的Heavy Actor使用HeavySector；Boss与其它source失败关闭。
5. GameMode增加M01 heavy产品入口与结构化family／sequence／action／impact／commit日志。
6. Heavy Actor新增Run-local next／active sequence，在windup前预留，在resolve／cancel／recovery边界清理active值。
7. M01新Run首次注册与encounter configuration重置sequence；重复注册保持幂等。
8. Heavy合法接触按M01 gate选择canonical或非M01 compatibility分支；canonical失败不fallback。
9. 新增`M01EnemyHeavySectorProduct`自动化，覆盖标准／精英来源、身份、defense、replay、reconstruction reject、Run隔离与lethal clamp。
10. EnemySkillFramework扩展heavy默认sequence契约。
11. 执行Editor构建、定向／全量自动化、三组兼容回归、静态扫描与Game构建。
12. 复跑全部五套自动化并记录SHA-256。

## 验证时间线

1. Editor Development：25/25，Succeeded，退出码0。
2. coordinator初跑：11/11 Success，0 Fail。
3. EnemySkillFramework初跑：44/44 Success，0 Fail。
4. V2RangedCompatibility初跑：22/22 Success，0 Fail。
5. ItemUseAndArmor初跑：46/46 Success，0 Fail。
6. 0.0.10全量初跑：107/107 Success，0 Fail。
7. `git diff --check`、唯一写入点、模块边界、随机身份与M01 definition扫描通过。
8. Game Development：24/24，Succeeded，退出码0。
9. coordinator／EnemySkillFramework／V2／Item／0.0.10全量最终复跑全部成功。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 25/25 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRunCoordinator定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- performed：11；result：11 Success、0 Fail；queue empty；退出码0；
- final：`Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_combat_run_automation_final.log`；
- SHA-256：`2F1DC28E814572AB6D93C103D74F4F2BEA38F30B3A56D0046405CAE1C82C9D68`。

### 0.0.10全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- performed：107；result：107 Success、0 Fail；queue empty；退出码0；
- final：`Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_full_automation_final.log`；
- SHA-256：`545053C252BDD1382BD77F8E885C90F4F12415F3DF99D063D8B1A1B2CE43C684`。

### EnemySkillFramework兼容回归

- performed：44；result：44 Success、0 Fail；queue empty；退出码0；
- final：`Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_enemy_skill_automation_final.log`；
- SHA-256：`DC3BB155FB68114A641BF3B85418B49B4DF60CD9D67436E3BCE0B5B915D5ACA1`。

### V2RangedCompatibility兼容回归

- performed：22；result：22 Success、0 Fail；queue empty；退出码0；
- final：`Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_v2_ranged_automation_final.log`；
- SHA-256：`43841EA3DFC9D2B76A379A0FF3DA74D29878DBE0EA0770A06CEC73C64BD4AC0D`。

### ItemUseAndArmor兼容回归

- performed：46；result：46 Success、0 Fail；queue empty；退出码0；
- final：`Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_item_armor_automation_final.log`；
- SHA-256：`AE05E5ED53598711C7B90C3728911363AA66A2E750D225041F134752373C1B2E`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 24/24 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### 静态检查

- `git diff --check`：退出码0；
- coordinator中legacy damage／随机GUID／RNG／地址身份API：0匹配；
- `ShanmenCombatRuntime`中`demo_map`／`UWorld`／`AActor`／`ApplyDamage`／随机API：0匹配；
- Heavy Actor保留1个`ApplyDamage`，只在非M01 compatibility分支；
- M01 authored heavy definitions：3个，均为Heavy Actor；Boss独立；
- final logs中fatal／unhandled exception／handled ensure：0。

## 初始日志证据

- coordinator 11/11：`2BBF9C8F23A9CBE8E5AD71DC21B61BB90BF05D583C9D004E2C4BAA535F73A3E2`；
- EnemySkillFramework 44/44：`483650EBEC5E7C26E8B730B6721596285345E4FCC4A8533FF1D69E61D2F9E760`；
- V2RangedCompatibility 22/22：`76D7444A08D3900961FD80B5F340C4DB4F757B9D7368C7A6EDB19295E126EBBB`；
- ItemUseAndArmor 46/46：`7B6CBFDA127F1BFDBE400CB19F4563A6B9AFC0A6D96AB6A243087818A5362C0D`；
- 0.0.10全量107/107：`A4930950A81D7FF9D41764F03D815842F12B08329AE7823F5DCCB1B334E997DC`。

## 最终不变量

1. M01 heavy合法sector contact只有canonical vitality写入；产品失败不fallback。
2. 非M01 heavy保留legacy路径；Boss不会借用HeavySector。
3. world geometry、LOS与faction授权留在Actor；resolver不查询World。
4. action sequence在windup前预留，取消会消费序号，resolve时不重新编号。
5. sequence不复用`ResolveCount`，并在新Run重置为1。
6. identity只依赖RunId、稳定source EntityId、冻结action、sequence、detector、target与ordinal。
7. invalid sequence／binding／source／target在mutation前拒绝。
8. receipt要求Shape detector kind与HitOrdinal 0，并保持伤害守恒。
9. fractional defense不截断。
10. exact receipt replay不重复生命、revision、ledger或broadcast。
11. same identity／different snapshot被拒绝，不能伪装成replay。
12. old-Run receipt不能写入新Run。
13. canonical成功或失败都进入既有recovery，不产生第二次legacy写入。

## 诊断与边界

- 初跑与最终复跑全部通过，没有源码、测试基线或环境失败。
- 五份最终日志各有13条UE测试发现阶段既有负向自检`Condition failed`；目标测试全部通过。
- Win64 SDK有效；非目标LinuxArm64／VisionOS SDK metadata警告不影响Win64构建。
- 未启动Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook或Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-10-heavy-attack-product/Docs/Report/Dev.D.UE.0.0.10.P4.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-10-heavy-attack-product/Docs/Log/Dev.D.UE.0.0.10.P4.10.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-10-heavy-attack-product>
