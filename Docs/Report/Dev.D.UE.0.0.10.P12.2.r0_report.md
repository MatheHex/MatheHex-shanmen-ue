# Dev.D.UE.0.0.10.P12.2.r0 Report

## 1. 结论

P12.2 已完成并通过 P 阶段门禁。

本阶段为太极剑第一条基础连招建立了正式、可版本化的产品时序配置，并把 P12.1 的纯 SwordRhythm Host 安装到 GameMode 的真实 Combat Run 生命周期。一次 BasicSword 只有在现有 `Fdemo_mapCombatRunCoordinator` 返回完整闭合的 `IsExecuted()` 结果后，才会使用同一 Run 的 canonical 30 Hz timeline sample 生成 immutable rhythm receipt。

最终结果：

- 初始产品配置：`0.0.10.P12.2 / Shanmen.SwordRhythm.ProductConfig.r1`；
- 基础剑连招窗口：`[8, 13)` ticks @ 30 Hz，约为上一接受动作后的 `267–433 ms`；
- SwordRhythm ProductSession focused：`2/2`；
- 0.0.10 全量：`551/551`；
- 6 份 Automation 日志原始合计 `669 Success / 0 Fail`，按 test identity 去重为 `667`；
- changed-file gate：`Changed=7 / Rules=2 / Required=43 / Logs=6`；
- regression gate self-test：`172/172`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

本阶段没有把 rhythm chain count、band 或 receipt 接入伤害、攻速、资源倍率或其它平衡数值。

## 2. 功能性

### 2.1 版本化产品配置

新增 `Fdemo_mapShanmenSwordRhythmProductConfig`，由产品层唯一持有第一版太极剑节奏内容：

- content version：`0.0.10.P12.2`；
- content digest：`Shanmen.SwordRhythm.ProductConfig.r1`；
- rule id：`Combat.Style.Sword.Taiji01.BasicLinkWindow.r1`；
- canonical timeline：30 Hz；
- link window：open tick `8`，close tick `13`，采用 `[open, close)` 语义；
- ConfigId 由版本、digest、style/action definition、rule、窗口和 tick rate 确定性派生。

窗口是可追踪的第一版产品内容，不宣称已经完成最终平衡。未来调整必须变更 version/digest，因此不会把两个不同平衡版本误认为同一配置。

### 2.2 Run-local ProductSession

新增 `Fdemo_mapShanmenSwordRhythmProductSession`：

- 在 Combat Run begin 时安装 canonical config 与 P12.1 Host；
- 同一 Run、同一配置的重复 begin 幂等；不同 active Run fail closed；
- 只接受 `IsExecuted()` 的 BasicSword product result 与同 Run canonical timeline sample；
- 在 Host 副本上观察后整体提交，失败不污染现有 chain；
- 保存最新 immutable receipt，exact replay 不重复增加 observation；
- 只允许相同 RunId teardown，结束后 config、Host 与 receipt 一起清空。

Session 不持有 World、Actor、input、animation、timer、damage 或 balance mutation。

### 2.3 GameMode 产品安装

GameMode 现在把 SwordRhythm Session 与既有产品 Run 一起管理：

1. Coordinator、ThrownWeapon lifecycle 与 canonical timeline 成功 begin 后，再安装 rhythm config；
2. rhythm begin 失败会回滚 timeline、ThrownWeapon 与 Coordinator；
3. `ExecuteM01PlayerBasicSwordSweep()` 返回 `IsExecuted()` 后，立即捕获同一 Run timeline sample 并提交观察；
4. 成功日志只记录 activation、receipt、tick、band 与前后 chain count；
5. 捕获或观察失败明确记录诊断，但不会伪装为另一条 BasicSword／damage path；
6. Run release 校验 Session 与 Coordinator RunId 一致，并记录 observation count 后精确清理。

这条安装复用现有 BasicSword action、Impact/Vitality 与 Combat Run 权威，没有第二套剑击或伤害实现。

## 3. 完整性

新增两个 focused tests：

1. `CanonicalConfig`：验证版本、digest、30 Hz、`[8,13)` 窗口、确定性 ConfigId、同 Run begin 幂等、跨 Run begin/teardown 原子拒绝与匹配 teardown；
2. `RealBasicSwordLifecycle`：使用真实 Coordinator 与 Run timeline 执行两个合法 miss；首个形成 `Started / 1`，第二个在 open boundary 形成 `PreciseLinked / 2`，同时证明两次均未产生 damage；并覆盖未执行结果拒绝、latest receipt、exact replay 与 Run teardown。

changed-file regression map 同步新增 Session 规则，并把 timeline consumer 列表和 GameMode 规则扩展到新 Session。Self-test 同时加入正向覆盖和“只有 focused Session/Host 证据仍不足”的负向 fail-closed 用例。

## 4. 兼容性与权威边界

- timing config 只属于 SwordRhythm 产品层；GameMode 不自行推导窗口；
- timeline 仍由 P12.1 的唯一 Run clock 提供，未建立第二套时间源；
- 动作完成事实仍来自既有 Coordinator frozen action result，合法 miss 仍可形成节奏观察；
- chain count 仅存在于 receipt/Session inspection 与日志，没有进入 `IncomingDamage`、`FinalDamage`、AttackPower 或任何 damage multiplier；
- 没有修改 Impact、Vitality、inventory、schema、资源事务、GAS ability、动画资产或输入绑定；
- 新增生产 Session 的 350 行未引用 `AActor`、`UWorld`、`GetWorld`、Timer、frame counter、wall clock、RNG、`ApplyDamage`、`TakeDamage` 或 `UGameplayStatics`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

不含本 Report/Log，共 7 个路径，`770 additions / 6 deletions`：

- 新增 SwordRhythm product config/session header 与 implementation；
- 新增 2 条 Session 自动化测试；
- 修改 GameMode 的 Run begin、BasicSword observation、Run release、状态校验与日志；
- 更新 changed-file regression map 与 self-test。

## 6. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 2 | 0 | 1 | `B7A64105B0932B0DC1919354EA4DCA785A9E1E5DE9074D676A498AE2C4DD141D` |
| `Shanmen.0_0_10` | 551 | 0 | 1 | `7766FF0AF91D9F195AD032DDF5FB24CC24582A20E02A622BC9006716910C6DCA` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `7079E2EBCAB1679756A11C1898AB81AE23460C10F40EA5C864B429D5C82EB2A1` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `C4F935C6077190503B579C2B857B9174F670C5D500BF96A73F2368F9675665FE` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `B68021208E9E7D94B98FC5A2E567ECB689FA9A6065A22E1794DA6E2AB632562E` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `FCBE9F125509D4D25DF6D68C303B066AF9E649CF62EAB2B2E9B27B63B9BAC77B` |

Focused 2 条已包含在 full 551 条内；原始日志合计为 669，按 test identity 去重为 667。每份日志均为 queue complete、Fail 0，且无 Fatal、Unhandled Exception、Assertion 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=43 Logs=6
SELF_TEST: PASS 172/172
BOUNDARY_SCAN: PASS ProductionLines=350 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

changed-file gate 日志 SHA-256：`5EBDE6668C1AF6861A54C4698CFB1A0DC560477A09384D631FBE7F2D602F1B60`。Self-test 日志 SHA-256：`FEA3C9D5EAD56572BB41CDE74F68BBE23D3A26203E612FE7E3B372FAFA69232C`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit | Final log SHA-256 |
|---|---|---|---:|---|
| Editor first | Succeeded | 24 / 118.55s | 0 | console evidence |
| Game first | Succeeded | 23 / 106.57s | 0 | console evidence |
| Editor final | Succeeded | up to date, 0 / 0.88s | 0 | `4704F0C28FF77589BD958E8D4B107242838D2D8F1742E15D0210B7730FFE997B` |
| Game final | Succeeded | up to date, 0 / 0.91s | 0 | `CFF4BCB4985E7C4C8D34EA26426971912F4B137933DFCA034731E772B0B2CD21` |

最终产物：

- `UnrealEditor-demo_map.dll`：12,983,808 bytes，SHA-256 `602E9E1E1006832B7FEF3E65DE2D2437C8DC18BD11A0440DED97063A84973AD4`；
- `demo_map.exe`：354,530,304 bytes，SHA-256 `E99C31782CE0E7320A40951FBD94D9C155EC6FD901FF4B034E6B8009B4E0F0DB`。

两目标首次构建即成功，没有源码失败，也没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

本阶段没有产品测试失败或构建失败。最终复核发现静态扫描初版模式误命中注释中的小写 `timer` 描述；改为只扫描真实 C++ 类型/API 标识后得到 `ForbiddenHits=0`。这属于证据查询精度修正，没有修改产品行为或放宽边界。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、版本化产品配置、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

建议 P12.3 把 immutable rhythm receipt 只读投影给表现／状态层，使 UI 或动画能够读取 band、tick 与 chain count；仍禁止把它直接转换为伤害。真实动画 marker、输入手感和窗口调优留到具备正式资产与 F 阶段运行验证时完成。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-2-sword-rhythm-product-install>
