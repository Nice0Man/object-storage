import { createSlice, createAsyncThunk, PayloadAction } from "@reduxjs/toolkit";
import apiClient from "../api/client";
import type { S3Object, ObjectInfo, ListObjectsParams } from "../api/types";
import type { RootState } from "./index";

interface ObjectsState {
  objects: S3Object[];
  prefixes: string[];
  selectedObject: ObjectInfo | null;
  currentPrefix: string;
  loading: boolean;
  uploadProgress: { [key: string]: number };
  error: string | null;
  total: number;
  isTruncated: boolean;
  nextMarker?: string;
}

const initialState: ObjectsState = {
  objects: [],
  prefixes: [],
  selectedObject: null,
  currentPrefix: "",
  loading: false,
  uploadProgress: {},
  error: null,
  total: 0,
  isTruncated: false,
};

// Async thunks
export const fetchObjects = createAsyncThunk(
  "objects/fetchObjects",
  async (
    { bucketName, params }: { bucketName: string; params?: ListObjectsParams },
    { rejectWithValue },
  ) => {
    try {
      const response = await apiClient.listObjects(bucketName, params);
      return response;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to fetch objects",
      );
    }
  },
);

export const uploadObject = createAsyncThunk(
  "objects/uploadObject",
  async (
    { bucketName, key, file, sseCustomerKey }: {
      bucketName: string;
      key: string;
      file: File;
      sseCustomerKey?: string;
    },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.uploadObject(
        bucketName,
        key,
        file,
        undefined, // contentType - auto-detect
        sseCustomerKey,
        (progress) => {
          dispatch(setUploadProgress({ key, progress }));
        }
      );
      // Refresh objects list after upload
      dispatch(fetchObjects({ bucketName }));
      return key;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to upload object",
      );
    }
  },
);

export const deleteObject = createAsyncThunk(
  "objects/deleteObject",
  async (
    { bucketName, key }: { bucketName: string; key: string },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.deleteObject(bucketName, key);
      // Refresh objects list after deletion
      dispatch(fetchObjects({ bucketName }));
      return key;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to delete object",
      );
    }
  },
);

export const batchDeleteObjects = createAsyncThunk(
  "objects/batchDeleteObjects",
  async (
    { bucketName, keys }: { bucketName: string; keys: string[] },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.batchDeleteObjects(bucketName, keys);
      // Refresh objects list after deletion
      dispatch(fetchObjects({ bucketName }));
      return keys;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to delete objects",
      );
    }
  },
);

export const fetchObjectInfo = createAsyncThunk(
  "objects/fetchObjectInfo",
  async (
    { bucketName, key }: { bucketName: string; key: string },
    { rejectWithValue },
  ) => {
    try {
      const info = await apiClient.getObjectInfo(bucketName, key);
      return info;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to fetch object info",
      );
    }
  },
);

export const downloadObject = createAsyncThunk(
  "objects/downloadObject",
  async (
    { bucketName, key, sseCustomerKey }: { bucketName: string; key: string; sseCustomerKey?: string },
    { rejectWithValue },
  ) => {
    try {
      const blob = await apiClient.downloadObject(bucketName, key, sseCustomerKey);
      // Create download link
      const url = window.URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.href = url;
      link.download = key.split("/").pop() || "download";
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      window.URL.revokeObjectURL(url);
      return key;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to download object",
      );
    }
  },
);

export const copyObject = createAsyncThunk(
  "objects/copyObject",
  async (
    {
      sourceBucket,
      sourceKey,
      destinationBucket,
      destinationKey,
    }: {
      sourceBucket: string;
      sourceKey: string;
      destinationBucket: string;
      destinationKey: string;
    },
    { rejectWithValue, dispatch },
  ) => {
    try {
      await apiClient.copyObject({
        source_bucket: sourceBucket,
        source_key: sourceKey,
        destination_bucket: destinationBucket,
        destination_key: destinationKey,
      });
      // Refresh objects list after copy
      dispatch(fetchObjects({ bucketName: destinationBucket }));
      return destinationKey;
    } catch (error: any) {
      return rejectWithValue(
        error.response?.data?.message || "Failed to copy object",
      );
    }
  },
);

const objectsSlice = createSlice({
  name: "objects",
  initialState,
  reducers: {
    clearError: (state) => {
      state.error = null;
    },
    setCurrentPrefix: (state, action: PayloadAction<string>) => {
      state.currentPrefix = action.payload;
    },
    clearSelectedObject: (state) => {
      state.selectedObject = null;
    },
    setUploadProgress: (
      state,
      action: PayloadAction<{ key: string; progress: number }>,
    ) => {
      state.uploadProgress[action.payload.key] = action.payload.progress;
    },
    clearUploadProgress: (state, action: PayloadAction<string>) => {
      delete state.uploadProgress[action.payload];
    },
  },
  extraReducers: (builder) => {
    // Fetch Objects
    builder.addCase(fetchObjects.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchObjects.fulfilled, (state, action) => {
      state.loading = false;
      state.objects = action.payload.objects;
      state.prefixes = action.payload.prefixes;
      state.total = action.payload.total;
      state.isTruncated = action.payload.is_truncated;
      state.nextMarker = action.payload.next_marker;
    });
    builder.addCase(fetchObjects.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Upload Object
    builder.addCase(uploadObject.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(uploadObject.fulfilled, (state, action) => {
      state.loading = false;
      delete state.uploadProgress[action.payload];
    });
    builder.addCase(uploadObject.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Delete Object
    builder.addCase(deleteObject.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(deleteObject.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(deleteObject.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Batch Delete Objects
    builder.addCase(batchDeleteObjects.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(batchDeleteObjects.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(batchDeleteObjects.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Fetch Object Info
    builder.addCase(fetchObjectInfo.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(
      fetchObjectInfo.fulfilled,
      (state, action: PayloadAction<ObjectInfo>) => {
        state.loading = false;
        state.selectedObject = action.payload;
      },
    );
    builder.addCase(fetchObjectInfo.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Download Object
    builder.addCase(downloadObject.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(downloadObject.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(downloadObject.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });

    // Copy Object
    builder.addCase(copyObject.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(copyObject.fulfilled, (state) => {
      state.loading = false;
    });
    builder.addCase(copyObject.rejected, (state, action) => {
      state.loading = false;
      state.error = action.payload as string;
    });
  },
});

export const {
  clearError,
  setCurrentPrefix,
  clearSelectedObject,
  setUploadProgress,
  clearUploadProgress,
} = objectsSlice.actions;

// Selectors
export const selectObjects = (state: RootState) => state.objects.objects;
export const selectPrefixes = (state: RootState) => state.objects.prefixes;
export const selectSelectedObject = (state: RootState) =>
  state.objects.selectedObject;
export const selectObjectsLoading = (state: RootState) => state.objects.loading;
export const selectObjectsError = (state: RootState) => state.objects.error;
export const selectUploadProgress = (state: RootState) =>
  state.objects.uploadProgress;

export default objectsSlice.reducer;
