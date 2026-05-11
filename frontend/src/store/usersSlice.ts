import { createSlice, createAsyncThunk, PayloadAction } from "@reduxjs/toolkit";
import apiClient from "../api/client";
import type { User, CreateUserRequest, UpdateUserRequest } from "../api/types";
import type { RootState } from "./index";

interface UsersState {
  users: User[];
  selectedUser: User | null;
  loading: boolean;
  error: string | null;
  total: number;
}

const initialState: UsersState = {
  users: [],
  selectedUser: null,
  loading: false,
  error: null,
  total: 0,
};

// Async thunks
export const fetchUsers = createAsyncThunk(
  "users/fetchUsers",
  async (_, { rejectWithValue }) => {
    try {
      const response = await apiClient.listUsers();
      return response;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to fetch users",
      );
    }
  },
);

export const createUser = createAsyncThunk(
  "users/createUser",
  async (request: CreateUserRequest, { rejectWithValue, dispatch }) => {
    try {
      await apiClient.createUser(request);
      // Refresh users list after creation
      dispatch(fetchUsers());
      return request.access_key;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to create user",
      );
    }
  },
);

export const deleteUser = createAsyncThunk(
  "users/deleteUser",
  async (accessKey: string, { rejectWithValue, dispatch }) => {
    try {
      await apiClient.deleteUser(accessKey);
      // Refresh users list after deletion
      dispatch(fetchUsers());
      return accessKey;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to delete user",
      );
    }
  },
);

export const updateUser = createAsyncThunk(
  "users/updateUser",
  async (
    { accessKey, request }: { accessKey: string; request: UpdateUserRequest },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.updateUser(accessKey, request);
      // Refresh users list after update
      dispatch(fetchUsers());
      return accessKey;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to update user",
      );
    }
  },
);

export const attachUserPolicy = createAsyncThunk(
  "users/attachUserPolicy",
  async (
    { accessKey, policyName }: { accessKey: string; policyName: string },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.attachUserPolicy(accessKey, policyName);
      // Refresh users list after attaching policy
      dispatch(fetchUsers());
      return { accessKey, policyName };
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to attach policy",
      );
    }
  },
);

export const detachUserPolicy = createAsyncThunk(
  "users/detachUserPolicy",
  async (
    { accessKey, policyName }: { accessKey: string; policyName: string },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.detachUserPolicy(accessKey, policyName);
      // Refresh users list after detaching policy
      dispatch(fetchUsers());
      return { accessKey, policyName };
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to detach policy",
      );
    }
  },
);

const usersSlice = createSlice({
  name: "users",
  initialState,
  reducers: {
    clearError: (state) => {
      state.error = null;
    },
    setSelectedUser: (state, action: PayloadAction<User | null>) => {
      state.selectedUser = action.payload;
    },
  },
  extraReducers: (builder) => {
    // Fetch Users
    builder.addCase(fetchUsers.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchUsers.fulfilled, (state, action) => {
      state.loading = false;
      state.users = action.payload.users;
      state.total = action.payload.total;
    });
    builder.addCase(fetchUsers.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Create User
    builder.addCase(createUser.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(createUser.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(createUser.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Delete User
    builder.addCase(deleteUser.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(deleteUser.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(deleteUser.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Update User
    builder.addCase(updateUser.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(updateUser.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(updateUser.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Attach User Policy
    builder.addCase(attachUserPolicy.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(attachUserPolicy.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(attachUserPolicy.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Detach User Policy
    builder.addCase(detachUserPolicy.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(detachUserPolicy.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(detachUserPolicy.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });
  },
});

export const { clearError, setSelectedUser } = usersSlice.actions;

// Selectors
export const selectUsers = (state: RootState) => state.users.users;
export const selectSelectedUser = (state: RootState) =>
  state.users.selectedUser;
export const selectUsersLoading = (state: RootState) => state.users.loading;
export const selectUsersError = (state: RootState) => state.users.error;

export default usersSlice.reducer;
