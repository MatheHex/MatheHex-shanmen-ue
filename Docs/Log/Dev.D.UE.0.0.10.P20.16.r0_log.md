# Dev.D.UE.0.0.10.P20.16.r0 Development Log

## 1. 目标与基线

- 基线：`62bc761f0bfb059900831c2b40a65bd0279c030e`（P20.15）；
- 分支：`agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner`；
- 目标：增加 consumer-local 显式 Arc confirmation owner，只在明确 intent 下捕获 current choice + policy 并调用 P20.15；
- 约束：不绑定物理设备、不改 hotbar 行为、不增加 UI/preview、World 产品权威、item、Run、inventory、projectile、碰撞或伤害路径。

## 2. 接入前审计

P20.11 的 GameMode-owned session 是唯一 choice 真值；P20.12/P20.13 负责 choice projection/composition；P20.14 从当前 pawn 采样 source basis；P20.15 已提供 PlayerController command route、input context gate 与 stale-choice fence。

剩余缺口是没有一个显式 confirmation owner 来区分“准备/编辑 choice”与“确认本次 launch”，也没有 event-level replay/conflict evidence。设计选择为 bounded recent-event owner，不复制 P20.15 或产品 lifecycle。

## 3. Intent 与 owner 实现

新增 immutable confirmation intent：event ID、slot、policy 生成确定性 IntentId，capture 失败清空输出。current choice 不进入 intent，而在 `Confirm` 时读取。

owner 固定执行 intent/owner/replay-conflict/local-context/choice/capture/route 顺序。新事件最多读取一次、路由一次；exact retained replay 两者均为 0；同 ID 不同 payload 拒绝。blocked 与失败结果也会 retain，避免相同事件被环境变化升级。

最近 32 个事件由 FIFO 保存，防止 controller 生命周期内无界增长；reset 回到 empty valid。该窗口不是全局永久 ledger，未来 caller 必须为每次新确认提供新 event ID。

## 4. PlayerController 接入

PlayerController 新增一个 owner 成员和 `RouteThrownWeaponArcConfirmation`。本地 gameplay/surface/mode 不允许时不读 World；允许时从 auth GameMode 读唯一 choice，然后把捕获 command 交给 `RouteThrownWeaponArcLaunchCommand`。

P20.15 仍重新核对 input context 和 current choice，因此 TOCTOU 变化失败关闭。本轮没有增加 BindKey、InputComponent、FKey、UI，也没有触碰 `UseHotbarSlot`。

## 5. 新增自动化

新增 exact 6 项：intent contract、capture gates、route/replay/conflict、input protocol、bounded retention、PlayerController boundary。

首次 Editor build：19 actions / native 0 / 48.41 秒。首次 exact：`6/0`、fatal/unhandled/ensure 0，SHA-256 `A47510C819B256DB4023EAD4D94188EE7D122B63852A1BD2B6C9380875EB31A4`。源码与断言均无返工。

## 6. 回归映射与流程自测

新增 `ThrownWeaponArcConfirmationOwner` path rule，要求 confirmation、launch、source basis、composition、projection、choice、旧 InputAdapter 与 full；`PlayerCombatController` 同步要求 confirmation exact。

流程自测增加 broad-full 正例与 unrelated-item 反例，最终 `325/325`，SHA-256 `8F03C6C132E866624F7709568A44B1F000BBAFFB24FC80C27EBF898533BD3DE3`。JSON 解析通过。

## 7. 最终自动化与门禁

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_confirmation_owner_final.log` | `6/0` | `F016AEC624CC463433351B92702FDF666FD9F3E0B3F0D05A8EAFE30C4B0BFFC2` |
| `arc_launch_input_final.log` | `6/0` | `35DBF20FFF00716014FF8485BC9C8F94A23B44693B0DD19DD34CACA81A7F8FC3` |
| `source_basis_adapter_final.log` | `6/0` | `C076AA5A870021874EACCEA9179D8B8E9C0E8C95A53E5D0BF07C5362DE83EA6A` |
| `arc_choice_composition_final.log` | `5/0` | `F23666F93BAC01C5B72D1225E2F707AB84A018D362367A07718E70EAAFC638A1` |
| `arc_choice_projection_final.log` | `5/0` | `F3578AA86019F199EE4FE2A65C6E879181CCE939F2057687AC3E233755D1B2CA` |
| `input_choice_final.log` | `9/0` | `49776AC72B682DBA331E28F3134FC9876B1450CC279CD7C4BAAC0AD4AA096439` |
| `input_adapter_final.log` | `9/0` | `C4ED2C477657C3F906AE95C159ADE75226AF70D08A8B098D9125C64B5103458E` |
| `legacy_input_restore_final.log` | `101/0` | `144DC69D56715F12FD14B6F830DE60A808527FC92955D0019598A41611B94E44` |
| `legacy_v2_ranged_final.log` | `22/0` | `5CEFAE4B91C93300F8AAF7ECC5483BA0EF3446840E64670A75F26ED9DE5011E1` |
| `full_0_0_10_final.log` | `900/0` | `86AA4946714C5CF946441CC8337D3CCBA1966339EC8C5368594D0D7EE3863770` |

日志审计为 `PASS Logs=10 RecordedSuccess=1069 RecordedFail=0 Invalid=0`，SHA-256 `271F5CC6058DE9ED49678430F093EB5886A401897D68288EEFC52510A1C456D4`。

预 Report changed-file gate：`PASS Changed=7 Rules=2 Required=17 Logs=10`，SHA-256 `4DC3E01E917AADA1EBFDB3410B232B5F35D4ADB03F1B78EAD6BBDCD3BAD808C4`。

纳入 Report/Log 后最终 gate：`PASS Changed=9 Rules=2 Required=17 Logs=10`，SHA-256 `BCDD47E7708D1FA92A903497C2E954349B694E415269FD177C854981C92ABF43`。最终 staged diff check：`PASS / 9 files / native 0`，SHA-256 `50F5B0B9A040CADE1E73434DC6E0EE6FDC9CA98C7AFA0DAFF0F2619DB5F2D629`。

## 8. 流程修正与静态证据

首次日志审计使用过宽 `*_final.log`，误收 `regression_selftest_final.log` 并得到 `Invalid=1`。失败证据 `log_audit_first.log` SHA-256 为 `6F05C4145DFD46A774D9D84A004B6CDF2E7A56CC9553B962B66249AD62A0FEA4`。改为十个明确 Unreal 日志后通过；没有重跑产品测试。

静态审计：owner forbidden matches 0；PlayerController 新增 33 行的 physical binding matches 0；RegressionMap JSON 与 325 项自测通过。`static_audit.log` SHA-256：`3D74C5D313B612693625613EEC731D45F1760934AF84FF81CE41B5434C0972E2`。

## 9. 最终构建与产物

- Editor：0 actions / native 0 / 1.27 秒 / `1A2656DED555CE54633E2055B9B01CAB7F875BAE0E5D953A6998921ACB68F43A`；
- Game：18 actions / native 0 / 50.37 秒 / `33BF8DB307CC5B644042D34B1203C88623068F6EDF587EB4AEF630A1101B1447`；
- `demo_map.exe`：357389312 bytes / `A108028D768A2BF39C83EBF49E153DF6B08AB1CB716C6BE991947782227A4061`；
- `UnrealEditor-demo_map.dll`：16083456 bytes / `2968B1C5FE2A2B254C8267638AEEB109C933CEEC95B8C6DBB083B20A179AE11C`。

只执行编译和无头自动化。full 中 `generate_204` timeout 与 large-delta 为既有环境噪声；最终队列 900/0、native 0。

## 10. P/F 边界与后续判断

本轮证明显式 Arc confirmation intent 可被 consumer-local owner 确认、近期重放和冲突拒绝，并只通过 P20.15 唯一入口继续。它不证明 physical hotbar 已接入、真实 event identity、产品 policy authority、UI/preview 或真实 product launch。

P20.17 建议复用现有 1–9 hotbar binding，增加 trajectory-aware adapter 与 controller-owned event sequence：Arc 进入 P20.16，Straight/非投掷物保持原 pass-through；policy 必须来自一个单一配置来源，不写死在按键函数。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner/Docs/Report/Dev.D.UE.0.0.10.P20.16.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner/Docs/Log/Dev.D.UE.0.0.10.P20.16.r0_log.md>
