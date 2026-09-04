# Dev.D.UE.0.0.10.P20.6.r0 Development Log

## 1. 目标与基线

- 基线：`23f377d537da7ec85db58bfd5a84811ed64a6b19`（P20.5）；
- 分支：`agent/0.0.10-p20-6-thrown-weapon-arc-product-session`；
- 目标：让既有 ProductSession 显式接受 Straight 或 Arc hotbar geometry，并把 immutable capture 交给 P20.5 Controller；
- 约束：不修改 lifecycle/input/UI，不建立第二 Action sequence、item transaction、Router、Host、planner 或 replay authority。

## 2. 接入前审计

P20.5 已让 ProductController 接受 Arc selection/product，但 ProductSession 仍把 hotbar intent、session config、selection capture 与 product capture全部固定为 Straight。

Session 还假设“任何已捕获 Action 都必有 RunCommand”。P20.5 的确定性 `ArcPlanRejected` 有合法 Action identity，却故意不生成命令、物品事务或 Actor。若直接接 Arc 而不修正该不变量，会在 Controller 正确拒绝后被 Session 二次改写为 `SessionInvalid`。

因此本轮同时处理两个会话缝：判别式轨迹载荷，以及无命令 Arc rejection 的合法终态。

## 3. Hotbar 判别式契约

原 `TryCapture` 保持 Straight 兼容并显式写入 Straight kind。新增 `TryCaptureArc`，只冻结 selection id、one-based slot、origin、target 与 apex clearance。

`IsValid` 按 kind 检查互斥字段；`Matches` 比较 kind 和两组完整载荷。不存在从 aim direction/maximum distance 推测 Arc，也不存在 Arc target 与 Straight payload 同时有效的状态。

## 4. SessionConfig Arc policy

新增 `TryCaptureArc`，通过现有 ProductCapture 验证并冻结 action definition、source tags、technique tier、gravity 与 maximum flight time。

Straight config 必须没有有效 Arc policy；Arc config 必须有有效 policy 且能重新构造合法 Arc ProductCapture。`Matches` 现在比较 trajectory kind 与对应 policy，Run bind 的幂等判定因此不会忽略 Arc 平衡参数。

## 5. Session 路由

`TrySubmitHotbar` 先验证 active session、intent 与 trajectory/config 一致性，再进入原 hotbar/item 路径。Mismatch 在 Coordinator sequence 与 item I/O 前失败关闭。

Selection capture 和 ProductCapture 各自按 kind 分派一次，之后共同进入原 `RouteCaptured`。Session 没有调用 planner、item authority mutation、Actor spawn 或 world query；这些仍由 P20.5 Controller 及既有下游边界拥有。

首次请求继续冻结 hotbar item 与 AttackPower。精确重放只读取 Session/Controller ledger，不再次采样 stats、不再扣物品、不生成新 Actor。同 SelectionId 异 slot/kind/geometry 继续冲突。

## 6. 不变量修正

Session `IsValid` 现在按 kind 重建 expected selection geometry 与 expected ProductCapture，并要求它们与冻结 hotbar/config 完整相等。

对 `ArcPlanRejected`，Controller ledger 中必须没有 command；Session 接受该确定性无命令记录。对任何其它已捕获 Action，command 仍必须存在且 item/AttackPower 必须匹配，原严格性没有降低。

因此不可达 Arc 可以精确重放并正常 `TryEnd`，而不会产生隐藏 recovery 或 flight gate。

## 7. 自动化扩展

在原 4 项 ProductSession 测试上新增：

- `ArcResolveFreezeReplayConflict`：验证 exact item、AttackPower、Arc request policy、Action identity、sequence、authority revision、Actor identity 与 conflict；
- `ArcPlanRejectionReplayEnd`：验证 unreachable rejection 的一次身份、零 authority 变更、空 Host、无 command、精确重放、冲突与无隐藏工作结束。

`ContractAndBinding` 同时扩展 Arc intent/config、无效端点/弧高和 Straight-session/Arc-intent mismatch。原 Straight 三组行为保持通过。

## 8. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `product_session_final.log` | 6/0 | `06F898D69CF038FD16B4C1AD6C9AB69D80AC3F08CEA0BBF9C4C04BD423AB700F` |
| `product_controller_final.log` | 7/0 | `3250511EFCE187F6461D18C7BB9F2FCEA052B8B73E302D255FE5148621D7B6D9` |
| `run_command_final.log` | 6/0 | `3D9AFAB7F48BF04D09C773D225E33A2564A426449B23BFFAF1DCC5B17BF04A1A` |
| `full_0_0_10_final.log` | 856/0 | `D4EAC4F66BB37CD70352732E5A442747FAB1B3E07FF89AB1490882BA4DDA2386` |
| `legacy_item_armor_final.log` | 46/0 | `67907B163E4ED443CCA19B0009A796B90C8D9095C06743B164C4B6853A78C81B` |

证据审计：`PASS Logs=5 RecordedSuccess=921`；SHA-256 `2E3A9BD92122DC46D770260B84A1ED084E21696533A5E51F96EED5927EA03002`。

`product_session_first.log` 也为 6/0，SHA-256 `6426E3502D5BEA67B588EEFD896A697B52CDF16928A559E861DEB5F2C2868892`。首次编译和首次测试均成功；没有失败日志需要隐藏、删除或包装。

## 9. Changed-file regression 与静态边界

最终三个改动路径全部命中 `ThrownWeaponProductSession` 规则：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=5
```

- regression gate SHA-256：`ECC2C5F06810D26E9169800A51E4504AEAC5584FD21080E1C97DC4E585A4F0C6`；
- production boundary：`PASS Files=2 AddedLines=283 Matches=0`；
- boundary SHA-256：`FE2D95E8733E7512C6D4EBDC23BBAA3A26AE9461A35D45F4003CE1EED7EE95A4`；
- `git diff --cached --check`：最终 5 文件 PASS / native 0，证据 SHA-256 `43539D43FBF860429DF7896823DD8295D9BEDDFD6C8C93FC3C1BEA90CFD7D953`。

边界扫描覆盖新增 production 行中的 direct damage、direct inventory mutation、spawn、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics。

## 10. 构建与产物

- initial Editor build：31 actions / native 0，SHA-256 `3FE797F0FCFF685C5E861916FE0B7358ACE60894BDE6272591455D19A976E764`；
- final Editor build：0 actions / native 0，SHA-256 `15F65C53A1BD7F0513E7011A1671DCFB26752B8DF63A1E08C1FA7C42E1BA3B74`；
- final Game build：30 actions / native 0，SHA-256 `BC21412A206E46F5725B19941A35391FEA525A4E91CCD3E2B8542D4C6930532C`；
- `demo_map.exe`：357,160,960 bytes / `D36CB4F47660AC4345BF757E82D0E933C96CF063E522A464C51574B6B91D7603`；
- `UnrealEditor-demo_map.dll`：15,824,384 bytes / `DBBA9F58615E0A85608D935B0051B217AEE0A1B5B843BBEA8892913C3BDED44B`。

## 11. P/F 边界与后续判断

本轮仅证明 ProductSession 的 typed Arc seam。ProductLifecycle 仍只构造 Straight SessionConfig，InputAdapter 仍只采样 aim direction；因此玩家入口尚未开放 Arc。

P20.7 应扩展既有 ProductLifecycle content capture/session bind，使它显式支持 Straight 或 Arc config，但继续不接玩家手势、目标预览或 UI。输入与呈现应在后续独立阶段完成。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-6-thrown-weapon-arc-product-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-6-thrown-weapon-arc-product-session/Docs/Report/Dev.D.UE.0.0.10.P20.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-6-thrown-weapon-arc-product-session/Docs/Log/Dev.D.UE.0.0.10.P20.6.r0_log.md>
