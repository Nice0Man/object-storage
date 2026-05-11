import { createSlice, createAsyncThunk, PayloadAction } from "@reduxjs/toolkit";
import apiClient from "../api/client";
import type { LoginRequest, SessionResponse } from "../api/types";
import type { RootState } from "./index";
import { buildCapabilities, can as canCapability, type Capability, type CapabilityMap } from "../auth/capabilities";

interface AuthState {
  isAuthenticated: boolean;
  username: string | null;
  accessKey: string | null;
  isAdmin: boolean;
  role: string;
  groups: string[];
  policies: string[];
  capabilities: CapabilityMap;
  expiresAt: string | null;
  loading: boolean;
  error: string | null;
}

const initialState: AuthState = {
  isAuthenticated: false,
  username: null,
  accessKey: null,
  isAdmin: false,
  role: "viewer",
  groups: [],
  policies: [],
  capabilities: buildCapabilities({ role: "viewer", isAdmin: false, policies: [] }),
  expiresAt: null,
  loading: false,
  error: null,
};

// Async thunks
export const login = createAsyncThunk(
  "auth/login",
  async (credentials: LoginRequest, { rejectWithValue }) => {
    try {
      await apiClient.login(credentials);
      // После успешного логина, получаем информацию о сессии
      const session = await apiClient.getSession();
      return session;
    } catch (error: any) {
      return rejectWithValue(error.response?.data?.message || "Login failed");
    }
  },
);

export const logout = createAsyncThunk(
  "auth/logout",
  async (_, { rejectWithValue }) => {
    try {
      await apiClient.logout();
      return undefined;
    } catch (error: any) {
      return rejectWithValue(error.response?.data?.message || "Logout failed");
    }
  },
);

export const checkSession = createAsyncThunk(
  "auth/checkSession",
  async (_, { rejectWithValue }) => {
    try {
      const session = await apiClient.getSession();
      return session;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Session check failed",
      );
    }
  },
);

export const refreshToken = createAsyncThunk(
  "auth/refreshToken",
  async (_, { rejectWithValue }) => {
    try {
      await apiClient.refreshToken();
      const session = await apiClient.getSession();
      return session;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Token refresh failed",
      );
    }
  },
);

const authSlice = createSlice({
  name: "auth",
  initialState,
  reducers: {
    clearError: (state) => {
      state.error = null;
    },
    resetAuth: () => {
      return initialState;
    },
  },
  extraReducers: (builder) => {
    // Login
    builder.addCase(login.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      login.fulfilled,
      (state, action: PayloadAction<SessionResponse>) => {
        state.loading = false;
        state.isAuthenticated = action.payload.authenticated;
        state.username = action.payload.username;
        state.accessKey = action.payload.access_key || null;
        state.isAdmin = Boolean(action.payload.is_admin);
        state.role = action.payload.role || (state.isAdmin ? "admin" : "viewer");
        state.groups = action.payload.groups || [];
        state.policies = action.payload.policies || [];
        state.capabilities = buildCapabilities({
          role: state.role,
          isAdmin: state.isAdmin,
          policies: state.policies,
        });
        state.expiresAt = action.payload.expires_at;
      },
    );
    builder.addCase(login.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
      state.isAuthenticated = false;
    });

    // Logout
    builder.addCase(logout.pending, (state) => {
      state.loading = true;
    });
    builder.addCase(logout.fulfilled, () => {
      return initialState;
    });
    builder.addCase(logout.rejected, () => {
      // Even if logout fails on server, clear local state
      return initialState;
    });

    // Check Session
    builder.addCase(checkSession.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      checkSession.fulfilled,
      (state, action: PayloadAction<SessionResponse>) => {
        state.loading = false;
        state.isAuthenticated = action.payload.authenticated;
        state.username = action.payload.username;
        state.accessKey = action.payload.access_key || null;
        state.isAdmin = Boolean(action.payload.is_admin);
        state.role = action.payload.role || (state.isAdmin ? "admin" : "viewer");
        state.groups = action.payload.groups || [];
        state.policies = action.payload.policies || [];
        state.capabilities = buildCapabilities({
          role: state.role,
          isAdmin: state.isAdmin,
          policies: state.policies,
        });
        state.expiresAt = action.payload.expires_at;
      },
    );
    builder.addCase(checkSession.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
      state.isAuthenticated = false;
    });

    // Refresh Token
    builder.addCase(
      refreshToken.fulfilled,
      (state, action: PayloadAction<SessionResponse>) => {
        state.isAuthenticated = action.payload.authenticated;
        state.username = action.payload.username;
        state.accessKey = action.payload.access_key || null;
        state.isAdmin = Boolean(action.payload.is_admin);
        state.role = action.payload.role || (state.isAdmin ? "admin" : "viewer");
        state.groups = action.payload.groups || [];
        state.policies = action.payload.policies || [];
        state.capabilities = buildCapabilities({
          role: state.role,
          isAdmin: state.isAdmin,
          policies: state.policies,
        });
        state.expiresAt = action.payload.expires_at;
      },
    );
  },
});

export const { clearError, resetAuth } = authSlice.actions;

// Selectors
export const selectAuth = (state: RootState) => state.auth;
export const selectIsAuthenticated = (state: RootState) =>
  state.auth.isAuthenticated;
export const selectUsername = (state: RootState) => state.auth.username;
export const selectCapabilities = (state: RootState) => state.auth.capabilities;
export const selectRole = (state: RootState) => state.auth.role;
export const selectCan =
  (capability: Capability) =>
  (state: RootState): boolean =>
    canCapability(state.auth.capabilities, capability);

export default authSlice.reducer;
