import { createSlice, createAsyncThunk, PayloadAction, createSelector } from "@reduxjs/toolkit";
import { apiClient } from "../api/client";
import type { RootState } from "./index";
import type {
  DashboardBoard,
  DashboardWidget,
  WidgetType,
  CreateBoardRequest,
  UpdateBoardRequest,
  UpdateWidgetRequest,
} from "../api/types";

// Empty array constant to avoid creating new references
const EMPTY_WIDGETS: DashboardWidget[] = [];

function createDefaultWidgets(boardId: string): DashboardWidget[] {
  const now = Date.now();
  const make = (
    suffix: string,
    widgetType: WidgetType,
    x: number,
    y: number,
    w: number,
    h: number,
    orderIndex: number
  ): DashboardWidget => ({
    id: `widget-${suffix}-${now}-${orderIndex}`,
    board_id: boardId,
    widget_type: widgetType,
    position: { x, y },
    size: { w, h },
    visible: true,
    refresh_interval: 30,
    settings: {},
    order_index: orderIndex,
  });

  return [
    make("capacity", "capacity", 0, 0, 2, 1, 0),
    make("servers", "servers", 2, 0, 1, 1, 1),
    make("drives", "drives", 3, 0, 1, 1, 2),
    make("buckets", "buckets", 0, 1, 1, 1, 3),
    make("api-errors", "api_errors", 1, 1, 1, 1, 4),
    make("throughput", "throughput", 2, 1, 2, 1, 5),
    make("encryption", "encryption", 0, 2, 1, 1, 6),
    make("pools", "pools", 1, 2, 1, 1, 7),
    make("quick-actions", "quick_actions", 2, 2, 2, 1, 8),
  ];
}

// State interface
interface DashboardState {
  boards: DashboardBoard[];
  widgets: Record<string, DashboardWidget[]>; // board_id -> widgets
  activeBoard: string | null;
  editMode: boolean;
  loading: boolean;
  savingWidgets: boolean;
  error: string | null;
}

// Initial state
const initialState: DashboardState = {
  boards: [],
  widgets: {},
  activeBoard: null,
  editMode: false,
  loading: false,
  savingWidgets: false,
  error: null,
};

// Async thunks
export const fetchBoards = createAsyncThunk(
  "dashboard/fetchBoards",
  async () => {
    const data = await apiClient.listDashboardBoards();
    return data;
  },
);

export const createBoard = createAsyncThunk(
  "dashboard/createBoard",
  async (request: CreateBoardRequest) => {
    const data = await apiClient.createDashboardBoard(request);
    return data;
  },
);

export const updateBoard = createAsyncThunk(
  "dashboard/updateBoard",
  async ({ id, request }: { id: string; request: UpdateBoardRequest }) => {
    const data = await apiClient.updateDashboardBoard(id, request);
    return data;
  },
);

export const deleteBoard = createAsyncThunk(
  "dashboard/deleteBoard",
  async (id: string) => {
    await apiClient.deleteDashboardBoard(id);
    return id;
  },
);

export const fetchWidgets = createAsyncThunk(
  "dashboard/fetchWidgets",
  async (boardId: string) => {
    const data = await apiClient.listDashboardWidgets(boardId);
    return { boardId, widgets: data };
  },
);

export const saveWidgets = createAsyncThunk(
  "dashboard/saveWidgets",
  async ({ boardId, widgets }: { boardId: string; widgets: DashboardWidget[] }) => {
    await apiClient.saveDashboardWidgets(boardId, widgets);
    return { boardId, widgets };
  },
);

export const updateWidget = createAsyncThunk(
  "dashboard/updateWidget",
  async ({ id, request }: { id: string; request: UpdateWidgetRequest }) => {
    const data = await apiClient.updateDashboardWidget(id, request);
    return data;
  },
);

// Initialize dashboard - fetch boards and widgets for first board
export const initializeDashboard = createAsyncThunk(
  "dashboard/initialize",
  async (_, { dispatch, getState }) => {
    // Fetch boards first
    await dispatch(fetchBoards());

    const state = getState() as RootState;
    const boards = state.dashboard.boards;

    // If no boards, create a default one
    if (boards.length === 0) {
      const result = await dispatch(createBoard({ name: "Main Dashboard", order_index: 0 }));
      if (createBoard.fulfilled.match(result)) {
        const createdBoardId = result.payload.id;
        await dispatch(fetchWidgets(createdBoardId));
        const createdState = getState() as RootState;
        const createdWidgets = createdState.dashboard.widgets[createdBoardId] ?? [];
        if (createdWidgets.length === 0) {
          const defaults = createDefaultWidgets(createdBoardId);
          await dispatch(saveWidgets({ boardId: createdBoardId, widgets: defaults }));
        }
        return result.payload.id;
      }
    } else {
      // Fetch widgets for first board
      const firstBoard = boards[0];
      await dispatch(fetchWidgets(firstBoard.id));
      const updatedState = getState() as RootState;
      const firstBoardWidgets = updatedState.dashboard.widgets[firstBoard.id] ?? [];
      if (firstBoardWidgets.length === 0) {
        const defaults = createDefaultWidgets(firstBoard.id);
        await dispatch(saveWidgets({ boardId: firstBoard.id, widgets: defaults }));
      }
      return firstBoard.id;
    }

    return null;
  },
);

// Slice
const dashboardSlice = createSlice({
  name: "dashboard",
  initialState,
  reducers: {
    setActiveBoard: (state, action: PayloadAction<string | null>) => {
      state.activeBoard = action.payload;
    },
    setEditMode: (state, action: PayloadAction<boolean>) => {
      state.editMode = action.payload;
    },
    updateLocalWidget: (state, action: PayloadAction<DashboardWidget>) => {
      const widget = action.payload;
      const boardWidgets = state.widgets[widget.board_id];
      if (boardWidgets) {
        const index = boardWidgets.findIndex((w) => w.id === widget.id);
        if (index !== -1) {
          boardWidgets[index] = widget;
        }
      }
    },
    updateLocalWidgets: (state, action: PayloadAction<{ boardId: string; widgets: DashboardWidget[] }>) => {
      state.widgets[action.payload.boardId] = action.payload.widgets;
    },
    clearDashboard: (state) => {
      state.boards = [];
      state.widgets = {};
      state.activeBoard = null;
      state.editMode = false;
      state.error = null;
    },
  },
  extraReducers: (builder) => {
    // Fetch boards
    builder.addCase(fetchBoards.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchBoards.fulfilled, (state, action: PayloadAction<DashboardBoard[]>) => {
      state.loading = false;
      state.boards = action.payload;
      // Set active board if not set
      if (!state.activeBoard && action.payload.length > 0) {
        state.activeBoard = action.payload[0].id;
      }
    });
    builder.addCase(fetchBoards.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch boards";
    });

    // Create board
    builder.addCase(createBoard.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(createBoard.fulfilled, (state, action: PayloadAction<DashboardBoard>) => {
      state.loading = false;
      state.boards.push(action.payload);
      state.activeBoard = action.payload.id;
    });
    builder.addCase(createBoard.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to create board";
    });

    // Update board
    builder.addCase(updateBoard.fulfilled, (state, action: PayloadAction<DashboardBoard>) => {
      const index = state.boards.findIndex((b) => b.id === action.payload.id);
      if (index !== -1) {
        state.boards[index] = action.payload;
      }
    });

    // Delete board
    builder.addCase(deleteBoard.fulfilled, (state, action: PayloadAction<string>) => {
      state.boards = state.boards.filter((b) => b.id !== action.payload);
      delete state.widgets[action.payload];

      // Set new active board if deleted board was active
      if (state.activeBoard === action.payload) {
        state.activeBoard = state.boards.length > 0 ? state.boards[0].id : null;
      }
    });

    // Fetch widgets
    builder.addCase(fetchWidgets.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(fetchWidgets.fulfilled, (state, action) => {
      state.loading = false;
      state.widgets[action.payload.boardId] = action.payload.widgets;
    });
    builder.addCase(fetchWidgets.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to fetch widgets";
    });

    // Save widgets
    builder.addCase(saveWidgets.pending, (state) => {
      state.savingWidgets = true;
      state.error = null;
    });
    builder.addCase(saveWidgets.fulfilled, (state, action) => {
      state.savingWidgets = false;
      state.widgets[action.payload.boardId] = action.payload.widgets;
    });
    builder.addCase(saveWidgets.rejected, (state, action) => {
      state.savingWidgets = false;
      state.error = action.error.message || "Failed to save widgets";
    });

    // Update widget
    builder.addCase(updateWidget.fulfilled, (state, action: PayloadAction<DashboardWidget>) => {
      const widget = action.payload;
      const boardWidgets = state.widgets[widget.board_id];
      if (boardWidgets) {
        const index = boardWidgets.findIndex((w) => w.id === widget.id);
        if (index !== -1) {
          boardWidgets[index] = widget;
        }
      }
    });

    // Initialize dashboard
    builder.addCase(initializeDashboard.pending, (state) => {
      state.loading = true;
      state.error = null;
    });
    builder.addCase(initializeDashboard.fulfilled, (state, action) => {
      state.loading = false;
      if (action.payload) {
        state.activeBoard = action.payload;
      }
    });
    builder.addCase(initializeDashboard.rejected, (state, action) => {
      state.loading = false;
      state.error = action.error.message || "Failed to initialize dashboard";
    });
  },
});

// Actions
export const {
  setActiveBoard,
  setEditMode,
  updateLocalWidget,
  updateLocalWidgets,
  clearDashboard,
} = dashboardSlice.actions;

// Base selectors
export const selectBoards = (state: RootState) => state.dashboard.boards;
export const selectActiveBoard = (state: RootState) => state.dashboard.activeBoard;
export const selectWidgetsMap = (state: RootState) => state.dashboard.widgets;
export const selectEditMode = (state: RootState) => state.dashboard.editMode;
export const selectDashboardLoading = (state: RootState) => state.dashboard.loading;
export const selectSavingWidgets = (state: RootState) => state.dashboard.savingWidgets;
export const selectDashboardError = (state: RootState) => state.dashboard.error;

// Memoized selectors to prevent unnecessary rerenders
export const selectActiveBoardData = createSelector(
  [selectBoards, selectActiveBoard],
  (boards, activeBoard) => boards.find((b) => b.id === activeBoard)
);

export const selectActiveWidgets = createSelector(
  [selectActiveBoard, selectWidgetsMap],
  (activeBoard, widgets) => {
    if (!activeBoard) return EMPTY_WIDGETS;
    return widgets[activeBoard] ?? EMPTY_WIDGETS;
  }
);

export const selectWidgets = createSelector(
  [selectWidgetsMap, (_state: RootState, boardId: string) => boardId],
  (widgets, boardId) => widgets[boardId] ?? EMPTY_WIDGETS
);

// Reducer
export default dashboardSlice.reducer;
