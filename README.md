# 机器人代理服务器（RPS）系统模块说明

本文档整理了机器人代理服务器项目中各个代码文件的作用和功能描述。

---

## 1. main.cpp

**作用**：程序入口，负责初始化全局配置并根据命令行参数启动不同运行模式。

- 设置 `SIGINT` 信号处理函数，确保程序可被 Ctrl+C 安全退出。
- 使用 `cmdline` 库解析命令行参数，支持 `--gui` 选项。
- 初始化 `spdlog` 异步日志系统，配置同时输出到控制台（带颜色）和滚动文件（`logs/rps_log.txt`，2MB轮转）。
- 根据是否存在 `--gui` 参数分支执行：
  - **GUI 模式**：创建 `QApplication`，实例化 `MapServer`、`RobotProxyServer` 和 `MainWindow`，将地图服务器指针设置给代理服务器，并显示主窗口。连接 `aboutToQuit` 信号以在退出时关闭日志。
  - **CLI 模式**：创建 `QCoreApplication`，实例化 `MapServer` 和 `RobotProxyServer`，设置地图服务器指针，然后调用 `rps.startRPS("127.0.0.1", 7788, "127.0.0.1", 7789)` 启动服务（监听下游端口 7788，上游地址 `127.0.0.1:7789` 仅保存未使用）。同样连接 `aboutToQuit` 信号。
- 最后进入 Qt 事件循环。

---

## 2. map_server.cpp / map_server.h

**作用**：地图数据服务模块，负责加载和查询地图点坐标。

- 构造函数中调用 `initGeekplusMap()`，从应用程序目录下的 `resources/map/map_112.csv` 文件读取地图数据。
- CSV 文件格式：每行包含 `id, x, y`（第一行可能为表头）。数据被存入 `QHash<int, QPointF>` 成员变量 `geekplusMap`。
- 提供公共方法 `getPointById(int id, QPointF &p)`，根据 ID 查询坐标，若存在则返回 `true` 并填充 `p`，否则返回 `false`。查询过程会记录日志。
- 在整个系统中作为只读地图数据源，供 `Agent` 在处理机器人注册请求（`INIT_POS_REQ`）时查询物理坐标。

---

## 3. robot_proxy_server.cpp / robot_proxy_server.h

**作用**：TCP 服务器类，负责监听下游机器人连接，并为每个连接创建独立线程和 `Agent` 对象进行管理。

- 继承自 `QTcpServer`，重写 `incomingConnection(qintptr socketDescriptor)` 方法。
- 保存上游服务器地址（`upIPAddr`、`upIPPort`），但未实际使用（可能为预留）。
- **连接处理流程**：
  1. 创建新的 `QThread` 对象（父对象设为 `this`，防止内存泄漏）。
  2. 创建 `Agent` 对象（无父对象，后续移入线程）。
  3. 将 `(thread, agt)` 插入 `threadPool`（`QMultiMap<QThread*, Agent*>`）以记录映射。
  4. 设置 Agent 的套接字描述符（`setSktDescriptor`）。
  5. 连接信号槽：
     - `thread->started()` → `agt->onCreateTimer()`：线程启动后让 Agent 创建定时器和套接字。
     - `agt->destroyed` → 清理 lambda：从 `threadPool` 移除该 Agent；若该线程上再无其他 Agent，则调用 `thread->quit()` 和 `thread->wait()` 终止线程，同时发出 `agentUpdate(false, agt)` 信号。
     - `thread->finished` → 记录日志。
     - `agt->agentCreated` → 发出 `agentCreated` 和 `agentUpdate(true, agt)` 信号，用于 GUI 更新。
     - `agt->downStreamClosed` → 空 lambda（预留）。
  6. 调用 `agt->moveToThread(thread)` 将 Agent 移至新线程。
  7. 设置 Agent 的地图服务器指针 `map`。
  8. 启动线程 `thread->start()`。
- 提供 `startRPS` 方法启动监听，成功返回 `true`。
- 构造函数中注册元类型 `QAbstractSocket::SocketError`。

---

## 4. agent.cpp / agent.h

**作用**：每个机器人连接对应的业务处理对象，运行在独立线程中，负责与机器人进行协议交互。

- **初始化**：`onCreateTimer()` 在线程启动后被调用，创建 `QTcpSocket`（`socket_down`），设置其描述符，连接 `readyRead`、`disconnected` 信号，并创建心跳定时器 `timer`。
- **协议解析**：采用有限状态机（`unpackingStatus`）逐字节解析自定义 TCP 协议：
  - 帧头：`0xFE 0xFE`
  - 长度字段：2 字节大端，表示总长度（含头部 6 字节）
  - 类型字段：2 字节大端（对应 `enMsgHeaderType` 枚举）
  - 消息体：JSON 字符串（由 Protobuf 消息序列化而来）
- **消息处理**：`messageProcess` 根据类型分发：
  - `SESSION_REGISTER_INFO`：保存机器人 ID，回复会话响应。
  - `HEART_BEAT`：启动心跳定时器（若未启动），回复心跳响应。
  - `AGV_REGISTER_REQUEST`：提取 AGV ID 和坐标，调用 `sendRegisterResponse` 回复注册确认。
  - `AGV_RT_STATUS`：解析实时状态，通过 `rtStatus` 信号转发给 GUI。
  - `AGV_ACTION_EVENT`：解析动作事件，通过 `actionInfo` 信号转发。
  - `INIT_POS_REQ`：根据请求中的 code（点 ID）调用 `map->getPointById` 获取物理坐标，回复初始化位置响应。
- **响应发送**：`downSocketWrite` 构造头部 + JSON 体并写入 socket。
- **动作指令**：提供多个公共槽函数，供 GUI 或其他模块调用以下发动作：
  - `sendActionLift`、`sendActionPut`、`sendActionTurn`、`sendActionCharge`、`sendActionMove` 等。
  - 这些函数构造相应的 Protobuf 消息，序列化为 JSON，然后通过 `downSocketWrite` 发送。
- **信号**：
  - `agentCreated`：新连接建立时发出，携带客户端地址端口。
  - `rtStatus`：实时状态 JSON 字符串。
  - `actionInfo`：动作事件信息列表。
  - `downWrite`：用于调试，记录发送的原始数据。
- **资源管理**：在 `disconnected` 信号中调用 `deleteLater()` 自我销毁，触发线程清理。

---

## 5. ctu_agent.cpp

**作用**：一个简单的 UDP 服务器示例，展示了如何解析二进制消息头并构造响应。可能用于测试或作为参考，未集成到主系统中。

- 定义 `MessageHeader` 结构（含消息标识、长度、类型、序号、版本、加密类型、协议内容类型、保留字节）。
- `Server` 类继承 `QObject`，内部创建 `QUdpSocket` 并绑定端口 8080。
- 当有数据报到达时，读取并解析头部，提取长度和类型（使用大端转换）。
- 假设消息体跟在头部之后，通过指针转换得到 `MessageBody`（未实际定义）。
- 然后构建响应头部和响应体，通过 `writeDatagram` 返回给客户端。
- `main` 函数创建 `QApplication` 和 `Server` 对象，进入事件循环。

---

## 6. virtual_robot.cpp / virtual_robot.h

**作用**：虚拟机器人仿真模块，模拟一个符合团标协议的 IMR（智能移动机器人），用于测试 RCS 或 RPS。

- **协议实现**：遵循附录 A、B 规定的帧格式：
  - 帧头：`0x02` + 方向标识（'T' 表示 RCS→IMR，'V' 表示 IMR→RCS）
  - 长度：4 字节小端（消息体长度）
  - 头部校验：CRC16_MODBUS（对前 6 字节）
  - 消息体：`[2字节类型][Protobuf二进制]`
  - 消息体校验：CRC16_MODBUS（对消息体）
  - 帧尾：`0x03`
- **初始化**：`initConnection` 连接 RCS 服务器（TCP），设置数据接收状态机。
- **注册**：`sendRegister` 发送注册消息（类型 `RG`），包含 IMR 型号、物理参数等。
- **状态上报**：`sendStatus` 定时（默认 1 秒）构造 `IMRState` 消息（含位置、心跳、任务状态等），序列化为 Protobuf 二进制，加上头部尾部发送。
- **任务接收**：在 `messageProcess` 中处理接收到的命令字：
  - `AT`：解析任务消息，存储到 `task` 成员，调用 `logicDecision` 进行任务逻辑处理。
  - 其他命令字（`GM`、`ST`、`SA`、`OP`、`RG`）目前仅记录日志或未处理。
- **任务逻辑**：`logicDecision` 根据当前任务（`task`）和机器人状态（`ImrSta`）决定是否接受、更新任务，并在到达指定点后触发动作执行（通过 `doRcsAction` 信号通知外部）。
- **位置与动作模拟**：
  - 外部控制程序（如控制面板）可通过调用 `linkP40_pos` 更新机器人实时位置、方向、电量、速度等。
  - 当需要执行动作（如升降）时，发出 `doRcsAction` 信号，外部模拟器响应后通过 `updateHeight` 更新高度。
  - 动作完成后，通过 `RunningState` 中的 `actionstates` 上报完成状态。
- **信号**：
  - `sendAimToP40`：通知外部目标点信息。
  - `doRcsAction`：请求执行具体动作。
- **调试**：`log_Sta` 字符串记录关键状态变化，便于输出到界面。

---

## 整体关系与数据流

- **真实机器人接入**：机器人 → `RobotProxyServer`（监听 7788） → 每个连接创建 `Agent`（独立线程） → `Agent` 解析协议，通过 `MapServer` 查询地图，将状态通过信号转发给 `MainWindow`（GUI 模式）；GUI 可调用 Agent 的槽函数下发动作指令。
- **虚拟机器人仿真**：虚拟机器人作为客户端连接 RCS（如菜鸟 RCS），模拟注册、状态上报、任务接收和执行，通过信号与外部控制界面交互，实现闭环测试。
- **日志系统**：`spdlog` 统一记录所有模块的运行信息，便于调试和监控。
- **行业标准**：代码中 `VirtualRobot` 的协议实现参照了 T/SSITS 204 团体标准，其他模块（如 `Agent` 的 JSON 协议）是自定义的，但可扩展以符合标准。

---
