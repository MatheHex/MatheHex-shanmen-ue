# Dev.D.UE.0.0.10.P20.25.r0 Log

## 阶段

- 任务：P20.25 thrown-weapon Ballistic Arc editing interaction composition；
- 基线：`6f3ed11fc4def20422d36dee5e071c21ee54de4a`；
- 分支：`agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增无状态 `Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition`。
2. 提供 target、apex adjustment、target clear 三个入口；只接受一份现有 P20.20 read result。
3. 每个入口先清空复用 request，要求 `Read.IsProjected()`，随后只委托一次对应 P20.21 capture。
4. 不新增 operation/intent/request/status/identity/revision/state/callback 协议。
5. 不读取 ID 或 revision，不直接调用 coordinator Execute、route、submit、reduce、TryEmit 或 command capture。
6. P20.19 继续负责 payload 规范化；P20.20 继续负责 capability；P20.21 继续负责 request identity 与 stale fence。
7. 没有修改 PlayerController、HUD、input registry 或任何物理设备映射。
8. 新增 7 项纯值自动化；完整 0.0.10 总数 `955→962`。
9. 新增 changed-file 规则，要求新组、P20.21、P20.20、P20.19 与完整 0.0.10。
10. 映射正/反 self-test 总数 `343→345`。

## 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_editing_interaction_composition_final.log` | `7/0` | `B15A8925AF2C1E56E0837733BEC243718785A19B099C6D10556056080613ADE8` |
| `interaction_request_coordinator_final.log` | `7/0` | `79F9761A443ACC4373A63536395B146205769C69D6668F6B4E7EFCE40F0654EF` |
| `interaction_port_final.log` | `7/0` | `3F724E8B7F3B7AEF8FF62C637CBA92074E505ECCE75DBE0135D0C0428DD56FB2` |
| `intent_adapter_final.log` | `7/0` | `095902D76C545E6C921CBCCC74D5406574C4C75A0D96C47E1355A55475C2E031` |
| `full_0_0_10_final.log` | `962/0` | `AD9185ED9AC0EAD31989A402FF0C8AA103C3852FE4B2CD369EE2558EE7AD9A1B` |

聚焦与回归合计 `990/0`。日志审计：`PASS Logs=5 RecordedSuccess=990 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`；SHA-256 `DDFD0AF2AE05D63E50D3C0957AD8E98545196B74E76706154AA485724AE3E2BF`。

完整 0.0.10 单进程在既有 retry/checkpoint/journal/codec 纯值测试区间进行长时间 CPU 计算；通过成功计数、末项、CPU 与进程响应持续核验，未重启或并行重复。末项为 `WorldGameplay.SweepAdapter`。

## 流程与静态证据

- regression self-test：`345/345`；SHA-256 `2EC1A0BFF1820CB7990E0E0E92E1C33FA26D6099361D598AAC79AA4FBDFDB0B5`；
- static audit：`PASS ExactTests=7 ForbiddenRuntime=0 PhysicalDevices=0 ExtraIdentity=0 RouteOrMutation=0 ComposeMethods=3 ReadGuards=3 Delegations=3 OutputClears=3`；SHA-256 `5B57076F468E13226E0F0B11263B142842F199E681E5F352BC12A0BEAFAE9274`；
- changed-file gate：`PASS Changed=3 Rules=1 Required=5 Logs=5`；SHA-256 `0EB2DB443B1C907C8A239CA478005B0587549D30D7FD436AA4C96BFB0980C359`；
- `git diff --cached --check`：PASS / native 0 / 7 个精确暂存文件；SHA-256 `FF49A1FBB409F33D781BCE03C4A012D3E11D1F9F871102F9A0E0BBF303E8EC81`。

## 构建与产物

- 首次 Editor 编译：5 actions / native 0 / 15.72 秒；该次终端输出未单独归档；
- final Editor：0 actions / native 0 / 1.07 秒；SHA-256 `3BDA91ED8541F64352B56CAD4557D7945316934D228D3354F2DFBB1BDC75E3CF`；
- final Game：4 actions / native 0 / 23.19 秒；SHA-256 `4C14B09966A921F84F594A88F5DBDB83CF9AE8C00743CC577542081440BA406A`；
- Game artifact：357,615,104 bytes，SHA-256 `5CD9023BAF11E51ED39AE9C2BA978954D96BA1AAB7EB98FEF9864604F49CE2FF`；
- Editor artifact：16,363,008 bytes，SHA-256 `EB26A6BB956DCF81CAE0F4BDA69D69BAF513C64AE3DCC97B5FEF862CB35D9D82`。

## P/F

PASS：同一次 P20.20 read 到 target/apex/clear 三种既有 P20.21 stale-safe request 的设备无关合成、失败输出清理、revision-neutral request 重放和改动路径完整回归。

未验证：真实 UI/设备输入、request route、choice 写入、轨迹预览、真实投掷、World 碰撞/命中、库存、伤害或产品启动。

## 下一步

P20.26：在现有 PlayerController 上增加三个设备无关 Arc 编辑调用入口，每次固定执行一次 P20.20 read、一次 P20.25 compose 和一次 P20.21 route；具体物理设备映射继续后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition/Docs/Report/Dev.D.UE.0.0.10.P20.25.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition/Docs/Log/Dev.D.UE.0.0.10.P20.25.r0_log.md>
