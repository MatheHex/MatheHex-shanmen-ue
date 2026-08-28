# Dev.D.UE.0.0.10.P6.2.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.2.r0`；
- 基线提交：`2112edc2786e3c9e8139f2e040f3fc37856c90d0`（P6.1）；
- 分支：`agent/0.0.10-p6-2-controlled-weapon-product-session`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.1 已证明 exact deployed flying-sword authority evidence，但产品层仍需自行同时管理 ActionRuntime 与 P6.0 execution。如果每个 Actor/input caller 分别推进两者，Recall、开放 emission、Action phase 与 termination 很容易产生半步状态。

因此 P6.2 先建立唯一 product session owner，再进入世界 Actor 接线。具体物品内容尚未冻结，本轮不新增或改名 legacy item。

## 实现记录

### Atomic start

`TryStart` 只接受 `Prepared.IsPrepared()`。它在局部 candidate 中启动 action、跨过 Startup commit point，并绑定 authority evidence 与 execution。最终 session 不变量通过后才写入输出。

### Control and recall

普通 `TryIssueControl` 拒绝 Recall，只处理 Launch/Redirect 与既有严格 sequence/replay。`TryRecallAndComplete` 要求 contact window 已关闭，并把 Recall、Active->Recovery、Recovery->Completed 作为一个 copy-validate-commit 操作。

### Contact windows

begin/resolve/end 都复制完整 session 后调用 P6.0；只有成功且全局不变量仍成立才提交。duplicate candidate、错误 ordinal、非法状态或不完整 target snapshots 不会污染正式 ledger。

### Interrupt cleanup

`TryInterrupt` 先在候选 execution 中执行 termination cleanup，再中断 action。提交后的 Interrupted session 必须 terminal 且 emission closed。

### Prepared-result fence

Adapter 的 `IsPrepared()` 增加 Action、evidence content、Definition、Offense 与 execution 的逐字段一致性。Execution 仅增加 `GetOffense()` const getter，用于验证冻结值，没有开放写入。

## 自动化覆盖

- `Shanmen.0_0_10.Product.ControlledWeaponSession.LifecycleAndCommands`；
- `Shanmen.0_0_10.Product.ControlledWeaponSession.ContactWindows`；
- `Shanmen.0_0_10.Product.ControlledWeaponSession.AtomicFailure`；
- `Shanmen.0_0_10.Product.ControlledWeaponSession.TamperFence`。

覆盖 action commit、exact item command、idempotent replay、redirect、recall completion、duplicate contact、later ordinal、interrupt cleanup、failed redirect/sequence/recall race 无 mutation，以及 content/action/definition/offense tamper rejection。

## 验证期间修正

首轮 Editor build 为 `12/12` Success，原生退出码 0；首轮 focused Session 测试为 `4/4` Success，日志 SHA-256：`FB1ADD1F7956D259F711E65B39846D495DCDAC19BD78DC2F733E642F7E630C65`。

随后静态复核把 Launch/Redirect、begin/end emission 与 candidate resolve 从“复制 execution 后提交”统一收紧为“复制完整 session、最终不变量通过后提交”，并将 Offense 一致性改为精确比较。该修正不是测试失败驱动；修正后重新编译并生成独立最终日志。全程无失败 build 或 failure log。

## 最终自动化

统一命令形态：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p62_controlled_weapon_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `14E94F19A2C63CAC76B142B74A1ED98FD358BEA7125E12E23ECBCE4671587E84` |
| `p62_controlled_weapon_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `A3E399F130F248C36E531F79CDAD37E88051FEAC6BA10BCA8F1B52DAADC415BB` |
| `p62_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `1A6763F913FD4209BB0B2AC03986AF15E03485D2195C4CAA2AB5654305E3114B` |
| `p62_items_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `9F451BB16C2E1DEB74C6BC6EA902174AFE6D2109B4CF1CB0377B9990FB6C6996` |
| `p62_full_final.log` | `Shanmen.0_0_10` | 137 | 0 | 0 | `5DDE0A93E9484669A43ADB8664ACD75911BC313E6BDE3674A9CADB560F775830` |

最终唯一计数 `137/137`。五条日志均 queue empty、native exit 0、目标 fail 0、handled ensure/assert/Fatal 0。每条日志保留测试发现前既有的 13 行 UE 启动诊断。

## Changed-file gate

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=3 Required=4 Logs=5
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p62_combat_runtime_final.log,p62_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=p62_items_final.log,p62_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponAdapter Evidence=p62_controlled_weapon_adapter_final.log,p62_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponSession Evidence=p62_controlled_weapon_session_final.log,p62_full_final.log
```

Regression coverage self-test：`8/8 PASS`。

## 构建

最终 Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- native exit `0`；
- `8.93s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `9/9` actions；
- `Result: Succeeded`；
- native exit `0`；
- `41.90s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- regression JSON parse：PASS；
- Session World/Actor/Input/Spawn/Damage/RNG 依赖：0；
- Session 外部 item/resource mutation call：0；
- item Definition、Profile schema、authority document、资源事务、通用 Projectile 与保存格式未改变；
- 长期未跟踪历史文件不纳入提交。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.3 可接 World contact/delivery：用 World Entity Registry 将 flying-sword Actor sweep/overlap 转为 session candidate，并将合法 Impact receipt 交给 CombatRunCoordinator 的 canonical vitality commit。世界层不能直接结算伤害或写库存。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-2-controlled-weapon-product-session/Docs/Report/Dev.D.UE.0.0.10.P6.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-2-controlled-weapon-product-session/Docs/Log/Dev.D.UE.0.0.10.P6.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-2-controlled-weapon-product-session>
