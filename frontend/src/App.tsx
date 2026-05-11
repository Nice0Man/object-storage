import { BrowserRouter, Routes, Route, Navigate } from "react-router-dom";
import { Provider } from "react-redux";
import store from "./store";
import { ThemeProvider } from "./contexts/ThemeContext";
import "./i18n/config";
import LoginPage from "./pages/LoginPage";
import DashboardPage from "./pages/DashboardPage";
import BucketsPage from "./pages/BucketsPage";
import ObjectsPage from "./pages/ObjectsPage";
import UsersPage from "./pages/UsersPage";
import SystemHealthPage from "./pages/SystemHealthPage";
import ProfilePage from "./pages/ProfilePage";
import MainLayout from "./components/Layout/MainLayout";
import ProtectedRoute from "./components/ProtectedRoute";
import { useAppSelector } from "./hooks/useAppSelector";
import { selectCan } from "./store/authSlice";
import type { Capability } from "./auth/capabilities";

function CapabilityRoute({
  capability,
  children,
}: {
  capability: Capability;
  children: React.ReactElement;
}) {
  const allowed = useAppSelector(selectCan(capability));
  if (!allowed) {
    return <Navigate to="/" replace />;
  }
  return children;
}

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
              <Route path="buckets" element={<CapabilityRoute capability="buckets.view"><BucketsPage /></CapabilityRoute>} />
              <Route path="objects" element={<CapabilityRoute capability="objects.view"><ObjectsPage /></CapabilityRoute>} />
              <Route path="users" element={<CapabilityRoute capability="users.view"><UsersPage /></CapabilityRoute>} />
              <Route path="system-health" element={<CapabilityRoute capability="system.view"><SystemHealthPage /></CapabilityRoute>} />
              <Route path="profile" element={<CapabilityRoute capability="profile.view"><ProfilePage /></CapabilityRoute>} />
            </Route>
          </Routes>
        </BrowserRouter>
      </ThemeProvider>
    </Provider>
  );
}

export default App;
