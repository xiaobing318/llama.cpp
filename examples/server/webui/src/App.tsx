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

function AppLayout() {
  const { showSettings, setShowSettings } = useAppContext();
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

export default App;
