import { HashRouter, Outlet, Route, Routes } from 'react-router';
import Header from './components/Header';
import Sidebar from './components/Sidebar';
import { AppContextProvider, useAppContext } from './utils/app.context';
import ChatScreen from './components/ChatScreen';
import SettingDialog from './components/SettingDialog';
/*
Notes:杨小兵-2025-03-05

1、从react-router模块中导入HashRouter, Outlet, Route, Routes，导入的方式采用的是花括号解析的方式，相当于从工具箱中挑选特定的工具。
2、从当前目录下的components/Header.tsx文件中导入Header组件，导入的方式采用的是默认导入方式，这种导入方式灵活性比较高。
3、从当前目录下的components/Sidebar.tsx文件中导入Sidebar组件，导入的方式采用的默认导入方式，这种导入方式灵活性比较高。
4、从当前目录下的utils/app.context.ts文件中导入AppContextProvider和useAppContext，导入的方式采用的是花括号解析的方式，相当于从工具箱中挑选特定的工具。
5、从当前目录下的components/ChatScreen.tsx文件中导入ChatScreen组件，导入的方式采用的默认导入方式，这种导入方式灵活性比较高。
6、从当前目录下的components/SettingDialog.tsx文件中导入SettingDialog组件，导入的方式采用的默认导入方式，这种导入方式灵活性比较高。
*/

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
Notes:杨小兵-2025-03-05

1、创建一个名为App的函数组件。
    1.1 typescript中的函数组件特征一：函数的返回值是一个JSX元素。
    1.2 typescript中的函数组件特征二：函数的参数是一个props对象，用于接收父组件传递的参数。
    1.3 typescript中的函数组件特征三：函数的名称是一个帕斯卡命名法的字符串，首字母大写。
2、函数组件的返回值是一个JSX元素，这里是一个HashRouter组件。
    2.1 HashRouter组件是一个路由容器，用于包裹整个应用程序，提供路由功能。
    2.2 HashRouter组件是react-router-dom模块中的一个组件。
    2.3 HashRouter组件的作用是将应用程序包裹在一个特定的路由容器中，用于管理路由。
    2.4 HashRouter组件的子组件是一个div元素，这个div元素是一个flex布局的容器。
3、问题
    3.1 对该部分的HashRouter组件的子组件的布局不是很理解，需要进一步的学习，目前先大致了解一下整体的结构以及如何进行调试。
*/

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
/*
Notes:杨小兵-2025-03-05

1、创建一个名为AppLayout的函数组件。
2、函数组件的返回值是一个JSX元素，这里是一个Fragment元素。
3、问题
    3.1 对该部分的HashRouter组件的子组件的布局不是很理解，需要进一步的学习。
*/

export default App;
/*
Notes:杨小兵-2025-03-05

1、导出App函数组件，导出的方式采用的是默认导出方式，这种导出方式灵活性比较高。
*/
