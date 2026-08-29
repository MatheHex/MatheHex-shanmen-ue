# Dev.D.UE.0.0.10.P8.9.r0 Report

## 1. 结论

P8.9 已建立显式 Formation coverage coordinator，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationCoverageCoordinator`：一次调用接收 caller-owned World、Area、entity registry、Actor 子集、命令与 P8.8 tracker，先调用 P8.6 取得 immutable coverage sample，再按 `Prime`／`Advance`／`Rebase` 模式提交 tracker。Coordinator 本身无状态，不持有 UObject 指针、不发现 Actor、不自行 Tick，也不施加 damage、Buff 或 effect。

最终 focused `4/4`、0.0.10 full `247/247`、changed-file gate、51/51 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 命令契约

`Fdemo_mapShanmenFormationCoverageCommand` 只有三种模式：

- `Prime`：不得携带 expected baseline；
- `Advance`：必须携带调用方观察到的 expected baseline receipt ID；
- `Rebase`：同样必须携带 expected baseline receipt ID，明确保护生命周期替换。

未知枚举、Prime 携带 expected 或 Advance/Rebase 缺 expected 都会在读取 World 前以 `CommandInvalid` 失败关闭。非 Prime 命令在 tracker 未初始化时也不会启动采样。

## 3. Sample → Tracker 编排

合法命令按以下有限链路执行：

1. 验证命令与 tracker 内部一致性；
2. 对 Rebase 先核对当前 baseline；
3. 调用 P8.6 对调用方提供的明确 Actor 子集做一次同步采样；
4. 把 P8.5 coverage receipt 交给 P8.8 `Prime`／`Advance`／`Rebase`；
5. 返回完整 sample 与 tracker result，不隐藏下层精确状态。

P8.6 拒绝时 tracker 完全不被调用；P8.8 拒绝时，结果保留 sampler 成功证据与 tracker 的准确失败原因。

## 4. Advance replay 与 Rebase CAS

Advance 不能在采样前简单比较 expected 与当前 baseline。P8.8 的有界幂等允许“旧 expected + 最近一次相同 current receipt”返回 `AdvanceReplayed`；提前拒绝会破坏发送成功但回执丢失后的安全重试。因此 Advance 先采样，再由 tracker 区分 exact replay 与真正 stale conflict。

Rebase 没有该 replay 语义，且代表显式生命周期替换，所以 coordinator 在采样前对 expected baseline 做 CAS 核验。过期 Rebase 返回 `BaselineConflict`，不读取 Actor 位置，也不替换 baseline。

## 5. 自动化证据

两份日志均为一次命令、一次 queue completion、Fail `0`、fatal／unhandled `0`，进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationCoverageCoordinator` / `FormationCoverageCoordinator.log` | 4 | 0 | `C5FB7FC0FBAF90BD1B918B92C084266DC982307A6BF3B4CF17246BFE96DC3BF8` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 247 | 0 | `C12EFBE5A1C7BD6408FA798FD22ED5AD6B0C43BB517B0946D814AAD1DACF7017` |

Focused 四项覆盖：

1. World Prime、Actor movement Advance、Leave fact 与 exact Advance replay；
2. 跨 Area Rebase、stale Rebase 采样前拒绝、当前 expected 恢复确定性 baseline；
3. sampler 失败不改变 baseline/replay slot，随后 exact Advance 仍可重放；
4. invalid/unknown command、pre-Prime 命令与不同 coverage 的第二次 Prime 围栏。

成功 result 的 sample receipt、tracker current baseline identity 与命令模式互相封印；变异任一身份后 `IsSuccess()` 失败。完整 suite 从 P8.8 的 `243` 增至 `247`，此前测试全部继续通过。

## 6. 改动—回归与静态门禁

新增 `FormationCoverageCoordinator` mapping rule，要求九组证据：Coordinator、Tracker、Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment。

- regression map JSON：PASS，`45` rules；
- mapping self-test：`51/51 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=9 Logs=2`；
- map SHA-256：`0256C5CA37A9A285B3503F5D7402C9B06EDC960C4D45DC19546BD44797AC6C4E`；
- self-test SHA-256：`6C1C4E5C0770B79F6F1E0A9F4504A5EBB015B6AE7AB5884A674BCB458CB30B52`；
- self-test 收口数字改为成功用例自动计数，消除新增用例后忘记手工更新总数的维护风险；
- production boundary scan：唯一命中是说明注释中的“无 Tick”；无 timer、Actor discovery、Spawn、RNG、damage/effect、AbilitySystem 或 item/profile subsystem 调用；
- `git diff --check` 与最终 staged `git diff --cached --check`：PASS。

## 7. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded | 0 | 9.27s | `C01E767748B72DF3846580AB249C8D7CE93972233AFC3704B19E52CE5C35E51E` |
| Editor final check | Succeeded / up to date | 0 | 0.88s | `D5F1F132C3A35826D63376D7B0F5CC28F0D0CE2607C4E52F4A93FC6F09F5E285` |
| Game final | Succeeded | 0 | 20.39s | `E2C1AFEB04246BEA8B85341A68B26B7AB1FEA67BD3DF5DDDF2B6723954ECDD8A` |

- Editor module：`11197952` bytes，UTC `2026-08-29T19:32:17.8038792Z`，SHA-256 `E16061BBCF04C1FDE04330AE3C304B9A44D5243286E2E1CB2913EA9C41904109`；
- Game executable：`352384000` bytes，UTC `2026-08-29T19:34:56.4799655Z`，SHA-256 `A725FA0D2C966CEB47895D6FF32823604E124AEA191E5B9346B34AC98E0440CF`。

UE 启动时只出现未安装非 Win64 SDK 的常规探测提示；Win64 SDK 判定为 VALID，所有本轮原生命令退出码均为 `0`。

## 8. 权威、完整性与兼容性

P8.9 不新增持久权威：

- World 与 Actor 生命周期仍由调用方拥有；
- stable entity binding 仍由现有 registry 拥有；
- Area/membership 真值仍由 P8.5 拥有；
- World location projection 仍由 P8.6 拥有；
- transition reduction 与 baseline 分别仍由 P8.7/P8.8 拥有；
- coordinator 只组合结果，不保存 cadence、pointer 或 effect state。

没有修改 P8.0—P8.8、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链，没有建立第二套 World／Actor／effect 系统。

## 9. 修改范围与 P/F 边界

新增：

- `demo_mapShanmenFormationCoverageCoordinator.h/.cpp`；
- `demo_mapShanmenFormationCoverageCoordinatorTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。长期未跟踪用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。测试只创建无 Tick、无渲染的 transient test World；未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一步与 GitHub

P8.10 建议把 P8.8 tracker 与 P8.9 coordinator 接入现有 `Fdemo_mapShanmenFormationProductHost`，由该唯一 host 在 active deployment 生命周期内显式拥有 Prime/Advance/Rebase/Reset；不要新建第二个 Formation host，也不要自动 Tick 或施加效果。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-9-formation-coverage-coordinator/Docs/Report/Dev.D.UE.0.0.10.P8.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-9-formation-coverage-coordinator/Docs/Log/Dev.D.UE.0.0.10.P8.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-9-formation-coverage-coordinator>
