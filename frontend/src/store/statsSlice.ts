import { createSlice, createAsyncThunk, PayloadAction } from "@reduxjs/toolkit";
import { apiClient } from "../api/client";
import type { RootState } from "./index";
import type {
  SystemStats,
  CapacityStats,
  ActivityStats,
  ServerStats,
  DriveStats,
  PoolInfo,
  ApiErrorStats,
  DataThroughputStats,
} from "../api/types";

// State interface
interface StatsState {
  system: SystemStats | null;
  capacity: CapacityStats | null;
  activity: ActivityStats | null;
  servers: ServerStats | null;
  drives: DriveStats | null;
  pools: PoolInfo[];
  apiErrors: ApiErrorStats | null;
  throughput: DataThroughputStats | null;
  loading: boolean;
  error: string | null;
}

// Initial state
const initialState: StatsState = {
  system: null,
  capacity: null,
  activity: null,
  servers: null,
  drives: null,
  pools: [],
  apiErrors: null,
  throughput: null,
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

export const fetchServerStats = createAsyncThunk(
  "stats/fetchServerStats",
  async () => {
    const data = await apiClient.getServerStats();
    return data as ServerStats;
  },
);

export const fetchDriveStats = createAsyncThunk(
  "stats/fetchDriveStats",
  async () => {
    const data = await apiClient.getDriveStats();
    return data as DriveStats;
  },
);

export const fetchPoolStats = createAsyncThunk(
  "stats/fetchPoolStats",
  async () => {
    const data = await apiClient.getPoolStats();
    return (Array.isArray(data) ? data : []) as PoolInfo[];
  },
);

export const fetchApiErrorStats = createAsyncThunk(
  "stats/fetchApiErrorStats",
  async () => {
    const data = await apiClient.getApiErrorStats();
    return data as ApiErrorStats;
  },
);

export const fetchThroughputStats = createAsyncThunk(
  "stats/fetchThroughputStats",
  async () => {
    const data = await apiClient.getDataThroughputStats();
    return data as DataThroughputStats;
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

export const fetchDashboardStats = createAsyncThunk(
  "stats/fetchDashboardStats",
  async (_, { dispatch }) => {
    await Promise.all([
      dispatch(fetchSystemStats()),
      dispatch(fetchCapacityStats()),
      dispatch(fetchActivityStats()),
      dispatch(fetchServerStats()),
      dispatch(fetchDriveStats()),
      dispatch(fetchPoolStats()),
      dispatch(fetchApiErrorStats()),
      dispatch(fetchThroughputStats()),
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
      state.servers = null;
      state.drives = null;
      state.pools = [];
      state.apiErrors = null;
      state.throughput = null;
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

    // Server stats
    builder.addCase(fetchServerStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchServerStats.fulfilled,
      (state, action: PayloadAction<ServerStats>) => {
        state.loading = false;
        state.servers = action.payload;
      },
    );
    builder.addCase(fetchServerStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch server stats";
    });

    // Drive stats
    builder.addCase(fetchDriveStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchDriveStats.fulfilled,
      (state, action: PayloadAction<DriveStats>) => {
        state.loading = false;
        state.drives = action.payload;
      },
    );
    builder.addCase(fetchDriveStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch drive stats";
    });

    // Pool stats
    builder.addCase(fetchPoolStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchPoolStats.fulfilled,
      (state, action: PayloadAction<PoolInfo[]>) => {
        state.loading = false;
        state.pools = action.payload;
      },
    );
    builder.addCase(fetchPoolStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch pool stats";
    });

    // API error stats
    builder.addCase(fetchApiErrorStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchApiErrorStats.fulfilled,
      (state, action: PayloadAction<ApiErrorStats>) => {
        state.loading = false;
        state.apiErrors = action.payload;
      },
    );
    builder.addCase(fetchApiErrorStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch API error stats";
    });

    // Throughput stats
    builder.addCase(fetchThroughputStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchThroughputStats.fulfilled,
      (state, action: PayloadAction<DataThroughputStats>) => {
        state.loading = false;
        state.throughput = action.payload;
      },
    );
    builder.addCase(fetchThroughputStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch throughput stats";
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

    // Fetch dashboard stats
    builder.addCase(fetchDashboardStats.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchDashboardStats.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(fetchDashboardStats.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch dashboard stats";
    });
  },
});

// Actions
export const { clearStats } = statsSlice.actions;

// Selectors
export const selectSystemStats = (state: RootState) => state.stats.system;
export const selectCapacityStats = (state: RootState) => state.stats.capacity;
export const selectActivityStats = (state: RootState) => state.stats.activity;
export const selectServerStats = (state: RootState) => state.stats.servers;
export const selectDriveStats = (state: RootState) => state.stats.drives;
export const selectPoolStats = (state: RootState) => state.stats.pools;
export const selectApiErrorStats = (state: RootState) => state.stats.apiErrors;
export const selectThroughputStats = (state: RootState) => state.stats.throughput;
export const selectStatsLoading = (state: RootState) => state.stats.loading;
export const selectStatsError = (state: RootState) => state.stats.error;

// Reducer
export default statsSlice.reducer;
