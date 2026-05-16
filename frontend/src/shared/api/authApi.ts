import type { AxiosInstance } from "axios";
import type {
  CurrentUserResponse,
  LoginRequest,
  LoginResponse,
  SessionResponse,
} from "../../api/types";

export interface AuthTokenStore {
  setToken(token: string): void;
  clearToken(): void;
}

export function createAuthApi(client: AxiosInstance, store: AuthTokenStore) {
  return {
    async login(credentials: LoginRequest): Promise<LoginResponse> {
      const response = await client.post<LoginResponse>(
        "/api/v1/auth/login",
        credentials,
      );
      if (response.data.token) {
        store.setToken(response.data.token);
      }
      return response.data;
    },
    async logout(): Promise<void> {
      await client.post("/api/v1/auth/logout");
      store.clearToken();
    },
    async refreshToken(): Promise<LoginResponse> {
      const response = await client.post<LoginResponse>("/api/v1/auth/refresh");
      if (response.data.token) {
        store.setToken(response.data.token);
      }
      return response.data;
    },
    async getSession(): Promise<SessionResponse> {
      const response = await client.get<SessionResponse>("/api/v1/auth/session");
      return response.data;
    },
    async getCurrentUser(): Promise<CurrentUserResponse> {
      const response = await client.get<CurrentUserResponse>("/api/v1/auth/me");
      return response.data;
    },
    async changePassword(request: {
      old_password: string;
      new_password: string;
    }): Promise<void> {
      await client.put("/api/v1/auth/password", request);
    },
  };
}
