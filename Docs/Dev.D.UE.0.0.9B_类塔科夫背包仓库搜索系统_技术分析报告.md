# Dev.D.UE.0.0.9B  
## 《山门》类塔科夫背包、仓库与搜索系统技术分析报告

---

## 一、项目信息卡

- **内部名称：** 0.0.9B 类塔科夫物品管理框架
- **项目编号：** `Dev.D.UE.0.0.9B`
- **文件性质：** 技术分析报告
- **开发基线：** `Dev.D.UE.0.0.8` 稳定镜像
- **原0.0.9处理：** 原0.0.9开发方向与实现不作为0.0.9B合并基础；仅在后续明确确认后，才可选择性复用其中个别成果
- **核心目标：** 建立统一的背包、仓库、装备、尸体搜索、容器搜索、物品信息、拖拽交互与撤离结算框架
- **当前优先形态：** 所有物品采用单格方块展示
- **未来兼容方向：** 多格物品、旋转、嵌套容器、重量、专用收纳、多人权威
- **说明：** 本文件是技术分析与架构建议，不是一次性执行Prompt，也不限定唯一实现路径

---

# 二、执行摘要

0.0.9B的重点不应被理解为“制作一个类似《逃离塔科夫》的网格UI”。

真正需要建立的是一套统一的物品权威：

> **每一件物品只有一个真实实例、一个真实位置、一个统一修改入口，并能在仓库、装备栏、玩家储物、尸体、普通容器、地面和撤离结算之间稳定转移。**

建议将系统拆分为以下层级：

```text
物品定义
Item Definition
        ↓
物品实例
Item Instance
        ↓
装备栏与容器实例
Equipment / Container Instance
        ↓
统一物品事务服务
Inventory Transaction Service
        ↓
搜索、世界掉落、快捷栏、撤离与存档
        ↓
UI表现
```

其中：

- **物品定义**描述“这种物品是什么”；
- **物品实例**描述“玩家实际拥有的这一件物品”；
- **容器系统**描述物品当前在哪里；
- **事务系统**统一处理移动、交换、装备、堆叠、拆分和丢弃；
- **搜索系统**控制玩家何时能够看到和拿取物品；
- **Run与结算系统**决定物品是否成功带回宗门；
- **UI**只负责展示和提交操作，不作为物品数据权威。

---

# 三、版本目标与范围

## 3.1 主要目标

0.0.9B建议实现：

1. 局外仓库；
2. 备战人物装备与储物页面；
3. 局内背包；
4. 普通容器搜索；
5. 敌人尸体搜索；
6. 玩家与敌方实体的统一装备布局；
7. 方格物品展示；
8. 物品详情页面；
9. 拖拽转移；
10. 拖拽装备与卸下；
11. 拖拽交换；
12. 可堆叠物品合并与拆分；
13. 拖拽丢弃和世界拾取；
14. 基础6格快捷物品区；
15. 1—9快捷使用栏引用；
16. 空间道具及其内部储物；
17. 容器开启读条；
18. 单件物品搜索与识别；
19. 灵石独立货币领取；
20. 撤离、死亡和存档中的唯一物品结算。

## 3.2 当前不必优先完成

以下能力可以保留接口，但不建议成为0.0.9B第一轮的主要开发压力：

- 多格不规则物品；
- 物品旋转；
- 无限层级“包中包”；
- 复杂重量系统；
- 装备耐久和维修；
- 自动整理高级算法；
- 多人服务器同步；
- 保险与尸体追回；
- 完整分类收纳箱体系；
- 大规模仓库筛选和搜索工具。

## 3.3 版本成功标准

0.0.9B的核心成功标准是：

```text
同一件真实物品
能够从地图容器或尸体
经过搜索、转移、装备、携带和撤离
最终进入局外仓库
且GUID、数量、属性、词条和容器关系保持一致。
```

---

# 四、现有《山门》物品结构要求

## 4.1 玩家装备区域

玩家当前建议拥有：

| 区域 | 数量 | 规则 |
|---|---:|---|
| 兵器栏 | 1 | 0—1件兵器 |
| 道袍栏 | 1 | 0—1件道袍 |
| 空间道具栏 | 1 | 0—1件空间道具 |
| 饰品栏 | 可配置 | 每栏0—1件饰品 |
| 基础快捷物品区 | 固定6格 | 无需装备空间道具也存在 |
| 空间道具储物区 | 6—30格 | 容量由已装备空间道具决定 |
| 1—9快捷使用栏 | 9个引用位 | 不提供额外储物空间 |

统一规则：

- 兵器、道袍、空间道具和饰品均不可堆叠；
- 一个装备栏只允许一件对应装备；
- 多个饰品来自多个独立饰品栏；
- 空间道具本体是装备；
- 空间道具内部拥有独立容器；
- 基础6格与1—9快捷使用栏是两个不同系统。

## 4.2 敌方实体

敌人尸体建议使用与玩家相近的实体结构：

```text
敌方实体
├─ 兵器栏
├─ 道袍栏
├─ 饰品栏
├─ 空间道具栏
├─ 基础6格物品区
├─ 空间道具内部储物
└─ 身体容器
```

身体容器可出现：

- 内丹；
- 血肉；
- 魂骨；
- 灵骨；
- 道骨；
- 后续其他身体材料。

敌人装备栏每栏只刷新0—1件对应装备。敌人储物区内可以出现额外未装备物品，但这些物品不能错误显示为第二件已装备道袍或第二件已装备兵器。

---

# 五、推荐总体技术架构

## 5.1 逻辑分层

```text
UItemDefinition / Data Asset
        ↓
FItemInstance
        ↓
FContainerInstance / FEquipmentSlotInstance
        ↓
UInventoryRepositorySubsystem
        ↓
UInventoryTransactionService
        ↓
UInventoryComponent / ULootContainerComponent
        ↓
UMG Widgets / World Item Actors
```

## 5.2 各层职责

### 物品定义层

负责：

- 名称；
- 图标；
-描述；
- 物品标签；
- 装备类型；
- 是否可堆叠；
- 最大堆叠；
- 默认尺寸；
- 允许进入的容器；
- 基础价值；
- 默认属性；
- 世界模型和展示资源。

### 物品实例层

负责：

- 唯一GUID；
- 数量；
- 等级；
- 随机词条；
- 随机种子；
- Jackpot状态；
- 实际生成价值；
- 空间道具内部容器引用；
- 当前实例状态。

### 容器层

负责：

- 容器唯一ID；
- 容器类型；
- 格子容量；
- 当前物品位置；
- 允许标签；
- 搜索状态；
- 是否已经生成奖励；
- 是否属于玩家、敌人、世界或局外仓库。

### 事务层

负责：

- 移动；
- 交换；
- 合并；
- 拆分；
- 装备；
- 卸下；
- 丢弃；
- 拾取；
- 快捷栏绑定；
- 消耗；
- 操作合法性；
- 原子提交；
- 错误结果；
- 事务日志。

### UI层

负责：

- 格子和物品图标；
- 拖拽视觉；
- 合法目标高亮；
- 物品详情；
- 搜索读条；
- 快捷栏显示；
- 数量变化；
- 错误提示；
- 页面切换。

UI不直接决定物品最终位置。

---

# 六、物品数据模型

## 6.1 物品定义

建议使用`UPrimaryDataAsset`或同等数据资产描述物品类型。

示例：

```cpp
UCLASS(BlueprintType)
class UItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly)
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly)
    FText Description;

    UPROPERTY(EditDefaultsOnly)
    FGameplayTagContainer ItemTags;

    UPROPERTY(EditDefaultsOnly)
    FIntPoint GridSize = FIntPoint(1, 1);

    UPROPERTY(EditDefaultsOnly)
    bool bCanRotate = false;

    UPROPERTY(EditDefaultsOnly)
    bool bStackable = false;

    UPROPERTY(EditDefaultsOnly)
    int32 MaxStack = 1;

    UPROPERTY(EditDefaultsOnly)
    int32 BaseValue = 0;

    UPROPERTY(EditDefaultsOnly)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditDefaultsOnly)
    TSoftObjectPtr<UStaticMesh> WorldMesh;

    UPROPERTY(EditDefaultsOnly)
    FGameplayTagQuery AllowedContainerQuery;
};
```

当前可以统一使用：

```text
GridSize = 1×1
bCanRotate = false
```

但保留字段，以便未来扩展多格物品。

## 6.2 物品实例

示例：

```cpp
USTRUCT(BlueprintType)
struct FItemInstance
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid ItemId;

    UPROPERTY(SaveGame)
    FPrimaryAssetId DefinitionId;

    UPROPERTY(SaveGame)
    int32 Quantity = 1;

    UPROPERTY(SaveGame)
    int32 Level = 1;

    UPROPERTY(SaveGame)
    int32 RandomSeed = 0;

    UPROPERTY(SaveGame)
    int32 GeneratedValue = 0;

    UPROPERTY(SaveGame)
    bool bJackpot = false;

    UPROPERTY(SaveGame)
    TArray<FAffixInstance> Affixes;

    UPROPERTY(SaveGame)
    FGuid NestedContainerId;
};
```

两件同名装备可以共享一个定义，但需要拥有不同GUID、词条和价值。

## 6.3 物品位置

```cpp
USTRUCT(BlueprintType)
struct FItemPlacement
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid ItemId;

    UPROPERTY(SaveGame)
    int32 X = 0;

    UPROPERTY(SaveGame)
    int32 Y = 0;

    UPROPERTY(SaveGame)
    bool bRotated = false;
};
```

当前单格系统中，`X`和`Y`仍然值得保留，未来可以直接扩展多格布局。

---

# 七、容器模型

## 7.1 容器实例

```cpp
USTRUCT(BlueprintType)
struct FContainerInstance
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid ContainerId;

    UPROPERTY(SaveGame)
    FGameplayTagContainer ContainerTags;

    UPROPERTY(SaveGame)
    FIntPoint GridSize;

    UPROPERTY(SaveGame)
    TArray<FItemPlacement> Placements;

    UPROPERTY(SaveGame)
    bool bRewardsMaterialized = false;

    UPROPERTY(SaveGame)
    int32 RewardSeed = 0;
};
```

## 7.2 容器类型建议

```text
Container.Player.Equipment
Container.Player.BaseQuick
Container.Player.Space
Container.Player.Stash

Container.Enemy.Equipment
Container.Enemy.BaseStorage
Container.Enemy.Space
Container.Enemy.Body

Container.World.Generic
Container.World.Wood
Container.World.Ore
Container.World.HighValue
Container.World.Ground
```

## 7.3 固定装备栏

装备栏可采用独立结构：

```cpp
USTRUCT(BlueprintType)
struct FEquipmentSlotInstance
{
    GENERATED_BODY()

    FGameplayTag SlotTag;
    FGuid EquippedItemId;
};
```

示例标签：

```text
Slot.Equipment.Weapon
Slot.Equipment.Robe
Slot.Equipment.SpaceItem
Slot.Equipment.Accessory
```

---

# 八、Gameplay Tags建议

标签用于表达“什么可以进入哪里”，不要由UI写死。

## 8.1 物品标签

```text
Item.Equipment.Weapon
Item.Equipment.Robe
Item.Equipment.Accessory
Item.Equipment.SpaceItem

Item.Consumable.Pill

Item.Material.Wood
Item.Material.Ore

Item.Body.Flesh
Item.Body.Bone
Item.Body.InnerCore

Item.Property.Stackable
Item.Property.QuickUsable
Item.Property.JackpotEligible
```

## 8.2 容器标签

```text
Container.Player.Quick
Container.Player.Space
Container.Player.Stash
Container.Corpse.Body
Container.Corpse.Space
Container.World.WoodBox
```

## 8.3 基础合法性示例

| 目标区域 | 合法条件 |
|---|---|
| 兵器栏 | `Item.Equipment.Weapon` |
| 道袍栏 | `Item.Equipment.Robe` |
| 空间道具栏 | `Item.Equipment.SpaceItem` |
| 饰品栏 | `Item.Equipment.Accessory` |
| 身体容器奖励池 | `Item.Body.*` |
| 木材箱奖励池 | `Item.Material.Wood` |
| 快捷栏绑定 | 位于基础6格且拥有`Item.Property.QuickUsable` |

---

# 九、统一物品事务系统

## 9.1 事务类型

```cpp
UENUM()
enum class EInventoryOperation : uint8
{
    Move,
    Swap,
    Merge,
    Split,
    Equip,
    Unequip,
    Drop,
    PickUp,
    BindHotbar,
    UnbindHotbar,
    Consume
};
```

## 9.2 移动请求

```cpp
USTRUCT()
struct FMoveItemRequest
{
    GENERATED_BODY()

    FGuid TransactionId;
    FGuid ItemId;

    FGuid SourceContainerId;
    FGuid TargetContainerId;

    FIntPoint TargetPosition;

    int32 Quantity = 1;
    bool bRotated = false;
};
```

## 9.3 推荐执行流程

```text
UI提交请求
→ 验证ItemId存在
→ 验证来源位置
→ 验证数量
→ 验证目标容器
→ 验证标签
→ 验证目标格和容量
→ 验证装备栏
→ 验证搜索状态
→ 验证Run状态
→ 预计算完整结果
→ 原子提交
→ 广播变化
→ UI刷新
```

## 9.4 原子操作

所有会影响多个对象的操作都建议先完整验证，再一次性提交。

例如装备交换：

```text
新兵器可以进入兵器栏
AND
旧兵器可以进入来源容器
AND
来源容器有空间
→ 同时完成交换
```

避免先删除旧装备，再发现新装备无法完成操作。

## 9.5 错误结果

```cpp
UENUM()
enum class EInventoryError : uint8
{
    None,
    ItemNotFound,
    SourceMismatch,
    TargetFull,
    InvalidSlot,
    TagRejected,
    InvalidQuantity,
    ItemLocked,
    SearchIncomplete,
    ContainerCycle,
    RunEnding
};
```

UI可以根据错误码显示：

- 空间不足；
- 不能装备到这里；
- 尚未搜索；
- 物品正在使用；
- 当前正在撤离；
- 目标已经改变。

---

# 十、拖拽交互

## 10.1 Drag Payload

拖拽对象应携带稳定ID，不只携带Widget引用。

```cpp
UCLASS()
class UInventoryDragOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    FGuid ItemId;
    FGuid SourceContainerId;
    FIntPoint SourcePosition;
    int32 Quantity;
};
```

## 10.2 拖拽过程

```text
OnDragDetected
→ 创建拖拽视觉
→ 保存ItemId和来源容器
→ 鼠标进入目标区域
→ 显示合法性预览
→ OnDrop
→ 提交事务请求
→ 成功后刷新
→ 失败后恢复原视觉
```

## 10.3 视觉反馈

建议：

- 绿色：可放置；
- 黄色：可交换或合并；
- 红色：不合法；
- 装备槽高亮：可装备；
- 丢弃区高亮：可丢弃；
- 搜索未完成：锁定或未知表现。

UI预览可以快速判断，但最终结果仍由事务服务确认。

---

# 十一、具体物品操作

## 11.1 移动

支持：

- 仓库 ↔ 玩家；
- 基础6格 ↔ 空间道具；
- 玩家 ↔ 普通容器；
- 玩家 ↔ 尸体；
- 装备栏 ↔ 储物区；
- 玩家 ↔ 地面。

## 11.2 交换

当目标位置已有物品时：

```text
检查A是否能进入B的位置
检查B是否能进入A的位置
两边均成立
→ 原子交换
```

## 11.3 合并堆叠

```cpp
MoveAmount =
Min(SourceQuantity, TargetMaxStack - TargetQuantity);
```

合并后：

- 目标数量增加；
- 来源数量减少；
- 来源归零时删除来源实例；
- 快捷栏引用需要同步处理。

## 11.4 拆分

示例：

```text
灵木×20
→ 拆分6
→ 原实例×14
→ 新实例×6
```

新堆叠需要新GUID。

## 11.5 装备

装备时建议检查：

- 标签与槽位匹配；
- 槽位是否存在；
- 当前状态是否允许换装；
- 原装备能否安全移动；
- 空间道具内容是否完整保留。

## 11.6 丢弃

丢弃不等于删除。

```text
玩家容器中的物品实例
→ 转移到世界容器
→ 创建世界拾取Actor
→ Actor引用原ItemId
```

世界Actor负责展示和交互，物品真实数据继续由物品系统维护。

---

# 十二、空间道具技术要求

## 12.1 基本关系

```text
空间道具ItemInstance
NestedContainerId
        ↓
空间道具内部ContainerInstance
```

## 12.2 容量

空间道具当前可以提供：

```text
6—30格
```

具体容量由物品定义决定。

## 12.3 装有物品时移动

推荐允许空间道具连同内部内容整体移动。

适用场景：

- 装备；
- 卸下；
- 放入仓库；
- 丢到地面；
- 尸体中拿取；
- 弃置空间道具撤离；
- 重新拾取。

## 12.4 嵌套风险

第一阶段建议不开放空间道具中再存放空间道具。

原因：

- 防止无限嵌套；
- 防止容器循环；
- 降低UI复杂度；
- 简化保存和恢复；
- 简化空间价值计算。

数据结构可以保留未来嵌套能力。

---

# 十三、基础6格与1—9快捷使用栏

## 13.1 基础6格

基础6格是真实物品容器：

```text
Container.Player.BaseQuick
```

特点：

- 无需空间道具；
- 永久存在；
- 物品真实占格；
- 可使用消耗品可从这里绑定快捷栏。

## 13.2 1—9快捷栏

快捷栏不是容器，只保存引用：

```cpp
USTRUCT()
struct FHotbarBinding
{
    int32 SlotIndex;
    FGuid ItemId;
};
```

## 13.3 绑定流程

```text
点击消耗品
→ 打开操作菜单
→ 选择“装备到快捷栏”
→ 弹出1—9选择UI
→ 选择位置
→ 保存ItemId引用
```

## 13.4 使用流程

```text
按下快捷键
→ 找到ItemId
→ 检查物品存在
→ 检查仍位于基础6格
→ 检查数量
→ 检查可使用状态
→ 应用物品效果
→ 数量减1
→ 更新快捷栏
→ 数量归零后解除绑定
```

## 13.5 同步要求

以下情况发生后，快捷栏应立即同步：

- 物品被移动；
- 物品被丢弃；
- 物品被出售；
- 堆叠被合并；
- 堆叠被拆分；
- 数量归零；
- Run结束；
- 存档重载。

---

# 十四、搜索系统

## 14.1 容器开启状态

```cpp
UENUM()
enum class EContainerSearchState : uint8
{
    Closed,
    Opening,
    Open,
    Interrupted
};
```

流程：

```text
玩家交互
→ 开启读条
→ 打开容器UI
```

## 14.2 单件物品识别

```cpp
UENUM()
enum class EItemRevealState : uint8
{
    Hidden,
    Searching,
    Revealed
};
```

流程：

```text
未知物品格
→ 开始单件搜索
→ 搜索读条
→ 显示图标、名称、数量和详情
```

## 14.3 尸体搜索

建议直接显示：

- 兵器；
- 道袍；
- 饰品；
- 空间道具本体。

建议需要搜索：

- 基础6格内部物品；
- 空间道具内部物品；
- 身体容器；
- 内丹、血肉和骨类物品。

## 14.4 搜索中断

可由以下情况中断：

- 玩家关闭页面；
- 离开交互距离；
- 玩家死亡；
- 目标被销毁；
- 当前Run结束；
- 玩家主动取消。

是否受到攻击立即中断，可以作为策划配置项。

---

# 十五、奖励生成与搜索分离

奖励系统负责创建真实物品，搜索系统只控制是否对玩家可见。

推荐流程：

```text
敌人死亡或容器首次物质化
→ Reward Generation System生成真实Item Instances
→ 放入真实Container Instance
→ UI将未搜索物品显示为未知状态
→ 玩家完成搜索后逐步揭示
```

不建议每次打开容器时重新抽取奖励。

对于大量地图容器，可以采用延迟物质化：

```text
容器保存固定Seed
→ 第一次打开时生成奖励
→ bRewardsMaterialized = true
→ 后续重新打开读取原结果
```

---

# 十六、灵石处理

灵石继续作为独立货币：

- 不占物品格；
- 不进入基础6格；
- 不进入空间道具；
- 在商城和宗门UI中显示余额；
- 敌人可以必定掉落小范围随机数量。

尸体可记录：

```cpp
int32 SpiritStoneAmount;
bool bSpiritStoneClaimed;
```

领取后：

```text
玩家灵石余额增加
→ bSpiritStoneClaimed = true
```

重新打开尸体时不能重复领取。

---

# 十七、UI架构

## 17.1 推荐Widget结构

```text
WBP_InventoryRoot
├─ WBP_PlayerEntityPanel
├─ WBP_TargetEntityPanel
├─ WBP_EquipmentPanel
├─ WBP_GridContainer
├─ WBP_GridCell
├─ WBP_ItemTile
├─ WBP_ItemDetail
├─ WBP_ContextMenu
├─ WBP_SplitStackDialog
├─ WBP_DropConfirmDialog
├─ WBP_SearchProgress
└─ WBP_HotbarSelection
```

## 17.2 局外备战页面

建议显示：

- 玩家人物或轮廓；
- 全部装备栏；
- 基础6格；
- 空间道具储物；
- 1—9快捷栏；
- 局外仓库；
- 当前攻击、生命、减伤和容量摘要。

## 17.3 局内背包

左侧显示玩家完整实体。

右侧可以根据状态显示：

- 当前地面附近物品；
- 详情区域；
- 空白辅助区域；
- 当前目标容器。

## 17.4 搜索UI

左侧：

- 玩家完整装备和储物。

右侧：

- 普通容器；
- 资源箱；
- 尸体实体；
- 身体容器；
- 搜索状态。

## 17.5 物品格显示

格子中只显示高频信息：

- 图标；
- 数量；
- 等级或品质；
- 快捷栏标记；
- 搜索状态；
- 新获得标记；
- Jackpot或特殊标记。

## 17.6 物品详情页

详情页面可以显示：

- 大图标；
- 名称；
- 类型；
- 等级；
- 稀有度；
- 描述；
- 数量；
- 基础属性；
- 随机词条；
- 使用效果；
- 出售价值；
- 来源信息；
- 可执行操作。

---

# 十八、存档设计

## 18.1 局外Profile存档

建议保存：

- 灵石余额；
- 宗门资源；
- 仓库容器；
- 玩家装备；
- 基础6格；
- 空间道具及内部容器；
- 1—9快捷栏引用；
- 长期物品实例；
- 城镇和其他宗门状态。

## 18.2 Run存档

建议保存：

- RunId；
- 地图Seed；
- 玩家当前携带物；
- 地图容器状态；
- 尸体状态；
- 搜索状态；
- 灵石领取状态；
- Boss状态；
- 撤离状态；
- 当前世界物品。

## 18.3 不建议保存

- Widget引用；
- DragOperation；
- 鼠标悬停；
- 临时UI坐标；
- 裸Actor指针；
- 临时动画状态。

## 18.4 存档版本

建议保存：

```cpp
int32 SaveVersion;
```

数据结构变化时，可以执行：

```text
读取旧版本
→ 迁移字段
→ 补充默认值
→ 检查物品不变量
→ 保存为新版本
```

---

# 十九、撤离、死亡与唯一结算

## 19.1 Run终局状态

```cpp
UENUM()
enum class ERunEndState : uint8
{
    Active,
    Extracting,
    Extracted,
    Dead,
    Abandoned
};
```

只有`Active`可以提交终局。

## 19.2 撤离

```text
完成撤离条件
→ 冻结携带状态
→ 创建结算快照
→ 转移合法物品到Profile
→ 提交资源和灵石
→ 保存
→ 标记Extracted
```

## 19.3 死亡

```text
玩家死亡
→ 冻结携带状态
→ 按死亡规则处理物品
→ 提交一次
→ 标记Dead
```

## 19.4 关键风险

系统需要避免：

- 撤离完成瞬间死亡导致双结算；
- 地图卸载重复触发结算；
- 保存回调再次提交；
- 同一件物品同时进入仓库和尸体；
- 弃置空间道具后内部物品被复制；
- Run结束后仍允许拖拽。

---

# 二十、性能与资源管理

## 20.1 数据优先

仓库和容器中的物品建议只保留为数据实例。

只有以下情况创建世界Actor：

- 物品被丢弃；
- 物品直接摆放在地图；
- 需要3D展示；
- 玩家可以靠近拾取。

## 20.2 软引用

图标、模型、音效等资源建议通过软引用按需加载，避免一次加载全部物品资源。

## 20.3 事件驱动UI

推荐：

```text
事务成功
→ OnContainerChanged
→ 更新受影响格子
```

避免：

```text
每帧重建全部仓库和背包Widget
```

## 20.4 大量地图容器

可采用：

- 固定ContainerId；
- 固定RewardSeed；
- 首次打开时物质化；
- 关闭UI后销毁Widget；
- 保留容器数据；
- 只刷新打开中的容器。

---

# 二十一、系统不变量

每次事务完成后，开发版本可以检查：

```text
每个ItemId只存在一次
每件物品只有一个父位置
不可堆叠装备Quantity必须为1
堆叠数量必须在1—MaxStack之间
同一网格不能重叠
装备标签必须匹配槽位
兵器栏最多一件
道袍栏最多一件
空间道具栏最多一件
快捷栏引用必须有效
快捷栏引用物品必须位于基础6格
空间道具NestedContainerId必须存在
容器不能包含自身
已领取灵石不能再次领取
已物质化容器不能重复生成
已结束Run不能再次结算
```

错误日志建议包含：

- RunId；
- TransactionId；
- ItemId；
- 来源容器；
- 目标容器；
- 操作类型；
- 操作前快照；
- 操作后快照。

---

# 二十二、测试建议

## 22.1 单元测试

- 单格放置；
- 满容器；
- 越界位置；
- 非法标签；
- 同容器移动；
- 两件交换；
- 部分合并；
- 完整合并；
- 堆叠拆分；
- 装备替换；
- 空间道具整体移动；
- 快捷栏绑定；
- 快捷栏失效；
- 搜索状态限制；
- 已领取灵石重复领取；
- Run重复结算。

## 22.2 随机事务测试

连续执行大量随机操作：

```text
Move
Swap
Merge
Split
Equip
Unequip
Drop
PickUp
BindHotbar
Consume
```

每次操作后检查不变量。

## 22.3 完整流程测试

```text
进入地图
→ 击杀敌人
→ 打开尸体
→ 搜索装备、空间道具和身体
→ 拖拽物品到玩家储物
→ 将丹药移动到基础6格
→ 绑定快捷栏
→ 使用丹药
→ 丢弃空间道具
→ 重新拾取
→ 撤离
→ 返回仓库
→ 保存
→ 重启游戏
→ 核对GUID、数量、词条和位置
```

## 22.4 异常场景

重点验证：

1. 拖拽中关闭UI；
2. 搜索中关闭页面；
3. 搜索中死亡；
4. 搜索目标被销毁；
5. 空间道具装满后更换；
6. 装有内容的空间道具被丢弃；
7. 丢弃后重新拾取；
8. 快捷栏物品被移走；
9. 快捷栏物品合并到另一堆；
10. 数量归零后绑定仍存在；
11. 仓库已满时撤离；
12. 撤离倒计时中移动物品；
13. 撤离完成瞬间死亡；
14. 同一操作重复点击；
15. 保存中断或失败；
16. 同一尸体重复打开；
17. 同一灵石奖励重复领取；
18. 连续至少五局；
19. 存档重载；
20. 容器循环。

---

# 二十三、主要技术风险

## 23.1 UI成为权威

风险：

- 拖动Widget等于复制物品；
- 关闭页面后数据丢失；
- 两个页面显示不同数量。

建议：

- UI只提交请求；
- 真实数据统一由Repository和Transaction Service维护。

## 23.2 空间道具内部内容分离

风险：

- 丢弃空间道具后内部物品消失；
- 捡回空间道具时内容重新生成；
- 撤离时本体和内部内容被分别结算。

建议：

- 空间道具实例稳定引用内部ContainerId；
- 所有移动都整体保持这一关系。

## 23.3 搜索时重复生成

风险：

- 每次打开尸体都重新随机；
- 关闭再打开获得第二批奖励。

建议：

- 奖励生成与搜索显示分开；
- `bRewardsMaterialized`只允许首次生成。

## 23.4 事务中途失败

风险：

- 来源已删除、目标放置失败；
- 交换只完成一半。

建议：

- 先完整验证；
- 之后原子提交。

## 23.5 快捷栏复制物品

风险：

- 快捷栏保存第二份数量；
- 背包和快捷栏消耗不同步。

建议：

- 快捷栏只引用ItemId；
- 数量读取真实物品实例。

## 23.6 过早增加多格旋转

风险：

- UI、保存、自动整理和拖拽复杂度显著上升；
- 延迟核心闭环稳定。

建议：

- 0.0.9B先以1×1物品完成完整流程；
- 保留尺寸字段和扩展接口。

## 23.7 原0.0.9残留污染

风险：

- 不完整的新逻辑被误合并到0.0.9B；
- 同类系统出现两套权威。

建议：

- 以0.0.8镜像为清晰基线；
- 原0.0.9成果只在明确审核后选择性迁移。

---

# 二十四、推荐开发顺序

以下顺序是风险控制建议，可根据工程实际调整。

## 阶段A：0.0.8镜像与边界确认

- 建立0.0.9B独立开发基线；
- 确认原0.0.9不自动合并；
- 盘点0.0.8物品、Run、存档、装备和搜索基础。

## 阶段B：统一物品和容器数据

- Item Definition；
- Item Instance；
- Container Instance；
- GUID；
- Repository；
- 存档结构。

## 阶段C：物品事务

- Move；
- Swap；
- Merge；
- Split；
- Equip；
- Unequip；
- 错误结果；
- 原子提交。

## 阶段D：玩家实体与仓库UI

- 玩家装备；
- 基础6格；
- 空间道具；
- 仓库；
- 物品详情；
- 方格组件。

## 阶段E：拖拽与快捷操作

- 拖拽转移；
- 拖拽装备；
- 拖拽交换；
- 拖拽丢弃；
- 快速转移；
- 合法性反馈。

## 阶段F：尸体和搜索

- 普通容器；
- 敌人尸体；
- 身体容器；
- 开启读条；
- 单件识别；
- 灵石领取。

## 阶段G：快捷栏和消耗品

- 基础6格绑定；
- 1—9选择UI；
- 图标与数量；
- 使用；
- 数量同步；
- 失效清理。

## 阶段H：世界物品与空间道具

- 世界拾取Actor；
- 丢弃；
- 拾取；
- 装有内容的空间道具整体移动；
- 弃置与重新拾取。

## 阶段I：撤离、死亡和存档

- 唯一终局；
- 结算快照；
- Profile转移；
- Run清理；
- 重启恢复。

## 阶段J：完整回归

- 自动测试；
- 随机事务测试；
- 连续多局；
- 人工拖拽和搜索体验；
- 数量、GUID和结算对账。

---

# 二十五、方案选择分析

## 25.1 单格物品与多格物品

| 方案 | 优点 | 风险 | 0.0.9B建议 |
|---|---|---|---|
| 全部1×1 | 快速稳定核心流程 | 空间拼图感较弱 | 优先采用 |
| 规则矩形多格 | 更接近塔科夫 | 拖拽、旋转和布局复杂 | 后续扩展 |
| 不规则形状 | 空间策略最强 | 技术和UI成本最高 | 暂不优先 |

## 25.2 空间道具卸下方式

| 方案 | 优点 | 风险 | 建议 |
|---|---|---|---|
| 必须清空后卸下 | 实现简单 | 操作僵硬，不利于弃置撤离 | 不优先 |
| 连同内容整体移动 | 符合搜打撤直觉 | 需稳定嵌套容器关系 | 推荐 |

## 25.3 搜索时奖励生成

| 方案 | 优点 | 风险 | 建议 |
|---|---|---|---|
| 每次打开重新生成 | 实现表面简单 | 可重复刷取、复制 | 不采用 |
| 敌人死亡时生成 | 状态直观 | 大量容器可能提前占用数据 | 尸体适用 |
| 首次打开延迟生成 | 节省资源 | 需要固定Seed和物质化标记 | 普通容器推荐 |

## 25.4 UI更新

| 方案 | 优点 | 风险 | 建议 |
|---|---|---|---|
| 每帧全量刷新 | 编写简单 | 性能和状态问题明显 | 不优先 |
| 事务事件增量刷新 | 数据一致、性能稳定 | 初始结构略复杂 | 推荐 |

---

# 二十六、0.0.9B建议验收结果

版本完成后，建议至少验证：

1. 玩家可以在局外仓库与人物之间转移物品；
2. 玩家可以拖拽装备兵器、道袍、饰品和空间道具；
3. 装备栏数量和标签规则正确；
4. 玩家始终拥有基础6格；
5. 空间道具正确提供6—30格；
6. 空间道具可以连同内部物品整体移动；
7. 1—9快捷栏只引用基础6格中的可用物品；
8. 快捷栏图标和数量与真实物品同步；
9. 普通容器具有开启和物品搜索状态；
10. 尸体显示装备、基础物品区、空间道具和身体容器；
11. 未搜索物品不能被直接拿取；
12. 拖拽移动、交换、堆叠、拆分和丢弃正确；
13. 丢弃后生成真实世界对象；
14. 重新拾取后保持原GUID和属性；
15. 灵石领取后不能重复获取；
16. 同一容器不能重复生成奖励；
17. 同一件物品不能同时存在于两个位置；
18. 撤离后合法物品进入局外仓库；
19. 死亡、撤离和放弃不会重复结算；
20. 保存并重启后物品位置、数量、词条和容器关系不变；
21. 连续多局后不出现物品复制、状态残留或输入锁死。

---

# 二十七、技术结论

0.0.9B的关键不是视觉上模仿《逃离塔科夫》，而是建立一套足够稳定的物品状态模型。

推荐核心组合：

```text
唯一物品实例
+ 统一容器模型
+ 原子物品事务
+ 玩家与尸体共用实体结构
+ 搜索状态机
+ 空间道具嵌套容器
+ 快捷栏真实引用
+ 世界丢弃与拾取
+ 撤离和死亡唯一结算
+ 可恢复存档
```

在这一基础上，仓库、背包、搜索、装备、空间道具、尸体、地面物品和快捷栏都可以共享同一套逻辑。

0.0.9B当前最稳妥的实现策略是：

> **先以全部1×1物品完成完整且无复制的物品闭环，再根据实际体验决定是否增加多格、旋转和更复杂的收纳玩法。**

这能够优先获得类塔科夫系统真正重要的体验：

- 有限空间；
- 快速搜刮；
- 物品价值判断；
- 携带风险；
- 仓库整理；
- 撤离后真实保留；
- 操作熟练度带来的效率提升。
