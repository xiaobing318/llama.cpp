import { useMemo, useState } from 'react';
import { useAppContext } from '../utils/app.context';
import { Message, PendingMessage } from '../utils/types';
import { classNames } from '../utils/misc';
import MarkdownDisplay, { CopyButton } from './MarkdownDisplay';
import {
  ArrowPathIcon,
  ChevronLeftIcon,
  ChevronRightIcon,
  PencilSquareIcon,
} from '@heroicons/react/24/outline';
import ChatInputExtraContextItem from './ChatInputExtraContextItem';
import { BtnWithTooltips } from '../utils/common';

interface SplitMessage {
  content: PendingMessage['content'];
  thought?: string;
  isThinking?: boolean;
}

export default function ChatMessage({
  msg,
  siblingLeafNodeIds,
  siblingCurrIdx,
  id,
  onRegenerateMessage,
  onEditMessage,
  onChangeSibling,
  isPending,
}: {
  msg: Message | PendingMessage;
  siblingLeafNodeIds: Message['id'][];
  siblingCurrIdx: number;
  id?: string;
  onRegenerateMessage(msg: Message): void;
  onEditMessage(msg: Message, content: string): void;
  onChangeSibling(sibling: Message['id']): void;
  isPending?: boolean;
}) {
  const { viewingChat, config } = useAppContext();
  const [editingContent, setEditingContent] = useState<string | null>(null);
  const timings = useMemo(
    () =>
      msg.timings
        ? {
            ...msg.timings,
            prompt_per_second:
              (msg.timings.prompt_n / msg.timings.prompt_ms) * 1000,
            predicted_per_second:
              (msg.timings.predicted_n / msg.timings.predicted_ms) * 1000,
          }
        : null,
    [msg.timings]
  );
  const nextSibling = siblingLeafNodeIds[siblingCurrIdx + 1];
  const prevSibling = siblingLeafNodeIds[siblingCurrIdx - 1];

  /* 
   * =================== 思维链解析的核心逻辑 ===================
   * 
   * 【什么是思维链（Chain of Thought）？】
   * 思维链是AI推理模型（如DeepSeek-R1, o1等）的特殊功能：
   * - AI在回答问题前会先"思考"
   * - 思考过程用特殊标签包围：<think>...思考内容...</think>
   * - 最终答案在思维链标签之外
   * - 用户可以选择是否查看AI的思考过程
   * 
   * 【思维链消息格式示例】
   * ```
   * <think>
   * 用户问的是数学问题，我需要一步步解决：
   * 1. 首先理解题目
   * 2. 确定求解方法
   * 3. 逐步计算
   * </think>
   * 
   * 这是一道关于面积的数学题。让我来为您解答：
   * 矩形面积 = 长 × 宽 = 5 × 3 = 15平方米
   * ```
   * 
   * 【解析目标】
   * 将混合内容分离成三部分：
   * 1. content: 实际回复内容（用户真正需要的答案）
   * 2. thought: 思考过程内容（AI的推理步骤）
   * 3. isThinking: 是否还在思考中（影响UI显示状态）
   * 
   * 【为什么使用useMemo？】
   * 1. 性能优化：避免每次渲染都重新解析
   * 2. 内容可能很长：解析过程计算密集
   * 3. 依赖稳定：只有msg变化时才重新解析
   * 4. 减少不必要的DOM更新
   */
  const { content, thought, isThinking }: SplitMessage = useMemo(() => {
    /* 
     * ========== 预处理检查 ==========
     * 
     * 【什么情况不需要解析思维链？】
     * 1. msg.content === null: 消息还没有内容（正在生成中）
     * 2. msg.role !== 'assistant': 不是AI回复（用户消息不会有思维链）
     * 
     * 这些情况直接返回原始content，避免不必要的处理
     */
    if (msg.content === null || msg.role !== 'assistant') {
      return { content: msg.content };
    }
    
    /* 
     * ========== 思维链标签的正则表达式定义 ==========
     * 
     * 【支持的开始标签格式】
     * - <think>: 标准的思维链开始标签
     * - <|channel|>analysis<|message|>: 某些模型使用的特殊格式
     * 
     * 【支持的结束标签格式】  
     * - </think>: 标准的思维链结束标签
     * - <|end|>: 某些模型使用的特殊结束标记
     * 
     * 【为什么用正则表达式？】
     * 1. 灵活性：支持多种标签格式
     * 2. 效率：直接分割字符串，无需复杂解析
     * 3. 兼容性：适应不同AI模型的标签约定
     * 4. 扩展性：未来可以轻松添加新的标签格式
     * 
     * 【正则表达式解释】
     * /<think>|<\|channel\|>analysis<\|message\|>/ 
     * - |: 或操作符，匹配任意一种格式
     * - \|: 转义管道符，因为|在正则中有特殊含义
     * - 匹配整个开始标签作为分割点
     */
    const REGEX_THINK_OPEN = /<think>|<\|channel\|>analysis<\|message\|>/;
    const REGEX_THINK_CLOSE = /<\/think>|<\|end\|>/;
    
    /* 
     * ========== 初始化解析状态变量 ==========
     * 
     * 这些变量会在解析过程中不断更新：
     */
    let actualContent = '';  // 最终的回复内容（去除思维链后）
    let thought = '';        // 累积的思考过程内容
    let isThinking = false;  // 当前是否处于思考状态
    
    /* 
     * ========== 开始解析：寻找第一个开始标签 ==========
     * 
     * 【split(REGEX_THINK_OPEN, 2)的含义】
     * - 用开始标签作为分隔符分割字符串
     * - 参数2表示最多分割成2部分
     * - 结果：[思维链前的内容, 思维链开始后的所有内容]
     * 
     * 【示例】
     * 输入: "你好<think>这是思考</think>答案"
     * 分割: ["你好", "这是思考</think>答案"]
     */
    let thinkSplit = msg.content.split(REGEX_THINK_OPEN, 2);
    
    /* 将思维链前的内容加入最终回复 */
    actualContent += thinkSplit[0];
    
    /* 
     * ========== 循环处理所有思维链块 ==========
     * 
     * 【为什么需要while循环？】
     * 消息中可能包含多个思维链块：
     * ```
     * 前言 <think>第1次思考</think> 中间内容 <think>第2次思考</think> 结论
     * ```
     * 
     * 【循环条件说明】
     * thinkSplit[1] !== undefined 表示找到了开始标签，存在思维链内容
     */
    while (thinkSplit[1] !== undefined) {
      /* 
       * ========== 处理思维链内容：寻找结束标签 ==========
       * 
       * 此时 thinkSplit[1] 包含从开始标签之后的所有内容
       * 需要找到对应的结束标签来确定思考内容的边界
       * 
       * 【split(REGEX_THINK_CLOSE, 2)的作用】
       * - 用结束标签分割剩余内容
       * - 结果：[思考内容, 结束标签后的内容]
       * 
       * 【示例】
       * 输入: "这是思考内容</think>这是后续内容"
       * 分割: ["这是思考内容", "这是后续内容"]
       */
      thinkSplit = thinkSplit[1].split(REGEX_THINK_CLOSE, 2);
      
      /* 将思考内容累加到thought变量中 */
      thought += thinkSplit[0];
      
      /* 标记进入思考状态 */
      isThinking = true;
      
      /* 
       * ========== 检查是否找到了结束标签 ==========
       * 
       * 【两种情况】
       * 1. thinkSplit[1] !== undefined: 找到了结束标签
       *    - 思维链块完整，可以继续处理后续内容
       *    - 将isThinking重置为false
       *    - 准备处理下一个可能的思维链块
       * 
       * 2. thinkSplit[1] === undefined: 没找到结束标签  
       *    - 思维链还在进行中（流式生成时常见）
       *    - isThinking保持为true，用于显示"思考中"状态
       *    - 循环将结束，因为没有更多内容可处理
       */
      if (thinkSplit[1] !== undefined) {
        /* 找到了结束标签，思考过程完成 */
        isThinking = false;
        
        /* 
         * 准备处理结束标签后的内容：继续寻找下一个思维链
         * 
         * 【递归式处理】
         * 将结束标签后的内容再次用开始标签分割
         * 这样可以处理多个思维链块的情况
         * 
         * 【示例】
         * 当前: "中间内容<think>第2次思考</think>结论"  
         * 分割: ["中间内容", "第2次思考</think>结论"]
         */
        thinkSplit = thinkSplit[1].split(REGEX_THINK_OPEN, 2);
        
        /* 将思维链之间的内容加入最终回复 */
        actualContent += thinkSplit[0];
        
        /* 
         * 循环继续：
         * - 如果thinkSplit[1]存在，表示还有思维链需要处理
         * - 如果不存在，循环结束，解析完成
         */
      }
      /* 如果没找到结束标签，循环会自然结束，isThinking保持为true */
    }
    
    /* 
     * ========== 返回解析结果 ==========
     * 
     * 【返回对象字段说明】
     * - content: 纯净的回复内容，去除了所有思维链标签和内容
     * - thought: 所有思考过程的内容，用于在UI中展示
     * - isThinking: 布尔值，指示AI是否还在思考中
     * 
     * 【实际应用】
     * 1. content用于显示主要回复内容
     * 2. thought用于可折叠的"思维过程"区域
     * 3. isThinking用于显示加载动画或"思考中"状态
     * 
     * 【完整示例】
     * 输入消息内容:
     * ```
     * 让我想想这个问题。<think>
     * 这是一个数学问题，需要计算面积。
     * 公式是：面积 = 长 × 宽
     * 所以：5 × 3 = 15
     * </think>
     * 
     * 根据计算，矩形的面积是15平方米。
     * ```
     * 
     * 解析结果:
     * {
     *   content: "让我想想这个问题。\n\n根据计算，矩形的面积是15平方米。",
     *   thought: "\n这是一个数学问题，需要计算面积。\n公式是：面积 = 长 × 宽\n所以：5 × 3 = 15\n",
     *   isThinking: false
     * }
     */
    return { content: actualContent, thought, isThinking };
  }, [msg]);

  if (!viewingChat) return null;

  const isUser = msg.role === 'user';

  return (
    <div
      className="group"
      id={id}
      role="group"
      aria-description={`Message from ${msg.role}`}
    >
      <div
        className={classNames({
          chat: true,
          'chat-start': !isUser,
          'chat-end': isUser,
        })}
      >
        {msg.extra && msg.extra.length > 0 && (
          <ChatInputExtraContextItem items={msg.extra} clickToShow />
        )}

        <div
          className={classNames({
            'chat-bubble markdown': true,
            'chat-bubble bg-transparent': !isUser,
          })}
        >
          {/* textarea for editing message */}
          {editingContent !== null && (
            <>
              <textarea
                dir="auto"
                className="textarea textarea-bordered bg-base-100 text-base-content max-w-2xl w-[calc(90vw-8em)] h-24"
                value={editingContent}
                onChange={(e) => setEditingContent(e.target.value)}
              ></textarea>
              <br />
              <button
                className="btn btn-ghost mt-2 mr-2"
                onClick={() => setEditingContent(null)}
              >
                Cancel
              </button>
              <button
                className="btn mt-2"
                onClick={() => {
                  if (msg.content !== null) {
                    setEditingContent(null);
                    onEditMessage(msg as Message, editingContent);
                  }
                }}
              >
                Submit
              </button>
            </>
          )}
          {/* not editing content, render message */}
          {editingContent === null && (
            <>
              {content === null ? (
                <>
                  {/* show loading dots for pending message */}
                  <span className="loading loading-dots loading-md"></span>
                </>
              ) : (
                <>
                  {/* render message as markdown */}
                  <div dir="auto" tabIndex={0}>
                    {thought && (
                      <ThoughtProcess
                        isThinking={!!isThinking && !!isPending}
                        content={thought}
                        open={config.showThoughtInProgress}
                      />
                    )}

                    <MarkdownDisplay
                      content={content}
                      isGenerating={isPending}
                    />
                  </div>
                </>
              )}
              {/* render timings if enabled */}
              {timings && config.showTokensPerSecond && (
                <div className="dropdown dropdown-hover dropdown-top mt-2">
                  <div
                    tabIndex={0}
                    role="button"
                    className="cursor-pointer font-semibold text-sm opacity-60"
                  >
                    Speed: {timings.predicted_per_second.toFixed(1)} t/s
                  </div>
                  <div className="dropdown-content bg-base-100 z-10 w-64 p-2 shadow mt-4">
                    <b>Prompt</b>
                    <br />- Tokens: {timings.prompt_n}
                    <br />- Time: {timings.prompt_ms} ms
                    <br />- Speed: {timings.prompt_per_second.toFixed(1)} t/s
                    <br />
                    <b>Generation</b>
                    <br />- Tokens: {timings.predicted_n}
                    <br />- Time: {timings.predicted_ms} ms
                    <br />- Speed: {timings.predicted_per_second.toFixed(1)} t/s
                    <br />
                  </div>
                </div>
              )}
            </>
          )}
        </div>
      </div>

      {/* actions for each message */}
      {msg.content !== null && (
        <div
          className={classNames({
            'flex items-center gap-2 mx-4 mt-2 mb-2': true,
            'flex-row-reverse': msg.role === 'user',
          })}
        >
          {siblingLeafNodeIds && siblingLeafNodeIds.length > 1 && (
            <div
              className="flex gap-1 items-center opacity-60 text-sm"
              role="navigation"
              aria-description={`Message version ${siblingCurrIdx + 1} of ${siblingLeafNodeIds.length}`}
            >
              <button
                className={classNames({
                  'btn btn-sm btn-ghost p-1': true,
                  'opacity-20': !prevSibling,
                })}
                onClick={() => prevSibling && onChangeSibling(prevSibling)}
                aria-label="Previous message version"
              >
                <ChevronLeftIcon className="h-4 w-4" />
              </button>
              <span>
                {siblingCurrIdx + 1} / {siblingLeafNodeIds.length}
              </span>
              <button
                className={classNames({
                  'btn btn-sm btn-ghost p-1': true,
                  'opacity-20': !nextSibling,
                })}
                onClick={() => nextSibling && onChangeSibling(nextSibling)}
                aria-label="Next message version"
              >
                <ChevronRightIcon className="h-4 w-4" />
              </button>
            </div>
          )}
          {/* user message */}
          {msg.role === 'user' && (
            <BtnWithTooltips
              className="btn-mini w-8 h-8"
              onClick={() => setEditingContent(msg.content)}
              disabled={msg.content === null}
              tooltipsContent="Edit message"
            >
              <PencilSquareIcon className="h-4 w-4" />
            </BtnWithTooltips>
          )}
          {/* assistant message */}
          {msg.role === 'assistant' && (
            <>
              {!isPending && (
                <BtnWithTooltips
                  className="btn-mini w-8 h-8"
                  onClick={() => {
                    if (msg.content !== null) {
                      onRegenerateMessage(msg as Message);
                    }
                  }}
                  disabled={msg.content === null}
                  tooltipsContent="Regenerate response"
                >
                  <ArrowPathIcon className="h-4 w-4" />
                </BtnWithTooltips>
              )}
            </>
          )}
          <CopyButton className="btn-mini w-8 h-8" content={msg.content} />
        </div>
      )}
    </div>
  );
}

/* 
 * =================== ThoughtProcess 思维链展示组件 ===================
 * 
 * 【组件目的】
 * 这是专门用于显示AI思维过程的可折叠组件
 * 让用户能够查看AI在回答问题时的推理步骤和思考过程
 * 
 * 【核心功能】
 * 1. 可折叠面板：用户可以展开/收起思维链内容
 * 2. 动态状态：根据AI是否还在思考显示不同的标题
 * 3. 富文本显示：思维内容支持Markdown格式
 * 4. 无障碍访问：支持键盘导航和屏幕阅读器
 * 
 * 【参数说明】
 * @param isThinking - 是否正在思考中（影响标题和动画显示）
 * @param content - 思维过程的具体内容（Markdown格式）
 * @param open - 是否默认展开面板（用户设置）
 */
function ThoughtProcess({
  isThinking,
  content,
  open,
}: {
  isThinking: boolean;  /* AI是否还在思考中，决定显示状态 */
  content: string;      /* 思维链的具体内容，支持Markdown */
  open: boolean;        /* 面板默认展开状态，来自用户配置 */
}) {
  return (
    <div
      role="button"
      aria-label="Toggle thought process display"
      tabIndex={0}
      className={classNames({
        'collapse bg-none': true,  /* 使用DaisyUI的折叠组件样式 */
      })}
    >
      {/* 
       * ========== 折叠控制机制 ==========
       * 
       * 【DaisyUI collapse组件的工作原理】
       * - input[type="checkbox"] 控制展开/收起状态
       * - defaultChecked={open} 设置初始状态
       * - 用户点击标题时会自动切换checkbox状态
       * - CSS根据checkbox状态显示/隐藏内容区域
       * 
       * 【为什么用defaultChecked而不是checked？】
       * - defaultChecked: 只设置初始状态，之后由用户控制
       * - checked: 完全由React控制，需要额外的状态管理
       * - 这里让DaisyUI自己管理展开状态更简单高效
       */}
      <input type="checkbox" defaultChecked={open} />
      
      {/* 
       * ========== 可点击的标题区域 ========== 
       * 
       * 【标题区域的作用】
       * 1. 显示当前状态（思考中 vs 思维过程）
       * 2. 提供点击交互，控制面板展开/收起
       * 3. 视觉上突出思维链功能的存在
       */}
      <div className="collapse-title px-0">
        <div className="btn rounded-xl">
          {/* 
           * ========== 动态标题和状态显示 ==========
           * 
           * 【状态判断：isThinking】
           * 根据AI是否还在思考，显示不同的标题和动画：
           * 
           * 1. isThinking = true (正在思考)
           *    显示：[🔄 Thinking] 
           *    - 转圈动画表示AI正在进行推理
           *    - "Thinking"文字表明当前状态
           *    - 用户知道需要等待思考完成
           * 
           * 2. isThinking = false (思考完成)
           *    显示：[💭 Thought Process]
           *    - 静态标题，表示思考过程已完成
           *    - 用户可以展开查看完整的推理步骤
           *    - 不再有动画，表明内容稳定
           * 
           * 【动画设计】
           * loading-spinner: 旋转圆圈动画
           * loading-md: 中等尺寸，适合按钮内显示
           * mr-2: 右侧间距，与文字保持适当距离
           * verticalAlign: 'middle': 垂直居中对齐
           */}
          {isThinking ? (
            <span>
              {/* 思考中状态：显示加载动画 */}
              <span
                className="loading loading-spinner loading-md mr-2"
                style={{ verticalAlign: 'middle' }}
              ></span>
              Thinking
            </span>
          ) : (
            /* 思考完成状态：显示静态标题 */
            <>Thought Process</>
          )}
        </div>
      </div>
      
      {/* 
       * ========== 可折叠的内容区域 ==========
       * 
       * 【内容区域的特点】
       * 1. 只有在面板展开时才显示
       * 2. 支持键盘焦点，便于无障碍访问
       * 3. 视觉上与主要回复内容区分开
       * 4. 内容使用较小字号，表明这是辅助信息
       * 
       * 【样式设计说明】
       * - text-base-content/70: 70%透明度的文本颜色，视觉上弱化
       * - text-sm: 较小的字号，节省空间
       * - p-1: 较小的内边距，紧凑布局
       * - tabIndex={0}: 可通过Tab键获得焦点
       * - aria-description: 为屏幕阅读器提供描述
       */}
      <div
        className="collapse-content text-base-content/70 text-sm p-1"
        tabIndex={0}
        aria-description="Thought process content"
      >
        {/* 
         * ========== 思维内容的视觉框架 ==========
         * 
         * 【左侧边框设计】
         * border-l-2: 左侧2px边框
         * border-base-content/20: 20%透明度的边框颜色  
         * pl-4: 左内边距4，与边框保持距离
         * mb-4: 底部外边距4，与下方内容保持距离
         * 
         * 【设计意图】
         * 1. 视觉分离：通过边框将思维内容与其他部分区分
         * 2. 层次感：缩进和边框暗示这是"内部思考"
         * 3. 专业感：类似代码编辑器中的引用块样式
         * 4. 阅读体验：左边框作为视觉引导线
         */}
        <div className="border-l-2 border-base-content/20 pl-4 mb-4">
          {/* 
           * ========== 思维内容的渲染 ==========
           * 
           * 【为什么使用MarkdownDisplay？】
           * 1. 格式支持：AI的思维过程可能包含列表、代码、公式等
           * 2. 一致性：与主要回复内容使用相同的渲染引擎
           * 3. 交互功能：支持代码复制、数学公式显示等
           * 4. 样式统一：保持整个应用的视觉一致性
           * 
           * 【思维内容的典型格式】
           * ```
           * 这是一个数学问题，我需要：
           * 1. 理解题目要求
           * 2. 确定计算公式：面积 = 长 × 宽  
           * 3. 代入数值：5 × 3 = 15
           * 4. 得出答案：15平方米
           * ```
           * 
           * 【注意事项】
           * - 不传递isGenerating参数，因为思维内容通常是完整的
           * - 思维内容不会像主回复那样流式更新
           * - 渲染优化：思维内容通常较长，需要高效的Markdown解析
           */}
          <MarkdownDisplay content={content} />
        </div>
      </div>
    </div>
  );
}
