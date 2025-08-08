// 使用 module 模式从 react 库中导入 useEffect、useMemo 和 useState 符号，这些符号是 React 的钩子函数，用于在函数组件中管理状态和副作用。
import { useEffect, useMemo, useState } from 'react';
// 使用 module 模式从自定义的 app.context 文件中导入 CallbackGeneratedChunk 和 useAppContext 符号，这些符号用于访问应用的上下文和处理生成的消息块。
import { CallbackGeneratedChunk, useAppContext } from '../utils/app.context';
// 使用 module 模式从当前目录下的 ChatMessage 文件中导入 ChatMessage 组件，这个组件用于渲染单个聊天消息。
import ChatMessage from './ChatMessage';
// 使用 module 模式从自定义的 types 文件中导入 CanvasType、Message 和 PendingMessage 符号，这些符号定义了应用中使用的数据类型。
import { CanvasType, Message, PendingMessage } from '../utils/types';
// 使用 module 模式从自定义的 misc 文件中导入 classNames 和 throttle 符号，这些符号用于处理类名和节流函数。
import { classNames, throttle } from '../utils/misc';
// 使用 module 模式从当前目录下的 CanvasPyInterpreter 文件中导入 CanvasPyInterpreter 组件，这个组件用于渲染 Python 解释器画布。
import CanvasPyInterpreter from './CanvasPyInterpreter';
// 使用 module 模式从自定义的 storage 文件中导入 StorageUtils 符号，这个符号用于处理存储相关的操作。
import StorageUtils from '../utils/storage';

/**
 * A message display is a message node with additional information for rendering.
 * For example, siblings of the message node are stored as their last node (aka leaf node).
 */
export interface MessageDisplay {
  msg: Message | PendingMessage;
  siblingLeafNodeIds: Message['id'][];
  siblingCurrIdx: number;
  isPending?: boolean;
}

// 定义一个名为 getListMessageDisplay 的函数，这个函数接受两个参数 msgs 和 leafNodeId，返回一个 MessageDisplay 数组。
function getListMessageDisplay(
  msgs: Readonly<Message[]>,
  leafNodeId: Message['id']
): MessageDisplay[] {
  const currNodes = StorageUtils.filterByLeafNodeId(msgs, leafNodeId, true);
  const res: MessageDisplay[] = [];
  const nodeMap = new Map<Message['id'], Message>();
  for (const msg of msgs) {
    nodeMap.set(msg.id, msg);
  }
  // find leaf node from a message node
  const findLeafNode = (msgId: Message['id']): Message['id'] => {
    let currNode: Message | undefined = nodeMap.get(msgId);
    while (currNode) {
      if (currNode.children.length === 0) break;
      currNode = nodeMap.get(currNode.children.at(-1) ?? -1);
    }
    return currNode?.id ?? -1;
  };
  // traverse the current nodes
  for (const msg of currNodes) {
    const parentNode = nodeMap.get(msg.parent ?? -1);
    if (!parentNode) continue;
    const siblings = parentNode.children;
    if (msg.type !== 'root') {
      res.push({
        msg,
        siblingLeafNodeIds: siblings.map(findLeafNode),
        siblingCurrIdx: siblings.indexOf(msg.id),
      });
    }
  }
  return res;
}

// 定义一个名为 scrollToBottom 的节流函数，这个函数用于滚动到页面底部，接受两个参数 requiresNearBottom 和 delay。
const scrollToBottom = throttle(
  (requiresNearBottom: boolean, delay: number = 80) => {
    const mainScrollElem = document.getElementById('main-scroll');
    if (!mainScrollElem) return;
    const spaceToBottom =
      mainScrollElem.scrollHeight -
      mainScrollElem.scrollTop -
      mainScrollElem.clientHeight;
    if (!requiresNearBottom || spaceToBottom < 50) {
      setTimeout(
        () => mainScrollElem.scrollTo({ top: mainScrollElem.scrollHeight }),
        delay
      );
    }
  },
  80
);

export default function ChatScreen() {
  /*
  1、从 useAppContext 函数返回的对象中获取 viewingChat、sendMessage、isGenerating、stopGenerating、pendingMessages、canvasData 和
  replaceMessageAndGenerate。
  */
  const {
    viewingChat,
    sendMessage,
    isGenerating,
    stopGenerating,
    pendingMessages,
    canvasData,
    replaceMessageAndGenerate,
  } = useAppContext();
  // 创建两个名为 inputMsg 和 setInputMsg 的常量，并且将其赋值为 useState('') 的返回值，即从返回值中解构出这两个变量。
  const [inputMsg, setInputMsg] = useState('');

  // keep track of leaf node for rendering
  // 从 useState(-1) 的返回值中解构出 currNodeId 和 setCurrNodeId 这两个变量，并且将其赋值为 useState(-1) 的返回值。
  const [currNodeId, setCurrNodeId] = useState<number>(-1);
  // 创建一个名为 messages 的常量，并且将其赋值为 useMemo 的返回值，这个常量用于存储当前会话的消息列表。
  const messages: MessageDisplay[] = useMemo(() => {
    if (!viewingChat) return [];
    else return getListMessageDisplay(viewingChat.messages, currNodeId);
  }, [currNodeId, viewingChat]);

  // 获取当前会话的 ID，如果没有当前会话，则为 null。
  const currConvId = viewingChat?.conv.id ?? null;
  // 获取当前会话的待处理消息，如果没有则为 undefined。
  const pendingMsg: PendingMessage | undefined =
    pendingMessages[currConvId ?? ''];

  // 使用 useEffect 钩子函数，当 currConvId 变化时，重置 currNodeId 并滚动到页面底部。
  useEffect(() => {
    // reset to latest node when conversation changes
    setCurrNodeId(-1);
    // scroll to bottom when conversation changes
    scrollToBottom(false, 1);
  }, [currConvId]);

  // 定义一个名为 onChunk 的函数，这个函数用于处理生成的消息块，接受一个可选参数 currLeafNodeId。
  const onChunk: CallbackGeneratedChunk = (currLeafNodeId?: Message['id']) => {
    if (currLeafNodeId) {
      setCurrNodeId(currLeafNodeId);
    }
    scrollToBottom(true);
  };

  /*
  1、定义一个异步函数，这个函数用于发送新的消息。
  2、这个异步函数就是当我们输入消息并且按下回车键或者点击发送按钮时触发的。
  3、这个函数内部调用真正发送消息的函数，其函数是处理前端与后端的消息交互。
  */
  const sendNewMessage = async () => {
    // 如果输入的消息为空或者当前会话正在生成消息，则直接返回。
    if (inputMsg.trim().length === 0 || isGenerating(currConvId ?? '')) return;
    // 将输入的消息设置为当前会话的输入消息，当后端模型推理失败后将其再次显示到输入框中。
    const lastInpMsg = inputMsg;
    // 因为将消息发送给后端模型处理的时候，需要清空输入框，这样看起来像是将消息发送出去。
    setInputMsg('');
    // 将当前页面滚动到最底部
    scrollToBottom(false);
    // 将当前节点 ID 设置为 -1，表示没有当前节点。
    setCurrNodeId(-1);
    // get the last message node
    // 获取当前会话的最后一条消息的 ID，如果没有消息则为 null。
    const lastMsgNodeId = messages.at(-1)?.msg.id ?? null;
    // 等待 sendMessage 函数执行完成，如果发送失败，则将输入的消息恢复为之前的值。
    if (!(await sendMessage(currConvId, lastMsgNodeId, inputMsg, onChunk))) {
      // restore the input message if failed
      setInputMsg(lastInpMsg);
    }
  };

  // 定义一个名为 handleEditMessage 的异步函数，这个函数用于处理编辑消息的操作。
  const handleEditMessage = async (msg: Message, content: string) => {
    if (!viewingChat) return;
    setCurrNodeId(msg.id);
    scrollToBottom(false);
    await replaceMessageAndGenerate(
      viewingChat.conv.id,
      msg.parent,
      content,
      onChunk
    );
    setCurrNodeId(-1);
    scrollToBottom(false);
  };

  // 定义一个名为 handleRegenerateMessage 的异步函数，这个函数用于处理重新生成消息的操作。
  const handleRegenerateMessage = async (msg: Message) => {
    if (!viewingChat) return;
    setCurrNodeId(msg.parent);
    scrollToBottom(false);
    await replaceMessageAndGenerate(
      viewingChat.conv.id,
      msg.parent,
      null,
      onChunk
    );
    setCurrNodeId(-1);
    scrollToBottom(false);
  };

  // 判断是否有画布数据
  const hasCanvas = !!canvasData;

  // due to some timing issues of StorageUtils.appendMsg(), we need to make sure the pendingMsg is not duplicated upon rendering (i.e. appears once in the saved conversation and once in the pendingMsg)
  const pendingMsgDisplay: MessageDisplay[] =
    pendingMsg && messages.at(-1)?.msg.id !== pendingMsg.id
      ? [
          {
            msg: pendingMsg,
            siblingLeafNodeIds: [],
            siblingCurrIdx: 0,
            isPending: true,
          },
        ]
      : [];

  return (
    <div
      className={classNames({
        'grid lg:gap-8 grow transition-[300ms]': true,
        'grid-cols-[1fr_0fr] lg:grid-cols-[1fr_1fr]': hasCanvas, // adapted for mobile
        'grid-cols-[1fr_0fr]': !hasCanvas,
      })}
    >
      <div
        className={classNames({
          'flex flex-col w-full max-w-[900px] mx-auto': true,
          'hidden lg:flex': hasCanvas, // adapted for mobile
          flex: !hasCanvas,
        })}
      >
        {/* chat messages */}
        <div id="messages-list" className="grow">
          <div className="mt-auto flex justify-center">
            {/* placeholder to shift the message to the bottom */}
            {viewingChat ? '' : 'Send a message to start'}
          </div>
          {[...messages, ...pendingMsgDisplay].map((msg) => (
            <ChatMessage
              key={msg.msg.id}
              msg={msg.msg}
              siblingLeafNodeIds={msg.siblingLeafNodeIds}
              siblingCurrIdx={msg.siblingCurrIdx}
              onRegenerateMessage={handleRegenerateMessage}
              onEditMessage={handleEditMessage}
              onChangeSibling={setCurrNodeId}
            />
          ))}
        </div>

        {/* chat input */}
        <div className="flex flex-row items-center pt-8 pb-6 sticky bottom-0 bg-base-100">
          <textarea
            className="textarea textarea-bordered w-full"
            placeholder="Type a message (Shift+Enter to add a new line)"
            value={inputMsg}
            onChange={(e) => setInputMsg(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter' && e.shiftKey) return;
              if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                sendNewMessage();
              }
            }}
            id="msg-input"
            dir="auto"
          ></textarea>
          {isGenerating(currConvId ?? '') ? (
            <button
              className="btn btn-neutral ml-2"
              onClick={() => stopGenerating(currConvId ?? '')}
            >
              Stop
            </button>
          ) : (
            <button
              className="btn btn-primary ml-2"
              onClick={sendNewMessage}
              disabled={inputMsg.trim().length === 0}
            >
              Send
            </button>
          )}
        </div>
      </div>
      <div className="w-full sticky top-[7em] h-[calc(100vh-9em)]">
        {canvasData?.type === CanvasType.PY_INTERPRETER && (
          <CanvasPyInterpreter />
        )}
      </div>
    </div>
  );
}
