import { createSlice, createAsyncThunk, PayloadAction } from "@reduxjs/toolkit";
import { apiClient } from "../api/client";
import type { RootState } from "./index";
import type {
  SystemStats,
  CapacityStats,
  ActivityStats,
} from "../api/types";

// State interface
interface StatsState {
  system: SystemStats | null;
  capacity: CapacityStats | null;
  activity: ActivityStats | null;
  loading: boolean;
  error: string | null;
}

// Initial state
const initialState: StatsState = {
  system: null,
  capacity: null,
  activity: null,
  loading: false,
  error: null,
};

// Async thunks
export const fetchSystemStats = createAsyncThunk(
  "stats/fetchSystemStats",
  async () => {
    const data = await apiClient.getSystemStats();
    return data;
  },
);

export const fetchCapacityStats = createAsyncThunk(
  "stats/fetchCapacityStats",
  async () => {
    const data = await apiClient.getCapacityStats();
    return data;
  },
);

export const fetchActivityStats = createAsyncThunk(
  "stats/fetchActivityStats",
  async () => {
    const data = await apiClient.getActivityStats();
    return data;
  },
);

export const fetchAllStats = createAsyncThunk(
  "stats/fetchAllStats",
  async (_, { dispatch }) => {
    await Promise.all([
      dispatch(fetchSystemStats()),
      dispatch(fetchCapacityStats()),
      dispatch(fetchActivityStats()),
    ]);
  },
);

// Slice
const statsSlice = createSlice({
  name: "stats",
  initialState,
  reducers: {
    clearStats: (state) => {
      state.system = null;
      state.capacity = null;
      state.activity = null;
      state.error = null;
    },
  },
  extraReducers: (builder) => {
    // System stats
    builder.addCase(fetchSystemStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchSystemStats.fulfilled,
      (state, action: PayloadAction<SystemStats>) => {
        state.loading = false;
        state.system = action.payload;
      },
    );
    builder.addCase(fetchSystemStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch system stats";
    });

    // Capacity stats
    builder.addCase(fetchCapacityStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchCapacityStats.fulfilled,
      (state, action: PayloadAction<CapacityStats>) => {
        state.loading = false;
        state.capacity = action.payload;
      },
    );
    builder.addCase(fetchCapacityStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch capacity stats";
    });

    // Activity stats
    builder.addCase(fetchActivityStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchActivityStats.fulfilled,
      (state, action: PayloadAction<ActivityStats>) => {
        state.loading = false;
        state.activity = action.payload;
      },
    );
    builder.addCase(fetchActivityStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch activity stats";
    });

    // Fetch all stats
    builder.addCase(fetchAllStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchAllStats.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(fetchAllStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch stats";
    });
  },
});

// Actions
export const { clearStats } = statsSlice.actions;

// Selectors
export const selectSystemStats = (state: RootState) => state.stats.system;
export const selectCapacityStats = (state: RootState) => state.stats.capacity;
export const selectActivityStats = (state: RootState) => state.stats.activity;
export const selectStatsLoading = (state: RootState) => state.stats.loading;
export const selectStatsError = (state: RootState) => state.stats.error;

// Reducer
export default statsSlice.reducer;

