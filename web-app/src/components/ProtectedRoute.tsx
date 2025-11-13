import React, { useEffect } from "react";
import { Navigate, useLocation } from "react-router-dom";
import { useAppDispatch } from "../hooks/useAppDispatch";
import { useAppSelector } from "../hooks/useAppSelector";
import { checkSession, selectAuth } from "../store/authSlice";
import Loader from "./Common/Loader";

interface ProtectedRouteProps {
  children: React.ReactNode;
}

const ProtectedRoute: React.FC<ProtectedRouteProps> = ({ children }) => {
  const dispatch = useAppDispatch();
  const { isAuthenticated, loading } = useAppSelector(selectAuth);
  const location = useLocation();
  const [checking, setChecking] = React.useState(true);

  useEffect(() => {
    const checkAuth = async () => {
      try {
        await dispatch(checkSession()).unwrap();
      } catch (error) {
        // Session check failed
      } finally {
        setChecking(false);
      }
    };

    if (!isAuthenticated) {
      checkAuth();
    } else {
      setChecking(false);
    }
  }, [dispatch, isAuthenticated]);

  if (checking || loading) {
    return <Loader message="Checking authentication..." />;
  }

  if (!isAuthenticated) {
    return <Navigate to="/login" state={{ from: location }} replace />;
  }

  return <>{children}</>;
};

export default ProtectedRoute;
