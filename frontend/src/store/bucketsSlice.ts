import { createSlice, createAsyncThunk, PayloadAction } from "@reduxjs/toolkit";
import apiClient from "../api/client";
import type {
  Bucket,
  CreateBucketRequest,
  BucketInfo,
  BucketPolicy,
} from "../api/types";
import type { RootState } from "./index";

interface BucketsState {
  buckets: Bucket[];
  selectedBucket: BucketInfo | null;
  selectedBucketPolicy: BucketPolicy | null;
  loading: boolean;
  error: string | null;
  total: number;
}

const initialState: BucketsState = {
  buckets: [],
  selectedBucket: null,
  selectedBucketPolicy: null,
  loading: false,
  error: null,
  total: 0,
};

// Async thunks
export const fetchBuckets = createAsyncThunk(
  "buckets/fetchBuckets",
  async (_, { rejectWithValue }) => {
    try {
      const response = await apiClient.listBuckets();
      return response;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to fetch buckets",
      );
    }
  },
);

export const createBucket = createAsyncThunk(
  "buckets/createBucket",
  async (request: CreateBucketRequest, { rejectWithValue, dispatch }) => {
    try {
      await apiClient.createBucket(request);
      // Refresh bucket list after creation
      dispatch(fetchBuckets());
      return request.name;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to create bucket",
      );
    }
  },
);

export const deleteBucket = createAsyncThunk(
  "buckets/deleteBucket",
  async (bucketName: string, { rejectWithValue, dispatch }) => {
    try {
      await apiClient.deleteBucket(bucketName);
      // Refresh bucket list after deletion
      dispatch(fetchBuckets());
      return bucketName;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to delete bucket",
      );
    }
  },
);

export const fetchBucketInfo = createAsyncThunk(
  "buckets/fetchBucketInfo",
  async (bucketName: string, { rejectWithValue }) => {
    try {
      const info = await apiClient.getBucketInfo(bucketName);
      return info;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to fetch bucket info",
      );
    }
  },
);

export const fetchBucketPolicy = createAsyncThunk(
  "buckets/fetchBucketPolicy",
  async (bucketName: string, { rejectWithValue }) => {
    try {
      const policy = await apiClient.getBucketPolicy(bucketName);
      return policy;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to fetch bucket policy",
      );
    }
  },
);

export const updateBucketPolicy = createAsyncThunk(
  "buckets/updateBucketPolicy",
  async (
    { bucketName, policy }: { bucketName: string; policy: BucketPolicy },
    { rejectWithValue },
  ) => {
    try {
      await apiClient.setBucketPolicy(bucketName, policy);
      return policy;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to update bucket policy",
      );
    }
  },
);

const bucketsSlice = createSlice({
  name: "buckets",
  initialState,
  reducers: {
    clearError: (state) => {
      state.error = null;
    },
    clearSelectedBucket: (state) => {
      state.selectedBucket = null;
      state.selectedBucketPolicy = null;
    },
  },
  extraReducers: (builder) => {
    // Fetch Buckets
    builder.addCase(fetchBuckets.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchBuckets.fulfilled, (state, action) => {
      state.loading = false;
      state.buckets = action.payload.buckets || [];
      state.total = action.payload.total || action.payload.buckets?.length || 0;
    });
    builder.addCase(fetchBuckets.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Create Bucket
    builder.addCase(createBucket.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(createBucket.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(createBucket.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Delete Bucket
    builder.addCase(deleteBucket.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(deleteBucket.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(deleteBucket.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Fetch Bucket Info
    builder.addCase(fetchBucketInfo.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchBucketInfo.fulfilled,
      (state, action: PayloadAction<BucketInfo>) => {
        state.loading = false;
        state.selectedBucket = action.payload;
      },
    );
    builder.addCase(fetchBucketInfo.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Fetch Bucket Policy
    builder.addCase(fetchBucketPolicy.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchBucketPolicy.fulfilled,
      (state, action: PayloadAction<BucketPolicy>) => {
        state.loading = false;
        state.selectedBucketPolicy = action.payload;
      },
    );
    builder.addCase(fetchBucketPolicy.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Update Bucket Policy
    builder.addCase(updateBucketPolicy.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      updateBucketPolicy.fulfilled,
      (state, action: PayloadAction<BucketPolicy>) => {
        state.loading = false;
        state.selectedBucketPolicy = action.payload;
      },
    );
    builder.addCase(updateBucketPolicy.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });
  },
});

export const { clearError, clearSelectedBucket } = bucketsSlice.actions;

// Selectors
export const selectBuckets = (state: RootState) => state.buckets.buckets;
export const selectSelectedBucket = (state: RootState) =>
  state.buckets.selectedBucket;
export const selectBucketsLoading = (state: RootState) => state.buckets.loading;
export const selectBucketsError = (state: RootState) => state.buckets.error;

export default bucketsSlice.reducer;
