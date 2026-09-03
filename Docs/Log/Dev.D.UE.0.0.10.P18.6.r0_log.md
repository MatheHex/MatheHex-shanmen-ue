# Dev.D.UE.0.0.10.P18.6.r0 Development Log

## 1. 目标与基线

- 基线提交：`64ab4e252fe7684683bb694b856ad5f6a162f85c`（P18.5 Sword Qi Command Adapter）；
- 分支：`agent/0.0.10-p18-6-sword-qi-command-owner`；
- 目标：建立 Run-scoped Sword Qi 逻辑命令事件 owner，为 P18.5 提供稳定、可验证、可显式重放的 InputEventId；
- 约束：仅做 P 阶段，不绑定真实输入，不拥有空间/装备/属性/库存/Actor/投射物/伤害/生命权威，不投放正式表现或关卡内容。

## 2. 设计选择

P18.5 明确把稳定 InputEventId 的所有权留给未来调用方。P18.6 采用一个轻量 Run owner，而不是 UI/InputComponent 或第二套产品状态机：RunId 负责命名空间，uint64 sequence 负责单调分配，事件值的私有字段与重派生校验负责防伪。

该 owner 不保存完整 ledger。一个事件属于当前 owner 当且仅当：令牌可由 `RunId + sequence` 重派生，RunId 与 active owner 相同，且 sequence 小于下一待分配值。这样用 O(1) 状态表达已提交身份范围，同时避免把产品 payload 或执行结果复制到新权威。

## 3. 新增契约

新增：

- `demo_mapShanmenSwordQiCommandEventOwner.h/.cpp`；
- `demo_mapShanmenSwordQiCommandEventOwnerTests.cpp`。

`Fdemo_mapShanmenSwordQiCommandEvent` 只读公开 RunId、InputEventId、EventSequence。`Fdemo_mapShanmenSwordQiCommandEventResult` 区分新事件/重放、是否已提交、P18.5 输入结果与诊断；`CanReplay` 只对已提交有效事件成立。teardown summary 保存 RunId 与 committed event count。

状态包括 Applied、OwnerInactive、OwnerInvalid、SequenceExhausted、EventInvalid、EventNotOwned、InputRejected、OwnerPostconditionFailed。adapter 返回身份与 owner 预期不一致时以 postcondition failure 失败关闭。

## 4. 分配、提交与重放

`MakeInputEventId` 使用 `demo_map.SwordQi.CommandEvent.r1`，由 RunId 与 EventSequence 确定性派生。序号 0 无效，保留上限不分配，避免回绕。

新事件只有在 P18.5 实际调用产品 route 后才提交并推进序号。调用前门禁失败仍保留同一候选 sequence，允许环境恢复后重新尝试而不制造空洞。一旦产品被调用，即使返回 HostBusy，事件仍提交，防止系统自动把一个可能产生过冻结或副作用的 identity 分配给另一逻辑命令。

显式 `TryReplay` 要求传回当前 Run 已提交令牌，不分配新 sequence。owner 不自动循环重试，也不冻结空间样本；若 P18.4 已捕获 intent，它继续负责 exact payload replay 与 conflict detection。

## 5. GameMode 集成

`Ademo_mapGameMode` 增加 `IssueSwordQiStartCommand` 与 `ReplaySwordQiStartCommand`，两者都只组合 owner 与既有 `RouteSwordQiStartInput`。没有新增到 `RouteSwordQiIntent` 之外的产品入口。

activation 顺序为 Combat Run 既有 authority -> Sword Qi Product Controller -> Sword Qi Command Event Owner。teardown 在 durable treatment recovery 后先结束 command owner，再结束 controller；失败回滚、孤儿态及最终 reset 都覆盖 owner。RunBound 增加 owner active 证据，RunReleased 增加 committed event count。

## 6. Automation fixture

新增四项 exact Automation：

1. `LifecycleIdentity`；
2. `PreRouteFences`；
3. `AppliedReplay`；
4. `BusyRetry`。

Fixture 使用 unattended GamePreview world、NullRHI 和真实 P18.4/P18.5 代码链。BusyRetry 先保留第一发 active flight，使第二事件在 HostBusy 时被产品看到并提交；第一发 terminal retirement 后，用同一事件显式重放，验证复用冻结 command、AttackPower 与 CommandId，且 owner sequence 不再推进。

## 7. 回归映射与静态边界

`Scripts/ShanmenRegressionMap.json` 新增 `SwordQiCommandEventOwner` 规则，要求 full、adapter、controller、item/session/host/world delivery、action/run、items/world/runtime/core 与相关 legacy 组。GameMode 的 M01 规则同步要求该 exact 组。

Self-test 增加正向完整证据 fixture 与“只有 exact 不得替代依赖/legacy”的负向 fixture，最终 287/287。改动驱动覆盖结果为 `Changed=7 / Rules=2 / Required=55 / Logs=10`。

生产 owner 两文件的边界扫描为 2 files / 0 matches；确认没有直接世界、Actor、随机数、库存写、装备/属性查询、物理输入、声音、特效或旧投射物依赖。

## 8. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_command_event_owner.log` | exact | 4/0 | `E6417A8B4EC960481E05F9DC6B329DA83358BE735587F6CA081DB0AB68C5F12A` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 781/0 | `041DE0950137FD66BF08DAC3AEB45574E5076A7F896896CEA02AD674E953A3BD` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `481198595A1695312E5B59E255E5FF811F084CFE2715B5ECD4434F9FA1946AA7` |
| `automation_profile.log` | legacy | 211/0 | `DF92122AA26720C00ADD2BCCA7FA270C55D217D4BF98C496B1B5378B813B9804` |
| `automation_code_b.log` | legacy | 60/0 | `114987905ACD3B98B93A0C1F26C4EFA07C69C1975F3A33182E05518A3739F946` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `A0C8B75AB381828ED138B3A37DD7C47CEEC2F8F8C0A46F559DCDEE943830D674` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `F66E3F79DDF83D8199024FEE4A736C430D4C12805CD204EEC96370CC3E0819D0` |
| `automation_v3.log` | legacy parent | 29/0 | `2D379CC8553429618420F28A9FF65CFF66F81F8326B8E06C5557CA8CD81995A8` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `4BBCF9C3E7BD26558618B16618FBF46C73E5A94B013A435962A8C4BCE9E0E92F` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `9E935992A7C814736A1F6167E3C6FCAE3A40821E986680B4045219D6EE794BE7` |

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

上述四项日志 SHA 依次为 `C2A4B6116A9BF72D4DC42C7AD9D0069ECD88F5AFCFE3187B2624E71B6AA4CF6C`、`41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`、`0D37EA04FB41FF5D5784CA3F2E447A2B54605F170FD31EE541661C2681B4C572`、`2617504A024FB5606992573FB635EAFB19096E71F4F520ECB1430757C8F4111C`。

## 9. 构建与产物

- Editor initial：27 actions、native 0、151.91s、SHA `55F89AF157D4DE8C7011B8EEC0D75A9EF725B53B10A6BE64FCC03D63500E4158`；
- Game final：26 actions、native 0、132.51s、SHA `40C081E812362533A9D2F37B52041E2E7CFEC0E7942AC67005CF6237095BD5B3`；
- Editor final：up to date、0 actions、native 0、0.97s、SHA `2C6C97BE7B04AC54520A9BC363196CA12634F3265CE4FE738425DF4D27125C51`；
- `demo_map.exe`：356,427,264 bytes、SHA `345B6174A687AF476063F0109CB405164D83ADBA4FEC61D4170F959D991934A9`；
- `UnrealEditor-demo_map.dll`：15,109,632 bytes、SHA `502EA2E798053803FF4776BFBEFC997C12078DA91E09F85C1274F1BF09415C38`。

## 10. 提交边界与后续

本轮提交 9 个文件：3 个新增源码、4 个修改文件、本 Report 与本 Development Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.6` raw logs 不入 Git。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。P18.7 建议冻结 `event identity + origin/aim sample` 为不可变逻辑 command request，补齐产品捕获前拒绝路径的精确重放语义，同时继续把 Enhanced Input 与正式表现留在 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-6-sword-qi-command-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-6-sword-qi-command-owner/Docs/Report/Dev.D.UE.0.0.10.P18.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-6-sword-qi-command-owner/Docs/Log/Dev.D.UE.0.0.10.P18.6.r0_log.md>
