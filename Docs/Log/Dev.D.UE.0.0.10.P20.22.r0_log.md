# Dev.D.UE.0.0.10.P20.22.r0 Log

## 阶段

- 任务：P20.22 configurable thrown-weapon trajectory-toggle physical input；
- 基线：`552fd900a678d8ea08a58d4a153c2915565964a2`；
- 分支：`agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 统一 input registry 新增 `ThrownWeaponTrajectoryToggle`，默认 `T`、Press-only；精确动作数 23→24。
2. 输入配置序列化版本 4→5；旧配置缺项沿既有迁移补齐，已占用默认键时保留旧覆盖并选择空闲 registry 默认键。
3. P20.21 request 新增无状态 toggle capture：Straight→BallisticArc、BallisticArc→Straight；无效 read model 清空输出并失败关闭。
4. toggle capture 委托既有 trajectory-selection factory，不复制 state/revision/session，也不新建 intent/command 通道。
5. PlayerController 通过 `Settings.GetKey` 增加一条可重映射 Press binding；产品回调没有 `EKeys::T`。
6. 回调固定为 P20.20 current read 1 → P20.21 capture 1 → P20.21 route 1，无循环、定时器或自动重试。
7. 新增非 Shipping evidence getter，验证实际 InputComponent 分发、revision 变化和 typed 下游拒绝。
8. 新增 7 项 exact 测试，覆盖 registry、逻辑切换、Version 4 迁移、冲突迁移、双向 Press、Release 去重、live remap 与 input lock。
9. 更新六组既有 registry 精确数量断言；更新 changed-file regression map 和正/反 self-test，总数 `337/337`。

## 首次证据与修正记录

- 首次 Editor 编译到新测试文件时失败：`Engine/URL.h` 在 UE 5.8 不存在，UBT `OtherCompilationError` / 60.17 秒；失败日志已从 UBT 自动备份恢复，SHA-256 `0A812F447ADE6AA5961C903A44D12B91BE468487F2C34642205842AA75235B8F`。
- 将测试 include 修正为提供 `FURL` 的 `Engine/EngineBaseTypes.h`，并显式包含 `Engine/GameInstance.h`；未修改产品契约。
- 修正后 Editor：4 actions / native 0 / 7.75 秒，SHA-256 `225D450265D46ECE8BCBC6E13847FC9792A9F9C1E161129D687F4132E4D57F96`。
- `trajectory_toggle_first.log` 首次为 `7/0`，源码此后未再修改，因此 final exact 复用同一冻结日志。
- 一次以旧 Windows PowerShell 5 启动 regression self-test，旧解析器不支持脚本已有的行首管道语法；该运行未执行测试器。改用项目随附 PowerShell 7 后为 `337/337`，错误宿主日志单独保留且不计为产品失败。
- 首版 static-audit 计数表达式把另一个 `FName` 数组 initializer 误算成 registry action，得到 25；限定为逐行动作 initializer 后为 24。该修正只改变证据查询，不改变源码。

## 最终聚焦与回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `trajectory_toggle_first.log` | `7/0` | `3C5A0020058E7537883FB0D95AB9050BE8695FC85C5DBEDDE6C910882161EC4A` |
| `legacy_full_system_loop_final.log` | `50/0` | `ECEBE1CFADA528E58B0061B179C2D31B097A1720D7ABF5962C766964E896F4AF` |
| `legacy_p7_integration_final.log` | `9/0` | `90947BA71D4B8606887122DF5BEC61C49BDFD4DB61DE697397EB030B1069428F` |
| `legacy_p5_runtime_interface_final.log` | `9/0` | `63E879EDC9FCA867A5D8D6B0E9337FB4FABFFEE6708055EFEF47F2B2E4E57332` |
| `legacy_input_restore_final.log` | `101/0` | `BD4341B0A4A53F66737B31F9C42B5D24E7C8E49D3E486D3620A5C850A23379B6` |
| `legacy_v2_ranged_final.log` | `22/0` | `7DA8C3F886A1954166F88913893939ED80BC29FF126B66C5CEAFF6329CA9B5FF` |
| `full_0_0_10_final.log` | `942/0` | `F098303EDE639FD1C560F018074B5E4E4FB1D6BEFC4EB29029474722C9F9686C` |

日志审计：`PASS Logs=7 RecordedSuccess=1140 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`，SHA-256 `FA176CB3769928124EBF5E1F32FBB513AA1156FEC525C9A51E3FDA09E95C1A90`。

全量中的既有 retry/checkpoint/journal 纯值测试在 650–700 区间进入长计算窗口；日志、CPU 和进程响应持续推进，未重启、未并行重复，也未把静默计算误判为失败。

## 流程与静态证据

- regression self-test：`337/337`，SHA-256 `EBB4BD7B48680821B39191A9BEF1E18B4454C6A444F5E78F149A410331A7E5A1`；
- static audit：`PASS RegistryActions=24 ToggleRegistryRefs=2 ToggleDefaultT=1 ConfigVersion5=1 PlayerRegistryBindings=1 PlayerHardcodedT=0 HandlerReads=1 HandlerCaptures=1 HandlerRoutes=1 RetryLoops=0 FactoryDelegates=1 FactoryRevisionReads=0 ForbiddenProduction=0 ExactTests=7`，SHA-256 `65ECF52D5D786F87EC19D9261E0915FE49069A899B57F1DE7E64DB807CAC5E06`；
- changed-file gate：`PASS Changed=18 Rules=7 Required=28 Logs=7`，SHA-256 `5E0BE5385B066D8C0D4ED67697A322999D0CDA037ECD0DB34B658E2F38E2DDAA`；
- `git diff --check`：PASS / native 0，SHA-256 `0445271063D3EE886AC538B06DEF958043B258D76CDFB5CA3E9F073EBB2F60CD`。

## 最终构建

- Editor：0 actions / native 0 / 0.95 秒，SHA-256 `A03BCA6E17EC033E3DE37131FB8B56B9109C4F053E873EC2187B9D6324B32988`；
- Game：35 actions / native 0 / 65.26 秒，SHA-256 `D08CBD3968FFED922B7899F70D9FC0CA0B4C1DB7C5DEC7B1E4CF5BF1768ED84A`；
- Game artifact：357,556,736 bytes，SHA-256 `18BD8F20AB116DA207B23737089F768E8F620D345F6903D4CE2E2958B520414E`；
- Editor artifact：16,289,792 bytes，SHA-256 `9C0C17CD52A5E8B2104CADC707336E86E2FBC4A9ECCC752BFB9CFAEFD79BE335`。

## P/F

PASS：统一可配置 trajectory toggle、Version 4→5 安全补项、冲突保留、Straight/Arc canonical request、一次 read/capture/route、stale/input-lock 继承、合成 Press 双向切换、Release 去重与 live remap。

未验证：真实设备输入、可见模式提示、Arc target/apex UI、轨迹预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。

## 下一步

P20.23：由 P20.20 当前 read model 提供最薄的只读 Straight/Arc 模式反馈并接入既有 HUD/presentation 刷新；不复制 choice state，不轮询 revision，不提前决定 Arc target/apex 指针语义。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input/Docs/Report/Dev.D.UE.0.0.10.P20.22.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-22-thrown-weapon-choice-trajectory-toggle-input/Docs/Log/Dev.D.UE.0.0.10.P20.22.r0_log.md>
