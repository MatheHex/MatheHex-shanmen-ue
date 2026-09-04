# Dev.D.UE.0.0.10.P20.15.r0 Development Log

## 1. 目标与基线

- 基线：`5815956b234edbfaa398dca03e73be0fcc700a6a`（P20.14）；
- 分支：`agent/0.0.10-p20-15-thrown-weapon-arc-launch-input`；
- 目标：在既有 PlayerController 输入边界增加纯值 Arc launch command，先固定 gameplay/surface/mode 与 stale-choice 栅栏，再委托唯一 P20.14 route；
- 约束：不做物理设备绑定、UI/preview、world trace、库存、projectile、命中或伤害开发。

## 2. 接入前审计

P20.8 已拥有 hotbar item、active Run、source、Action gate、selection ordinal 与 world delivery；P20.11 保存唯一 consumer-owned choice；P20.12 投射 choice+basis+policy；P20.13 负责惰性组合；P20.14 从 caller-supplied source Actor 采样一次 basis。

剩余缺口是 PlayerController 尚无一个与设备无关的命令入口来冻结 choice/policy、检查自身输入上下文，并只在仍匹配当前 choice 时进入 P20.14。本轮只填这一层，不移动任何产品权威。

## 3. Command 实现

新增不可变 `Fdemo_mapShanmenThrownWeaponArcLaunchCommand`。capture 要求 hotbar slot 1–9、BallisticArc、target intent 与有效 policy；identity 由 slot、choice StateId/revision 与 PolicyId 在专属命名空间中确定性派生。`IsValid()` 重算 identity，失败 capture 清空输出。

命令只含纯值，不保存 key、controller、pawn、GameMode、World 或 callback。相同字段重放同一 identity，任一 slot/choice/policy 差异都会改变 identity。

## 4. Adapter 栅栏与证据

新增无状态 launch adapter 与只读结果。固定顺序为 command、gameplay、Gameplay surface、GameOnly mode、一次 product route resolve、一次 current choice read、frozen/current match、一次 P20.14 delegate。

每个提前结束状态都规定严格的 0/1 次数与允许 evidence。local gate 不触发 World/GameMode；route unavailable 不读 choice；invalid/stale choice 不调用产品；无效下游结果记录一次调用并返回 protocol rejection。`IsAccepted()` 只透传有效 P20.14 acceptance。

## 5. PlayerController 接入

新增 `RouteThrownWeaponArcLaunchCommand`，把现有 `IsGameplayInputAllowed()`、`InputSurfaceState` 和 `InputModeState` 交给 adapter。只有 gate 通过才解析 auth GameMode；随后读取 `GetThrownWeaponInputChoiceState()`，并调用 `RouteThrownWeaponArcChoiceFromSourceHotbarInput(slot, GetPawn(), policy)`。

旧 hotbar、Arc、source basis、choice composition 与 direct product routes 均未删除或改写。本轮没有注册按键、触碰 InputComponent 或增加 UI/preview。

## 6. 新增自动化与首次验证

新增 exact 6 项：command contract、local gate order、context fences、delegate/replay、product protocol、PlayerController boundary。

首次 Editor build：18 actions / native 0 / 47.91 秒。首次 exact：`6/0`、fatal/unhandled/ensure 0，SHA-256 `05570928A69B0BA9D691FF3BE4C2115A542887E867F1E9D9E62BAF60FA4807B0`。没有源码或断言修复。

## 7. 映射与流程验证

新增 ArcLaunchInputAdapter changed-file rule；PlayerCombatController rule 增加本轮 exact。新 rule 要求 exact + source basis + composition + projection + choice + InputAdapter + 0.0.10 full。

流程自测新增 broad full evidence 正例与 unrelated item evidence 反例，最终 `323/323`，SHA-256 `27B4C31C4B4F5DECC5AA083963F158C33DD9D55F9FF95C4BB3E9CD9B8FE0D33B`。映射 JSON 可解析。

## 8. 最终自动化与 changed-file gate

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_launch_input_final.log` | `6/0` | `3F2B331901CCB0DE26A48364F5836A7FAB6C9A35D8D5A3F4B8B8E515CBE3AF1D` |
| `source_basis_adapter_final.log` | `6/0` | `FEF0412C392CB7F24E5F87D1037D48C3A29B8700F48B4EB621F71B91A77B4F04` |
| `arc_choice_composition_final.log` | `5/0` | `D197D8CCDC614FA65FB6CEE97762FD8F839B962301099E0E219D75BD0DAD0455` |
| `arc_choice_projection_final.log` | `5/0` | `84899D2A8F7B987ABCD2F84689A5716F8D4C6F195CD9E67FDFBDBA49C464B20F` |
| `input_choice_final.log` | `9/0` | `77D986316AC631FA950586CA98950DA9B5DF6E36879BF5947ABC70E44D0BEBA3` |
| `input_adapter_final.log` | `9/0` | `B3A431467A2E18261ACA69E53977658929AFC5FFB27CEC3131AF417EBAC9CAAB` |
| `legacy_input_restore_final.log` | `101/0` | `C98C295D0288D1853BD214EB2C4F384E49E365E705EFDC17C3654F4E3EEA6791` |
| `legacy_v2_ranged_final.log` | `22/0` | `3AF46393C59E0276E122296B65A37C318A6CCB2D4E320D6620F314EBA6DF0091` |
| `full_0_0_10_final.log` | `894/0` | `F2E0CEFB04F3BE9A6E4F601EA7282A092A9E009D5FF00DD5977C1E1DE5897CCD` |

日志审计：合计 `1057/0`、no-match 0、fatal/unhandled/ensure 0；SHA-256 `00827E5187EE9F72B14A621B6664FEDCC3ED52F4FA6B34AC649565D4C3D2611B`。

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=16 Logs=9
```

- gate SHA-256：`01B1B781979EA823508EC8CF95EDBE8A9E820B75F0CA9606F710A7D55DAAB763`；
- process selftest：`323/323` / `27B4C31C4B4F5DECC5AA083963F158C33DD9D55F9FF95C4BB3E9CD9B8FE0D33B`；
- production/static boundary：`PASS` / `5B56FE799EC83EF358C788711DB7C9364F0EE99074F296CFB7121F1910C73EAF`；
- `git diff --cached --check`：精确暂存 9 个文件，`PASS / native 0` / `C0297E0DB69BC4079A82B13C8735416999C4518B83F1780973319E551A1F631B`；
- 长期未跟踪文件不纳入暂存、提交或推送。

## 9. 最终构建与产物

- final Editor：`0 actions / native 0 / 1.59 秒` / `22F603357209EAD4F51A72D13A71DC0A306FD5086504C9B3BDE9AFE05A06E8AD`；
- final Game：`0 actions / native 0 / 1.08 秒` / `6C565A2D73DE1FBC3EB257A1595518E1B0769DEB1B108CD265D6F14C6A7D95B3`；
- `demo_map.exe`：`357347840` bytes / `95811883E66560B34EB98F38E1EDAA05509974C5893F4CFCEE63619839F1B4E0`；
- `UnrealEditor-demo_map.dll`：`16039424` bytes / `EA98E979DC1A3FA674C77682F8D02795BAEC185156E82D4819AE3DD3F14AF7D9`。

只执行编译与无头自动化，没有启动产品。完整回归出现既有 `generate_204` 网络探测超时与 large-delta 警告，但进程保持响应；最终测试 terminal、fail count 与原生退出码单独审计。

## 10. P/F 边界与后续判断

本轮证明纯值 Arc launch command 可在 PlayerController 边界按固定输入栅栏、唯一 choice 复核和一次下游委派协议进入 P20.14。它不证明物理输入、真实焦点事件、UI/preview、真实产品成功 launch、trace、碰撞、命中、库存或伤害。

P20.16 建议增加 consumer-owned Arc confirmation intent/command owner：只负责在显式确认意图下捕获 current choice+policy 并调用本轮入口，继续保持设备与 UI 无关。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-15-thrown-weapon-arc-launch-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-15-thrown-weapon-arc-launch-input/Docs/Report/Dev.D.UE.0.0.10.P20.15.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-15-thrown-weapon-arc-launch-input/Docs/Log/Dev.D.UE.0.0.10.P20.15.r0_log.md>
