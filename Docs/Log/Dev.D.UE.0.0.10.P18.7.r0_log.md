# Dev.D.UE.0.0.10.P18.7.r0 Development Log

## 1. 目标与基线

- 基线提交：`2b1ba2d2011ae75c0d5643a0e870cf5464999b2f`（P18.6 Sword Qi Command Event Owner）；
- 分支：`agent/0.0.10-p18-7-sword-qi-frozen-command-request`；
- 目标：把逻辑事件身份与首次有效 origin/aim 样本冻结为不可变请求，使显式重放在类型层不再接受新空间采样；
- 约束：仅做 P 阶段，不绑定真实输入，不拥有物品、属性、库存、Actor、投射物、伤害或生命权威。

## 2. 缺口复核

P18.6 已保证稳定 EventId 与显式 replay，但 `TryReplay` 和 GameMode replay 仍接收新的采样 callback。P18.4 通常会用 IntentId conflict 阻止同身份异轨迹；然而这个保护发生在更深的产品层，而且在产品捕获前拒绝的路径中，命令 owner 自身没有冻结空间真值。

P18.7 把该不变量前移到逻辑命令边界：首次有效 sample 即形成 request，之后重放只能使用 request 内样本。P18.4 继续拥有产品 intent/command，P18.5 继续拥有一次采样与输入门禁，职责没有复制。

## 3. 请求契约

`Fdemo_mapShanmenSwordQiCommandRequest` 私有保存 Event 与 InputSample，并只提供 const getter。默认请求无效；只有 command owner 能用已验证事件和有效样本构造。Result 新增 `Request`，`IsAccepted` 同时核对 Event、Request、Input identity 与 Sample；`CanReplay` 要求已提交且完整 request 有效。

状态增加 `RequestInvalid` 与 `RequestNotOwned`，分别定位无效请求和不属于当前 active Run/committed sequence 的请求。原 `EventInvalid` 保留给新事件派生失败。

## 4. 提交语义

`RouteNewEvent` 先调用 P18.5 seam，再验证返回的 EventId、RunId 与（若产品被调用）IntentId。有效 sample 捕获后立即生成 request、推进 sequence 并标记 committed。这样 intent capture 前的异常拒绝仍有可重放的冻结命令。

门禁前未采样和无效 sample 不提交；产品 route 已被调用但 evidence 不一致时仍保守消耗 identity，避免潜在副作用后自动复用。sequence 上限继续失败关闭，不回绕。

## 5. 重放与 GameMode

`TryReplay` 现在接收 FrozenRequest，并把 EventId 与 const FrozenSample 传给固定 route callback。返回后核对 identity、IntentId、sample 与 route shape；重放不分配 sequence。

`Ademo_mapGameMode::ReplaySwordQiStartCommand` 删除 origin/aim callback，只接受 FrozenRequest 与 gameplay gate。内部用冻结向量调用既有 `RouteSwordQiStartInput`，再沿 P18.5 -> P18.4 -> 唯一 `RouteSwordQiIntent` 链执行。`IssueSwordQiStartCommand` 与 Run begin/end 顺序不变。

## 6. Automation 改动

新增 `FrozenBeforeProductRoute`：构造有效 sample 已捕获但产品尚未调用的受控拒绝，验证 request committed、sequence 前进、显式 replay 收到同一 sample，原始 source capture 计数不再增加，跨 Run owner 在 callback 前拒绝。

`AppliedReplay` 与 `BusyRetry` 改用 FrozenRequest。两者分别验证 authority drift 与 HostBusy 后 replay 都不再调用外部 sampler，同时继续复用 P18.4 冻结 item、AttackPower 与 CommandId。exact 从 4 增至 5，full 从 781 增至 782。

## 7. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_command_event_owner.log` | exact | 5/0 | `DDCFBCD916615A8B014259A2938D9363D17D26D944F1A0376D8E1146535EAA9C` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 782/0 | `7F2DE6B2C26CABCD9640346E718818C18EE6B30AC218834466A80A48F7075307` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `D32B3443945AAE94A1CE385E76377131691AA2B8D32337514BA5F6C1858A0079` |
| `automation_profile.log` | legacy | 211/0 | `8EEA8AB32DA3941F1609B6A03BB3C9B993786D3AA6BF4C0BEB0C15670AE8B6C6` |
| `automation_code_b.log` | legacy | 60/0 | `CBAFE0CBFCA6F393C3832C40A0FE4E7534DD1E588E86FDD1F796F2316C51ABE9` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `C83E469D71B226512DF99DE2321A26FF28CFFE283C87C5A500F471B8CA65D923` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `575936027EC487033C6EA013C2F06D94C2F9887C8C4D3708BF68B1B2936145C5` |
| `automation_v3.log` | legacy parent | 29/0 | `14E9F625F12933930B21BC18F515A81147632548D7ECE9F50FF0BEF430B6D156` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `574A1DF202956048BB7812E35CEFA0EE4A13AA499D69A0C4030CB4BF58D85828` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `9AD6FD5C21A06EC74F691D48134037680AA424EEA8D3B5346F2E85243FE1658A` |

全部采用日志 Fail/Fatal 为 0，且各含一个 native terminal success marker。

## 8. 门禁与覆盖

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

对应 SHA：coverage `8E1BBE591019DC3A1F58403C0BBACF938BF2EBBA12C903E006DA3204584F36CF`；self-test `41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；boundary `0D37EA04FB41FF5D5784CA3F2E447A2B54605F170FD31EE541661C2681B4C572`；diff-check `547436D81BD9D3FCECD61E5223E114ABA690765645EF62894DBD389D8B013FE0`。

## 9. 构建与产物

- Editor initial：27 actions、native 0、152.26s、SHA `EC815F8D09403C9DF901590C01FF5EA15FD7839725DB093F02636B618E4CE797`；
- Game final：26 actions、native 0、124.04s、SHA `3CAAAB10BD940250B7AFE2E261138FC7E63DE8C9EBF802D398AA6E7B2F374174`；
- Editor final：up to date、0 actions、native 0、1.43s、SHA `EAE37894A702F5D760BEF7F8326D447B2F7D5A6A19E64701D8AFAE56E3CD0F93`；
- `demo_map.exe`：356,433,408 bytes、SHA `3806EF7AD100EC5905D6244B41F3C5D8E57588FECA012288C863EE453CF13CF5`；
- `UnrealEditor-demo_map.dll`：15,116,800 bytes、SHA `6C383E1218510E5E40FC7BD508344584FACEF964C60DD171E3CEBB6A157903BD`。

## 10. 提交边界与后续

本轮提交 7 个文件：5 个修改源码、本 Report 与本 Development Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.7` raw logs 不入 Git。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。P18.8 建议增加容量为 1、仅显式 retry/cancel 的 pending frozen-request slot，继续禁止自动重试和重新采样。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-7-sword-qi-frozen-command-request>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-7-sword-qi-frozen-command-request/Docs/Report/Dev.D.UE.0.0.10.P18.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-7-sword-qi-frozen-command-request/Docs/Log/Dev.D.UE.0.0.10.P18.7.r0_log.md>
