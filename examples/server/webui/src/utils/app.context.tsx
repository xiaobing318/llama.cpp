import React, { createContext, useContext, useEffect, useState } from 'react';
import {
  APIMessage,
  CanvasData,
  Conversation,
  Message,
  PendingMessage,
  ViewingChat,
} from './types';
import StorageUtils from './storage';
import {
  filterThoughtFromMsgs,
  normalizeMsgsForAPI,
  getSSEStreamAsync,
} from './misc';
import { BASE_URL, CONFIG_DEFAULT, isDev } from '../Config';
import { matchPath, useLocation, useNavigate } from 'react-router';

interface AppContextValue {
  // conversations and messages
  viewingChat: ViewingChat | null;
  pendingMessages: Record<Conversation['id'], PendingMessage>;
  isGenerating: (convId: string) => boolean;
  sendMessage: (
    convId: string | null,
    leafNodeId: Message['id'] | null,
    content: string,
    onChunk: CallbackGeneratedChunk
  ) => Promise<boolean>;
  stopGenerating: (convId: string) => void;
  replaceMessageAndGenerate: (
    convId: string,
    parentNodeId: Message['id'], // the parent node of the message to be replaced
    content: string | null,
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

  const setPending = (convId: string, pendingMsg: PendingMessage | null) => {
    // if pendingMsg is null, remove the key from the object
    if (!pendingMsg) {
      setPendingMessages((prev) => {
        const newState = { ...prev };
        delete newState[convId];
        return newState;
      });
    } else {
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

  const isGenerating = (convId: string) => !!pendingMessages[convId];

  /*
  1、创建一个名为 `generateMessage` 的异步函数，这个函数用于生成消息。
  2、函数接受三个参数：
    2.1 `convId`（当前会话的 ID）
    2.2 `leafNodeId`（当前消息的叶子节点 ID）
    2.3 `onChunk`（处理生成的消息块的回调函数
  */
  const generateMessage = async (
    convId: string,
    leafNodeId: Message['id'],
    onChunk: CallbackGeneratedChunk
  ) => {
    // 如果当前会话正在生成消息，则直接返回。
    if (isGenerating(convId)) return;

    // 创建一个名为 config 的常量，用于存储当前会话的配置，并且从 StorageUtils 中获取当前会话的配置。
    const config = StorageUtils.getConfig();
    // 创建一个名为 currConversation 的常量，用于存储当前会话的信息，并且从 StorageUtils 中获取当前会话的信息。
    const currConversation = await StorageUtils.getOneConversation(convId);
    // 如果当前会话不存在，则抛出错误。
    if (!currConversation) {
      throw new Error('Current conversation is not found');
    }

    // 创建一个名为 currMessages 的常量，用于存储当前会话的消息，并且从 StorageUtils 中获取当前会话的消息。
    const currMessages = StorageUtils.filterByLeafNodeId(
      await StorageUtils.getMessages(convId),
      leafNodeId,
      false
    );
    // 创建一个名为 abortController 的常量，用于控制生成消息的请求。
    const abortController = new AbortController();
    // 调用 setAbort 函数，将当前会话的 abortController 存储到 aborts 对象中，以便后续可以取消请求。
    setAbort(convId, abortController);

    // 如果当前会话的消息为空，则抛出错误。
    if (!currMessages) {
      throw new Error('Current messages are not found');
    }

    // 创建一个名为 pendingId 的常量，用于存储当前待处理消息的 ID，将其设置为当前时间戳加 1。
    const pendingId = Date.now() + 1;
    /*
    1、创建一个名为 pendingMsg 的变量，用于存储当前待处理消息的对象。
    2、这个对象包含以下属性：
      2.1 `id`（待处理消息的 ID）
      2.2 `convId`（当前会话的 ID）
      2.3 `type`（消息类型，这里是 'text'）
      2.4 `timestamp`（消息的时间戳）
      2.5 `role`（消息的角色，这里是 'assistant'）
      2.6 `content`（消息内容，这里是 null，因为还没有生成内容）
      2.7 `parent`（消息的父节点 ID，这里是当前消息的叶子节点 ID）
      2.8 `children`（消息的子节点 ID，这里是一个空数组，因为还没有子节点）。
    3、将这个待处理消息存储到 pendingMessages 对象中，以便后可以在界面上显示这个待处理消息。
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
    // 调用 setPending 函数，这个函数的目的是将待处理消息存储到 pendingMessages 对象中。
    setPending(convId, pendingMsg);

    try {
      // prepare messages for API
      /*
      1、let messages：声明一个名为 messages 的变量。
      2、APIMessage[]：使用 TypeScript 的类型注解，指定 messages 变量的类型为 APIMessage 类型的数组（[] 表示数组）。
      3、APIMessage：这是项目中 types.ts 文件中定义的类型，描述了消息对象的结构。
      4、...（扩展运算符）：用于展开数组或对象
      */
      let messages: APIMessage[] = [
        ...(config.systemMessage.length === 0
          ? []
          : [{ role: 'system', content: config.systemMessage } as APIMessage]),
        ...normalizeMsgsForAPI(currMessages),
      ];

      // 如果配置中 excludeThoughtOnReq 为 true，则过滤掉消息中的思考内容。
      if (config.excludeThoughtOnReq) {
        // 过滤思考内容
        messages = filterThoughtFromMsgs(messages);
      }
      // 如果当前处于开发模式，则将 messages 中的内容输出到控制台中。
      if (isDev) console.log({ messages });

      // prepare params
      // 准备请求参数，这些参数将会通过 HTTP/REST 请求发送到后端模型进行处理。
      const params = {
        messages,
        stream: true,
        cache_prompt: true,
        samplers: config.samplers,
        temperature: config.temperature,
        dynatemp_range: config.dynatemp_range,
        dynatemp_exponent: config.dynatemp_exponent,
        top_k: config.top_k,
        top_p: config.top_p,
        min_p: config.min_p,
        typical_p: config.typical_p,
        xtc_probability: config.xtc_probability,
        xtc_threshold: config.xtc_threshold,
        repeat_last_n: config.repeat_last_n,
        repeat_penalty: config.repeat_penalty,
        presence_penalty: config.presence_penalty,
        frequency_penalty: config.frequency_penalty,
        dry_multiplier: config.dry_multiplier,
        dry_base: config.dry_base,
        dry_allowed_length: config.dry_allowed_length,
        dry_penalty_last_n: config.dry_penalty_last_n,
        max_tokens: config.max_tokens,
        timings_per_token: !!config.showTokensPerSecond,
        ...(config.custom.length ? JSON.parse(config.custom) : {}),
      };

      // send request
      // 使用 fetch API 发送请求，这里使用了异步函数来处理请求，将请求返回的结果保存到 fetchResponse 中。
      const fetchResponse = await fetch(`${BASE_URL}/v1/chat/completions`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          ...(config.apiKey
            ? { Authorization: `Bearer ${config.apiKey}` }
            : {}),
        },
        body: JSON.stringify(params),
        signal: abortController.signal,
      });
      // 如果响应的状态码不是 200，则抛出错误。
      if (fetchResponse.status !== 200) {
        const body = await fetchResponse.json();
        throw new Error(body?.error?.message || 'Unknown error');
      }
      // 处理响应数据
      const chunks = getSSEStreamAsync(fetchResponse);

      /*
      1、迭代处理每个数据块，这里的 chunk 就是后端模型输出的 token ，只不过这个 chunk 是一个包含多个字段的对象，里面不仅仅包含 token ，还有一些其他内容。
      */
      for await (const chunk of chunks) {
        // const stop = chunk.stop;
        // 如果数据块中有错误，则抛出错误。
        if (chunk.error) {
          throw new Error(chunk.error?.message || 'Unknown error');
        }
        // 创建一个名为 addedContent 的常量，用于保存当前数据块中的 token 即后端模型推理输出的 token 。
        const addedContent = chunk.choices[0].delta.content;
        // 如果 pendingMsg.content 不为空将其赋值给 lastContent，否则将 lastContent 设置为空字符串。
        const lastContent = pendingMsg.content || '';
        /*
        1、如果输出的 token 不为空，则将 pendingMsg.content 设置为 lastContent 加上后端模型输出的 token 。
        2、整体的作用就是不断的将新产生的 token 添加到 pendingMsg.content 中，形成一个完整的消息内容。
        */
        if (addedContent) {
          pendingMsg = {
            ...pendingMsg,
            content: lastContent + addedContent,
          };
        }
        // 如果当前数据块中包含时间信息，并且配置中开启了每秒 token 数量的显示，则将时间信息添加到 pendingMsg 中。
        const timings = chunk.timings;
        if (timings && config.showTokensPerSecond) {
          // only extract what's really needed, to save some space
          pendingMsg.timings = {
            prompt_n: timings.prompt_n,
            prompt_ms: timings.prompt_ms,
            predicted_n: timings.predicted_n,
            predicted_ms: timings.predicted_ms,
          };
        }
        // 调用 setPending 函数，这个函数的目的是将待处理消息存储到 pendingMessages 对象中。
        setPending(convId, pendingMsg);
        onChunk(); // don't need to switch node for pending message
      }
    } catch (err) {
      setPending(convId, null);
      if ((err as Error).name === 'AbortError') {
        // user stopped the generation via stopGeneration() function
        // we can safely ignore this error
      } else {
        console.error(err);
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        alert((err as any)?.message ?? 'Unknown error');
        throw err; // rethrow
      }
    }

    /*
    1、如果 pendingMsg.content 不为空，则将其添加到 StorageUtils 中。
    2、代码执行到这里，pendingMsg 变量中保存的是由后端模型推理产生的 tokens ，这不过在 pendingMsg 对象中还包含了一些其他的信息。
    */
    if (pendingMsg.content !== null) {
      await StorageUtils.appendMsg(pendingMsg as Message, leafNodeId);
    }
    setPending(convId, null);
    onChunk(pendingId); // trigger scroll to bottom and switch to the last node
  };

  /*
  1、创建一个名为 `sendMessage` 的异步函数，这个函数用于发送新的消息到后端模型进行处理。
  2、函数接受四个参数：
    2.1 `convId`（当前会话的 ID）
    2.2 `leafNodeId`（当前消息的叶子节点 ID）
    2.3 `content`（要发送的消息内容）
    2.4 `onChunk`（处理生成的消息块的回调函数）。
  */
  const sendMessage = async (
    convId: string | null,
    leafNodeId: Message['id'] | null,
    content: string,
    onChunk: CallbackGeneratedChunk
  ): Promise<boolean> => {
    // 如果当前会话正在生成消息或者内容为空，则直接返回 false。
    if (isGenerating(convId ?? '') || content.trim().length === 0) return false;
    // 如果 convId 为空或者长度为 0，或者 leafNodeId 为空，则创建一个新的会话。
    if (convId === null || convId.length === 0 || leafNodeId === null) {
      /*
      1、创建一个新的会话，使用 StorageUtils.createConversation() 函数，并将内容的前 256 个字符作为会话的标题。
      2、关于这个会话标题可以使用模型自动总结一下，然后将标题设置为会话的标题。
      */
      const conv = await StorageUtils.createConversation(
        content.substring(0, 256)
      );
      // 将 convId 设置为新创建的会话的 ID。
      convId = conv.id;
      // 将 leafNodeId 设置为当前会话的当前节点 ID。
      leafNodeId = conv.currNode;
      // if user is creating a new conversation, redirect to the new conversation
      navigate(`/chat/${convId}`);
    }
    // 创建一个名为 now 的常量，获取当前的时间戳。
    const now = Date.now();
    // 创建一个名为 currMsgId 的常量，将其赋值为当前的时间戳，这个常量用于表示当前消息的 ID。
    const currMsgId = now;
    // 使用 StorageUtils.appendMsg() 函数将当前消息添加到会话中，传入一个包含消息内容的对象和叶子节点 ID。
    StorageUtils.appendMsg(
      {
        id: currMsgId,
        timestamp: now,
        type: 'text',
        convId,
        role: 'user',
        content,
        parent: leafNodeId,
        children: [],
      },
      leafNodeId
    );
    // 调用 onChunk 回调函数，传入当前消息的 ID，以便在生成消息时进行处理。
    onChunk(currMsgId);

    try {
      // 调用 generateMessage 函数，传入会话 ID、当前消息 ID 和 onChunk 回调函数，以便生成消息。
      await generateMessage(convId, currMsgId, onChunk);
      // 如果生成消息成功，返回 true。
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
      }}
    >
      {children}
    </AppContext.Provider>
  );
};

export const useAppContext = () => useContext(AppContext);
