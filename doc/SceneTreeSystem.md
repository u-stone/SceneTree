# Scene Tree System 核心文档

## 1. 概述
Scene Tree System 是一个高性能、支持异步操作和批处理优化的场景管理库。它采用组件化设计，支持复杂的有向无环图（DAG）结构，并提供了完善的查询、序列化和场景生命周期管理功能。

## 2. 核心组件

### 2.1 SceneNode
场景中的基本单元。
- **属性管理**：支持 ID、名称、状态（Active, Inactive, Broken 等）。
- **标签系统**：支持多标签（Tags）管理，便于逻辑分类。
- **脏标记系统**：使用 `NodeProperty` 位掩码记录属性变更，支持批处理优化。
- **观察者模式**：通过 `INodeObserver` 接口实现属性变更通知。

### 2.2 SceneTree
管理节点层级结构和索引。
- **高效查询**：内置 ID 索引、名称索引和标签索引，支持 $O(1)$ 或 $O(K)$ 复杂度的快速检索。
- **层级操作**：支持 `attach`（合并树）和 `detach`（拆分树）操作，并能正确处理多父节点（DAG）情况下的节点保留逻辑。
- **批处理更新**：通过 `setBatchingEnabled(true)` 开启。属性变更（如名称、状态）会先进入脏队列，在 `update()` 时统一处理，减少索引更新频率。

### 2.3 SceneManager
全局场景管理器，负责场景的生命周期。
- **场景注册**：管理多个 `Scene` 配置。
- **同步/异步加载**：提供 `loadScene` (同步) 和 `loadSceneAsync` (异步) 接口。
- **任务合并**：如果多个请求同时加载同一场景，系统会自动合并为一个后台任务，避免重复 I/O。

## 3. 异步操作机制

### 3.1 AsyncOperation
异步操作返回一个 `std::shared_ptr<AsyncOperation>` 句柄，支持两种处理模式：
1. **轮询模式**：调用 `op->IsDone()` 检查状态，`op->GetResult()` 获取结果（未完成时会阻塞）。
2. **回调模式**：在发起请求时传入 `SceneAsyncCallback`，任务完成时在主线程自动触发。

### 3.2 异步卸载
`unloadSceneAsync` 会将场景树的所有权转移到后台线程进行析构，有效避免主线程因销毁大量对象而产生的卡顿。

## 4. 序列化 (SceneIO)

基于 RapidJSON 实现，支持：
- **版本控制**：JSON 头部包含 `format_version`，支持向前兼容。
- **递归结构**：完整保留节点层级、标签和属性。
- **错误处理**：完善的解析错误提示和文件校验。

## 5. 代码示例

### 5.1 异步加载与轮询
```cpp
auto manager = std::make_unique<SceneManager>();

// 发起异步加载
auto op = manager->loadSceneAsync("Level1", "data/level1.json", 
    [](const std::string& name, bool success) {
        std::cout << "加载完成: " << name << std::endl;
    });

// 在主循环中轮询
while (!op->IsDone()) {
    manager->update(); // 必须调用 update 处理任务收割
    renderLoadingScreen();
}
```

### 5.2 批处理属性更新
```cpp
tree->setBatchingEnabled(true);

node->setName("NewName");
node->setStatus(ObjectStatus::Inactive);

// 此时索引尚未更新，查询仍返回旧结果
assert(tree->findNodeByName("NewName") == nullptr);

tree->update(0.016); // 触发批处理

// 此时索引已更新
assert(tree->findNodeByName("NewName") != nullptr);
```

## 6. 性能优化建议
1. **频繁变更**：对于每帧都在变化的属性，建议开启 `Batching`。
2. **大型场景**：切换场景时优先使用 `preloadSceneAsync` 提前加载到内存。
3. **内存管理**：使用 `unloadSceneAsync` 释放大型场景，以保持帧率平稳。
4. **查询优化**：尽量使用 `findNode` (ID) 或 `findFirstNodeByTag`，避免在大规模场景中频繁进行全量名称搜索。

## 7. 架构解耦
系统引入了 `SceneNodePropertyObserver` 作为中间层。`SceneTree` 不再直接监听 `SceneNode`，而是通过观察者对象进行转发，这使得未来扩展自定义处理策略（如针对不同节点类型的特殊更新逻辑）变得更加容易。