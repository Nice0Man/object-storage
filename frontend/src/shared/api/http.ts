import axios, { AxiosError, AxiosInstance, InternalAxiosRequestConfig } from "axios";
import type { ApiError, LoginResponse } from "../../api/types";
import { getApiUrlFromEnv } from "../../env";

export function resolveApiBaseURL(explicit?: string): string {
  if (explicit !== undefined && explicit.length > 0) {
    return explicit;
  }
  return getApiUrlFromEnv() ?? "";
}

export interface AuthHandlers {
  getToken: () => string | null;
  setToken: (token: string) => void;
  clearToken: () => void;
  refreshToken: () => Promise<LoginResponse>;
}

function isAuthBypassUrl(url?: string): boolean {
  if (!url) return false;
  return (
    url.includes("/api/v1/auth/login") ||
    url.includes("/api/v1/auth/refresh") ||
    url.includes("/api/v1/health") ||
    url.includes("/api/v1/ready") ||
    url.includes("/api/v1/live")
  );
}

function redirectToLogin(): void {
  if (typeof window !== "undefined" && !window.location.pathname.startsWith("/login")) {
    window.location.href = "/login";
  }
}

export function createHttpClient(
  baseURL: string,
  auth: AuthHandlers,
): AxiosInstance {
  const client = axios.create({
    baseURL,
    headers: {
      "Content-Type": "application/json",
    },
  });

  const token = auth.getToken();
  if (token) {
    client.defaults.headers.common.Authorization = `Bearer ${token}`;
  }

  client.interceptors.request.use((config: InternalAxiosRequestConfig) => {
    const current = auth.getToken();
    if (current) {
      config.headers.Authorization = `Bearer ${current}`;
    }
    return config;
  });

  let refreshPromise: Promise<string | null> | null = null;

  client.interceptors.response.use(
    (response) => response,
    async (error: AxiosError<ApiError>) => {
      const status = error.response?.status;
      const originalRequest = error.config as InternalAxiosRequestConfig & {
        _retry?: boolean;
      };

      if (status !== 401 || !originalRequest) {
        return Promise.reject(error);
      }

      if (isAuthBypassUrl(originalRequest.url)) {
        return Promise.reject(error);
      }

      if (originalRequest._retry) {
        auth.clearToken();
        redirectToLogin();
        return Promise.reject(error);
      }

      if (!refreshPromise) {
        refreshPromise = auth
          .refreshToken()
          .then((data) => data.token ?? null)
          .catch(() => null)
          .finally(() => {
            refreshPromise = null;
          });
      }

      const newToken = await refreshPromise;
      if (!newToken) {
        auth.clearToken();
        redirectToLogin();
        return Promise.reject(error);
      }

      auth.setToken(newToken);
      originalRequest._retry = true;
      originalRequest.headers.Authorization = `Bearer ${newToken}`;
      return client.request(originalRequest);
    },
  );

  return client;
}
