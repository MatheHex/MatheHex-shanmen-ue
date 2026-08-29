# Dev.D.UE.0.0.10.P7.9.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.9.r0`；
- 基线：`c756c86608a034af9100e84b4c494395dc50d08e`（P7.8）；
- 分支：`agent/0.0.10-p7-9-thrown-weapon-input-adapter`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

P7.8 已把 device-independent thrown hotbar intent 接入真实 Combat Run，但尚未连接既有 1–9 输入。P7.9 建立唯一薄输入适配器：按 durable active-Run 快捷栏中的 exact item semantic 分流，普通消耗品继续旧路径，typed 暗器才采样 aim 并提交 P7.8 lifecycle。

## 审查结论

现有 `Ademo_mapPlayerController::UseHotbarSlot` 已是 1–9 输入的统一入口，旧物品消费由 `Ademo_mapV3ProgressionManager::RequestUseBoundQuickSlot` 执行。ShanmenItems 的 active-Run correlation 已冻结 exact hotbar item IDs，canonical catalog 已为练习飞刀声明 exact `Item.Weapon.Thrown` tag。

因此本轮没有重建输入映射、快捷栏或物品投影。PlayerController 只在旧调用前询问 typed adapter，并仅在 `PassThrough` 时执行原调用。

## 实现

### Input adapter

新增：

- `demo_mapShanmenThrownWeaponInputAdapter.h`；
- `demo_mapShanmenThrownWeaponInputAdapter.cpp`；
- `demo_mapShanmenThrownWeaponInputAdapterTests.cpp`。

Adapter 依次证明 ready authority、durable Run、frozen hotbar exact item、snapshot revision、owner/scope、canonical definition 与 exact typed semantic。非暗器在任何 transform/aim sampling 之前返回 `PassThrough`。

typed 暗器继续验证 matching P7.8 lifecycle、CombatRunCoordinator 与 canonical player source Actor。只有证据完整时才调用 aim sampler，生成稳定 SelectionId，捕获 immutable HotbarIntent 并提交 lifecycle。

### Identity

Selection identity 使用：

```text
namespace = demo_map.ShanmenThrownWeapon.HotbarInputSelection.r1
parts     = CorrelationId, RunId, ItemInstanceId,
            HotbarSlotNumber, AuthorityRevision, SelectionOrdinal
```

Ordinal 从 `1` 开始，只在有效 intent 捕获后推进；无效 aim 和所有采样前拒绝均不消耗 identity。Run 激活和释放后由 GameMode 显式 reset。

### PlayerController / GameMode

GameMode 持有 adapter 并暴露 `RouteThrownWeaponHotbarInput`。PlayerController 保留现有 `UseHotbarSlot` 和 1–9 绑定：

- `PassThrough`：原 `RequestUseBoundQuickSlot` 执行一次；
- typed handled：不再执行旧消费路径；
- typed fail-closed：同样不回落，避免事务旁路。

## 新增测试

`Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` 包含：

1. `DeterministicIdentity`：canonical replay、ordinal isolation、非法 slot；
2. `TypedRouteAndPassThrough`：丹药和空槽位零采样/零 mutation，练习飞刀单次采样、稳定 identity、authority `+2` revision；
3. `FailClosedBeforeSampling`：invalid slot、foreign source、unbound lifecycle、invalid aim 与序号不变。

测试夹具通过真实 Profile → Cutover → Preparation → active Run 流程建立丹药和练习飞刀两个槽位，没有伪造第二套 hotbar truth。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `ThrownWeaponInputAdapter.log` | `Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` | 3 | 0 | 0 | `BE62942FD613E805FD0CBB663EC2AFC63EF69390B4554AC690109A30B80A4147` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 207 | 0 | 0 | `90C777CB8A90DE90C1090C1DC9B065FFE4404CA1BD502AB161C1D51C1ADC8F24` |
| `ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `3B6579B37394AC0603C22D2D38FE73728696C2814E482B40D281E83E0727AEBB` |
| `V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `791699AFA0E16913764DD569BFB696AD423D9CEEE051426F87C1C6195BAD8E42` |

全部最终日志满足一个 RunTests、queue-empty、Fail `0`、fatal/assert/ensure marker `0`；所有进程原生退出码为 `0`。

## Changed-file regression gate

回归映射新增 typed input rule，并把 GameMode / PlayerController 的新 seam 纳入要求。自检同时增加正向覆盖与“缺少普通快捷栏证据必须失败”的反向场景。

```text
REGRESSION_MAP_JSON: PASS
SELF_TEST: PASS 30/30
REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=15 Logs=4
```

15 个必跑组全部由四份健康日志覆盖；没有仅凭 targeted 新测试替代旧快捷栏或远程兼容回归。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Actions | Result | Exit | Total | UBT SHA-256 |
|---|---:|---|---:|---:|---|
| `demo_mapEditor` | 24/24 | Succeeded | 0 | 116.25s | `FAEB7785BC385FF22EA9DB488AE4F675C92D5EFADEA0A75DD42D546F83608EE9` |
| `demo_map` | 23/23 | Succeeded | 0 | 89.30s | `847F25033F05DCB401A8699A0C447D228EBDF9AF412582552453F155E8FFC0B0` |

Editor DLL：`10739712` bytes，UTC `2026-08-29T14:19:54Z`。Game executable：`351923200` bytes，UTC `2026-08-29T14:25:51Z`。

首次 Editor、四组自动化和首次 Game 均直接成功，没有重试或失败日志。

## 静态与边界

- regression JSON parse：PASS；
- boundary scan：无 Tick、timer、input rebinding、legacy item subsystem、ApplyDamage 或 RNG；
- `git diff --check`：exit `0`；
- 未修改 schema、Content、Build.cs、GameplayTags、Code B 或 legacy item authority；
- 长期用户未跟踪文件未修改、未 stage。

## P/F 边界与下一步

只执行 P 阶段源码、静态检查、`-NullRHI` Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

P7 初级暗器直线投掷的 P 阶段纵切已闭合。真实输入、碰撞、视觉与手感属于 F 阶段。若继续 P 阶段，P8 应从阵图/阵眼/材料需求/部署状态机的纯契约开始，继续保持材料事务、世界 Actor 与 UI 分阶段接入。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-9-thrown-weapon-input-adapter/Docs/Report/Dev.D.UE.0.0.10.P7.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-9-thrown-weapon-input-adapter/Docs/Log/Dev.D.UE.0.0.10.P7.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-9-thrown-weapon-input-adapter>
