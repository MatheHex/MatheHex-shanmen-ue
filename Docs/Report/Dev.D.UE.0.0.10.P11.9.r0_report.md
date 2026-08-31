# Dev.D.UE.0.0.10.P11.9.r0 Report

## 1. 结论

P11.9 已完成并通过 P 阶段门禁。

本阶段在 P11.8 唯一 `WeaponGuardProductRoute` 之前增加一个无状态 `WeaponGuardInputAdapter`。它先检查调用方提供的既有 gameplay gate 与产品 route 可用状态；只有两者均开放时，才对调用方拥有的单调 timeline 取样一次，并把同一 timeline ID / start tick 原样委托给 P11.8。它不绑定物理按键、不接受 item ID、不创建时钟，也不复制 Run、host 或 Impact 权威。

最终结果：

- 新增唯一 guard start input-to-product seam；
- blocked / unavailable 输入在 timeline 取样前关闭；
- eligible 输入恰好取样一次、委托一次，downstream rejection 不重试；
- timeline 使用 opaque GUID + tick，adapter 不解释单位、不换算、不补造；
- exact equipped item authorization 继续完全由 P11.7/P11.8 负责；
- 新增 5 个 focused tests，0.0.10 全量达到 510/510；
- 5 组最终 Automation 日志共记录 591 次 Success、0 次 Fail；
- regression mapping 98 rules，自检 156/156；
- changed-file gate：`PASS Changed=5 Rules=1 Required=11 Logs=5`；
- `git diff --check` 与精确生产边界扫描通过；
- Editor Development 与 Game Development 单并发构建均首次成功，原生退出码均为 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 前置 gate 与单次取样

`Fdemo_mapShanmenWeaponGuardInputAdapter::RouteStartInput` 接收：

- 当前 gameplay input 是否开放；
- P11.8 product route 是否可用；
- 一个返回不可变 timeline sample 的 callback；
- 一个只接 timeline ID / start tick 的 P11.8 route callback。

固定顺序为：

```text
gameplay gate
  -> product route availability
  -> capture caller-owned monotonic timeline exactly once
  -> validate opaque timeline sample
  -> invoke only P11.8 route exactly once
  -> preserve nested product proof or typed rejection
```

前两个 gate 失败时，timeline callback 与 product callback 都不会执行，Combat Run activation sequence 不被消费。sample 无效时不会调用 product route。product route 拒绝时不会自动重试。

### 2.2 不可变 timeline sample

`Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture` 只接受：

- 有效 timeline GUID；
- 非负 start tick；
- 能容纳 canonical perfect-window tick count、不会发生 `int64` 上溢的 tick。

成功后只暴露只读 getter。adapter 不知道 tick 的帧率或时间单位，不读取平台时钟，不调用 `FGuid::NewGuid`，也不把 start tick 转换成秒。

### 2.3 完整委托证据

`Fdemo_mapShanmenWeaponGuardInputResult` 保留：

- typed input status；
- 是否实际取样；
- 是否实际调用 product route；
- exact timeline sample；
- 完整 P11.8 product-route result；
- diagnostic。

`IsAccepted` 同时要求本层为 `Applied`、取样与委托各发生一次语义成立、nested P11.8 proof ready，并验证 active host 的 timing policy 精确等于本层 sample。任何 identity 或 tick 偏差都会 fail closed。

## 3. 完整性

输入结果显式区分：

- `Applied`；
- `GameplayBlocked`；
- `ProductRouteUnavailable`；
- `TimelineRejected`；
- `ProductRejected`。

adapter 不接受 source item、balance、Run ID、host、retry policy 或 incoming Impact，因此无法在 input seam 重建既有产品权威。item 缺失继续由 P11.8 分类，成功时 exact equipped item 继续贯穿 authorization、reservation、action 与 host。

## 4. 兼容性与权威边界

- 调用方继续拥有 gameplay gate 与 monotonic timeline source；
- P11.8 继续是唯一 item-authorization-to-product route；
- P11.7 继续拥有 current exact equipped weapon authorization；
- Combat Run 继续拥有 activation identity 与 sequence；
- P11.6 host 继续拥有 active guard lifecycle 与 timing policy；
- adapter 只采样并转发，不拥有 key mapping、clock、item、Run、host、balance、retry、Actor 或 Impact；
- 未改 GameMode、PlayerController、Enhanced Input、Tick、timer、RNG、ApplyDamage 或 inventory mutation；
- 未引入第二套 input registry、产品配置或 action lifecycle。

## 5. 修改范围

实现与门禁共 5 个文件、522 行新增：

- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapterTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

加入本 Report/Log 后 exact stage 为 7 个文件、802 行新增。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 文档与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `Gates`：blocked / unavailable 在取样前关闭，Run sequence 不变；
2. `Timeline`：missing / negative / overflow sample 失败，route 不执行；
3. `SingleSample`：取样一次、委托一次、opaque 值原样转发、拒绝不重试；
4. `ItemFence`：未装备武器仍由 P11.8 拒绝，input 层不越权；
5. `AppliedProof`：装备成功时 exact item 与 timeline 到达 host，item authority 不被修改，Run sequence 只消费一次。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardInputAdapter` | 5 | 0 | 1 | `E3926E2392EADE3C5594894C4D6F25A94556F34FF7707B54655A4EF15B708DC3` |
| `Shanmen.0_0_10` | 510 | 0 | 1 | `A81DB5597E7537E92796E72D2512DE8B9D5D45F2B01A2BB2260EE8B97F201904` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `BD38818E4B9D5C8EDDBE1221B2B72411252A513EF57DE55A9872C80ACC714311` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `E93CF7C84CCF8E856259B11268EDC7109F99AAB25CB2F5D6403E821F762F0E4B` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `5D6DA3D426B6B12F36BCDAD1D9BCFD79D75BD1606DB83DE7A5C90F4B860CA932` |

合计 591 Success / 0 Fail；该合计包含 focused 与 `Shanmen.0_0_10` 全量之间的重复覆盖。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=98
SELF_TEST: PASS 156/156
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=5
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`702FB2A97E1B9BEFFCBD168759057DFFC5261C6B8F6431EE746C6DC153EFAD2B`；
- self-test SHA-256：`BC594759AD044D369D7D327BF36CD50F39B74820264D9FCC869B7297FCEB4F62`；
- input-adapter rule 要求 full、focused adapter、P11.8 route、P11.7 item adapter、product authority/host、Combat Run、Items 与三组 legacy item/Hotbar evidence；
- 精确生产扫描未发现 GameMode、Actor、World、PlayerController、physical input binding、platform clock、timer、ApplyDamage、new GUID 或 Impact 调用。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 5 / 31.76s | 0 | `32558B7C9173E7FD9613CB0D1C05A5FF36BB2E71566F7113CDAE15A7CAE7A346` |
| Game Development | Succeeded | 4 / 28.20s | 0 | `E91940B5CED462A8E629387CE1D7CCB3B8E9B1A848003C59DBBA5D118216C7BB` |

产物：

- `UnrealEditor-demo_map.dll`：12,754,944 bytes，SHA-256 `EA9292BE1A39E62928DAE0E404BCB5F85EB59E7D85D8362844FD5EF5B48F2CDC`；
- `demo_map.exe`：354,289,152 bytes，SHA-256 `DA103B5DD2BF056E551B7B7533A89EA561607DE37316EC0AD8CF13C8020C67D9`。

## 9. 真实异常

- 产品实现、focused/full/legacy Automation、Editor 构建与 Game 构建均首次通过；没有源码失败、测试失败、构建重试或 Windows commit-memory/pagefile 错误。
- changed-file validator 首次由嵌套 `pwsh -File` 接收两个数组时发生参数展平，`MappingPath` 被误当作数组成员；改为当前 PowerShell 进程中的 hashtable splatting 后进入脚本本体。
- 第二次验证使用了不存在的推测日志名；改为磁盘上的 `Full.log` / `Hotbar.log` 后门禁通过。两次均为验证命令编排错误，未改产品代码、未重跑 Automation 或构建。
- UE SDK 检查仍打印与本目标无关的非 Win64 platform metadata invalid；Win64 SDK `10.0.22621.0` 有效，两个目标命令原生退出码均为 0。
- LF/CRLF 提示仅是 Git 工作树换行策略提示，`git diff --check` 无错误。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段应在既有统一玩家输入入口绑定真实 guard command，并明确选择项目唯一的 monotonic timeline source 与 tick 单位，再调用本 adapter。该阶段不得把物理按键、平台时钟、item 选择或 host lifecycle 反向塞入本 adapter。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-9-weapon-guard-input-adapter>
