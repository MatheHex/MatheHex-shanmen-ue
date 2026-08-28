# Dev.D.UE.0.0.10.P3.0.r0 Development Log

## 目标

在 P2.1 稳定 Entity／Emission 身份之后建立 GAS 编排边界，但不让 GAS 取得伤害数学、世界查询、目标身份或库存权威。P3.0 只解决动作何时处于 Startup／Active／Recovery、何时跨提交点、何时允许候选进入后续纵切。

## 设计决策

### GAS 是调度者，不是阶段真值

Montage notify、AbilityTask、timer 和网络回调都可能重复或迟到，因此它们不能直接写阶段。调用方必须携带 `ExpectedPhase` 请求转换；纯 `FShanmenActionOrchestrator` 比对当前状态后才生成 receipt。拒绝不会消耗 sequence，也不会修改提交事实。

### 唯一提交点

`Startup -> Active` 是唯一提交边：

- Startup 取消意味着没有提交；
- Active 开始后 commit fact 永久保留；
- Active／Recovery 的外部终止必须是 Interrupted，不能伪装成可回滚 Cancelled；
- Active 是唯一允许 P2 detector session 发射候选的阶段。

CombatRuntime 只报告 commit fact，不直接调用 `ShanmenItems`、生命 Adapter 或伤害 API。后续协调器根据 receipt 完成跨模块提交。

### GAS 基类保持抽象

`UShanmenCombatGameplayAbility` 不可直接授予，固定 InstancedPerExecution，并把合法转换转发为 Blueprint 观察事件。P3.1 的具体 Ability 负责捕获权威 ActionSnapshot 和调度阶段；基类不读取 Avatar／World，避免在基础层烧死角色结构。

## 实现与复核过程

1. 新增 `ShanmenCombatRuntime` 模块、Target／uproject 注册并启用 GameplayAbilities 插件。
2. 新增 Action transition receipt、纯阶段 orchestrator、GAS 抽象基类与 `Shanmen.Ability.Combat.Action` native tag。
3. 首次 Editor 编译含 UHT、新模块与测试共 13/13 actions，成功。
4. 首次定向测试 4/4、首次全量 80/80；首次 Game 8/8 actions，成功。
5. 源码复核移除尚未被 runtime 消费的阶段 tags，避免产生惰性公共 API；同时修正 `TryStart(Runtime.GetAction(), Runtime, ...)` 的输入／输出 alias：先复制 frozen Action 再清空 output，并补自动化断言。
6. 最终 Editor 增量 7/7、定向 4/4、全量 80/80、Game 增量 6/6，全部原生退出码 0。

整个过程没有产品源码失败、测试 fail 或 handled ensure。

## 最终自动化

### CombatRuntime 定向

- filter：`Shanmen.0_0_10.CombatRuntime`；
- found／started：4；
- result：4 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- SHA-256：`CFDBEC869C0A04B92B128A98983B7A3C8C88DE5F5B25929CCD1DD77BE0384C5C`。

### 0.0.10 全量

- filter：`Shanmen.0_0_10`；
- found／started：80；
- result：80 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- SHA-256：`C289A7224CC6AF98D2E43EC1644C62DF1FEEFF1B4046CB65FDE8D6FA8EF5FFEE`。

两份最终日志各含 UE 5.8 在 test discovery 前打印的既有 13 条 `Condition failed` 启动诊断；目标测试均在其后成功。平台验证中的 LinuxArm64／VisionOS metadata 缺失不影响 Win64 VALID 目标。

## 最终不变量

1. 每个 frozen Activation 只有一个独立 runtime。
2. 合法主链固定为 Startup／Active／Recovery／Completed。
3. 过期 ExpectedPhase 请求失败且不消费 sequence。
4. Startup -> Active 是唯一 commit crossing。
5. 只有 Active 可以发射候选。
6. 提交前 Cancelled 与提交后 Interrupted 在 receipt 中不可混淆。
7. terminal runtime 不能继续推进。
8. 相同输入与相同事件序列产生相同 receipts。
9. GAS 不执行 World query、伤害、生命或物品修改。
10. 旧 `demo_map` 产品行为保持不变。

## 静态与构建证据

- `git diff --check`：0；
- Editor：首次 13/13、最终 7/7，均成功；
- Game：首次 8/8、最终 6/6，均成功；
- 产品源边界扫描：旧模块、World／Actor、damage、Items、WorldGameplay、RNG、LineTrace／SweepMulti／OverlapMulti 全部 0。

## P/F 边界

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；未触碰 unrelated 未跟踪文件。
