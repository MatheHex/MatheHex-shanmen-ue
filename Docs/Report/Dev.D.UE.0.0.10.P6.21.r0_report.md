# Dev.D.UE.0.0.10.P6.21.r0 Report

## 1. 结论

P6.21 已完成 Run Host 的多飞剑 Orbit threat sample 稳定顺序原子批次，结论为 **PASS**。

P6.20 已保证单一 exact item 的完整采样事务，但多个 item 仍由调用方逐个提交；若后项失败，前项的 detector ordinal、sample checkpoint 与 presence authority revision 已经不可逆推进。本阶段增加一个显式 item 子集批次：先按稳定 GUID 顺序预检，在完整 Host 候选上逐项复用 P6.20，只有全部 item finalization 与批次回执自检通过后才一次提交。

## 2. 功能性

- 新增 `Fdemo_mapShanmenControlledWeaponThreatSampleRequest`，每项只包含 exact `ItemInstanceId` 与调用方已采集 contacts；
- 新增 `Fdemo_mapShanmenControlledWeaponThreatSampleBatchEntry`，绑定 exact item 与既有 finalization audit；
- 新增 `Fdemo_mapShanmenControlledWeaponThreatSampleBatch`，记录 Run、尝试数、完成数与稳定顺序 entries；
- 新增 `TrySampleOrbitThreatsInOrder()`，只处理调用方明确列出的 item 子集；
- 输入顺序不影响执行顺序：复制请求后按 GUID canonical order 排序；
- 空批次、无效 ID、未知 item 与重复 exact item 在任何 sample 开始前失败；
- 整个 Run Host 先复制为批次候选，每项继续复用 P6.20 的 Begin/Project/End/Finalize 原子事务；
- 任一后项失败时丢弃整个候选，前项产生的 ordinal、checkpoint、intent 与 authority revision 均不外泄；
- 全部完成后，批次回执与候选 Host 同时通过自检才提交；
- 批次回执强制 exact item 唯一、严格 GUID 顺序、RunId 一致及每项 finalization 完整。

## 3. 原子性与确定性证据

新增 `AtomicThreatSampleBatch` 自动化直接验证：

- 以 `high, low` 反序提交两项请求，回执与执行顺序固定为 `low, high`；
- 两项首轮 sample ordinal 均为 `0`；
- low/high 的 presence receipt authority revision 依次为 `1`、`2`；
- 将成功回执 entries 交换后，`IsFullyFinalized()` 失败；
- 将成功回执 RunId 篡改后，`IsFullyFinalized()` 失败；
- 失败批次中，low 空 sample 可在候选上先成功，high duplicate contact 随后失败；原 Host 仍保持两项 ordinal `0`、checkpoint `2`、authority revision `2`；
- 合法重试两项空 sample 均使用 ordinal `1`，checkpoint 精确推进到 `4`，authority revision 保持 `2`；
- duplicate request、unknown item 与 empty batch 都在预检阶段失败且不改变状态；
- 所有路径均不泄漏活动 contact window；
- 目标 vitality 与两件武器 impact ledger 始终不变。

## 4. 兼容性与产品边界

P6.21 不替换 P6.20 单 item API，也不复制 geometry、evidence、policy、presence 或 authority 逻辑。批次只在 Host 层组合既有事务，并保留分步 API 供低层测试与专用调用方使用。

接口不会扫描或隐式采样所有绑定飞剑；调用方显式决定本次 item 子集和每项 contacts。由此避免把 cadence、World overlap query 或产品投放策略冻结进 Host。阶段未定义 Tick/timer、采样频率、威慑伤害、控制效果、持续时间、冷却或 Gameplay Effect。

## 5. 测试覆盖

最终无头自动化：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 9 | 0 | 0 | `0.148s` | `7F6657A8F23FC2AF7AE723C1F2C363EA9E7E058CD45194AB120502354AB4AD9F` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 33 | 0 | 0 | `0.592s` | `CCB4DB43F241F7B77B0F048CF6FDE5B41EEDDE55D20F9CC4458D4D9610965D41` |
| `Shanmen.0_0_10` | 166 | 0 | 0 | `8.245s` | `CF5FA9DCF9D315CA7F51EA81A61EB645A41B7BB8D0EA028EAE7FFF811AD9CCE0` |

每份最终日志只有一个实际 `Automation RunTests` 命令、一个 queue-empty 终止、Fail `0`，Fatal/unhandled/assert/ensure `0`，进程原生退出码均为 `0`。

本阶段没有失败的构建或自动化轮次。首轮实现通过后，仅因新增回执抗篡改断言而执行一次最终重建与全量重跑；两轮均为绿色，不存在被隐藏的失败日志。

## 6. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=3`；
- mapping self-test：`PASS 14/14`；
- `git diff --check`：native exit `0`；
- 新增生产行扫描 Tick/Timer、ApplyDamage/GameplayEffect、SpawnActor、World/RNG、Cooldown/Duration：命中 `0`；
- Source 新增 `315` 行、删除 `0` 行，其中生产 `148` 行、测试 `167` 行；
- 没有修改 Build.cs、GameplayTags、schema、存档、item authority、输入或资产。

## 7. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor：`28/28` actions，`Result: Succeeded`，native exit `0`，`90.14s`；
- 回执断言后的最终 Editor：`4/4` actions，native exit `0`，`5.53s`；
- 首次 Game：`27/27` actions，`Result: Succeeded`，native exit `0`，`87.43s`；
- 最终 Game：`3/3` actions，native exit `0`，`11.37s`；
- 最终 Editor DLL UTC：`2026-08-29T07:28:28.7348060Z`；
- 最终 Game executable UTC：`2026-08-29T07:30:16.7266437Z`。

没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 仅构建，未启动。

## 8. 修改范围与 P/F 边界

- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`
- 本 Report 与同名 Development Log。

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 9. 后续建议

Host 现在具备单 item 与显式多 item 两级原子采样。下一阶段应在独立 cadence owner 中把一次外部采样时机和调用方获得的 contacts 适配为本批次；cadence owner 仍不应进入 Host，威慑 gameplay effect、数值与持续时间继续保持未冻结。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-21-atomic-threat-batch/Docs/Report/Dev.D.UE.0.0.10.P6.21.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-21-atomic-threat-batch/Docs/Log/Dev.D.UE.0.0.10.P6.21.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-21-atomic-threat-batch>
