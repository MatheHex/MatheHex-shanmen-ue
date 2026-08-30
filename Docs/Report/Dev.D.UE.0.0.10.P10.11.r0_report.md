# Dev.D.UE.0.0.10.P10.11.r0 Report

## 1. 结论

P10.11 **PASS**。Spirit Evasion 已进入统一 InputAction registry，默认物理键为当前无冲突的 `SpaceBar`，只绑定 `IE_Pressed`，并只调用 P10.10 的 `RouteSpiritEvasionStartInput()`。按键层没有创建 SpiritEnergy、资源事务、身份、移动或 fallback 逻辑。

输入配置格式由 Version 2 前向提升到 Version 3。旧配置缺少 Spirit Evasion 时，迁移会保留全部已有用户覆盖：默认键空闲则使用 `SpaceBar`；若 `SpaceBar` 已被旧覆盖占用，则为新动作确定性选择第一个仍空闲的 registry 默认键，不移动旧动作，也不产生重复键。

首次 focused 物理输入测试为 `5/6`：重映射后旧 `SpaceBar` 仍能触发。源代码核验确认 UE 5.8 的 `UInputComponent::ClearBindingsForObject` 只清理 `CachedKeyToActionInfo`，不会删除 raw `KeyBindings`。本阶段改为记录、验证并精确删除本 Controller 拥有的连续绑定区间；修正后 focused 与正式回归全部通过。

正式证据为 9 份日志、合计 `502` 条 Success（跨组包含重复覆盖）、`0` Fail；0.0.10 全量由 P10.10 的 `449` 增至 `455`。changed-file gate、136/136 mapping self-test、静态边界扫描、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 统一动作与默认键

- 新动作 ID：`Fdemo_mapInputActionIds::SpiritEvasion`；
- registry 精确动作数：`21 -> 22`；
- 默认键：`EKeys::SpaceBar`；
- `bRequiresReleasedEvent=false`；
- 显示名：`灵息闪避`，分类：`战斗`；
- `ValidateExactDefaults` 继续拒绝重复 ActionId、重复默认键和不完整定义。

当前 registry 与既有产品绑定中没有其他 `SpaceBar` 默认占用。

### 2.2 Version 3 前向迁移

`Fdemo_mapInputBindingSettings::MergeMissingDefaults` 由无结果写入改为 typed fail-closed 合并：

1. 已存在动作的用户键保持不变；
2. 缺失动作优先取得自身默认键；
3. 默认键冲突时，按 registry 固定顺序选择第一个未占用默认键；
4. 没有合法键时返回诊断并拒绝载入；
5. 合并后仍由 `ValidateBindings` 做完整动作、合法键与唯一键验证。

测试覆盖 Version 2 正常迁移，以及旧配置将 `Interact` 覆盖为 `SpaceBar` 的冲突迁移。后者保留 `Interact=SpaceBar`，为 Spirit Evasion 分配释放出的 `G`。

### 2.3 Press-only 产品路由

`BindProductInputActions` 增加且只增加一条 Spirit Evasion 绑定：

```text
current SpiritEvasion key + IE_Pressed
  -> PlayerController::StartSpiritEvasion
  -> P10.10 RouteSpiritEvasionStartInput
  -> shared gameplay gate
  -> authoritative GameMode product route
```

没有 `IE_Released`、Hold、Repeat 或双写旧技能入口。`StartSpiritEvasion` 只保存非 Shipping 自动化审计结果，不改变产品行为。

### 2.4 可靠 live remap

Controller 在初次绑定时记录：

- owning `UInputComponent`；
- product `KeyBindings` 起始下标；
- 连续绑定数量。

重建时先验证范围仍有效且每个 delegate 仍归本 Controller；任何范围或 ownership 不一致都记录错误并 fail closed。验证成功后只删除该精确区间，再重新绑定当前配置并 flush pressed keys，不删除无关组件或其他对象的 binding。

## 3. 完整性与兼容性

- 复用 P10.10 输入适配器和 P10.9 GameMode 产品路由；
- 复用现有 gameplay/UI/settlement input lock；
- 复用既有 `ApplyOverrideWithSwap` 与配置持久化；
- 不创建第二套 physical input registry；
- 不读取、预留或扣除 SpiritEnergy；
- 不创建 GUID、ordinal、Timer、Tick 或 retry；
- 不直接调用 `AddMovementInput`、`SetActorLocation`、`LaunchCharacter` 或 `TeleportTo`；
- 不修改 item/profile/schema/CodeB；
- 不改变 thrown weapon、普通攻击、旧技能、Back 或 UI 键位语义；
- V2RangedCompatibility `22/22`、P7Integration `9/9` 与精确 legacy input cases 全部通过。

## 4. 关键不变量

1. Spirit Evasion registry 动作恰好一项，默认键与其他默认键无冲突；
2. 物理输入恰好一条 Press binding，Release binding 为零；
3. 一次 Press 恰好调用 P10.10 route 一次；
4. Release 不产生第二次调用；
5. gameplay lock 在方向采样与产品路由之前拒绝；
6. GameMode 缺失保持 `ProductRouteUnavailable`，不得绕过 authority；
7. live remap 后旧键不再调用，新键立即调用；
8. 迁移不得移动或丢失旧用户覆盖；
9. 迁移不得产生重复键；
10. 无可用键时必须 fail closed；
11. remap 只删除本 Controller 已验证的精确 binding 区间；
12. physical handler 不拥有资源、身份、方向政策或移动逻辑。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput` 六个测试：

- `RegistryDefault`：22-action 精确 registry、SpaceBar、Press-only metadata；
- `VersionTwoMigration`：缺失动作取得空闲默认键且旧键保持；
- `MigrationConflict`：旧覆盖占用 SpaceBar 时保持旧覆盖并确定性分配 G；
- `PressOnlyBinding`：真实 Controller input stack 上 Press 一次、Release 零次；
- `LiveRemap`：SpaceBar -> C 后旧键失效、新键立即生效；
- `InputLock`：settlement lock 返回 GameplayBlocked，方向与产品 route 均未执行。

测试使用 transient `GamePreview` World 和真实 `PlayerInput` / `InputComponent` 绑定栈；它不发送操作系统级键盘事件，也不启动产品 UI。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionPhysicalInputTests.cpp`。

更新：

- `Source/demo_map/demo_mapInputActionRegistry.h/.cpp`；
- `Source/demo_map/demo_mapInputBindingSettings.h/.cpp`；
- `Source/demo_map/demo_mapPlayerController.h/.cpp`；
- `Source/demo_map/demo_mapFullSystemLoopTests.cpp`；
- `Source/demo_map/demo_mapP7IntegrationTests.cpp`；
- `Source/demo_map/demo_mapRuntimeInterfaceSliceTests.cpp`；
- `Source/demo_map/demo_mapInputRestoreTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

产品、测试与流程共 `13` 个文件，约 `538` insertions、`20` deletions；加入本 Report 与同名 Log 后 exact stage 为 `15` 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI 文档与用户资料未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.11-SpiritEvasionPhysicalInput-final.log` | focused physical input | 6 | 0 | `F03F1A921A464276D8E285ECB3F0B44D0F19D1DD3A0A4695F3B4085DCD6832F4` |
| `P10.11-SpiritEvasionInputAdapter-final.log` | P10.10 adapter | 6 | 0 | `2A7335F25F22D994CD09C9B08903EFC929EDE21C66FEA4A2AC6535F69E97454F` |
| `P10.11-Shanmen-0_0_10-final.log` | 0.0.10 full | 455 | 0 | `048566AD67BB4B5621ACE9A30BB9DF8799FF73D34091E0650F7458A7AB132183` |
| `P10.11-FullSystemRegistry-final.log` | FullSystemLoop.41 | 1 | 0 | `3C0D6AF45AE9A63AF2E74B55C962E1F513ADA2CAC68AA05F7B4A1321DBD9C602` |
| `P10.11-FullSystemRestore-final.log` | FullSystemLoop.47 | 1 | 0 | `C32EE2E36BAFDAEE1D8DB62310C3AB01C000901DA150075BD889B309FB1AFCAE` |
| `P10.11-P7Integration-final.log` | P7Integration | 9 | 0 | `8147907D45103E681F9C0289BBD2E3E53A7BB9F02B36E644C10EE8E9993EBBA8` |
| `P10.11-P5RuntimeInput-final.log` | P5RuntimeInterface.06 | 1 | 0 | `BB7D8A3CFEC77DCB930CDA1A418820EC2AB0FE2EC17ED812F731A7564CE433A0` |
| `P10.11-InputRestoreRegistry-final.log` | InputRestore.32 | 1 | 0 | `19904A033862E3014FDD3FFAE2660B4ADFDFDDC84133BF9E97012F27F1BD7261` |
| `P10.11-V2RangedCompatibility-final.log` | V2 compatibility | 22 | 0 | `77CC4B7D08C107498C74852E4A815683F1F47AF13D34A0490D40A23EC8374548` |

所有正式进程原生退出码均为 `0`，每份日志均有 native terminal-success 标记；selected Fail、fatal、unhandled 与 ensure 均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=88
SELF_TEST: PASS 136/136
REGRESSION_COVERAGE: PASS Changed=13 Rules=2 Required=10 Logs=9
git diff --check: PASS
SPIRIT_EVASION_PRESS_BINDINGS=1
SPIRIT_EVASION_RELEASE_BINDINGS=0
SPIRIT_HANDLER_FORBIDDEN_HITS=0
```

mapping SHA-256：`A5BC2267C83C9E685F869F25FA03352E5FDB150986DF36797C57218C1F340AAA`；self-test SHA-256：`7DC759F5274B81C0A4F1673DB93B04E7D57141309BF900EAD8953ABAB735345A`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 26 / 123.75s | 0 | `8C225C6460D0898A1A398D0F770056A3D8A093C97D5D8454C68A6A1F159B8BCF` |
| Editor post-fix | Succeeded | 15 / 54.85s | 0 | `C45EF77E0179716C44755AD1E073021FF780FD59959E9D0FA2F06166330A62C9` |
| Editor post-review | Succeeded | 15 / 52.96s | 0 | `9A4091B21F694A26801102BBD97779A5A4BA4ED46307CA1902C2E6CB2C88EDD9` |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 | `18E8822D65AFFFA6ED5F233065565596615FED4B5D14935EF988217C7B17C106` |
| Game final | Succeeded | 25 / 110.34s | 0 | `D3282550DECFD5360E27356A4E5B768F30B6D26B04FFA756C7EEA15CB419326F` |

最终 `UnrealEditor-demo_map.dll`：`12511744` bytes / SHA-256 `F41B20029BAFA0C71F9CB84CD403A12725C181D95D89A3AED00628CE96DF9C76`；`demo_map.exe`：`353951744` bytes / SHA-256 `C20F4928B19C133694F977B2827D300243E570D013BCA197CACA6BB8AC1AE5B9`。

## 9. 真实异常与审计发现

### 9.1 首次 focused failure

首次 `P10.11-SpiritEvasionPhysicalInput-focused.log` 为 `5 Success / 1 Fail`，SHA-256 `2DC0BB790BE74BCB9063A8642E17CEC7788C9D8EE8A7BBC5C3FF3517D04BF4CD`。失败为 `LiveRemap`：旧键调用计数仍增加，新键断言未成立。UE 进程原生退出仍为 `0`，说明不能只用进程码判断 selected Automation 成功。

引擎源码 `Engine/Source/Runtime/Engine/Private/Components/InputComponent.cpp:113` 显示 `ClearBindingsForObject` 仅遍历并删除 `CachedKeyToActionInfo`，没有删除 `KeyBindings`。精确 ownership-range 修正后，post-fix `6/6`，SHA-256 `0C6D0BEF01C5309AED3C94C1E762548E4DBBBF2449EC19900954866C35D97342`；最终日志再次 `6/6`。

### 9.2 Legacy whole-suite exploratory audit

为审计映射粒度，曾额外运行三个 legacy 整组：

- FullSystemLoop：`39 Success / 11 Fail`，失败集中于旧 schema、容量、materialization 与 preparation 断言；
- P5RuntimeInterface：`8 Success / 1 Fail`，旧断言仍期待 4 个 equipment cells，而产品当前为 5；
- InputRestore：`98 Success / 3 Fail`，涉及旧 automation plan/source-string 断言。

这些失败不位于本轮 registry、配置迁移或 physical route 逻辑。为避免把范围外旧债伪装成 P10.11 成功，也不修改无关产品/测试来强行转绿，changed-file map 对本轮实际改动的 legacy assertions 使用精确测试路径，并保留 0.0.10 全量、P7 全组与 V2 全组覆盖。相关精确用例全部通过。

九次最终 UnrealEditor-Cmd 启动阶段仍出现引擎自带 `UE::UnifiedErrorTest` 的 13 行初始化噪声。它发生在 selected suite 之前；随后 selected tests 均 Success、Fail 为 0、native terminal exit 为 0，原始日志未删改。

没有源码编译失败、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、transient World 无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、操作系统级真实输入、截图、Smoke、Cook 或 Package。

下一步建议进入一次 F 阶段实机验收：确认 SpaceBar 默认键、设置页重绑定、UI/settlement lock 与实际闪避方向/手感；该阶段只验证现有链路，不在按键 handler 临时增加 SpiritEnergy。若继续 P 阶段，则应先建立唯一 SpiritEnergy authority 与事务契约，再把成本接入 P10.8/P10.9 产品层，而不是接入物理输入层。
