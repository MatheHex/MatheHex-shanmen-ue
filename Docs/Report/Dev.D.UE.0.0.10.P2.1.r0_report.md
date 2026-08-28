# Dev.D.UE.0.0.10.P2.1.r0 开发报告

## 结论

`PASS`。P2.1 已在 `ShanmenWorldGameplay` 内闭合 P2.0 留下的两项身份前置条件：Run／Spawner 现在可以显式、可重放地建立 World Entity 身份，detector runtime 也有唯一 emission session 权威分配 HitOrdinal 并去除同一 emission 内的重复目标接触。

P2.0 的 Sweep／Overlap／Projectile adapters 已强化为必须把冻结 Action 的 RunId 交给 entity resolver。Registry 只解析当前 Run 的有效对象绑定；跨 Run、对象身份冲突、未绑定对象和过期 emission 候选全部失败关闭。

本轮仍没有接入旧 `demo_map` 战斗、没有执行 World query、没有决定目标策略、没有计算／应用伤害，也没有启动 GAS。P0 路线图中的 P2 候选适配与身份链已经具备进入 P3 编排层的条件。

## 实现内容

### 确定性 World Entity 身份

新增 `FShanmenWorldEntityIdFactory::MakeEntityId`，只使用：

- `RunId`；
- 稳定 `SpawnSourceId`；
- 非负 `SpawnOrdinal`。

规范输入进入 `Shanmen.World.Entity.r1` deterministic namespace。相同 spawn tuple 可重放同一 EntityId；Run、source 或 ordinal 改变都会产生不同身份。无效输入返回 invalid GUID，不以对象指针、对象名或随机数兜底。

### Run-scoped Entity Registry

新增 `FShanmenWorldEntityRegistry`：

- `TryBeginRun`／`TryEndRun` 显式控制唯一活动 Run；live Registry 不会静默切换 Run；
- `BindObject` 接收 Spawner 已决定的 EntityId，支持对象全 body 或精确 physics BodyIndex；
- 同一对象的重复同值绑定幂等；改绑为另一 EntityId 返回 `Conflict`；
- Actor 与 Component 可以作为同一 EntityId 的别名；若两条可见绑定不一致，解析失败关闭；
- `UnbindObject`、Run end 与 `Reset` 清理瞬态映射；
- `FObjectKey` 保留 UE object index／serial 身份，不持有对象、对象名或裸指针哈希作为战斗 ID。

P2.0 的 `IShanmenWorldEntityResolver` 现额外接收 `ExpectedRunId`；adapter 从冻结 Action 自动传入，因此 foreign-Run contact 无法借当前 Registry 生成候选。

### Detector Emission Session

新增 `FShanmenDetectorEmissionSession`，每个实例只绑定一个 Action＋Detector：

1. `TryBeginEmission` 为一次 scheduled sweep、overlap sample 或 projectile contact event 生成冻结 context；
2. 同一 emission 中所有目标共享同一 HitOrdinal；ImpactId 仍由 TargetEntityId 区分，因此目标回调顺序不影响身份；
3. `TryAcceptCandidate` 只接受 exact Activation／Source／Detector／Kind／Ordinal，并按 TargetEntityId 去重；
4. `TryEndEmission` 只递增一次 ordinal；同一目标在下一 emission 可以再次合法命中；
5. nested begin、旧 ordinal、emission 外候选和不匹配候选均拒绝。

建议的 P3 使用链为：

`BeginEmission -> World Query -> P2 Adapter -> Session Accept -> target policy / Impact -> EndEmission`。

## 自动化覆盖

WorldGameplay 最终共 `10` 项：P2.0 原有 6 项继续通过，P2.1 新增：

| 测试 | 结果 |
|---|---|
| `EntityIdFactory` | PASS；spawn tuple 重放、Run／ordinal 隔离、非法输入失败关闭 |
| `EntityRegistry` | PASS；begin/bind/conflict/resolve/unbind/rebind/end 与跨 Run 隔离 |
| `RegistryAdapterIntegration` | PASS；精确 component/body 绑定进入 Sweep candidate，foreign Run 拒绝 |
| `DetectorEmissionSession` | PASS；目标去重、同 ordinal 多目标、下一 emission 递增与重放 |

最终结果：

- `Shanmen.0_0_10.WorldGameplay`：`10/10 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`76/76 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P2.1.r0_worldgameplay_automation.log`：`4EEDAE5908B4629A6532F0DCFDD07E7BAE2A0DBB7328CA2469196A336F12B09A`；
- `Saved/Logs/Dev.D.UE.0.0.10.P2.1.r0_full_automation.log`：`D918267A38A98E38A6144F36218051E8713C7EABACB38D5E93254ADD98A39263`。

首次实际定向执行为 `9/10`：测试 fixture 错误地实例化了 UE 标记为 abstract 的裸 `UObject`，触发 handled ensure。fixture 改为合法 transient `UBoxComponent` 后，最终定向与全量均通过，最终日志中该 ensure 为 `0`。这不是 Registry 产品源码失败，Development Log 保留完整过程。

UE 5.8 在测试发现前仍各打印既有 13 条 `LogAutomationTest: Error: Condition failed` 启动诊断，并报告非目标 LinuxArm64／VisionOS SDK metadata 缺失；Win64 SDK 为 VALID，目标测试随后全部成功。

## 构建与静态检查

- `git diff --check`：退出码 `0`；
- Editor 首次 P2.1 编译：11/11 actions，最终增量：4/4 actions，`Result: Succeeded`，原生退出码 `0`；
- Game：9/9 actions，`Result: Succeeded`，原生退出码 `0`；
- 均使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- 产品源边界扫描中：`demo_map`、`GetWorld(`、`UWorld`、damage API、`UGameplayStatics`、RNG、LineTrace／SweepMulti／OverlapMulti、`GetName(`、`PointerHash` 均为 `0`。

## 修改范围

- `Source/ShanmenWorldGameplay/Public/ShanmenWorldHitAdapter.h`
- `Source/ShanmenWorldGameplay/Private/ShanmenWorldHitAdapter.cpp`
- `Source/ShanmenWorldGameplay/Public/ShanmenWorldEntityRegistry.h`
- `Source/ShanmenWorldGameplay/Private/ShanmenWorldEntityRegistry.cpp`
- `Source/ShanmenWorldGameplay/Public/ShanmenDetectorEmissionSession.h`
- `Source/ShanmenWorldGameplay/Private/ShanmenDetectorEmissionSession.cpp`
- `Source/ShanmenWorldGameplay/Private/Tests/*`
- `Source/ShanmenWorldGameplay/ShanmenWorldGameplay.Build.cs`
- 本 Report 与同名 Development Log

工作区长期未跟踪文件均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P3：建立 GAS／动作阶段编排层，并以一式基础剑击形成第一个纵切。GAS 只负责 Startup／Active／Recovery 与事件编排；WorldGameplay 负责候选，CombatCore 继续独占纯函数 Impact 语义，生命与物品提交仍经过显式 commit point。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p2-1-world-identity-sessions/Docs/Report/Dev.D.UE.0.0.10.P2.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p2-1-world-identity-sessions/Docs/Log/Dev.D.UE.0.0.10.P2.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p2-1-world-identity-sessions>

`READY_FOR_0_0_10_P3`
