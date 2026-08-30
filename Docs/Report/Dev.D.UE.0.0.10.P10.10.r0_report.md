# Dev.D.UE.0.0.10.P10.10.r0 Report

## 1. 结论

P10.10 **PASS**。本阶段建立了 Spirit Evasion 的无状态 device-to-product input adapter，并把它接入 PlayerController 的唯一预绑定入口。适配器先复用既有 gameplay input surface gate，再确认 authoritative GameMode 产品路由存在；只有两者都成立时才恰好采样一次当前战斗方向，并把原始样本恰好一次交给 P10.9 `RouteSpiritEvasionStartIntent`。

适配器不验证、归一化或改写方向，不创建 config/identity，不自动重试，也不回退旧技能路径。方向有效性与 XY 产品语义继续由 P10.8/P10.9 authority 统一判定。PlayerController 当前使用已有 `GetLastValidAimDirection()` 作为单次方向样本；没有新增物理按键或修改 InputAction registry。

最终验证为 focused `6/6`、0.0.10 全量 `449/449`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、V3.Attributes `4/4`、FormationInfluenceConsumerWorldResolution `1/1`，六份正式日志合计 `526` 条 Success、`0` Fail。changed-file gate、134/134 mapping self-test、静态边界扫描、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 Input adapter

`Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput` 接收四项显式输入：

- 当前 gameplay surface 是否允许产品输入；
- authoritative GameMode product route 是否可用；
- 一次惰性方向采样回调；
- 一次 P10.9 产品路由回调。

执行顺序固定为：

1. gameplay blocked 时返回 `GameplayBlocked`；
2. GameMode route 不可用时返回 `ProductRouteUnavailable`；
3. 记录 `bDirectionSampled=true`，调用 sampler 一次并保存原始向量；
4. 记录 `bProductRouteInvoked=true`，用同一向量调用 product route 一次；
5. 只有完整 P10.9 proof accepted 时返回 `Applied`，否则返回 `ProductRejected`。

前两种拒绝不执行 sampler 或 route。后两种路径都不重采样、不重试、不创建 fallback action。

### 2.2 Audit result

`Fdemo_mapShanmenSpiritEvasionInputResult` 保留：

- typed status；
- 是否已采样；
- 是否已调用产品路由；
- 原始 sampled direction；
- 完整 P10.9 ProductRoute result；
- 原始产品诊断。

`IsAccepted()` 同时要求 status Applied、两个一次性 proof 均存在，并且下游 ProductRoute 自身完整 accepted，不能用仅有布尔成功掩盖不完整 reservation/receipt。

### 2.3 PlayerController seam

新增 `Ademo_mapPlayerController::RouteSpiritEvasionStartInput()`：

- 使用既有 `IsGameplayInputAllowed()`，不复制 UI/defeat/reset gate；
- 从当前 World 解析 authoritative `Ademo_mapGameMode`；
- 只在 eligible 时读取一次 `GetLastValidAimDirection()`；
- 只调用一次 `GameMode::RouteSpiritEvasionStartIntent`；
- 空 GameMode 回调 fail closed，不解引用空指针。

该方法尚未加入 `BindProductInputActions`，因此本阶段冻结的是输入适配契约，不是物理按键上线。

## 3. 完整性与兼容性

- 复用 PlayerController 既有 gameplay/UI lock 语义；
- 复用现有 last-valid combat aim，不新增第二套 cursor trace 或方向 cache；
- 复用 P10.9 GameMode product route，不直接接触 component/host；
- 复用 P10.8 direction validation、canonical config 与 Run-owned reservation；
- 不增加 input ordinal、GUID、Timer、Tick 或 adapter mutable state；
- 不增加 InputAction、默认按键或配置迁移；
- 不读取、预留或写入 SpiritEnergy；
- 不直接移动 Character/Actor，不访问 World collision；
- 不修改 item/profile/schema/CodeB；
- 不改变 thrown weapon hotbar fallback、普通攻击或旧技能绑定；
- Enemy/V2、legacy attributes 与 0.0.10 全量回归均通过。

## 4. 关键不变量

1. gameplay blocked 时 sampler 与 product route 调用次数都必须为零；
2. GameMode route 缺失时 sampler 与 product route 调用次数都必须为零；
3. eligible input 恰好采样一次；
4. route 接收的向量逐值等于该唯一原始样本；
5. eligible input 恰好委托产品路由一次；
6. adapter 不归一化、不修复或替换方向；
7. invalid direction 由 P10.8 在 reservation 前拒绝；
8. downstream rejection 不触发 adapter 自动重试；
9. reservation 发布后的 rejection 仍保持 sequence 已消费；
10. accepted input 必须包含完整 P10.9 ProductRoute proof；
11. PlayerController 只调用 GameMode direction-intent 入口；
12. 物理按键策略与 SpiritEnergy authority 不属于本阶段。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionInputAdapter` 六个测试：

- `GameplayFence`：input lock 拒绝且 sampler/route 都为零次；
- `RouteFence`：GameMode route 缺失时不采样、不委托；
- `SingleSample`：原始三维向量逐值保留，sampler/route 各一次，rejection 不重试；
- `InvalidIntent`：零方向只委托一次，由 product authority 拒绝，sequence 不变、preflight 为零；
- `AppliedProof`：真实 transient 产品链 accepted，P10.8 负责平面归一化，sequence 恰好推进一次；
- `RejectedExecution`：preflight 拒绝时 adapter 不重试，已发布 reservation 保持消费。

0.0.10 全量由 P10.9 的 `443` 增至 `449`。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapter.h`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapterTests.cpp`。

更新：

- `Source/demo_map/demo_mapPlayerController.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

产品、测试和流程本体共 `7` 个文件、`561` insertions、`0` deletions；加入本 Report 与同名 Log 后 exact stage 为 `9` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.10-SpiritEvasionInputAdapter-final.log` | focused input adapter | 6 | 0 | `0BE352E725F4707ED9086EAA328075620FE48968B9EB194B8387EE672609EF63` |
| `P10.10-Shanmen-0_0_10-final.log` | 0.0.10 full | 449 | 0 | `A9AF65122CE3312BB379E39C8934ED9280BB9D3AA8FB6F10F9A0D8E04974882B` |
| `P10.10-EnemySkillFramework-final.log` | enemy skills | 44 | 0 | `BEAE91FB19756889ADDC3DF70193BF19985D7B946DA8D647E2FAE225B0C6A145` |
| `P10.10-V2RangedCompatibility-final.log` | V2/input compatibility | 22 | 0 | `828B61C758C9005FA16B47452A026D7351D461DEE2569306728FC850DD12137A` |
| `P10.10-V3-Attributes-final.log` | legacy attributes | 4 | 0 | `08732B2C87614F43523BCF9DAC1283F55EDF62A3637C4EBD946C05E923B38B16` |
| `P10.10-FormationInfluenceConsumerWorldResolution-final.log` | focused coordinator dependency | 1 | 0 | `FB9E14C8132E8C50F56277BBB3D24FD0AA206BAF7D18DC7C9F4F4C5DBA8AC906` |

所有正式进程原生退出码均为 `0`，每份日志均有 native terminal-success 标记；selected test Fail、fatal、unhandled 与 ensure 均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=87
SELF_TEST: PASS 134/134
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=20 Logs=6
git diff --check: PASS
PHYSICAL_BINDING_HITS=0
RESOURCE_HITS=0
MUTATION_HITS=0
IDENTITY_FACTORY_HITS=0
PLAYER_CONTROLLER_BINDING_HITS=0
PLAYER_CONTROLLER_ROUTE_CALLS=1
```

mapping SHA-256：`56F5192AD15A3BEFCA78C73048DF9977B3CA04DDE2ED570CA9FCBA608A9E6D70`；self-test SHA-256：`834DB80DACC4BD5332B0D2097564B29C479990FB5B1F45F694D185A7C44FC34C`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 16 / 88.31s | 0 | `AD659B259A6FD2717ED2AE2512B35CB407B07A5BC3F43B4976AD7D63DB1F21E7` |
| Editor post-review | Succeeded | 4 / 14.84s | 0 | `1BA6A6F785CC2F2AC5E5ECF57AC5E5DFD09E8E90E9B2BD529FC82E06A996CC1C` |
| Editor final | Succeeded, up to date | 0 / 0.89s | 0 | `45E0867D85C8A3C33E4682BA06AE2A12B7E4851F2FE71D08B622855B4285F9E7` |
| Game final | Succeeded | 3 / 23.71s | 0 | `B05FDCDDF8A1708E5595681C990A74E128284FFF9F41BCCFFFFE9F0AE5F56D20` |

最终 `UnrealEditor-demo_map.dll`：`12484096` bytes / SHA-256 `A8F223415172289D072B920B8768FC9756A2427604641D849710AE4681F380D6`；`demo_map.exe`：`353930752` bytes / SHA-256 `2F42C3723C47301FBD54A84FFD5F606FC7E0CA8B702FBE384727F3F163CAC3F3`。

## 9. 真实异常

没有源码编译失败、selected Automation failure、changed-file gate failure、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

最终代码审查把 PlayerController route lambda 改为显式空 GameMode fail closed，防止未来 route-availability 契约被错误调用时解引用空指针。随后执行 4-action Editor 增量构建，并重新生成全部正式 Automation 与最终双目标构建证据；这是审查加固，不是失败修复。

六次 UnrealEditor-Cmd 启动阶段仍出现引擎自带 `UE::UnifiedErrorTest` 初始化噪声，包括 13 行 `LogAutomationTest: Error: Condition failed`。这些行发生在 Engine 初始化、selected suite 运行前；随后所有 selected tests 均 Success、Fail 为 0、native terminal exit 为 0，且没有 ensure/fatal/unhandled。原始日志未删改。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 使用 callbacks 与 transient GamePreview World 验证 adapter 和完整产品链；没有发送真实键盘事件，也没有验证连续画面手感。

下一阶段建议 P10.11 把 Spirit Evasion 纳入统一 InputAction registry，选择无冲突、可重绑定的默认物理键，并在 `BindProductInputActions` 中只绑定 Press 到本阶段入口；同步补齐默认配置迁移、冲突/重绑定与 input-lock 自动化。SpiritEnergy 仍必须等待唯一资源 authority，不能在按键 handler 中临时扣除。
