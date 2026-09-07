# Dev.D.UE.0.0.10.P21.1.r0 Development Log

## 1. 基线与目标

- base：`1bdd6b0ca1017c0680578da8ca1a021f6c620b76`（P21.0 canonical 练习飞剑内容）；
- branch：`agent/0.0.10-p21-1-canonical-flying-sword-active-run`；
- 目标：让同一 canonical 练习飞剑实例从 durable item authority 贯通到当前 Combat Run 的 P6 Host；
- 边界：不生成 Actor，不增加输入、AI、表现或伤害路径，不启动产品。

## 2. 起始审计

P21.0 已让 `Prototype.Item.Weapon.TrainingFlyingSword` 通过 Profile、Code B 与 Shanmen migration 获得 Deploy/FlyingSword 权威语义；P6 也已有 active-Run adapter、Session、physical controller、Host 与 teardown。

剩余缺口位于组合层：GameMode 只暴露接收 caller-prepared 结果的低层 attach seam，没有一条产品入口同时约束 durable active Run、transient Runtime、canonical player identity 与 Host。各段分别通过仍不能证明同一 item instance 真正穿透整条链。

## 3. 实现

新增：

- `demo_mapShanmenControlledWeaponActiveRunRoute.h/.cpp`；
- `Fdemo_mapShanmenControlledWeaponActiveRunIntent`；
- 完整 preparation + attachment result 与明确错误枚举；
- `Ademo_mapGameMode::StartControlledWeaponForActiveCombatRun` facade。

route 的固定顺序：

1. 验证 authority、Runtime、intent 与 Actor/collision binding；
2. 要求 CombatRunCoordinator ready；
3. 由 Coordinator registry 解析 Source Actor，且必须等于 canonical player；
4. 内部构造 P6 prepare request，不接受 caller-supplied EntityId；
5. 调用 `PrepareActiveRun` 读取 durable Run/item evidence；
6. 准备成功后调用 `RunHost.TryAttach`；
7. 对 preparation 与 attachment 的 item/activation identity 做最终交叉验证。

该入口不调用 Spawn、World、Tick、RNG、ApplyDamage、Reserve、Commit 或 StartPreparedRun。

## 4. 贯通测试

`FPreparationAdapterFixture` 增加一把 exact canonical TrainingFlyingSword。新增：

```text
Shanmen.0_0_10.Product.ControlledWeaponActiveRunRoute.CanonicalCutoverStart
```

用例执行 Profile save → Session/Code B open → Shanmen cutover → exact equipment selection → durable `StartPreparedRun` → Runtime materialization → same-Run Coordinator → route → Host。

核心断言：

- correlation weapon identity 等于 seed flying-sword GUID；
- preparation definition 等于 canonical DefinitionId；
- action RunId 与 durable ActiveRunId 一致；
- action source identity 由 Coordinator 派生；
- Player、Deploy、FlyingSword tags 完整；
- Host 仅有一个 Orbiting exact item；
- authority snapshot 在启动与重放后均不变；
- 重放返回 AttachmentRejected / ItemAlreadyBound；
- teardown 后 Host empty 且 Coordinator inactive。

## 5. 编译与自动化

首次 Editor build：33 actions / 162.36s / Succeeded，一次通过，没有源码修复轮。

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.1.r0_focused_first.log` | Active-Run route | 1/0 | 262,877 | `6C14BAB6A36F531A433FD35FB08956D9B6A2B475D7001E2BA6EC761B90DBFB61` |
| `P21.1.r0_full.log` | `Shanmen.0_0_10` | 1224/0 | 1,876,254 | `86522678F4A7A10CEE2D771F519841FA9B901242CC291985EB14F39DE95A3C1B` |
| `P21.1.r0_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 265,119 | `4FCC02CCA10E33B5BB914662914D87036CE3E00F36D6B06D7F65B004DDEC88F4` |
| `P21.1.r0_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | 304,961 | `4728AFC6084F703D0EFA8A44CC37638B5597D28BD3DEB873BAF5C034F9986C1F` |
| `P21.1.r0_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,964 | `52CEEAF46B05550C0CC24CF4CAB303F934B0E7A4AEF202EEB21A33FC93A8D4CA` |
| `P21.1.r0_item_use.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,625 | `75A7C72B2475D5F12852C6E857AFC50070B865689A3ED91E08B19452F6DB3714` |
| `P21.1.r0_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 267,226 | `E5E9B72C05FB7D659B24789F9B1CDBE303DE43C64DFFAED625AF5A4EEFD191E4` |

七份日志合计 1348/0（有意重叠）。full suite 为 1224/0，相对 P21.0 精确增加 1；单一 UnrealEditor-Cmd 从首项到 queue empty 用时约 71m53.310s，无 Fatal、Unhandled 或 Ensure。

## 6. Regression map 与首败保留

新增生产文件第一次进入 changed-file gate 前没有映射。没有把它列为 ignored，也没有降低 unknown-path 检查；新增 `ControlledWeaponActiveRunRoute` rule，声明其全部直接 authority seam。

Self-test 同步增加：

- full evidence 应覆盖 route；
- coordinator-only evidence 必须被拒绝。

最终 self-test：425/425，42,116 bytes，SHA `0750F41556C15744947C71B7C4917B43779EDD581A13F7C1EFBBF5ADB5CC0D9D`。

首轮 self-test 曾由 Windows PowerShell 5.1 错误解析 PowerShell 7 管线语法并退出 1；原始日志保留为 `P21.1.r0_regression_selftest_first_failure.log`，3,499 bytes，SHA `CD715414D92DF2CDE105820340DEC4592560E8AE686CFEF62ACBD8BFA521C1EA`。修复仅改为用 `pwsh` 调用，没有修改 validator 逻辑。

最终 gate：`PASS Changed=7 Rules=3 Required=67 Logs=6`；7,772 bytes，SHA `A3CADE29395823822FBD9887128D9EEDDB30F4A6794C25826297E08A66102BC5`。

## 7. 静态与最终构建

静态日志：route forbidden-call hits 0、`git diff --check` exit 0、5 tracked changed + 2 owned new、103 unrelated untracked、residual process 0；863 bytes，SHA `6A5C77CB7D8CC22B4121FE2BC851765DA49B735C4BDAED65B31AAE4466490CC9`。

- Editor initial：33 actions / 162.36s / SHA `503E81737FF042AED02618CB650A3F8B6CB9DFE4937B98B980CE9311C50B5C55`；
- Game final：32 actions / 144.00s / native 0 / SHA `66AABB47938DDDBDE0ADCC7C7A87243AA7F87502F4885C2CF5C9BB8930FF786F`；
- Editor final：0 actions / 1.02s / native 0 / SHA `125A9190F6CBC443585403486304E02810F108DA11977A6E54AE7E19BED04389`；
- `demo_map.exe`：359,314,432 bytes / SHA `6EDAA4DD3E374CD97A6FA96A2E193525E4AFA0F0D2618EAE8B18D937D3ED88BD`；
- `UnrealEditor-demo_map.dll`：18,456,064 bytes / SHA `8EF87CE563C80655E54393A3E04568DD4E2DB63EEB999582680B27E9C8B87EE1`。

## 8. 提交边界

计划提交 7 个实现、测试和流程文件、本 Report 与本 Development Log，共 9 个文件。103 份用户原有 untracked 文档保持未暂存；`Saved/Codex/P21.1` raw logs 不入 Git。

未修改 Content、地图、资源、配置、Engine、Windows、save schema、P6 runtime 数学或既有库存写路径。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-1-canonical-flying-sword-active-run>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-1-canonical-flying-sword-active-run/Docs/Report/Dev.D.UE.0.0.10.P21.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-1-canonical-flying-sword-active-run/Docs/Log/Dev.D.UE.0.0.10.P21.1.r0_log.md>
