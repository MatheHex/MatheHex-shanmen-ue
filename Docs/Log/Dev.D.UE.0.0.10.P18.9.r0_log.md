# Dev.D.UE.0.0.10.P18.9.r0 Development Log

## 1. 目标与基线

- 基线提交：`c35759823a28ac329b5474324d9d96b974c943f1`（P18.8 pending retry slot）；
- 分支：`agent/0.0.10-p18-9-sword-qi-command-availability`；
- 目标：把 Sword Qi command owner 的结构可用性投影给未来输入/UI，同时保持单一 owner 与产品权威；
- 约束：不接键位或 UI，不触发命令或自动 retry，不公开 mutable request/sample/payload，不读写库存、属性、伤害或生命。

## 2. 投影设计

新增单一枚举 `OwnerInactive / IssueReady / PendingRetry`。`CanIssue / CanRetry / CanCancel` 全部由该枚举与投影有效性派生，没有增加可独立漂移的 bool 字段。

投影类的数据成员保持 private；公开接口只读。默认实例缺少确定性身份而失败关闭。owner 先构造、计算 ID、验证，再复制输出，调用失败时输出保持默认无效状态。

## 3. 身份与不变量

`ProjectionId` 采用命名空间 `demo_map.SwordQi.CommandAvailability.r1`，规范输入为 state、RunId、next sequence、pending InputEventId 和 pending sequence。

状态形状验证：

- inactive：Run 无效、next sequence 精确为 1、无 pending event；
- issue-ready：Run 有效、sequence 非零、无 pending event；
- pending-retry：Run 有效、sequence 大于 1、pending event 有效且属于同一 Run，event sequence 小于 next sequence。

pending 投影只复制 event identity。完整 request 与 frozen origin/aim 继续只由 P18.7/P18.8 owner 保存。

## 4. 组合入口

在 `Ademo_mapGameMode` 增加 `TryProjectSwordQiCommandAvailability`，直接委托 owner。没有新增缓存、镜像 ledger 或另一路命令路由；issue、replay、retry、cancel 与产品控制器代码保持原路径。

## 5. Automation 改动

在既有测试文件新增 1 条 `AvailabilityProjection`，exact 从 6 增至 7，full 从 783 增至 784。测试覆盖默认失败关闭、inactive、Run A issue-ready、pre-route 稳定性、HostBusy pending、重复 HostBusy 稳定性、cancel、旧快照不可变、teardown 与 Run B 身份隔离。

测试明确断言 pre-route gate 不调用 sampler/product callback；pending 状态只返回 Event；重复 pending retry 不改变 ID；cancel 后 next sequence 仍推进且 ID 改变。

## 6. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_command_event_owner.log` | exact | 7/0 | `DA2E69BE2D6E903F82BA5AA2D064C8AFD22A744BC2174361BA1AF82B9B462B13` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 784/0 | `17095B7364B4431F8941D36F0CEAA67027E4E3AA50DC682F2D052B163CECB5D4` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `8560E8F2E887BBF45E140BD6CA80A8E8900ADF272A115B3C2844ED45ADCDA12E` |
| `automation_profile.log` | legacy | 211/0 | `9B29DA165372A18F22A53AE18617D48596FAF1D251928A2F304A9F1F63FAF649` |
| `automation_code_b.log` | legacy | 60/0 | `A4F1F49DFB83E2FC3E90FF4AF60338BD75DAA72B77817C38D615D87A5DC40CF6` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `73E210649C8E5F8F698AE82F14C71B62E755D254E06494EA0E1779E05BACBD5A` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `248A2851363E122BC7225AE9EC9CB18F7C1A09760CF5AC9EB724FBB1B9BBFD23` |
| `automation_v3.log` | legacy parent | 29/0 | `B27191371DF5DA8554A1FF879D54760A38C8EAA7FD391E00407CD397D29C5C66` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `9962C2CEFFB6BCC5388B415EC5291F2AA66EC8A28856BBEF9745D08BE5F31C56` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `BC6FC90E1DD97D26BFDE7C876F3A099EF8BCB425C008FB15749D9D1F5BFBC49D` |

全部日志 Result Fail 为 0，并各含一个原生 terminal success marker。完整套件用时约 32m58.789s；保留其网络探测超时与调度停顿原始记录，没有修改 raw log。

## 7. 门禁与覆盖

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS (0 forbidden matches)
GIT_DIFF_CHECK: PASS
```

SHA：coverage `6201E6868ACF36E8EAD6930400BDAB9CEE1418915D1F2D4115A24B459DEDE948`；self-test `41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；boundary `00BF897E6C49484DFB767E4D6E979534DDA5B5A07A6A2A1D814655333C613386`；diff-check `6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`。

## 8. 构建与产物

- Editor initial：27 actions、native 0、136.24s、SHA `9DED72A8B8CFDBD2D2B1FAE12134BA85E395EB3C159E7EC5BE6E34135DA3E690`；
- Game final：26 actions、native 0、117.09s、SHA `D20F5181A3701A077C72A7DB65B543C33E7095B62952EA8784F06A551D5B0559`；
- Editor final：up to date、0 actions、native 0、1.02s、SHA `C7726219EF7E1D815A9EEEA344B516B0F25159F8807BF59F3C11A39EF61ACF12`；
- `demo_map.exe`：356,454,400 bytes、SHA `D86604FF16B7780A6B8B7ED52D59004583EE668D9F473D509630D526A335C8A5`；
- `UnrealEditor-demo_map.dll`：15,140,864 bytes、SHA `88BB7DA1DDC6393AAA08E060553DDACF4A260E11159879EC201D6B2A28FA8145`。

## 9. 改动边界

源码改动 5 个文件、369 行新增：owner header/cpp、owner tests、GameMode header/cpp。另新增本 Report 与本 Development Log，计划精确提交 7 个文件。

长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件未修改、未暂存。`Saved/Codex/P18.9` raw logs 仅本地保留。未修改 Content、地图、资产、配置、Windows、UE Engine 或存档 schema。

## 10. 后续

P18.10 建议增加携带 expected `ProjectionId` 的设备无关命令决策路由。路由先比较当前投影，拒绝 stale 或与 enum 不兼容的 `Issue / Retry / Cancel`，再委托既有唯一入口；仍不接真实输入、UI 或自动 retry。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-9-sword-qi-command-availability>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-9-sword-qi-command-availability/Docs/Report/Dev.D.UE.0.0.10.P18.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-9-sword-qi-command-availability/Docs/Log/Dev.D.UE.0.0.10.P18.9.r0_log.md>
