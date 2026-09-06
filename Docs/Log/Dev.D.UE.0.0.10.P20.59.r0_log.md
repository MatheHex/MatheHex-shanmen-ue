# Dev.D.UE.0.0.10.P20.59.r0 Development Log

## 基线与目标

- base：f60ca858692c2f635766c4ec93e504170572a914；
- branch：agent/0.0.10-p20-59-thrown-weapon-arc-preview-recovery-multi-generation-cycle-proof；
- 目标：证明同一 recovery lineage 能经既有 completion/adoption authority 完成两次相邻 pending generation rotation；
- 权限边界：只组合 caller-supplied immutable evidence，不读写 authority/storage，不调用 surface，不建立自动重试或后台任务。

## 设计判断

1. P20.58 已证明一次 G→G+1 rotation，但尚未证明 G+1 能重新走完 admission/recovery/completion/adoption 并继续旋转。
2. 跨代证明必须绑定具体 bundle、journal、receipt、completion 与 adoption，不能只比较 generation 数字。
3. 中间代是两次 rotation 的唯一桥；如果 first pending 与 intermediate source、intermediate terminal 与 second source 任一不精确相等，应失败关闭。
4. pending、completion、adoption 三个既有 authority domain 已足够，proof 不应成为第四权威。
5. proof identity 必须确定性派生并在 `IsValid` 中重算，避免任意 GUID 绕过证据一致性。
6. journal 比较只允许遍历既有 canonical 16-record 上限，不允许目录扫描或无界 retry。
7. generation capacity 与 journal capacity 必须在同一端到端测试中收敛，不能各自孤立声明。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.cpp。

核心对象：immutable `Fdemo_map...MultiGenerationCycleProof`。

创建路径：validate four evidence values → validate adjacent generations → bind first pending bundle to intermediate completion source → compare first pending/source journals → validate one-record RecoveryCommitted extension → bind receipt/checkpoint → match intermediate adoption → bind second rotation adoption/source terminal journal → reject equal checkpoints → validate lineage and three authority domains → derive deterministic ProofId → self-validate。

## 测试 harness 与关键覆盖

ProductLifecycle tests 新增 test-only harness，按既有公开 API 构造并执行 P20.55–P20.58 链；生产代码没有依赖 harness。

新增 5 项：

- ExactG1ToG3Cycle；
- DeterministicIdentityAndCrossChainFence；
- ReplayAtEveryBoundary；
- HistoricalCheckpointReuseFence；
- MaximumGenerationCapacity。

容量用例从 G1 运行到 G8，形成 6 个相邻 proof；pending/completion/adoption authority 最终均处于 G8，terminal journal 为 16 records，generation 9 请求失败。rotation、completion、adoption 各边界 replay 均不增加写入；历史 checkpoint 复用在 callback 前拒绝。

## 自动化测试

正式结果：

- focused：5/0，native 0，490.429 s，278,568 bytes，SHA-256 1275DE38EF2CC5D2D5958554043E23DA0EB53D381A7B09A72473E02DAA5440D5；
- legacy ItemUseAndArmor：46/0，native 0，1.463 s，310,339 bytes，SHA-256 E0A400F64AECFF1D985BD46C2AC7A52CFBA318390C507A9C4745A8DAA1C91F2B；
- full Shanmen.0_0_10：1198/0，native 0，4316.444 s，1,841,077 bytes，SHA-256 A837BD101329154448C3CD83D7B3ADDD72808B20A5373AB8064A652B3CA2F043；
- formal total：1249/0；
- terminal-success markers：三份正式日志均为原生 exit 0；
- automation test failure / Fatal / Unhandled / Ensure：0 / 0 / 0 / 0；
- first focused result：5/0，因此没有 first-failure artifact。

## Changed-file regression 与静态边界

- mapping JSON：230 rules / parse PASS；
- 新增 exact P20.59 rule，RequiredGroups=44；
- P20.59 focus 回接 next rotation、terminal adoption、completion、admission、watermark authority、bundle storage、bundle、payload storage、payload envelope、journal 与 recovery 11 条上游规则；
- mapping self-test：411/411；
- focused-only gate：预期失败，43 required groups missing；
- final gate：PASS，Changed=7 / Rules=2 / Required=44 / Logs=3；
- production：2 files / 383 physical lines；
- loops：2 bounded journal comparisons / `while` 0；
- callback、authority/storage operation、direct CAS：0；
- World/Actor/UObject/damage/RNG/global path/sleep/timer/async：0；
- 临时占位项：0；
- git diff --check：PASS。

## 构建与产物

- test integration Editor：4 actions / native 0 / 16.62 s；
- final Editor：Succeeded / native 0 / 0 actions / 1.88 s / up to date；
- final Game：Succeeded / native 0 / 4 actions / 33.13 s；
- Editor DLL：18,243,584 bytes / SHA-256 F2C89F74808841F304F1F21DFCAE9A0DF20D83566F733B6366A4E23A2FFE7DC3；
- Game EXE：359,137,280 bytes / SHA-256 FE76D11393417F67F76633CADCDB6524A3AEE1A2A7E65EDCBDBB39E17FDBDF6C。

## P/F 边界

PASS：G→G+1→G+2 exact proof、deterministic identity、single lineage/authority chain、journal prefix-extension receipt bridge、cross-chain fence、replay、historical checkpoint fence、G1→G8 capacity closure、mapping/self-test/gate、legacy/full regression、Editor/Game builds。

未声明：真实 protected backend/fsync/process lock/remote consensus、长期 archive/retention、自动发现/恢复/清理、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.60：停止继续扩展 recovery proof 栈，转回真实产品集成边界。建立 preview presentation command 到 MainHUD renderer adapter 的显式契约、caller-owned lifecycle 与 typed acknowledgement；先以 headless contract/adapter tests 验证，真实 UI 运行仍等待用户单独授权。
