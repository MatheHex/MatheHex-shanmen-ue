# Dev.D.UE.0.0.10.P18.9.r0 Report

## 1. 结论

P18.9 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P18.8 的 Sword Qi 逻辑命令所有者上增加一个不可变、Run-scoped、设备无关的结构可用性投影。调用方现在可从同一个枚举状态派生 `CanIssue / CanRetry / CanCancel`，并取得确定性 `ProjectionId`；pending 状态只公开逻辑事件身份，不公开冻结 request、origin/aim 样本或产品 payload。

```text
Sword Qi Command Event Owner exact:    7 Success / 0 Fail
Shanmen.0_0_10 full:                 784 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=5 / Rules=2 / Required=55 / Logs=10)
Regression gate self-test:             PASS 287/287
Boundary scan:                         PASS (Files=2 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入，也没有执行截图、Smoke、Cook 或 Package。结论只覆盖 C++ 契约、无头自动化、静态门禁和 Development 构建，不宣称最终键位、UI、表现或手感通过。

## 2. 单一状态契约

新增 `Edemo_mapShanmenSwordQiCommandAvailabilityState`，只表达 owner 自身的结构状态：

| State | CanIssue | CanRetry | CanCancel |
|---|---:|---:|---:|
| `OwnerInactive` | false | false | false |
| `IssueReady` | true | false | false |
| `PendingRetry` | false | true | true |

三个公开能力不是可独立修改的 bool 字段，而是由一个已验证枚举派生，避免出现 `CanIssue=true` 与 `CanRetry=true` 等互相矛盾的组合。

`IssueReady` 只表示 command owner 已绑定 Run 且 pending slot 为空，不承诺后续 gameplay gate、装备读取、动作仲裁、产品 Host 或世界交付必然接受请求。该语义边界防止只读投影复制产品权威。

## 3. 不可变投影与失败关闭

新增 `Fdemo_mapShanmenSwordQiCommandAvailabilityProjection`。所有数据成员均为 private，外部只能读取：

- `ProjectionId`；
- 当前状态；
- `RunId`；
- 下一逻辑事件序号；
- pending 状态下的不可变 `Fdemo_mapShanmenSwordQiCommandEvent` 身份。

默认构造的投影没有 `ProjectionId`，因此 `IsValid()` 返回 false。owner 采用候选值构造、计算身份、完整验证、最后复制到输出的模式；owner 自身无效时不返回半成品。旧投影在 owner 后续发生取消或 teardown 后仍保持原值和有效性，符合快照语义。

## 4. 确定性投影身份

`ProjectionId` 使用命名空间 `demo_map.SwordQi.CommandAvailability.r1`，由以下规范字段确定：

```text
state
RunId
NextEventSequence
Pending InputEventId
Pending EventSequence
```

未发生 owner 状态变化时重复读取可得到同一个 ID；pre-route gameplay 拒绝不消费序号，因此 ID 不变；首次 `HostBusy` 保存 pending event 后 ID 改变；对同一冻结请求再次得到 `HostBusy` 时 ID 保持；显式取消后状态和序号共同产生新 ID；更换 Run 后即使同为 `IssueReady` 也产生不同 ID。

该 ID 是内容确定的只读投影身份，不是产品 CommandId，也不是另一套 ledger 或单调 revision。

## 5. Pending 最小披露

`PendingRetry` 投影只复制 P18.8 request 中的 Event：`RunId / InputEventId / EventSequence`。它不暴露：

- origin/aim 空间样本；
- 完整 frozen request；
- exact item、AttackPower 或 launch command；
- 控制器、Actor、投射物、伤害、生命或库存状态。

因此未来输入或 UI 可以显示“可重试/可取消”并关联稳定事件，却不能通过投影改写 P18.7/P18.8 冻结请求或绕开 P18.4 产品控制器。

## 6. GameMode 组合入口

`Ademo_mapGameMode::TryProjectSwordQiCommandAvailability` 只委托唯一 `SwordQiCommandEventOwner`，不缓存、不镜像且不触发任何命令。现有 issue、replay、pending retry/cancel、input adapter 和产品 route 均未改变。

该入口没有 UInputAction、Enhanced Input mapping、键位、UI widget、Tick 或自动重试逻辑；设备绑定继续属于明确授权后的 F 阶段。

## 7. 聚焦与全量自动化

exact 从 6 增至 7。新增 `AvailabilityProjection` 覆盖：

1. 默认投影失败关闭，空 owner 生成规范 inactive 投影；
2. 无状态变化时身份稳定；
3. Run A 进入 `IssueReady`，只有 issue 为 true；
4. pre-route 拒绝既不采样也不调用产品，并保持相同投影身份；
5. `HostBusy` 进入 `PendingRetry`，只公开 pending event 身份；
6. 同一冻结请求显式重试仍忙时保持相同 pending 身份；
7. cancel 恢复 issue，并以已推进序号产生新身份；
8. 取消后旧投影副本仍不可变且有效；
9. teardown 恢复规范 inactive 内容，新 Run B 改变 issue-ready 身份。

完整 `Shanmen.0_0_10` 从 P18.8 的 783 增至 784，实测 784/0。首末 Success 为 `2026.09.03 06:02:58.473 -> 06:35:57.262 UTC`，约 32m58.789s。慢速耐久用例期间出现引擎后台 `generate_204` 网络探测超时及一次 87.60 秒调度停顿；相关用例随后均明确 `Result={Success}`，最终原生 `TEST COMPLETE / EXIT CODE 0`，未将环境警告伪装为产品失败或静默删除。

## 8. 证据与改动驱动门禁

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Command Event Owner exact | 7 | 0 | `DA2E69BE2D6E903F82BA5AA2D064C8AFD22A744BC2174361BA1AF82B9B462B13` |
| `Shanmen.0_0_10` full | 784 | 0 | `17095B7364B4431F8941D36F0CEAA67027E4E3AA50DC682F2D052B163CECB5D4` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `8560E8F2E887BBF45E140BD6CA80A8E8900ADF272A115B3C2844ED45ADCDA12E` |
| `demo_map.Profile` | 211 | 0 | `9B29DA165372A18F22A53AE18617D48596FAF1D251928A2F304A9F1F63FAF649` |
| `demo_map.CodeB` | 60 | 0 | `A4F1F49DFB83E2FC3E90FF4AF60338BD75DAA72B77817C38D615D87A5DC40CF6` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `73E210649C8E5F8F698AE82F14C71B62E755D254E06494EA0E1779E05BACBD5A` |
| `demo_map.P4.Hotbar` | 7 | 0 | `248A2851363E122BC7225AE9EC9CB18F7C1A09760CF5AC9EB724FBB1B9BBFD23` |
| `demo_map.V3` | 29 | 0 | `B27191371DF5DA8554A1FF879D54760A38C8EAA7FD391E00407CD397D29C5C66` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `9962C2CEFFB6BCC5388B415EC5291F2AA66EC8A28856BBEF9745D08BE5F31C56` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `BC6FC90E1DD97D26BFDE7C876F3A099EF8BCB425C008FB15749D9D1F5BFBC49D` |

每份采用日志均含一个原生 terminal success marker，Result Fail 为 0。五个改动源码触发 2 条路径规则、55 个必跑组，并由 10 份独立日志满足：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS (0 forbidden matches)
GIT_DIFF_CHECK: PASS
```

对应 SHA：coverage `6201E6868ACF36E8EAD6930400BDAB9CEE1418915D1F2D4115A24B459DEDE948`；self-test `41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；boundary `00BF897E6C49484DFB767E4D6E979534DDA5B5A07A6A2A1D814655333C613386`；diff-check `6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`。

## 9. 构建、产物与边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded / native 0 | 27 / 136.24s | `9DED72A8B8CFDBD2D2B1FAE12134BA85E395EB3C159E7EC5BE6E34135DA3E690` |
| Game Development final | Succeeded / native 0 | 26 / 117.09s | `D20F5181A3701A077C72A7DB65B543C33E7095B62952EA8784F06A551D5B0559` |
| Editor Development final | Succeeded / native 0 / up to date | 0 / 1.02s | `C7726219EF7E1D815A9EEEA344B516B0F25159F8807BF59F3C11A39EF61ACF12` |

最终产物：

- `demo_map.exe`：356,454,400 bytes，SHA-256 `D86604FF16B7780A6B8B7ED52D59004583EE668D9F473D509630D526A335C8A5`；
- `UnrealEditor-demo_map.dll`：15,140,864 bytes，SHA-256 `88BB7DA1DDC6393AAA08E060553DDACF4A260E11159879EC201D6B2A28FA8145`。

生产 owner 边界扫描为 0 命中：没有 GameplayStatics/ApplyDamage/TakeDamage、旧 SkillProjectile、UWorld/AActor、RNG、库存写事务、物品/属性读取、物理输入绑定、声音或 Niagara 依赖。

## 10. 提交边界、下一阶段与 GitHub

基线提交为 `c35759823a28ac329b5474324d9d96b974c943f1`。本轮精确提交 5 个修改源码、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.9` raw logs 不进入 Git。

P18.10 建议增加设备无关的 availability-checked command router：调用方携带刚读取的 `ProjectionId` 与单一 `Issue / Retry / Cancel` 决策，router 在执行前重新投影并拒绝 stale 或与当前 enum 不兼容的决策，再委托现有唯一入口。该阶段仍不绑定按键、不加入 UI、不自动 retry，也不复制 owner 或产品权威。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-9-sword-qi-command-availability>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-9-sword-qi-command-availability/Docs/Report/Dev.D.UE.0.0.10.P18.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-9-sword-qi-command-availability/Docs/Log/Dev.D.UE.0.0.10.P18.9.r0_log.md>
