# Dev.D.UE.0.0.10.P6.22.r0 Report

## 1. 结论

P6.22 已完成独立的 Run-scoped controlled-weapon threat sample Router，结论为 **PASS**。

P6.21 已能把调用方提供的多 item contacts 原子提交给 Host，但没有定义外部采样脉冲的身份、顺序与重放语义。P6.22 在 GameMode 与 Host 之间增加一个有界 Router：调用方显式提供 Run、IntentId、连续 sample sequence 与 contacts；Router 规范化世界证据、拒绝乱序/冲突、缓存最新回执，并与 Host 候选一次提交。阶段没有猜测 Tick、timer、overlap query 或采样频率。

## 2. 功能性

- 新增 `Fdemo_mapShanmenControlledWeaponThreatSampleIntent`，冻结外部脉冲的 `IntentId`、`RunId`、`SampleSequence` 和明确 item 子集；
- capture 使用 Coordinator 的 Run-scoped `WorldEntityRegistry` 把 Actor/Component 转成稳定 `TargetEntityId`；
- item 按 GUID 排序，contact 按 EntityId、body、location、normal、blocking flag 排序，调用方/World 返回顺序不影响 authority 顺序；
- Router 持久状态只保存 UObject-free 指纹与最新一次成功回执，不保存 Actor、Component 或无界历史；
- sequence 从 `0` 开始且必须连续；future gap、stale history 与不可递增的 `MAX_int64` 均失败关闭；
- 最新完全相同的脉冲返回原回执，不再次推进 Host；同一最新 sequence 的不同 ID/载荷及最新 IntentId 跨 sequence 复用均拒绝；
- 新脉冲提交前重新解析 contact identity，拒绝 capture/route 间的实体绑定变化；最新精确 replay 不依赖当前 World 对象仍存在；
- Host 与 Router 均在局部候选上推进，P6.21 batch 或 Router 自检任一失败时两者都不提交；
- 失败不消耗 sequence 或 IntentId，修正后的请求可使用原 identity 重试；
- GameMode 提供显式 route seam，并在 Run 激活预检、正常释放、孤儿回收和失败回收中统一管理 Router 生命周期。

## 3. 有界性、原子性与确定性证据

新增三项聚焦自动化：

- `StableReplayAndConflict`：反序 high/low 输入稳定提交为 low/high；两项 sample ordinal 均为 `0`，authority revision 为 `1/2`；精确 replay 不改变 checkpoint、authority 或 next sequence；改变最新 payload 后冲突失败；
- `AtomicFailureAndRetry`：low 空 sample 可先在候选成功，high duplicate contact 随后失败；真实 Host checkpoint/authority 与 Router sequence 均保持 `0`；同一 IntentId/sequence 的修正空批次成功并产生两个 ordinal `0` NoOp；
- `SequenceRunAndResetFences`：拒绝 sequence gap、`MAX_int64`、stale history、最新 IntentId 跨 sequence 复用和跨 Run Router；连续 `0/1` 映射到 Host ordinal `0/1`；无稳定注册身份的 geometry 无法 capture；Reset 返回有效空状态。

Router 只保留 latest replay record，因此内存不随运行时长增长。较旧 sequence 始终由单调 fence 拒绝，不依赖保存完整历史。

## 4. 兼容性与产品边界

P6.22 复用 P6.21 `TrySampleOrbitThreatsInOrder()`，不复制 detector、evidence、policy、presence 或 authority 逻辑。既有单 item、批次、Run command 和 movement API 未替换。

GameMode 没有自动调用新 Router。调用方仍是采样时机、空间查询、item 子集和 contacts 的唯一所有者。阶段未增加 Tick/timer、World overlap query、威慑伤害、控制效果、持续时间、冷却、Gameplay Effect、资源事务或库存修改。

## 5. 测试覆盖

最终无头自动化：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponThreatSampleRouter` | 3 | 0 | 0 | `0.034s` | `B1E51E05763BBB5C6A65B6A2BA433BD0CD9FEA3D55A9C015393A686E8ABEA64B` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 36 | 0 | 0 | `0.618s` | `1E220C6076F2E1D845000992B3BE5BE992FB46D08E2F459D65C57C7EA9582D03` |
| `Shanmen.0_0_10` | 169 | 0 | 0 | `9.145s` | `B6F8CAA7A3C8B2B5430B6ED0246AC1F7E0E371D628F743FFB0EF72FFBE944E3D` |

每份最终日志只有一个实际 `Automation RunTests` 命令、一个 queue-empty 终止、Fail `0`，Fatal/assert/ensure `0`，进程原生退出码均为 `0`。

实现首轮自动化也是全绿；随后增加 capture/route 身份竞态防护和极限序号断言，重新编译并生成以上最终日志。没有失败自动化或构建轮次。

## 6. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=11 Logs=3`；
- mapping self-test 从 `14/14` 扩展为 `16/16`，新增 Router 正/反例均通过；
- `git diff --check`：native exit `0`；
- Router 生产文件对 Tick、timer、GetWorld/overlap query、ApplyDamage、SpawnActor 与 RNG 调用扫描命中 `0`；
- Source 合计 `+1451/-6`，其中生产 `+802/-6`、自动化测试 `+649/-0`；回归映射与自检 `+32/-1`；
- 没有修改 Build.cs、GameplayTags、schema、存档、item authority、输入或资产。

## 7. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 初始 Editor：`24/24` actions，`Result: Succeeded`，native exit `0`，`114.63s`；
- 身份竞态增强后的 Editor：`24/24` actions，native exit `0`，`77.05s`；
- 最终 Editor：`5/5` actions，native exit `0`，`8.17s`；
- 初始 Game：`23/23` actions，`Result: Succeeded`，native exit `0`，`87.06s`；
- 最终 Game：`4/4` actions，native exit `0`，`13.61s`；
- 最终 Editor DLL UTC：`2026-08-29T08:13:00.4113431Z`；
- 最终 Game executable UTC：`2026-08-29T08:14:31.3638832Z`。

没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 仅构建，未启动。

## 8. 修改范围与 P/F 边界

- `Source/demo_map/demo_mapShanmenControlledWeaponThreatSampleRouter.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponThreatSampleRouterTests.cpp`；
- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 9. 后续建议

Router 已建立外部 cadence pulse 的稳定身份与事务边界。下一阶段若接入真实 overlap collector，应由产品层明确采样频率、查询形状与参与 item 策略，再把一次查询结果捕获成 Intent；不要把这些策略下沉到 Host/Router。威慑效果、数值与持续时间仍应作为后续独立契约。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-22-threat-sample-router/Docs/Report/Dev.D.UE.0.0.10.P6.22.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-22-threat-sample-router/Docs/Log/Dev.D.UE.0.0.10.P6.22.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-22-threat-sample-router>
