import { BrowserRouter, Routes, Route } from "react-router-dom";
import { Provider } from "react-redux";
import store from "./store";
import { ThemeProvider } from "./contexts/ThemeContext";
import "./i18n/config";
import LoginPage from "./pages/LoginPage";
import DashboardPage from "./pages/DashboardPage_new";
import BucketsPage from "./pages/BucketsPage";
import ObjectsPage from "./pages/ObjectsPage";
import UsersPage from "./pages/UsersPage";
import SystemHealthPage from "./pages/SystemHealthPage";
import ProfilePage from "./pages/ProfilePage";
import MainLayout from "./components/Layout/MainLayout";
import ProtectedRoute from "./components/ProtectedRoute";

function App() {
  return (
    <Provider store={store}>
      <ThemeProvider>
        <BrowserRouter>
          <Routes>
            <Route path="/login" element={<LoginPage />} />
            <Route
              path="/"
              element={
                <ProtectedRoute>
                  <MainLayout />
                </ProtectedRoute>
              }
            >
              <Route index element={<DashboardPage />} />
              <Route path="buckets" element={<BucketsPage />} />
              <Route path="objects" element={<ObjectsPage />} />
              <Route path="users" element={<UsersPage />} />
              <Route path="system-health" element={<SystemHealthPage />} />
              <Route path="profile" element={<ProfilePage />} />
            </Route>
          </Routes>
        </BrowserRouter>
      </ThemeProvider>
    </Provider>
  );
}

export default App;
