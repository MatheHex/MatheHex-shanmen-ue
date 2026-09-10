# Dev.D.UE.0.0.10.P24.6.r0 Report

## 1. 结论

P24.6 已把 P24.5 的真实剑气发射阻挡原因送达玩家 HUD。

当剑气因发射点被占用，或玩家到发射点之间存在阻挡而被拒绝时，HUD 现在显示：

`SWORD QI · LAUNCH BLOCKED`

提示沿用现有 2.25 秒剑气输入反馈与琥珀色失败语义。发射、碰撞、伤害、输入、重试和动作规则均未改变，也没有增加新的 Manager、Subsystem、Actor 或反馈运行时。

## 2. 玩家价值

P24.5 已能安全阻止墙内生成和隔墙发射，但此前 HUD 把精确的 `LaunchPathBlocked` 折叠为普通 `SWORD QI · UNAVAILABLE`。玩家无法判断是站位受阻，还是系统、装备或属性不可用。

本阶段让玩家获得可行动的反馈：调整站位、离开墙面后即可再次发射，而无需猜测装备或系统状态。

## 3. 现有链路复用

底层错误继续由既有链路携带：

`WorldAdapter → RunHost → ProductSession → ProductController → InputAdapter → CommandEventOwner → AvailabilityCommandRouter → PlayerController → HUD`

没有复制路由，也没有新增中间包装层。原本位于 HUD 文件内部的纯文本投影函数被公开为 `Ademo_mapHUD::TryBuildSwordQiFeedback()`，供实际 HUD 和无头测试共同调用。

## 4. 精确类型门禁

专用提示只有在下列完整结果链同时成立时才出现：

- Availability：`Dispatched`；
- CommandEvent：`InputRejected`；
- Input：`ProductRejected`；
- ProductController：`RouteRejected`；
- ProductSession：`LaunchRejectedInterrupted`；
- RunHost：`LaunchRejected`；
- WorldAdapter：`LaunchPathBlocked`；
- HostStart 未开始。

任何一层不匹配都不能声称路径受阻。本轮测试把 RunHost 错误改为 `AdoptionRejected` 后，HUD 按既有规则回退为 `SWORD QI · UNAVAILABLE`，证明专用提示不会由字符串、摘要或不完整证据触发。

## 5. 真实碰撞验证

升级现有 `SwordQiWorldDelivery.LaunchCorridorGate`：

1. 在启用 physics scene 与 trace collision 的临时 World 中，把真实 `UBoxComponent` 障碍放到剑气发射点；
2. WorldAdapter 返回实际 `LaunchPathBlocked`；
3. 将该真实结果嵌入完整上层类型链；
4. HUD 输出精确文本与琥珀色；
5. 破坏其中一层后，专用文本不再出现；
6. 原有载体 inert、薄墙 sweep、清路恢复与安全取消断言继续通过。

聚焦 `SwordQiWorldDelivery` 为 4/0。

## 6. 改动文件驱动回归

3 个非文档改动路径命中 `MainHUDTrajectoryPresentation` 与 `SwordQiWorldDelivery` 两条映射规则，共要求 23 个独立测试组。

覆盖结果：

`REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=23 Logs=23`

- 映射测试总计：400/0；
- 输入恢复：101/0；
- V2 远程兼容：22/0；
- CombatCore：9/0；
- CombatRuntime：146/0；
- Sword Qi 物理输入：5/0；
- Sword Qi 世界交付：4/0；
- 其余 17 个 HUD、神识、受控武器与投掷物组：113/0。

覆盖日志 SHA-256：`2946DB997BFB3E40B54E9A60F6CE7C7124BACF3AA4276723E5D02ED6DCCC7559`。

覆盖器自检：442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 7. 构建证据

| Evidence | Result | Actions | SHA-256 |
|---|---|---:|---|
| `P24.6_EditorBuild_initial.log` | Succeeded / native 0 | 7 | `7CE81E043182ADB71C19B932B128D5DD0B5F02C9D94E9CE0838247CA142A85DC` |
| `P24.6_EditorBuild_final.log` | Succeeded / native 0 | 0 / up to date | `D7F98335CA43B7E7F1ECB6550037183A46298C9329664F5B88330BE0D9542AB1` |
| `P24.6_GameBuild_final.log` | Succeeded / native 0 | 6 | `AE7B8F3FE89A0671E7A8B8C9015516002471DFAA0DA9D845C530CEE5EE0EF0B2` |

最终二进制：

- `demo_map.exe`：359,708,160 bytes，SHA-256 `774D33AD6C4C770B7362BB0CAC675DF32C4A3AD0429967D474120CAD37CB8A4A`；
- `UnrealEditor-demo_map.dll`：18,974,208 bytes，SHA-256 `8F26C2F7214F07AE503FB3525441EE7EA10775206958B7158EFFA15F7BF38FF8`。

## 8. 静态边界

- 非文档增量：3 files，`+118/-49`；
- 49 个删除行来自把既有匿名 HUD helper 移到类静态方法，原有分支保持不变；
- `git diff --check`：native 0；
- 生产新增行对 Timer、RNG、`ApplyDamage`、`SpawnActor`、`Destroy`、Manager、Subsystem：0 命中；
- 没有新增 UCLASS/USTRUCT、Actor、输入动作、存档字段、Config 或 Content 资产；
- 验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：真实碰撞错误能通过完整既有类型链投影为明确 HUD 文本；不完整链路不能冒充路径受阻；原有 Sword Qi、输入、HUD、远程兼容与世界玩法回归 400/0；覆盖门禁、自检和双目标构建通过。

未声明：真实关卡中的字体可读性、持续时间、屏幕遮挡、语言本地化或玩家手感已经验收。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub 交接

基线提交：`6a9369bc214f2b04821d9ac5937b4479a67fcc05`。

工作分支：`agent/0.0.10-p24-6-sword-qi-launch-feedback`。

本阶段仅提交 2 个 HUD 生产文件、1 个既有 WorldAdapter 测试文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.6` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-6-sword-qi-launch-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-6-sword-qi-launch-feedback/Docs/Report/Dev.D.UE.0.0.10.P24.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-6-sword-qi-launch-feedback/Docs/Log/Dev.D.UE.0.0.10.P24.6.r0_log.md>
