# Dev.D.UE.0.0.10.P2.1.r0 Development Log

## 目标

在 P2.0 world-hit adapters 上补齐稳定实体绑定和 detector emission ordinal 权威，使 P3 不需要从 Actor 指针、对象名、UE callback order 或 hit-array order 猜测 Impact 身份。

## 契约复核

P2.0 已统一 Sweep／Overlap／Projectile 到 `FShanmenHitCandidate`，但 resolver 不知道候选所属 Run，且 HitOrdinal 仍由 adapter caller 直接传入。若立即接 P3，错误的 live Registry 或 callback-level ordinal 可能让 foreign Run 对象进入 Impact，或让多目标 query 的数组顺序改变身份。

本轮冻结两条原则：

1. EntityId 只能由 Run／Spawner 的稳定输入派生并注入；Registry 不创造内容身份。
2. HitOrdinal 属于一次 detector emission，不属于单个 callback。一次 query/tick/contact event 内所有目标使用同一 ordinal，TargetEntityId 已足够区分 Impact。

## 实现过程

### 1. EntityId factory

`FShanmenWorldEntityIdFactory` 使用 `Shanmen.World.Entity.r1` namespace 和长度前缀 deterministic ID 工具处理 `RunId / SpawnSourceId / SpawnOrdinal`。不接受 invalid Run、None source 或负 ordinal。

### 2. Entity Registry

`FShanmenWorldEntityRegistry` 实现 P2.0 resolver：

- live registry 只有一个 ActiveRunId；
- bind 只接受 exact expected Run；
- 每个 `FObjectKey` 只允许一个 EntityId；
- generic body 与 exact BodyIndex 可为同一对象增加同值 alias；
- bind/unbind/rebind 有显式结果枚举；
- actor/component 双路径解析若得到不同 ID 则拒绝；
- end/reset 清空映射和 Run。

P2.0 resolver API 增加 `ExpectedRunId`，adapter 从 context 的 frozen Action 自动传入。原 6 项 adapter 测试同步记录并断言 resolver 收到正确 Run。

### 3. Detector emission session

`FShanmenDetectorEmissionSession` 冻结 Action、DetectorId 与 DetectorKind。状态机：

- idle -> BeginEmission；
- active -> candidate acceptance；
- active -> EndEmission -> next ordinal；
- nested begin、double end、emission 外 candidate、字段不一致和 target duplicate 均失败。

AcceptedTargetIds 只在当前 emission 内存在。下一 emission ordinal 增加后，同一 target 可以再次接受；这满足 persistent zone tick 与多段动作，同时避免一个 sweep 因多个 physics components 重复命中同一 entity。

## 自动化过程

### 首次实际定向执行

WorldGameplay 找到 10 项：`9 Success / 1 Fail`。唯一失败 `EntityRegistry` 在测试第 82 行执行 `NewObject<UObject>()`；UE 5.8 报告 UObject class abstract 并产生 handled ensure。其它新增测试，包括真实 `UBoxComponent -> Registry -> P2.0 Adapter -> Session` 集成均通过。

修正仅限 fixture：改用合法 transient `UBoxComponent`。没有为测试修改 Registry 规则，也没有屏蔽 ensure。

### 最终定向

- filter：`Shanmen.0_0_10.WorldGameplay`；
- found：10；
- result：10/10 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志中 fixture handled ensure：0。

### 最终全量回归

- filter：`Shanmen.0_0_10`；
- found：76；
- result：76/76 Success、0 Fail；
- queue empty；
- 原生退出码：0。

### 日志 SHA-256

- 定向：`4EEDAE5908B4629A6532F0DCFDD07E7BAE2A0DBB7328CA2469196A336F12B09A`；
- 全量：`D918267A38A98E38A6144F36218051E8713C7EABACB38D5E93254ADD98A39263`。

两次最终日志各保留 UE 5.8 测试发现前既有 13 条 `Condition failed` 启动诊断；没有目标 test fail 或新增 handled ensure。UnrealEditor-Cmd 仍先报告 LinuxArm64／VisionOS 非目标 SDK metadata 缺失，Win64 SDK 为 VALID。

## 构建过程

统一命令参数：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`。

- Editor 首次：UHT、新 source、测试与模块链接 11/11，成功，原生退出码 0；
- fixture 修正后 Editor 增量：4/4，成功，原生退出码 0；
- 增加 unbind/rebind assertions 后 Editor 增量：4/4，成功，原生退出码 0；
- Game 最终：9/9，成功，原生退出码 0。

## 最终不变量

1. EntityId 不由 UE object pointer/name/random 派生。
2. Registry 不接受 foreign Run，也不静默切换 Run。
3. 一 object 不能同时声明两个 EntityId；Actor/Component alias 冲突失败关闭。
4. Adapter 必须把 frozen Action RunId 传给 resolver。
5. 一个 emission 内 Target callback order 不影响 HitOrdinal。
6. 一个 target 每 emission 最多接受一次；下一 emission 才能再次命中。
7. Session 不生成 Damage、ImpactResult 或资源提交。
8. 旧 `demo_map` 产品路径保持不变。

## 静态边界

`git diff --check` 退出码 0。排除 Tests 后，WorldGameplay 产品源码中以下匹配均为 0：

- `demo_map`；
- `GetWorld(`、`UWorld`；
- `ApplyDamage`、`TakeDamage`、`UGameplayStatics`；
- RNG；
- 直接 LineTrace／SweepMulti／OverlapMulti；
- `GetName(`、`PointerHash`。

## P/F 边界

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；未触碰 unrelated 未跟踪文件。
