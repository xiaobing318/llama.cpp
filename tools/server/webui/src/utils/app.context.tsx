import React, { createContext, useContext, useEffect, useState } from 'react';
import {
  APIMessage,
  CanvasData,
  Conversation,
  LlamaCppServerProps,
  Message,
  PendingMessage,
  ViewingChat,
} from './types';
import StorageUtils from './storage';
import {
  filterThoughtFromMsgs,
  normalizeMsgsForAPI,
  getSSEStreamAsync,
  getServerProps,
} from './misc';
import { BASE_URL, CONFIG_DEFAULT, isDev } from '../Config';
import { matchPath, useLocation, useNavigate } from 'react-router';
import toast from 'react-hot-toast';

interface AppContextValue {
  // conversations and messages
  viewingChat: ViewingChat | null;
  pendingMessages: Record<Conversation['id'], PendingMessage>;
  isGenerating: (convId: string) => boolean;
  sendMessage: (
    convId: string | null,
    leafNodeId: Message['id'] | null,
    content: string,
    extra: Message['extra'],
    onChunk: CallbackGeneratedChunk
  ) => Promise<boolean>;
  stopGenerating: (convId: string) => void;
  replaceMessageAndGenerate: (
    convId: string,
    parentNodeId: Message['id'], // the parent node of the message to be replaced
    content: string | null,
    extra: Message['extra'],
    onChunk: CallbackGeneratedChunk
  ) => Promise<void>;

  // canvas
  canvasData: CanvasData | null;
  setCanvasData: (data: CanvasData | null) => void;

  // config
  config: typeof CONFIG_DEFAULT;
  saveConfig: (config: typeof CONFIG_DEFAULT) => void;
  showSettings: boolean;
  setShowSettings: (show: boolean) => void;

  // props
  serverProps: LlamaCppServerProps | null;
}

// this callback is used for scrolling to the bottom of the chat and switching to the last node
export type CallbackGeneratedChunk = (currLeafNodeId?: Message['id']) => void;

// eslint-disable-next-line @typescript-eslint/no-explicit-any
const AppContext = createContext<AppContextValue>({} as any);

const getViewingChat = async (convId: string): Promise<ViewingChat | null> => {
  const conv = await StorageUtils.getOneConversation(convId);
  if (!conv) return null;
  return {
    conv: conv,
    // all messages from all branches, not filtered by last node
    messages: await StorageUtils.getMessages(convId),
  };
};

export const AppContextProvider = ({
  children,
}: {
  children: React.ReactElement;
}) => {
  const { pathname } = useLocation();
  const navigate = useNavigate();
  const params = matchPath('/chat/:convId', pathname);
  const convId = params?.params?.convId;

  const [serverProps, setServerProps] = useState<LlamaCppServerProps | null>(
    null
  );
  const [viewingChat, setViewingChat] = useState<ViewingChat | null>(null);
  const [pendingMessages, setPendingMessages] = useState<
    Record<Conversation['id'], PendingMessage>
  >({});
  const [aborts, setAborts] = useState<
    Record<Conversation['id'], AbortController>
  >({});
  const [config, setConfig] = useState(StorageUtils.getConfig());
  const [canvasData, setCanvasData] = useState<CanvasData | null>(null);
  const [showSettings, setShowSettings] = useState(false);

  // get server props
  useEffect(() => {
    getServerProps(BASE_URL, config.apiKey)
      .then((props) => {
        console.debug('Server props:', props);
        setServerProps(props);
      })
      .catch((err) => {
        console.error(err);
        toast.error('Failed to fetch server props');
      });
    // eslint-disable-next-line
  }, []);

  // handle change when the convId from URL is changed
  useEffect(() => {
    // also reset the canvas data
    setCanvasData(null);
    const handleConversationChange = async (changedConvId: string) => {
      if (changedConvId !== convId) return;
      setViewingChat(await getViewingChat(changedConvId));
    };
    StorageUtils.onConversationChanged(handleConversationChange);
    getViewingChat(convId ?? '').then(setViewingChat);
    return () => {
      StorageUtils.offConversationChanged(handleConversationChange);
    };
  }, [convId]);

  /* 
   * =================== setPending 函数详解 ===================
   * 
   * 【函数目的】
   * 这是管理待处理消息状态的核心函数，负责添加、更新或删除特定会话的pending消息
   * 相当于一个"消息生成状态管理器"，告诉系统哪些会话正在生成AI回复
   * 
   * 【解决什么问题】
   * 1. 状态同步：确保UI能实时显示AI生成的内容
   * 2. 防重复操作：防止用户在AI生成时重复发送消息
   * 3. 错误恢复：出错时能正确清除生成状态
   * 4. 多会话管理：支持多个聊天窗口同时生成不同回复
   * 
   * 【参数说明】
   * 
   * convId: string - 会话ID
   *   作用：指定要操作的会话
   *   示例值：
   *     - "conv-1693234200000" （2023年8月28日创建的会话）
   *     - "conv-1693320600000" （2023年8月29日创建的会话）
   * 
   * pendingMsg: PendingMessage | null - 待处理消息对象或null
   *   作用：要设置的pending消息，null表示清除
   *   两种情况：
   *     - PendingMessage对象：设置或更新该会话的pending状态
   *     - null：清除该会话的pending状态（生成完成或出错）
   * 
   * 【函数的两种工作模式】
   * 
   * 🔧 模式1：设置/更新Pending状态 (pendingMsg非null)
   * 调用时机：
   *   - AI开始生成回复时（创建初始pending）
   *   - 收到新的数据片段时（更新pending内容）
   * 
   * 📋 模式2：清除Pending状态 (pendingMsg为null)
   * 调用时机：
   *   - AI生成完成时（转为正式消息）
   *   - 用户主动停止生成时
   *   - 出现错误需要重置时
   * 
   * 【实际应用场景】
   * 
   * 场景1：开始生成AI回复
   * ```
   * const newPending = {
   *   id: 1693234567890,
   *   content: null,  // 刚开始，还没有内容
   *   role: 'assistant',
   *   ...
   * };
   * setPending("conv-123", newPending);
   * // 结果：isGenerating("conv-123") → true
   * ```
   * 
   * 场景2：更新AI生成的内容
   * ```
   * const updatedPending = {
   *   ...existingPending,
   *   content: "你好！很高兴认识你"  // 新的完整内容
   * };
   * setPending("conv-123", updatedPending);
   * // 结果：用户看到更新后的内容
   * ```
   * 
   * 场景3：生成完成，清除状态
   * ```
   * setPending("conv-123", null);
   * // 结果：isGenerating("conv-123") → false
   * ```
   * 
   * 【数据结构变化】
   * 
   * pendingMessages对象的结构：
   * ```
   * {
   *   "conv-123": { id: 1001, content: "你好...", role: "assistant", ... },
   *   "conv-456": { id: 1002, content: "正在思考...", role: "assistant", ... }
   * }
   * ```
   * 
   * 操作前后对比：
   * 
   * 添加操作：
   * 之前：{ "conv-456": {...} }
   * 调用：setPending("conv-123", newMsg)
   * 之后：{ "conv-456": {...}, "conv-123": newMsg }
   * 
   * 更新操作：
   * 之前：{ "conv-123": oldMsg, "conv-456": {...} }
   * 调用：setPending("conv-123", updatedMsg)
   * 之后：{ "conv-123": updatedMsg, "conv-456": {...} }
   * 
   * 删除操作：
   * 之前：{ "conv-123": {...}, "conv-456": {...} }
   * 调用：setPending("conv-123", null)
   * 之后：{ "conv-456": {...} }  // conv-123被删除
   */
  const setPending = (convId: string, pendingMsg: PendingMessage | null) => {
    /* 
     * ========== 清除模式：删除特定会话的pending状态 ==========
     * 
     * 当pendingMsg为null时，表示要清除该会话的生成状态
     * 常见触发情况：
     * 1. AI回复生成完成
     * 2. 用户主动停止生成
     * 3. 发生错误需要重置
     * 4. 网络连接中断
     */
    if (!pendingMsg) {
      /* 
       * 使用React的函数式状态更新模式：
       * - prev参数是当前的pendingMessages状态
       * - 必须返回新的状态对象（不能直接修改prev）
       * 
       * 【为什么要创建新对象？】
       * React使用对象引用来检测状态变化：
       * - 如果直接修改prev，React认为状态没变化，不会重新渲染
       * - 创建新对象确保React能检测到变化并重新渲染UI
       * 
       * 类比C++：
       * ```cpp
       * std::map<string, PendingMessage> newState = prevState;  // 复制
       * newState.erase(convId);  // 删除指定key
       * return newState;  // 返回新的map
       * ```
       */
      setPendingMessages((prev) => {
        /* 使用展开运算符创建prev的浅拷贝 */
        const newState = { ...prev };
        /* 删除指定会话的pending信息 */
        delete newState[convId];
        /* 返回新的状态对象，触发React重新渲染 */
        return newState;
      });
    } else {
      /* 
       * ========== 设置/更新模式：添加或更新pending消息 ==========
       * 
       * 当pendingMsg不为null时，表示要设置或更新该会话的生成状态
       * 常见触发情况：
       * 1. 开始生成AI回复（创建初始pending）
       * 2. 收到新的内容片段（更新existing pending）
       * 3. 收到性能计时信息（更新timing数据）
       * 
       * 【对象合并语法解释】
       * { ...prev, [convId]: pendingMsg }
       * 
       * 这是ES6的对象展开语法：
       * 1. ...prev: 复制现有的所有键值对
       * 2. [convId]: pendingMsg: 设置或覆盖指定key的值
       * 
       * 举例：
       * prev = { "conv-A": msgA, "conv-B": msgB }
       * convId = "conv-C"
       * pendingMsg = msgC
       * 结果 = { "conv-A": msgA, "conv-B": msgB, "conv-C": msgC }
       * 
       * 如果convId已存在：
       * prev = { "conv-A": msgA, "conv-B": oldMsgB }  
       * convId = "conv-B"
       * pendingMsg = newMsgB
       * 结果 = { "conv-A": msgA, "conv-B": newMsgB }  // 覆盖了oldMsgB
       * 
       * 类比C++：
       * ```cpp
       * std::map<string, PendingMessage> newState = prevState;  // 复制
       * newState[convId] = pendingMsg;  // 设置或更新
       * return newState;  // 返回新map
       * ```
       */
      setPendingMessages((prev) => ({ ...prev, [convId]: pendingMsg }));
    }
  };

  const setAbort = (convId: string, controller: AbortController | null) => {
    if (!controller) {
      setAborts((prev) => {
        const newState = { ...prev };
        delete newState[convId];
        return newState;
      });
    } else {
      setAborts((prev) => ({ ...prev, [convId]: controller }));
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // public functions

  /* 
   * =================== isGenerating 函数详解 ===================
   * 
   * 【函数目的】
   * 检查指定会话是否正在生成AI回复，这是一个状态检查函数
   * 类似于C++中检查某个操作是否正在进行的布尔函数
   * 
   * 【解决什么问题】
   * 1. 防止重复生成：避免用户在AI还在回复时重复发送消息
   * 2. UI状态控制：决定是否显示"正在生成中"的加载动画
   * 3. 按钮状态管理：控制发送按钮、停止按钮等的启用状态
   * 4. 资源保护：防止同时启动多个AI生成任务造成资源浪费
   * 
   * 【参数说明】
   * convId: string - 会话ID，用于检查特定会话的生成状态
   * 实际值示例：
   *   - "conv-1693234200000" （某个具体会话）
   *   - "conv-1693320600000" （另一个会话）
   * 
   * 【返回值】
   * boolean - 是否正在生成
   *   - true: 该会话正在生成AI回复
   *   - false: 该会话没有正在进行的生成任务
   * 
   * 【核心逻辑解析】
   * !!pendingMessages[convId] 这行代码的含义：
   * 
   * 1. pendingMessages[convId] - 从pendingMessages对象中获取指定会话的待处理消息
   *    pendingMessages的结构：
   *    {
   *      "conv-1693234200000": { id: 1693234567890, content: "正在生成中...", ... },
   *      "conv-1693320600000": { id: 1693234567891, content: "另一个回复...", ... }
   *    }
   * 
   * 2. 如果该会话正在生成：pendingMessages[convId] 返回PendingMessage对象
   * 3. 如果该会话没有生成：pendingMessages[convId] 返回undefined
   * 
   * 4. !! 双重否定操作符（布尔转换）：
   *    - !pendingMessages[convId]: 如果对象存在则为false，如果undefined则为true
   *    - !!pendingMessages[convId]: 再次取反，如果对象存在则为true，如果undefined则为false
   *    
   *    这是JavaScript中将任意值转换为布尔值的常用技巧：
   *    - !!undefined → false
   *    - !!null → false  
   *    - !!{...} → true (任何非空对象)
   *    - !!"" → false (空字符串)
   *    - !!"hello" → true (非空字符串)
   * 
   * 【实际运行示例】
   * 
   * 场景1：会话未在生成
   * pendingMessages = {}  (空对象)
   * isGenerating("conv-123") → !!undefined → false
   * 
   * 场景2：会话正在生成  
   * pendingMessages = {
   *   "conv-123": { id: 1693234567890, content: "正在思考...", ... }
   * }
   * isGenerating("conv-123") → !!{对象} → true
   * 
   * 场景3：其他会话在生成，当前会话没有
   * pendingMessages = {
   *   "conv-456": { id: 1693234567890, content: "正在生成...", ... }
   * }
   * isGenerating("conv-123") → !!undefined → false
   * 
   * 【调用时机】
   * 
   * 1. 用户点击发送消息前：
   *    if (isGenerating(convId)) {
   *      return; // 阻止重复发送
   *    }
   * 
   * 2. UI渲染时决定按钮状态：
   *    <button disabled={isGenerating(convId)}>
   *      {isGenerating(convId) ? "生成中..." : "发送"}  
   *    </button>
   * 
   * 3. 显示加载动画：
   *    {isGenerating(convId) && <LoadingSpinner />}
   * 
   * 【与其他函数的配合】
   * 
   * - setPending(convId, pendingMsg): 设置生成状态，isGenerating变为true
   * - setPending(convId, null): 清除生成状态，isGenerating变为false  
   * - stopGenerating(convId): 主动停止生成，内部调用setPending(convId, null)
   * 
   * 【类似C++的概念】
   * 这个函数类似于C++中的：
   * ```cpp
   * bool isProcessing(const std::string& sessionId) {
   *     return processingTasks.find(sessionId) != processingTasks.end();
   * }
   * ```
   * 都是检查某个任务是否正在执行的状态查询函数
   */
  const isGenerating = (convId: string) => !!pendingMessages[convId];

  /* 
   * =================== generateMessage 函数详解 ===================
   * 
   * 【函数目的】
   * 这是生成AI回复消息的核心函数，负责向AI服务器发送用户的聊天历史，
   * 接收AI的实时流式回复，并在界面上逐字显示生成过程。
   * 
   * 【解决什么问题】
   * 1. 实现实时AI对话：用户发送消息后能立即看到AI开始回复
   * 2. 流式显示：AI边生成边显示，而不是等全部完成再显示
   * 3. 处理对话分支：支持编辑历史消息并创建新的对话分支
   * 4. 错误处理：处理网络错误、用户取消等异常情况
   * 
   * 【参数详细说明】
   * 
   * 1. convId (string) - 会话ID
   *    作用：标识当前聊天会话，类似于C++中的会话句柄
   *    格式：通常是 "conv-1693234567890" 这样的格式（conv-时间戳）
   *    示例值：
   *      - "conv-1693234567890"  （新会话）
   *      - "conv-1693235678901"  （另一个会话）
   *    用途：用于从数据库中查找该会话的所有消息历史
   * 
   * 2. leafNodeId (number) - 叶子节点ID
   *    作用：指定消息树中的叶子节点，表示对话分支的末端位置
   *    这是理解消息树最关键的概念！
   *    数据类型：number (时间戳，如 1693234567890)
   *    示例值：
   *      - 1693234567890 （某个用户消息的ID）
   *      - 1693234568901 （某个AI回复的ID）
   *      - 1693234569912 （编辑后产生的新消息ID）
   *    
   *    【为什么需要leafNodeId？】
   *    - 对话不是简单的线性列表，而是树状结构
   *    - 用户可以编辑历史消息，产生新的对话分支
   *    - leafNodeId告诉系统从哪个节点开始生成新的AI回复
   *    - 类似于C++中的指针，指向树结构中的特定节点
   * 
   * 3. onChunk (CallbackGeneratedChunk) - 回调函数
   *    作用：每当收到AI生成的新内容片段时调用此函数
   *    类型：(currLeafNodeId?: Message['id']) => void
   *    用途：
   *      - 通知UI滚动到底部显示最新内容
   *      - 切换到正确的消息节点
   *      - 触发界面重新渲染
   *    调用时机：
   *      - AI每生成一小段文字时调用（实现打字机效果）
   *      - 消息生成完成时调用（传入新消息的ID）
   * 
   * 【消息树结构详解】
   * 
   * 消息树是什么？
   * 这是支持对话分支功能的数据结构，类似于C++中的树型数据结构。
   * 
   * 为什么需要树结构而不是简单列表？
   * 1. 支持编辑历史消息：用户可以修改之前的消息重新生成回复
   * 2. 保留对话历史：编辑后原来的对话分支仍然保存
   * 3. 多种回复选择：可以让AI对同一个消息生成多种不同回复
   * 
   * 树结构示例：
   * 
   *     root (系统根节点，不显示)
   *      │
   *   [1001] 用户: "你好"
   *      │
   *   [1002] AI: "你好！有什么可以帮助你的吗？"
   *      │
   *   [1003] 用户: "告诉我关于编程的知识"
   *      │
   *   [1004] AI: "编程是..."
   *      │
   *   [1005] 用户: "具体说说JavaScript"  
   *      ├── [1006] AI: "JavaScript是一种脚本语言..."  (分支A)
   *      └── [1007] AI: "JavaScript主要用于网页开发..."  (分支B - 重新生成的回复)
   * 
   * 当用户在节点1005后重新生成AI回复时：
   * - leafNodeId = 1005 (从这个用户消息开始生成)
   * - 会产生新的AI回复节点1007
   * - 原来的回复1006仍然保留，形成分支
   * 
   * 【leafNodeId的实际运行示例 - 真实数据】
   * 
   * 情况1：正常对话流程
   * 时间：2023-08-28 14:30:15
   * 1. 用户输入"Hello" -> 创建消息ID：1693234215000
   * 2. 调用：generateMessage("conv-1693234200000", 1693234215000, onChunk)  
   * 3. 结果：生成AI回复，新消息ID：1693234215001，父节点：1693234215000
   * 4. 消息树：root -> [1693234215000] 用户:"Hello" -> [1693234215001] AI:"Hi there!"
   * 
   * 情况2：用户重新生成AI回复
   * 时间：2023-08-28 14:32:20  
   * 1. 用户对AI回复不满意，点击"重新生成"
   * 2. 调用：generateMessage("conv-1693234200000", 1693234215000, onChunk)
   *    注意：还是用同一个leafNodeId（用户消息的ID）
   * 3. 结果：生成新的AI回复，ID：1693234340000，父节点：1693234215000
   * 4. 消息树：
   *    root -> [1693234215000] 用户:"Hello" 
   *                ├── [1693234215001] AI:"Hi there!"      (原回复)
   *                └── [1693234340000] AI:"Hello! How are you?" (新回复)
   * 
   * 情况3：编辑历史消息产生分支
   * 时间：2023-08-28 14:35:10
   * 1. 用户编辑之前的消息"Hello"改为"Hi" -> 产生新节点ID：1693234510000  
   * 2. 调用：generateMessage("conv-1693234200000", 1693234510000, onChunk)
   * 3. 结果：生成新AI回复，ID：1693234510001，父节点：1693234510000
   * 4. 最终消息树：
   *    root 
   *    ├── [1693234215000] 用户:"Hello" 
   *    │   ├── [1693234215001] AI:"Hi there!"
   *    │   └── [1693234340000] AI:"Hello! How are you?"
   *    └── [1693234510000] 用户:"Hi"           (编辑产生的新分支)
   *        └── [1693234510001] AI:"Hi! Nice to meet you!"
   * 
   * 【参数的真实值示例】
   * 
   * convId 的实际值：
   * - "conv-1693234200000" (2023年8月28日创建的会话)
   * - "conv-1693320600000" (2023年8月29日创建的会话)
   * - "conv-1693407000000" (2023年8月30日创建的会话)
   * 
   * leafNodeId 的实际值（都是数字时间戳）：
   * - 1693234215000 (2023-08-28 14:30:15.000 的消息)
   * - 1693234340000 (2023-08-28 14:32:20.000 的消息)  
   * - 1693234510000 (2023-08-28 14:35:10.000 的消息)
   * 
   * onChunk 回调函数的调用：
   * - 每收到AI的一小段文字时调用：onChunk() (无参数)
   * - 消息生成完成时调用：onChunk(1693234215001) (传入新消息ID)
   * 
   * 【关键问题解答：为什么需要setPending AND onChunk？】
   * 
   * 这是一个经典的"关注点分离"设计模式：
   * 
   * 📊 setPending - 负责"数据层"
   * ├── 管理React状态更新
   * ├── 保存AI生成的内容数据  
   * ├── 触发组件重新渲染
   * └── 维护消息的完整信息
   * 
   * 🎨 onChunk - 负责"表现层" 
   * ├── 控制UI滚动行为
   * ├── 管理用户焦点和注意力
   * ├── 触发界面动画效果
   * └── 处理消息树导航状态
   * 
   * 【实际工作流程】
   * 1. 收到AI数据片段
   * 2. setPending() → 更新React状态 → 新内容出现在界面
   * 3. onChunk() → 滚动到底部 → 用户能看到新内容
   * 4. 重复步骤2-3直到生成完成
   * 5. onChunk(messageId) → 设置最终的活跃消息节点
   * 
   * 【缺一不可的原因】
   * - 只有setPending：内容更新了但用户可能看不到（需要手动滚动）
   * - 只有onChunk：UI行为正确但没有实际内容（空的滚动）
   * - 两者配合：完美的用户体验（内容实时更新+自动可见）
   * 
   * 【数据流向】
   * 1. 用leafNodeId找到对话分支的路径
   * 2. 构建从root到leafNodeId的完整消息链
   * 3. 将消息链发送给AI服务器
   * 4. AI基于完整上下文生成新回复
   * 5. 新回复作为leafNodeId的子节点保存
   */
  const generateMessage = async (
    convId: string,
    leafNodeId: Message['id'],
    onChunk: CallbackGeneratedChunk
  ) => {
    /* 防御性检查：如果当前会话正在生成消息，直接返回避免重复请求 */
    if (isGenerating(convId)) return;

    /* 获取用户配置信息（如AI参数设置、系统消息等） */
    const config = StorageUtils.getConfig();
    /* 从本地存储获取当前会话的完整信息 */
    const currConversation = await StorageUtils.getOneConversation(convId);
    /* 安全检查：如果会话不存在，抛出错误避免后续操作失败 */
    if (!currConversation) {
      throw new Error('Current conversation is not found');
    }

    /* 
     * ========== 关键步骤：基于leafNodeId构建消息链 ==========
     * 
     * 这里是leafNodeId发挥作用的地方！
     * 
     * 1. 获取该会话的所有消息（包括所有分支的消息）
     * 2. 使用leafNodeId作为起点，向上追溯到root节点
     * 3. 构建从root到leafNodeId的完整路径，这就是AI需要的上下文
     * 
     * 举例说明：
     * 假设消息树如下：
     *   root
     *    └── [1001] 用户: "你好" 
     *        ├── [1002] AI: "你好！"
     *        │   └── [1003] 用户: "今天天气怎么样？"
     *        │       └── [1004] AI: "今天天气很好"
     *        └── [1005] AI: "Hi there!"  (重新生成的回复)
     *            └── [1006] 用户: "介绍一下你自己"
     *                └── [1007] AI: "我是AI助手" 
     * 
     * 如果 leafNodeId = 1006，那么过滤后的消息链是：
     * [root] -> [1001] -> [1005] -> [1006]
     * 
     * 注意：不包括1002、1003、1004这个分支，因为它们不在1006的路径上
     * 这样AI就能基于正确的上下文(你好->Hi there!->介绍一下你自己)生成回复
     * 
     * false参数的含义：
     * - true: 不包含leafNodeId本身，只要它的祖先
     * - false: 包含leafNodeId，构建完整路径用于生成回复
     */
    const currMessages = StorageUtils.filterByLeafNodeId(
      await StorageUtils.getMessages(convId),  // 获取当前会话的所有消息而不是其他会话，因为需要从当前会话（消息树）中提取传给服务器的上下文消息
      leafNodeId,                              // 从这个节点开始追溯其父节点，即获取以当前节点为止的所有父节点作为传给服务器进行推理的上下文消息
      false                                    // 包含leafNodeId本身
    );
    /* 
     * 创建中止控制器：类似于C++中的取消操作机制
     * 允许用户在生成过程中随时停止AI回复
     */
    const abortController = new AbortController();
    /* 将中止控制器保存到状态中，供其他函数调用 */
    setAbort(convId, abortController);

    /* 再次安全检查：确保消息链存在 */
    if (!currMessages) {
      throw new Error('Current messages are not found');
    }

    /* 
     * 创建待处理消息的唯一ID：使用当前时间戳+1确保唯一性
     * 类似于C++中生成唯一标识符的做法
     */
    const pendingId = Date.now() + 1;
    /* 
     * =============== Pending 概念的直观理解 ===============
     * 
     * 【什么是Pending？】
     * Pending就像是一个"占位符"或"预留座位"的概念：
     * 
     * 🎬 电影院比喻：
     * - 你买了电影票（用户发送消息）
     * - 工作人员为你预留座位，放上"已预订"标识（创建pendingMsg）
     * - 座位还是空的，但显示"有人要坐"（content: null但显示加载状态）
     * - 你到达后坐下（AI生成完内容后保存为正式消息）
     * 
     * 🍕 外卖订单比喻：
     * - 你下单披萨（请求AI回复）
     * - 系统创建订单号，状态显示"制作中"（pendingMsg）
     * - 披萨还在做，但你知道有一份属于你的披萨在路上（UI显示正在生成）
     * - 披萨做好送达（AI回复完成，转为正式消息）
     * 
     * 🚗 停车位比喻：
     * - 你的车正在进入停车场（AI开始生成）
     * - 系统为你预留一个停车位，立即显示"占用中"（pendingMsg出现）
     * - 车位暂时空着，但其他人知道有车要停（UI显示占位符）
     * - 你的车停好了（内容生成完成）
     * 
     * 【Pending在聊天中的作用】
     * 
     * 1️⃣ 即时反馈：
     * - 用户一发送消息，立即看到"AI正在思考..."
     * - 不需要等5-10秒才有反应，避免用户焦虑
     * - 类似于打电话时听到"嘟嘟"声，知道在连接中
     * 
     * 2️⃣ 占位显示：
     * - 在消息列表中预先占用一个位置
     * - 防止后续消息位置跳跃，保持界面稳定
     * - 就像Word文档中的分页符，提前占好位置
     * 
     * 3️⃣ 状态管理：
     * - 系统知道"这个会话正在生成中"
     * - 防止用户重复点击发送按钮
     * - 类似于ATM机取钱时显示"处理中，请稍候"
     * 
     * 【PendingMessage对象详解】
     * 
     * 创建待处理消息对象，它就像一个"消息模板"：
     * - id: 唯一身份证号（1693234567890）
     * - convId: 属于哪个聊天室（"conv-1693234200000"）
     * - type: 消息类型，这里是文本（'text'）
     * - timestamp: 出生时间（1693234567890）
     * - role: 角色标识，'assistant'表示这是AI的回复
     * - content: null ⭐ 关键！表示"内容正在路上，请稍候"
     * - parent: 父消息ID，告诉系统这条回复是对哪条消息的回应
     * - children: 子消息列表，初始为空数组
     * 
     * 【content: null 的深层含义】
     * 
     * 这是Pending的核心特征！
     * - null ≠ 空字符串 ""
     * - null 表示"数据还不存在，但很快就有了"
     * - "" 表示"数据存在，但内容是空的"
     * 
     * 类比：
     * - null: 包裹还在运输途中（快递显示"运输中"）
     * - "": 包裹到了，但里面是空的（快递显示"已签收"但箱子是空的）
     * 
     * 【实际用户体验 - 完整时间线】
     * 
     * ⏰ T=0ms: 用户点击发送"你好"
     * 📝 T=10ms: 立即显示
     *    用户: 你好
     *    AI: [正在思考中...] ⬅️ pendingMsg创建，content=null
     * 
     * ⏰ T=2000ms: AI开始回复（第1个数据包）
     * 📝 T=2010ms: 显示
     *    用户: 你好
     *    AI: 你好 ⬅️ content从null变为"你好"
     * 
     * ⏰ T=2200ms: 第2个数据包到达
     * 📝 T=2210ms: 显示  
     *    用户: 你好
     *    AI: 你好！很高兴 ⬅️ content = "你好" + "！很高兴"
     * 
     * ⏰ T=2400ms: 第3个数据包到达
     * 📝 T=2410ms: 显示
     *    用户: 你好
     *    AI: 你好！很高兴认识你！ ⬅️ content = "你好！很高兴" + "认识你！"
     * 
     * ⏰ T=2500ms: 生成完成
     * 📝 T=2510ms: 最终显示
     *    用户: 你好
     *    AI: 你好！很高兴认识你！ ⬅️ pendingMsg转为正式Message并保存
     * 
     * 【Pending的完整生命周期】
     * 
     * 🌱 诞生阶段：
     * - 创建pendingMsg对象
     * - content = null (表示"即将有内容")
     * - 用户看到加载状态
     * 
     * 🌿 成长阶段：
     * - 接收数据流，content逐渐增长
     * - 从null → "你" → "你好" → "你好！" → ...
     * - 用户看到打字机效果
     * 
     * 🌳 成熟阶段：
     * - 数据流结束，内容完整
     * - pendingMsg包含完整的AI回复
     * - 准备转为正式消息
     * 
     * 💾 保存阶段：
     * - 将pendingMsg保存到数据库
     * - 从临时状态转为永久记录
     * - 清除pending状态标记
     * 
     * 【为什么这样设计？】
     * 
     * 1. 用户体验优先：即时反馈，不让用户等待
     * 2. 状态可见：用户随时知道系统在做什么
     * 3. 渐进显示：模拟人类思考和说话的自然过程
     * 4. 错误处理：如果出错可以清除pending状态
     */
    let pendingMsg: PendingMessage = {
      id: pendingId,
      convId,
      type: 'text',
      timestamp: pendingId,
      role: 'assistant',
      content: null,
      parent: leafNodeId,
      children: [],
    };
    /* 将待处理消息设置到状态中，UI会显示"正在生成"的状态 */
    setPending(convId, pendingMsg);

    /* 开始try-catch块处理整个生成过程，类似于C++的异常处理 */
    try {
      /* 
       * 准备发送给AI服务器的消息数组：
       * 1. 如果用户设置了系统消息（类似AI的"人设"），添加到开头
       * 2. 将当前对话历史消息转换为API所需的格式
       * 使用展开运算符(...)将数组合并，类似于C++中的vector合并
       */
      let messages: APIMessage[] = [
        /* 条件性添加系统消息：如果系统消息不为空，则添加到消息列表开头 */
        ...(config.systemMessage.length === 0
          ? [] /* 空数组 */
          : [{ role: 'system', content: config.systemMessage } as APIMessage]),
        /* 将历史消息标准化为API格式 */
        ...normalizeMsgsForAPI(currMessages),
      ];
      /* 
       * 如果用户选择排除"思考过程"（某些AI模型会显示推理过程），
       * 则过滤掉这些内容，只保留最终回答
       */
      if (config.excludeThoughtOnReq) {
        messages = filterThoughtFromMsgs(messages);
      }
      /* 开发模式下打印消息内容，便于调试 */
      if (isDev) console.log({ messages });

      /* 
       * 准备API请求参数：这些参数控制AI的生成行为
       * 类似于调用C++函数时传递的参数结构体
       */
      const params = {
        messages, /* 要发送的消息历史 */
        stream: true, /* 启用流式传输：AI边生成边发送，而不是生成完再一次性发送 */
        cache_prompt: true, /* 缓存提示词：提高响应速度 */
        reasoning_format: 'none', /* 推理格式：不显示AI的思考过程 */
        
        /* 以下是AI生成文本的各种控制参数，影响输出的随机性和质量： */
        samplers: config.samplers, /* 采样器配置 */
        temperature: config.temperature, /* 温度：控制随机性，值越高越随机 */
        dynatemp_range: config.dynatemp_range, /* 动态温度范围 */
        dynatemp_exponent: config.dynatemp_exponent, /* 动态温度指数 */
        top_k: config.top_k, /* Top-K采样：只考虑概率最高的K个词 */
        top_p: config.top_p, /* Top-P采样：考虑累计概率达到P的词 */
        min_p: config.min_p, /* 最小概率阈值 */
        typical_p: config.typical_p, /* 典型概率采样 */
        xtc_probability: config.xtc_probability, /* XTC概率参数 */
        xtc_threshold: config.xtc_threshold, /* XTC阈值参数 */
        
        /* 重复惩罚相关参数：防止AI重复说同样的话 */
        repeat_last_n: config.repeat_last_n, /* 检查最近N个token的重复 */
        repeat_penalty: config.repeat_penalty, /* 重复惩罚强度 */
        presence_penalty: config.presence_penalty, /* 存在惩罚：降低重复话题概率 */
        frequency_penalty: config.frequency_penalty, /* 频率惩罚：根据词频调整概率 */
        
        /* DRY（Don't Repeat Yourself）相关参数：更高级的重复控制 */
        dry_multiplier: config.dry_multiplier,
        dry_base: config.dry_base,
        dry_allowed_length: config.dry_allowed_length,
        dry_penalty_last_n: config.dry_penalty_last_n,
        
        max_tokens: config.max_tokens, /* 最大生成token数量：限制回复长度 */
        timings_per_token: !!config.showTokensPerSecond, /* 是否显示生成速度信息 */
        
        /* 
         * 自定义参数：如果用户设置了额外参数，解析并添加
         * ...运算符展开JSON对象，类似于C++的结构体合并
         */
        ...(config.custom.length ? JSON.parse(config.custom) : {}),
      };

      /* 
       * 发送HTTP POST请求到AI服务器：
       * 类似于C++中的网络请求，但这里使用JavaScript的fetch API
       */
      const fetchResponse = await fetch(`${BASE_URL}/v1/chat/completions`, {
        method: 'POST', /* 使用POST方法发送数据 */
        headers: {
          'Content-Type': 'application/json', /* 告诉服务器我们发送的是JSON数据 */
          /* 
           * 条件性添加API密钥：如果用户设置了API密钥，添加到请求头中
           * Bearer token是一种常见的API认证方式
           */
          ...(config.apiKey
            ? { Authorization: `Bearer ${config.apiKey}` }
            : {}),
        },
        body: JSON.stringify(params), /* 将参数对象转换为JSON字符串发送 */
        signal: abortController.signal, /* 绑定中止信号，允许取消请求 */
      });
      /* 
       * 检查HTTP响应状态：200表示成功，其他状态码表示出错
       * 类似于C++中检查函数返回值是否成功
       */
      if (fetchResponse.status !== 200) {
        const body = await fetchResponse.json();
        /* 抛出错误，包含服务器返回的具体错误信息 */
        throw new Error(body?.error?.message || 'Unknown error');
      }
      
      /* 
       * ========== 获取并处理服务器端事件流(SSE) ==========
       * 
       * 这里开始处理AI服务器返回的流式数据：
       * - AI不会等到完整回复生成完毕才返回
       * - 而是边生成边发送，实现实时的"打字机"效果
       * - getSSEStreamAsync()将HTTP流转换为可迭代的JavaScript对象
       * - 完整的数据流向：服务器SSE流 → getSSEStreamAsync解析 → chunks循环处理 → pendingMsg累积 → 思维链解析 → ThoughtProcess + MarkdownDisplay渲染
       */
      const chunks = getSSEStreamAsync(fetchResponse);
      
      /* 
       * ========== 流式数据处理的核心循环 ==========
       * 
       * 【循环机制说明】
       * for await...of 是异步迭代语法，用于处理流式数据：
       * - 每次循环等待下一个数据块的到达
       * - 数据块按时间顺序依次处理
       * - 自动处理网络延迟和背压控制
       * - 循环在数据流结束时自动退出
       * 
       * 【实际数据流时序】
       * T=0ms:    开始循环，等待第一个chunk
       * T=500ms:  收到chunk1: {content: "你"}
       * T=600ms:  收到chunk2: {content: "好"}  
       * T=750ms:  收到chunk3: {content: "！很高兴"}
       * T=900ms:  收到chunk4: {content: "认识你"}
       * T=1000ms: 收到[DONE]标记，循环结束
       * 
       * 【为什么用异步循环？】
       * 1. 非阻塞：不会冻结UI线程
       * 2. 实时性：数据到达立即处理
       * 3. 错误处理：可以优雅处理网络异常
       * 4. 内存效率：逐块处理，不需要缓存全部数据
       */
      for await (const chunk of chunks) {
        /* 
         * ========== 错误检测和处理 ==========
         * 
         * 服务器可能在数据流中发送错误信息：
         * - 模型加载失败
         * - 内存不足
         * - 推理超时
         * - API限制等
         * 
         * 错误格式通常是：{error: {message: "具体错误信息"}}
         */
        if (chunk.error) {
          throw new Error(chunk.error?.message || 'Unknown error');
        }
        
        /* 
         * ========== 提取新生成的文本内容 ==========
         * 
         * 【数据结构解析】
         * 标准的OpenAI兼容响应格式：
         * ```json
         * {
         *   "choices": [{
         *     "delta": {
         *       "content": "新生成的文本片段"
         *     }
         *   }]
         * }
         * ```
         * 
         * chunk.choices[0].delta.content 包含本次新增的文本
         * 
         * 【为什么是choices[0]？】
         * - OpenAI API支持同时生成多个候选回复
         * - choices数组包含所有候选项
         * - [0]表示选择第一个（通常也是唯一的）候选回复
         * 
         * 【为什么是delta而不是content？】
         * - delta表示增量变化（新增的部分）
         * - content会是完整内容（会造成数据冗余）
         * - 流式传输中使用delta更高效
         * 
         * 【实际数据示例】
         * chunk1: {choices: [{delta: {content: "你"}}]}
         * chunk2: {choices: [{delta: {content: "好"}}]} 
         * chunk3: {choices: [{delta: {content: "！很高兴认识你"}}]}
         */
        const addedContent = chunk.choices[0].delta.content;
        
        /* 
         * 获取已累积的内容：
         * - pendingMsg.content 存储到目前为止已生成的所有文本
         * - 第一次时content为null，所以使用||操作符提供空字符串默认值
         * - 这是JavaScript中处理null/undefined的常用模式
         */
        const lastContent = pendingMsg.content || '';
        
        /* 
         * ========== 累积式内容更新 - 实现打字机效果 ==========
         * 
         * 只有当addedContent存在时才进行更新：
         * - 有时服务器会发送只包含metadata的chunk（如timing信息）
         * - 这些chunk的content字段为null或undefined
         * - 通过if条件避免将null追加到字符串中
         */
        if (addedContent) {
          
          /* 
           * ========== 核心：流式内容累积算法 ==========
           * 
           * 【算法原理】
           * 这里实现了增量文本拼接，是实现AI"打字机效果"的关键：
           * 
           * 初始状态：content = null
           * 第1次更新：content = "" + "你" = "你"
           * 第2次更新：content = "你" + "好" = "你好"  
           * 第3次更新：content = "你好" + "！" = "你好！"
           * 第4次更新：content = "你好！" + "很高兴认识你" = "你好！很高兴认识你"
           * 
           * 【为什么不直接替换而是拼接？】
           * 1. 服务器发送的是增量数据（delta），不是完整内容
           * 2. 拼接保持了文本的完整性和连续性
           * 3. 用户看到文本逐渐"生长"，体验更自然
           * 4. 避免了重复传输已生成的内容，节省带宽
           * 
           * 【React不可变更新模式】
           * 使用扩展运算符{...pendingMsg}创建新对象：
           * - React通过对象引用判断状态是否发生变化
           * - 直接修改pendingMsg.content不会触发重新渲染  
           * - 创建新对象确保React能检测到状态变化
           * - 这是React函数组件的最佳实践
           * 
           * 【等价的命令式代码】
           * ```javascript
           * // React不推荐的写法（会导致渲染问题）
           * pendingMsg.content = lastContent + addedContent;
           * 
           * // React推荐的写法（确保正确渲染）
           * pendingMsg = { ...pendingMsg, content: lastContent + addedContent };
           * ```
           * 
           * 【类比C++的字符串拼接】
           * ```cpp
           * std::string accumulated = "";
           * while (有新数据) {
           *     std::string newChunk = getNextChunk();
           *     accumulated += newChunk;  // 等价于lastContent + addedContent
           *     updateUI(accumulated);    // 等价于setPending()
           * }
           * ```
           */
          pendingMsg = {
            ...pendingMsg, /* 保持其他字段（id, role, timestamp等）不变 */
            content: lastContent + addedContent, /* 关键：拼接新内容实现累积效果 */
          };
        }
        
        /* 
         * ========== 处理性能统计信息（可选） ==========
         * 
         * 【什么是timings？】
         * 服务器可能在响应中包含性能统计数据：
         * - 提示词处理时间和token数量
         * - 文本生成时间和token数量  
         * - 用于显示生成速度（如"25.3 tokens/sec"）
         * 
         * 【为什么要条件检查？】
         * 1. timings字段是可选的，不是所有chunk都包含
         * 2. config.showTokensPerSecond 是用户设置，可能被禁用
         * 3. 避免不必要的数据处理和内存使用
         */
        const timings = chunk.timings;
        if (timings && config.showTokensPerSecond) {
          /* 
           * 提取关键性能指标：
           * 
           * prompt_n: 提示词包含的token数量（输入长度）
           * prompt_ms: 处理提示词耗时，毫秒（理解用户问题的时间）
           * predicted_n: 已生成的token数量（输出长度）
           * predicted_ms: 生成这些token的总耗时，毫秒
           * 
           * 【为什么只保存这4个字段？】
           * 1. 原始timings对象可能包含更多调试信息
           * 2. 只保存UI需要显示的核心指标
           * 3. 减少内存使用和JSON序列化开销
           * 4. ChatMessage组件会根据这些数据计算速度
           */
          pendingMsg.timings = {
            prompt_n: timings.prompt_n,
            prompt_ms: timings.prompt_ms,
            predicted_n: timings.predicted_n,
            predicted_ms: timings.predicted_ms,
          };
        }
        
        /* 
         * ========== 双重更新机制：数据+UI ==========
         * 
         * 这里同时调用两个函数实现完整的实时更新：
         * 
         * 【第1步：setPending - 数据层更新】
         * 作用：更新React组件的状态数据
         * 触发：React重新渲染，新内容出现在DOM中
         * 负责：确保用户能看到最新的AI生成内容
         * 
         * 类比：更新C++类的成员变量，改变对象状态
         * ```cpp
         * messageObject.content = newContent;  // 相当于setPending
         * ```
         * 
         * 【第2步：onChunk - 表现层更新】
         * 作用：处理UI行为和用户体验
         * 触发：滚动到消息底部，确保新内容可见
         * 负责：焦点管理、动画效果、界面交互
         * 
         * 类比：调用C++的UI更新函数，处理界面行为  
         * ```cpp
         * scrollToBottom();     // 相当于onChunk
         * refreshDisplay();     // 相当于onChunk
         * ```
         * 
         * 【为什么需要两个函数？】
         * 关注点分离（Separation of Concerns）设计原则：
         * - setPending专注数据管理
         * - onChunk专注用户体验
         * - 各司其职，职责清晰
         * - 便于维护和调试
         * 
         * 【如果缺少其中一个会怎样？】
         * 
         * 只有setPending，没有onChunk：
         * ❌ 内容更新了，但用户可能看不到（需要手动滚动）
         * ❌ 新内容可能出现在屏幕可视区域之外
         * ❌ 用户体验差，感觉"卡顿"
         * 
         * 只有onChunk，没有setPending：
         * ❌ UI行为正确（滚动正常），但内容是空的
         * ❌ 用户看到滚动动作，但没有实际的新文字出现
         * ❌ React组件状态与实际显示不同步
         * 
         * 两者配合的完美效果：
         * ✅ 新内容实时出现 + 自动滚动到可见位置
         * ✅ 流畅的"打字机"效果
         * ✅ 用户始终能看到最新生成的内容
         * 
         * 【调用时序】
         * 1. 收到新的chunk数据
         * 2. 更新pendingMsg对象（内存中的数据）
         * 3. setPending() → React状态更新 → 触发重新渲染
         * 4. onChunk() → 滚动控制 → 新内容变为可见
         * 5. 用户看到AI在"输入"新文字
         * 
         * 【类比生活场景】
         * 就像看电视时：
         * - setPending = 电视播放新画面（内容更新）
         * - onChunk = 你把头转向电视（确保能看到）
         * - 缺少任何一个，你都无法正常观看节目
         */
        setPending(convId, pendingMsg);
        
        /* 
         * 触发UI更新回调：
         * - 注意这里不传递参数给onChunk()
         * - 表示这是中间过程的更新，不是最终消息
         * - onChunk内部会处理滚动和焦点管理
         * - 在生成完成时会调用onChunk(messageId)传递最终消息ID
         */
        onChunk();
      }
    /* 
     * 异常处理块：处理生成过程中可能出现的错误
     * 类似于C++的catch块
     */
    } catch (err) {
      /* 清除待处理状态：移除"正在生成中"的显示 */
      setPending(convId, null);
      
      /* 
       * 区分错误类型：
       * AbortError是用户主动取消生成，这是正常操作，不需要报错
       */
      if ((err as Error).name === 'AbortError') {
        /* 
         * 用户通过stopGeneration()函数停止了生成
         * 这是预期行为，可以安全忽略这个错误
         */
      } else {
        /* 其他错误需要处理：记录到控制台便于调试 */
        console.error(err);
        /* 显示错误提示给用户：使用toast显示友好的错误信息 */
        toast.error((err as any)?.message ?? 'Unknown error');
        /* 重新抛出错误：让调用方知道操作失败了 */
        throw err;
      }
    }

    /* 
     * 最终处理：如果成功生成了内容，保存到本地存储
     * 只有当content不为null时才保存（即成功生成了内容）
     */
    if (pendingMsg.content !== null) {
      /* 
       * 将临时的待处理消息转换为正式消息并保存：
       * - 保存到本地存储（类似于写入数据库）
       * - leafNodeId是父节点ID，用于构建消息树结构
       */
      await StorageUtils.appendMsg(pendingMsg as Message, leafNodeId);
    }
    
    /* 清除待处理状态：生成完成，移除"正在生成中"显示 */
    setPending(convId, null);
    
    /* 
     * ========== 最终的onChunk调用 - 完成通知 ==========
     * 
     * 这里的onChunk调用与循环中的调用有重要区别：
     * 
     * 【循环中的onChunk()】（无参数）
     * - 表示："正在生成中，请滚动显示最新内容"
     * - 频繁调用，每收到一小段文字就调用一次
     * - 用于实现打字机效果的实时滚动
     * 
     * 【最终的onChunk(pendingId)】（有参数）
     * - 表示："生成完成，请切换到这个新消息节点"
     * - 只调用一次，在整个生成过程结束时
     * - 传入的pendingId是新生成消息的完整ID
     * - 用于更新消息树的当前节点指针
     * 
     * 【为什么需要传递pendingId？】
     * 1. 节点切换：告诉UI当前应该显示哪个消息作为"活跃节点"
     * 2. 状态同步：确保消息树的导航状态与实际显示一致
     * 3. 历史记录：用户可以通过前进/后退浏览不同的AI回复
     * 4. 分支管理：在有多个AI回复分支时，标记当前查看的是哪一个
     * 
     * 【实际效果】
     * - 用户会看到聊天界面滚动到最底部
     * - 新生成的AI消息被标记为"当前消息"
     * - 如果有多个AI回复选项，会显示当前选中的是哪一个
     * - 为后续的用户操作（如重新生成、编辑等）做好准备
     * 
     * 【类比C++】
     * ```cpp
     * // 类似于完成后的状态更新
     * currentActiveNode = newMessageId;
     * scrollToPosition(BOTTOM);
     * updateNavigationState(newMessageId);
     * ```
     */
    onChunk(pendingId);
  };

  const sendMessage = async (
    convId: string | null,
    leafNodeId: Message['id'] | null,
    content: string,
    extra: Message['extra'],
    onChunk: CallbackGeneratedChunk
  ): Promise<boolean> => {
    if (isGenerating(convId ?? '') || content.trim().length === 0) return false;

    if (convId === null || convId.length === 0 || leafNodeId === null) {
      const conv = await StorageUtils.createConversation(
        content.substring(0, 256)
      );
      convId = conv.id;
      leafNodeId = conv.currNode;
      // if user is creating a new conversation, redirect to the new conversation
      navigate(`/chat/${convId}`);
    }

    const now = Date.now();
    const currMsgId = now;
    StorageUtils.appendMsg(
      {
        id: currMsgId,
        timestamp: now,
        type: 'text',
        convId,
        role: 'user',
        content,
        extra,
        parent: leafNodeId,
        children: [],
      },
      leafNodeId
    );
    onChunk(currMsgId);

    try {
      await generateMessage(convId, currMsgId, onChunk);
      return true;
    } catch (_) {
      // TODO: rollback
    }
    return false;
  };

  const stopGenerating = (convId: string) => {
    setPending(convId, null);
    aborts[convId]?.abort();
  };

  // if content is undefined, we remove last assistant message
  const replaceMessageAndGenerate = async (
    convId: string,
    parentNodeId: Message['id'], // the parent node of the message to be replaced
    content: string | null,
    extra: Message['extra'],
    onChunk: CallbackGeneratedChunk
  ) => {
    if (isGenerating(convId)) return;

    if (content !== null) {
      const now = Date.now();
      const currMsgId = now;
      StorageUtils.appendMsg(
        {
          id: currMsgId,
          timestamp: now,
          type: 'text',
          convId,
          role: 'user',
          content,
          extra,
          parent: parentNodeId,
          children: [],
        },
        parentNodeId
      );
      parentNodeId = currMsgId;
    }
    onChunk(parentNodeId);

    await generateMessage(convId, parentNodeId, onChunk);
  };

  const saveConfig = (config: typeof CONFIG_DEFAULT) => {
    StorageUtils.setConfig(config);
    setConfig(config);
  };

  return (
    <AppContext.Provider
      value={{
        isGenerating,
        viewingChat,
        pendingMessages,
        sendMessage,
        stopGenerating,
        replaceMessageAndGenerate,
        canvasData,
        setCanvasData,
        config,
        saveConfig,
        showSettings,
        setShowSettings,
        serverProps,
      }}
    >
      {children}
    </AppContext.Provider>
  );
};

export const useAppContext = () => useContext(AppContext);
