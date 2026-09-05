# Dev.D.UE.0.0.10.P20.22.r0 Report

## 1. 结论

P20.22 已把 P20.21 的 stale-safe interaction request 接到现有统一、可重映射的产品输入体系。新增逻辑动作 `ThrownWeaponTrajectoryToggle`，默认键为 `T` 且仅响应 Press；产品处理链固定为“读取当前 P20.20 model 一次 → 捕获 P20.21 toggle request 一次 → 路由该 request 一次”。它不直接修改 choice state，不复制 revision/session，不增加自动重试，也不绕过 P20.18/P20.19/P20.21 的既有栅栏。

Straight 与 BallisticArc 现在可由同一可配置动作往返切换。已有 Version 4 输入配置会保留全部旧映射并补入新动作；若旧覆盖已经占用 `T`，迁移保留旧覆盖并为新动作选择首个空闲 registry 默认键。聚焦自动化新增 `7/0`，最终全量为 `942/0`。本轮只执行编译、合成按键分发与无头自动化；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实设备输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`552fd900a678d8ea08a58d4a153c2915565964a2`（P20.21）；
- 分支：`agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 统一输入与配置迁移

`Fdemo_mapInputActionRegistry` 从 23 个扩展到 24 个精确动作。新动作的唯一 ID、默认 `T`、中文标签与战斗分类都由 registry 持有；`Ademo_mapPlayerController` 只通过 `Settings.GetKey(...)` 绑定，产品回调中没有硬编码 `EKeys::T`。

序列化版本从 4 升为 5。既有 forward-compatible `MergeMissingDefaults` 仍是唯一迁移入口：

- Version 4 未含新动作时补入 `T`；
- 旧用户覆盖已占用 `T` 时不移动该覆盖，新动作取得首个空闲 registry 默认键；
- 最终仍必须通过 24 项完整性、数字键/鼠标键合法性与无重复键校验；
- Live remap 后重建产品绑定，旧键立即失效，新键立即生效并以 Version 5 持久化。

## 4. Toggle Request 语义

`TryCaptureTrajectoryToggle` 只接收 P20.20 的 immutable interaction read model。当前为 Straight 时请求 BallisticArc；当前为 BallisticArc 时请求 Straight；无效 model 会清空复用输出并失败关闭。

该 helper 不创建新协议或第二状态，而是委托 P20.21 的 `TryCaptureTrajectorySelection`。因此 capability 校验、canonical intent、expected read-model identity 与 request identity 全部继续由既有链负责；Arc target、apex、clear 与 launch 的含义没有被物理键擅自推断。

## 5. PlayerController 产品边界

`BindProductInputActions` 为 registry 中的新动作增加一条 Press-only `BindKey`。回调执行顺序固定为：

1. `ReadThrownWeaponInputChoiceInteraction()` 一次；
2. 只有 projected model 才捕获一个 toggle request；
3. `RouteThrownWeaponInputChoiceInteractionRequest()` 一次；
4. 记录 accepted/rejected 诊断后结束。

没有循环、定时器、异步重试、直接 session submit 或 choice-state 写入。读失败、stale model、input lock、surface/mode/Run/lifecycle 拒绝仍沿原有 typed evidence 返回。新增计数与最后一次 evidence 只存在于非 Shipping 自动化边界。

## 6. 产品行为与兼容性

合成产品 Press 证明默认 `T` 第一次把 revision 0 的 Straight 切到 revision 1 的 BallisticArc，第二次把 revision 1 的 Arc 切回 revision 2 的 Straight；Release 没有重复绑定。

把新动作实时重映射为 `C` 后，旧 `T` 不再触发，新 `C` 立即按同一链提交。settlement input lock 下，按键仍能抵达共享交互链以留下可审计 evidence，但 P20.18 下游拒绝写入，state 保持 Straight/revision 0。既有 FullSystemLoop、P7、P5 runtime、InputRestore 与 V2 ranged 组均继续通过。

## 7. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput` 新增 7 项：

- `RegistryDefault`：24 项 registry、唯一 press-only `T` 默认与标签；
- `LogicalToggleCapture`：Straight/Arc 双向 canonical request 与无效输出清理；
- `VersionFourMigration`：旧 23 项配置补入新动作；
- `MigrationConflict`：旧覆盖占用 `T` 时保留旧键并分配空闲键；
- `PressRoundTrip`：真实 PlayerController/InputComponent 合成 Press 双向切换且 Release 不重复；
- `LiveRemap`：T→C 重映射、即时重建与 Version 5 持久化；
- `InputLock`：共享下游 gameplay fence 拒绝且状态不变。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `trajectory_toggle_first.log` | `Product.ThrownWeaponTrajectoryTogglePhysicalInput` | `7/0` | `3C5A0020058E7537883FB0D95AB9050BE8695FC85C5DBEDDE6C910882161EC4A` |
| `legacy_full_system_loop_final.log` | `demo_map.FullSystemLoop` | `50/0` | `ECEBE1CFADA528E58B0061B179C2D31B097A1720D7ABF5962C766964E896F4AF` |
| `legacy_p7_integration_final.log` | `demo_map.P7Integration` | `9/0` | `90947BA71D4B8606887122DF5BEC61C49BDFD4DB61DE697397EB030B1069428F` |
| `legacy_p5_runtime_interface_final.log` | `demo_map.P5RuntimeInterface` | `9/0` | `63E879EDC9FCA867A5D8D6B0E9337FB4FABFFEE6708055EFEF47F2B2E4E57332` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `BD4341B0A4A53F66737B31F9C42B5D24E7C8E49D3E486D3620A5C850A23379B6` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `7DA8C3F886A1954166F88913893939ED80BC29FF126B66C5CEAFF6329CA9B5FF` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `942/0` | `F098303EDE639FD1C560F018074B5E4E4FB1D6BEFC4EB29029474722C9F9686C` |

日志审计：`PASS Logs=7 RecordedSuccess=1140 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`，SHA-256 `FA176CB3769928124EBF5E1F32FBB513AA1156FEC525C9A51E3FDA09E95C1A90`。

## 9. 流程、静态边界与构建

- regression self-test：`337/337`，SHA-256 `EBB4BD7B48680821B39191A9BEF1E18B4454C6A444F5E78F149A410331A7E5A1`；
- static audit：`PASS RegistryActions=24 ToggleRegistryRefs=2 ToggleDefaultT=1 ConfigVersion5=1 PlayerRegistryBindings=1 PlayerHardcodedT=0 HandlerReads=1 HandlerCaptures=1 HandlerRoutes=1 RetryLoops=0 FactoryDelegates=1 FactoryRevisionReads=0 ForbiddenProduction=0 ExactTests=7`，SHA-256 `65ECF52D5D786F87EC19D9261E0915FE49069A899B57F1DE7E64DB807CAC5E06`；
- changed-file regression gate：`PASS Changed=18 Rules=7 Required=28 Logs=7`，SHA-256 `5E0BE5385B066D8C0D4ED67697A322999D0CDA037ECD0DB34B658E2F38E2DDAA`；
- `git diff --check`：PASS / native 0，SHA-256 `0445271063D3EE886AC538B06DEF958043B258D76CDFB5CA3E9F073EBB2F60CD`；
- 首次 Editor 编译：新测试误用不存在的 `Engine/URL.h`，UBT 为 `OtherCompilationError`；失败日志已保留，SHA-256 `0A812F447ADE6AA5961C903A44D12B91BE468487F2C34642205842AA75235B8F`；
- 改为 UE 5.8 实际提供 `FURL` 的 `Engine/EngineBaseTypes.h` 后：4 actions / native 0 / 7.75 秒，SHA-256 `225D450265D46ECE8BCBC6E13847FC9792A9F9C1E161129D687F4132E4D57F96`；
- exact test 首次即为 `7/0`，且修复 include 后生产与测试源码未再修改；final exact 复用同一冻结日志；
- final Editor：0 actions / native 0 / 0.95 秒，SHA-256 `A03BCA6E17EC033E3DE37131FB8B56B9109C4F053E873EC2187B9D6324B32988`；
- final Game：35 actions / native 0 / 65.26 秒，SHA-256 `D08CBD3968FFED922B7899F70D9FC0CA0B4C1DB7C5DEC7B1E4CF5BF1768ED84A`。

产物：

- `Binaries/Win64/demo_map.exe`：357,556,736 bytes，SHA-256 `18BD8F20AB116DA207B23737089F768E8F620D345F6903D4CE2E2958B520414E`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,289,792 bytes，SHA-256 `9C0C17CD52A5E8B2104CADC707336E86E2FBC4A9ECCC752BFB9CFAEFD79BE335`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：统一可配置动作、Version 4→5 兼容补项与冲突保留、Straight/Arc canonical toggle capture、一次 read/capture/route、stale-safe 与 input-lock 继承、默认键双向切换、release 去重、live remap，以及改动文件映射出的旧输入回归。

未验证：真实键鼠/手柄/触控设备、可见模式提示、Arc target/apex UI、轨迹预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。合成 InputComponent 分发、无头自动化与 Development 构建不能描述为真实产品运行验收。

建议 P20.23 只增加最薄的只读模式反馈：从 P20.20 当前 read model 生成 Straight/Arc 显示文本并接入现有 HUD/presentation 刷新，不复制 choice state、不引入 revision 轮询，也不提前决定 Arc target/apex 的指针语义。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input/Docs/Report/Dev.D.UE.0.0.10.P20.22.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input/Docs/Log/Dev.D.UE.0.0.10.P20.22.r0_log.md>
