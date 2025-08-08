//  使用 import 语句从 react-router 库中导入 HashRouter、Outlet、Route 和 Routes 组件，这些组件用于设置应用的路由。
import { HashRouter, Outlet, Route, Routes } from 'react-router';
// 使用 Module 特性从当前目录下的组件文件中导入 AppLayout、Header、Sidebar、ChatScreen 和 SettingDialog 组件，这些组件构成了应用的主要界面和功能。
import Header from './components/Header';
import Sidebar from './components/Sidebar';
import { AppContextProvider, useAppContext } from './utils/app.context';
import ChatScreen from './components/ChatScreen';
import SettingDialog from './components/SettingDialog';

function App() {
  return (
    <HashRouter>
      <div className="flex flex-row drawer lg:drawer-open">
        <AppContextProvider>
          <Routes>
            <Route element={<AppLayout />}>
              <Route path="/chat/:convId" element={<ChatScreen />} />
              <Route path="*" element={<ChatScreen />} />
            </Route>
          </Routes>
        </AppContextProvider>
      </div>
    </HashRouter>
  );
}

/*
1、这是一个 React 的组件，就我目前对 React 的理解，这个组件是一个函数组件，在 React 中，函数组件是通过函数定义的组件，它可以接收 props 作为参数，
并返回一个 React 元素。
*/
function AppLayout() {
  // 从 useAppContext 函数返回的对象中获取 showSettings 和 setShowSettings。
  const { showSettings, setShowSettings } = useAppContext();
  // 返回
  return (
    <>
      <Sidebar />
      <div
        className="drawer-content grow flex flex-col h-screen w-screen mx-auto px-4 overflow-auto"
        id="main-scroll"
      >
        <Header />
        <Outlet />
      </div>
      {
        <SettingDialog
          show={showSettings}
          onClose={() => setShowSettings(false)}
        />
      }
    </>
  );
}

//  这是 JavaScript 中导出默认符号的语法，通过这种方式可以使得其他模块导入和使用这个符号/组件。
export default App;
