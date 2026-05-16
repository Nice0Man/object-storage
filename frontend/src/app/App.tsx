import React, { Suspense } from "react";
import { BrowserRouter, Routes, Route, Navigate } from "react-router-dom";
import { Provider } from "react-redux";
import store from "../store";
import { ThemeProvider } from "../contexts/ThemeContext";
import "../i18n/config";
import LoginPage from "../pages/LoginPage";
import MainLayout from "../components/Layout/MainLayout";
import ProtectedRoute from "../components/ProtectedRoute";
import Loader from "../components/Common/Loader";
import { useAppSelector } from "../hooks/useAppSelector";
import { selectCan } from "../store/authSlice";
import type { Capability } from "../auth/capabilities";

const DashboardPage = React.lazy(() => import("../pages/DashboardPage"));
const BucketsPage = React.lazy(() => import("../pages/BucketsPage"));
const ObjectsPage = React.lazy(() => import("../pages/ObjectsPage"));
const UsersPage = React.lazy(() => import("../pages/UsersPage"));
const SystemHealthPage = React.lazy(() => import("../pages/SystemHealthPage"));
const ProfilePage = React.lazy(() => import("../pages/ProfilePage"));

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

function LazyPage({ children }: { children: React.ReactNode }) {
  return (
    <Suspense fallback={<Loader message="Loading..." />}>
      {children}
    </Suspense>
  );
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
              <Route
                index
                element={
                  <LazyPage>
                    <DashboardPage />
                  </LazyPage>
                }
              />
              <Route
                path="buckets"
                element={
                  <LazyPage>
                    <CapabilityRoute capability="buckets.view">
                      <BucketsPage />
                    </CapabilityRoute>
                  </LazyPage>
                }
              />
              <Route
                path="objects"
                element={
                  <LazyPage>
                    <CapabilityRoute capability="objects.view">
                      <ObjectsPage />
                    </CapabilityRoute>
                  </LazyPage>
                }
              />
              <Route
                path="users"
                element={
                  <LazyPage>
                    <CapabilityRoute capability="users.view">
                      <UsersPage />
                    </CapabilityRoute>
                  </LazyPage>
                }
              />
              <Route
                path="system-health"
                element={
                  <LazyPage>
                    <CapabilityRoute capability="system.view">
                      <SystemHealthPage />
                    </CapabilityRoute>
                  </LazyPage>
                }
              />
              <Route
                path="profile"
                element={
                  <LazyPage>
                    <CapabilityRoute capability="profile.view">
                      <ProfilePage />
                    </CapabilityRoute>
                  </LazyPage>
                }
              />
            </Route>
          </Routes>
        </BrowserRouter>
      </ThemeProvider>
    </Provider>
  );
}

export default App;
