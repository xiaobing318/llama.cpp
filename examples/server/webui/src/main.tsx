// 使用 Module 特性从 React 库中导入 StrictMode 元素以供后续使用。
import { StrictMode } from 'react';
// 使用 Module 特性从 React DOM 库中导入 createRoot 函数，用于创建 React 应用的根节点。
import { createRoot } from 'react-dom/client';
// 使用 Module 特性从当前目录下的 index.scss 文件中导入样式表，以便在应用中使用这些样式。
import './index.scss';
// 使用 Module 特性从当前目录下的 App.tsx 文件中导入 App 组件，这个组件是应用的主要部分。
import App from './App.tsx';

/*
Note:杨小兵-2025-08-03
    使用 createRoot 函数创建一个 React 应用的根节点，并将其挂载到 HTML 文档中的 id 为 'root' 的元素上。
通过 StrictMode 包裹 App 组件，以便在开发模式下启用额外的检查和警告，帮助开发者发现潜在的问题。
*/
createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>
);
