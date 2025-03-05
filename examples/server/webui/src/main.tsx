import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './index.scss';
import App from './App.tsx';
/*
Notes:杨小兵-2025-03-05

1、从react模块中添加StrictMode组件，导入的方式采用的是花括号解析的方式，相当于从工具箱中挑选特定的工具。
2、从react-dom/client模块中添加createRoot方法，导入的方式采用的是花括号解析的方式，相当于从工具箱中挑选特定的工具。
3、从当前目录下的index.scss文件中导入样式表。
4、从当前目录下的App.tsx文件中导入App组件，导入的方式采用的是花括号解析的方式，相当于从工具箱中挑选特定的工具。
*/

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>
);
/*
Notes:杨小兵-2025-03-05

1、使用react-dom/client模块中的createRoot方法创建根组件，将根组件挂载到id为root的DOM节点上。
    1.1 createRoot方法的参数是一个DOM节点，返回值是一个Root对象。
    1.2 createRoot方法中的!表示root不为空。
2、通过使用createRoot方法创建的Root对象的render方法，将StrictMode组件作为根组件，App组件作为StrictMode组件的子组件，渲染到DOM节点上。
    2.1 StrictMode组件是一个用于检查应用程序中潜在问题的工具，它会在开发和生产模式下具有不同的行为。
    2.2 App组件是整个应用程序的根组件。
3、在typescript中调用额外的组件函数的时候，具体的方式例如上述的<App />，这种方式是JSX的语法糖，实际上是调用React.createElement方法。
*/
