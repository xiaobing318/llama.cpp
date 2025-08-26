// @ts-expect-error this package does not have typing
import TextLineStream from 'textlinestream';
import {
  APIMessage,
  APIMessageContentPart,
  LlamaCppServerProps,
  Message,
} from './types';

// ponyfill for missing ReadableStream asyncIterator on Safari
import { asyncIterator } from '@sec-ant/readable-stream/ponyfill/asyncIterator';

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export const isString = (x: any) => !!x.toLowerCase;
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export const isBoolean = (x: any) => x === true || x === false;
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export const isNumeric = (n: any) => !isString(n) && !isNaN(n) && !isBoolean(n);
export const escapeAttr = (str: string) =>
  str.replace(/>/g, '&gt;').replace(/"/g, '&quot;');

/* 
 * =================== SSE响应体解析的核心函数 ===================
 * 
 * 【函数目的】
 * 这是解析服务器端事件流(Server-Sent Events, SSE)的核心函数
 * 将HTTP流式响应转换为可迭代的JavaScript对象，实现AI回复的实时显示
 * 
 * 【解决什么问题】
 * 1. 流式数据处理：AI生成内容时边生成边发送，用户无需等待完整回复
 * 2. 实时用户体验：模拟AI"打字"效果，让对话更自然
 * 3. 错误处理：及时捕获和处理服务器返回的错误信息
 * 4. 内存效率：流式处理大文本，避免一次性加载大量数据
 * 
 * 【SSE协议格式说明】
 * SSE是基于HTTP的流协议，数据格式如下：
 * ```
 * data: {"choices":[{"delta":{"content":"你"}}]}
 * data: {"choices":[{"delta":{"content":"好"}}]}  
 * data: {"choices":[{"delta":{"content":"！"}}]}
 * data: [DONE]
 * ```
 * 
 * 【参数说明】
 * fetchResponse: Response - fetch API返回的HTTP响应对象
 *   - response.body 包含服务器的流式数据
 *   - 数据格式遵循SSE协议规范
 * 
 * 【返回值】
 * AsyncGenerator<any, void, unknown> - 异步生成器，产出解析后的JSON对象
 *   - 每个yield返回一个AI生成的数据片段
 *   - 可通过for await循环逐个处理数据块
 * 
 * 【实际数据流示例】
 * 服务器发送：
 * "data: {"choices":[{"delta":{"content":"你好"}}]}\n"
 * "data: {"choices":[{"delta":{"content":"！很高兴认识你"}}]}\n"
 * "data: [DONE]\n"
 * 
 * 解析结果：
 * 第1次yield: {choices:[{delta:{content:"你好"}}]}
 * 第2次yield: {choices:[{delta:{content:"！很高兴认识你"}}]}
 * 然后结束迭代
 */
export async function* getSSEStreamAsync(fetchResponse: Response) {
  /* 防御性检查：确保响应体存在，避免后续处理出错 */
  if (!fetchResponse.body) throw new Error('Response body is empty');
  
  /* 
   * ========== 流式数据处理管道 ==========
   * 
   * 这里使用了流处理的管道模式，类似于Unix的管道：
   * 原始字节流 → 文本流 → 行流 → JSON对象
   * 
   * 第1步：TextDecoderStream() - 字节转文本
   * - 将HTTP响应的二进制数据(Uint8Array)解码为UTF-8文本
   * - 处理多字节字符（如中文）的正确解码
   * - 类似于C++中的编码转换：bytes -> string
   * 
   * 第2步：TextLineStream() - 文本按行分割  
   * - 将连续的文本流按换行符(\n)分割成独立的行
   * - 每一行对应一个SSE事件（data: 或 error:）
   * - 处理跨数据包的行边界问题
   * 
   * 【数据流转换过程】
   * 原始数据: Uint8Array[100, 121, 116, 101, 58, 32, 123, ...]
   *     ↓ TextDecoderStream
   * 文本流: "data: {\"choices\":[{\"delta\":{\"content\":\"你好\"}}]}\ndata: ..."
   *     ↓ TextLineStream  
   * 行流: ["data: {\"choices\":[{\"delta\":{\"content\":\"你好\"}}]}", "data: ...", ...]
   */
  const lines: ReadableStream<string> = fetchResponse.body
    .pipeThrough(new TextDecoderStream())    // 二进制 → 文本
    .pipeThrough(new TextLineStream());      // 文本 → 按行分割
  
  /* 
   * ========== 异步迭代处理每一行 ==========
   * 
   * 使用异步迭代器遍历流式数据：
   * - for await 会等待每个数据块的到达
   * - 实现背压控制：处理速度跟上数据产生速度
   * - 自动处理流的开始、进行和结束状态
   * 
   * asyncIterator是polyfill，解决Safari浏览器兼容性问题
   */
  // @ts-expect-error asyncIterator complains about type, but it should work
  for await (const line of asyncIterator(lines)) {
    /* 开发模式下可取消注释来调试数据流 */
    //if (isDev) console.log({ line });
    
    /* 
     * ========== 解析标准SSE数据行 ==========
     * 
     * SSE协议规定：
     * - 数据行格式：'data: {JSON数据}'
     * - 结束标记：'data: [DONE]' 
     * - 错误格式：'error: {错误信息}'
     * 
     * 【为什么检查 !line.endsWith('[DONE]')？】
     * [DONE]是OpenAI兼容API的标准结束标记
     * 表示AI完成了所有内容生成，不需要解析这行数据
     * 避免解析 'data: [DONE]' 时出现JSON.parse错误
     */
    if (line.startsWith('data:') && !line.endsWith('[DONE]')) {
      /* 
       * 解析JSON数据：
       * - line.slice(5) 移除 'data: ' 前缀（5个字符）
       * - JSON.parse() 将字符串转换为JavaScript对象
       * - yield 将解析结果返回给调用方
       * 
       * 示例转换：
       * 输入: 'data: {"choices":[{"delta":{"content":"你好"}}]}'
       * slice(5): '{"choices":[{"delta":{"content":"你好"}}]}'  
       * JSON.parse(): {choices: [{delta: {content: "你好"}}]}
       * yield: 返回解析后的对象给generateMessage函数
       */
      /* 
       * 解析JSON数据并产出给调用方：
       * 1. line.slice(5): 移除 'data: ' 前缀，获取纯JSON字符串
       * 2. JSON.parse(): 将JSON字符串转换为JavaScript对象
       * 3. yield: 关键字，将解析后的对象返回给外部调用方
       * 
       * 【yield的核心作用】
       * - 这是异步生成器的核心机制，不同于return
       * - yield会暂停函数执行，返回当前数据块
       * - 外部可通过for await循环立即获取并处理这个数据
       * - 处理完后函数会从yield处继续执行，处理下一行数据
       * - 这样实现了流式数据的实时处理：边接收边处理边显示
       * 
       * 【数据流转过程】
       * 服务器发送: "data: {"choices":[{"delta":{"content":"你好"}}]}"
       * slice(5)后: "{"choices":[{"delta":{"content":"你好"}}]}"
       * JSON.parse: {choices: [{delta: {content: "你好"}}]}
       * yield产出: 立即返回给generateMessage函数进行UI更新
       */
      const data = JSON.parse(line.slice(5));
      yield data;
    } 
    /* 
     * ========== 处理服务器错误信息 ==========
     * 
     * 如果服务器遇到错误（如模型崩溃、内存不足等），
     * 会发送 'error: {错误详情}' 格式的消息
     */
    else if (line.startsWith('error:')) {
      /* 
       * 解析错误信息并抛出异常：
       * - line.slice(6) 移除 'error: ' 前缀（6个字符）
       * - 解析JSON获取具体错误信息
       * - 抛出Error会中断生成过程，触发catch块处理
       * - generateMessage函数会捕获这个错误并显示给用户
       */
      const data = JSON.parse(line.slice(6));
      throw new Error(data.message || 'Unknown error');
    }
    /* 其他格式的行（如注释、空行等）直接忽略，符合SSE协议规范 */
  }
}

// copy text to clipboard
export const copyStr = (textToCopy: string) => {
  // Navigator clipboard api needs a secure context (https)
  if (navigator.clipboard && window.isSecureContext) {
    navigator.clipboard.writeText(textToCopy);
  } else {
    // Use the 'out of viewport hidden text area' trick
    const textArea = document.createElement('textarea');
    textArea.value = textToCopy;
    // Move textarea out of the viewport so it's not visible
    textArea.style.position = 'absolute';
    textArea.style.left = '-999999px';
    document.body.prepend(textArea);
    textArea.select();
    document.execCommand('copy');
  }
};

/**
 * filter out redundant fields upon sending to API
 * also format extra into text
 */
export function normalizeMsgsForAPI(messages: Readonly<Message[]>) {
  return messages.map((msg) => {
    if (msg.role !== 'user' || !msg.extra) {
      return {
        role: msg.role,
        content: msg.content,
      } as APIMessage;
    }

    // extra content first, then user text message in the end
    // this allow re-using the same cache prefix for long context
    const contentArr: APIMessageContentPart[] = [];

    for (const extra of msg.extra ?? []) {
      if (extra.type === 'context') {
        contentArr.push({
          type: 'text',
          text: extra.content,
        });
      } else if (extra.type === 'textFile') {
        contentArr.push({
          type: 'text',
          text: `File: ${extra.name}\nContent:\n\n${extra.content}`,
        });
      } else if (extra.type === 'imageFile') {
        contentArr.push({
          type: 'image_url',
          image_url: { url: extra.base64Url },
        });
      } else if (extra.type === 'audioFile') {
        contentArr.push({
          type: 'input_audio',
          input_audio: {
            data: extra.base64Data,
            format: /wav/.test(extra.mimeType) ? 'wav' : 'mp3',
          },
        });
      } else {
        throw new Error('Unknown extra type');
      }
    }

    // add user message to the end
    contentArr.push({
      type: 'text',
      text: msg.content,
    });

    return {
      role: msg.role,
      content: contentArr,
    };
  }) as APIMessage[];
}

/**
 * recommended for DeepsSeek-R1, filter out content between <think> and </think> tags
 */
export function filterThoughtFromMsgs(messages: APIMessage[]) {
  console.debug({ messages });
  return messages.map((msg) => {
    if (msg.role !== 'assistant') {
      return msg;
    }
    // assistant message is always a string
    const contentStr = msg.content as string;
    return {
      role: msg.role,
      content:
        msg.role === 'assistant'
          ? contentStr
              .split(/<\/think>|<\|end\|>/)
              .at(-1)!
              .trim()
          : contentStr,
    } as APIMessage;
  });
}

export function classNames(classes: Record<string, boolean>): string {
  return Object.entries(classes)
    .filter(([_, value]) => value)
    .map(([key, _]) => key)
    .join(' ');
}

export const delay = (ms: number) =>
  new Promise((resolve) => setTimeout(resolve, ms));

export const throttle = <T extends unknown[]>(
  callback: (...args: T) => void,
  delay: number
) => {
  let isWaiting = false;

  return (...args: T) => {
    if (isWaiting) {
      return;
    }

    callback(...args);
    isWaiting = true;

    setTimeout(() => {
      isWaiting = false;
    }, delay);
  };
};

export const cleanCurrentUrl = (removeQueryParams: string[]) => {
  const url = new URL(window.location.href);
  removeQueryParams.forEach((param) => {
    url.searchParams.delete(param);
  });
  window.history.replaceState({}, '', url.toString());
};

export const getServerProps = async (
  baseUrl: string,
  apiKey?: string
): Promise<LlamaCppServerProps> => {
  try {
    const response = await fetch(`${baseUrl}/props`, {
      headers: {
        'Content-Type': 'application/json',
        ...(apiKey ? { Authorization: `Bearer ${apiKey}` } : {}),
      },
    });
    if (!response.ok) {
      throw new Error('Failed to fetch server props');
    }
    const data = await response.json();
    return data as LlamaCppServerProps;
  } catch (error) {
    console.error('Error fetching server props:', error);
    throw error;
  }
};
