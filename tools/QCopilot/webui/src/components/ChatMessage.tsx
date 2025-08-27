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

  // 使用正则表达式循环处理，原来的代码存在一个逻辑缺陷，只能正确的处理单个思考块，但是存在多个思考块的时候会出现错误，带轮次标记的消息分割逻辑
  const { content, thought, isThinking }: SplitMessage = useMemo(() => {
    if (msg.content === null || msg.role !== 'assistant') {
      return { content: msg.content };
    }

    const REGEX_THINK_OPEN = /<think>|<\|channel\|>analysis<\|message\|>/g;
    const REGEX_THINK_CLOSE = /<\/think>|<\|end\|>/g;

    let actualContent = '';
    let thought = '';
    let isThinking = false;

    // 用于追踪轮次
    let thoughtRound = 0;
    const contentSegments: string[] = [];
    const thoughtSegments: string[] = [];

    let currentPos = 0;
    const text = msg.content;

    while (currentPos < text.length) {
      // 查找下一个思考块开始标记
      REGEX_THINK_OPEN.lastIndex = currentPos;
      const openMatch = REGEX_THINK_OPEN.exec(text);

      if (openMatch) {
        // 添加思考块之前的内容
        const contentBefore = text
          .substring(currentPos, openMatch.index)
          .trim();
        if (contentBefore) {
          contentSegments.push(contentBefore);
        }

        // 查找对应的结束标记
        REGEX_THINK_CLOSE.lastIndex = openMatch.index + openMatch[0].length;
        const closeMatch = REGEX_THINK_CLOSE.exec(text);

        if (closeMatch) {
          // 提取思考内容
          const thinkContent = text
            .substring(openMatch.index + openMatch[0].length, closeMatch.index)
            .trim();

          if (thinkContent) {
            thoughtRound++;
            thoughtSegments.push(
              `第 ${thoughtRound} 轮思考：\n${thinkContent}`
            );
          }

          currentPos = closeMatch.index + closeMatch[0].length;
        } else {
          // 没有找到结束标记，说明思考块未完成
          const unfinishedThought = text
            .substring(openMatch.index + openMatch[0].length)
            .trim();
          if (unfinishedThought) {
            thoughtRound++;
            thoughtSegments.push(
              `第 ${thoughtRound} 轮思考（进行中）：\n${unfinishedThought}`
            );
          }
          isThinking = true;
          break;
        }
      } else {
        // 没有更多的思考块，添加剩余内容
        const remainingContent = text.substring(currentPos).trim();
        if (remainingContent) {
          contentSegments.push(remainingContent);
        }
        break;
      }
    }

    // 组装思考内容
    thought = thoughtSegments.join('\n\n');

    // 组装实际内容 - 为所有内容段添加轮次标记
    if (contentSegments.length > 1) {
      actualContent = contentSegments
        .map((seg, idx) => {
          // 为所有内容段添加轮次标记，包括工具调用
          return `第 ${idx + 1} 轮输出：\n${seg}`;
        })
        .join('\n\n');
    } else if (contentSegments.length === 1) {
      // 即使只有一个内容段，如果有思考块，也添加轮次标记以保持一致性
      if (thoughtSegments.length > 0) {
        actualContent = `第 1 轮输出：\n${contentSegments[0]}`;
      } else {
        actualContent = contentSegments[0];
      }
    } else {
      actualContent = '';
    }

    return {
      content: actualContent.trim(),
      thought: thought.trim(),
      isThinking,
    };
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

function ThoughtProcess({
  isThinking,
  content,
  open,
}: {
  isThinking: boolean;
  content: string;
  open: boolean;
}) {
  return (
    <div
      role="button"
      aria-label="Toggle thought process display"
      tabIndex={0}
      className={classNames({
        'collapse bg-none': true,
      })}
    >
      <input type="checkbox" defaultChecked={open} />
      <div className="collapse-title px-0">
        <div className="btn rounded-xl">
          {isThinking ? (
            <span>
              <span
                className="loading loading-spinner loading-md mr-2"
                style={{ verticalAlign: 'middle' }}
              ></span>
              Thinking
            </span>
          ) : (
            <>Thought Process</>
          )}
        </div>
      </div>
      <div
        className="collapse-content text-base-content/70 text-sm p-1"
        tabIndex={0}
        aria-description="Thought process content"
      >
        <div className="border-l-2 border-base-content/20 pl-4 mb-4">
          <MarkdownDisplay content={content} />
        </div>
      </div>
    </div>
  );
}
