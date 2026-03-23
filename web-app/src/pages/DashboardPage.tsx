// Dashboard page with grid layout
import React, { useEffect, useState, useCallback, useMemo } from "react";
import {
  Box,
  IconButton,
  Button,
  Tooltip,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  Menu,
  MenuItem,
  Snackbar,
  Alert,
  Chip,
  Typography,
  alpha,
} from "@mui/material";
import {
  Add,
  Save,
  Close,
  MoreVert,
  Visibility,
  Dashboard as DashboardIcon,
  Tune,
  Refresh,
  Delete,
} from "@mui/icons-material";
import { Responsive } from "react-grid-layout";

// Layout item type for react-grid-layout
interface LayoutItem {
  i: string;
  x: number;
  y: number;
  w: number;
  h: number;
  minW?: number;
  minH?: number;
  maxW?: number;
  maxH?: number;
}

// Custom hook for container width (replaces WidthProvider)
function useContainerWidth() {
  const [width, setWidth] = React.useState(1200);
  const containerRef = React.useRef<HTMLDivElement>(null);

  React.useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const resizeObserver = new ResizeObserver((entries) => {
      for (const entry of entries) {
        setWidth(entry.contentRect.width);
      }
    });

    resizeObserver.observe(container);
    // Initial measurement
    setWidth(container.offsetWidth);

    return () => resizeObserver.disconnect();
  }, []);

  return { width, containerRef };
}
import { useTranslation } from "react-i18next";
import { useAppDispatch } from "../hooks/useAppDispatch";
import { useAppSelector } from "../hooks/useAppSelector";
import {
  initializeDashboard,
  fetchWidgets,
  saveWidgets,
  createBoard,
  updateBoard,
  deleteBoard,
  setActiveBoard,
  setEditMode,
  updateLocalWidgets,
  selectBoards,
  selectActiveBoard,
  selectActiveBoardData,
  selectActiveWidgets,
  selectEditMode,
  selectDashboardLoading,
  selectSavingWidgets,
} from "../store/dashboardSlice";
import Loader from "../components/Common/Loader";
import {
  CapacityWidget,
  ServersWidget,
  DrivesWidget,
  BucketsWidget,
  ApiErrorsWidget,
  ThroughputWidget,
  EncryptionWidget,
  PoolsWidget,
  QuickActionsWidget,
} from "../components/Dashboard/widgets";
import WidgetSettingsPanel from "../components/Dashboard/WidgetSettingsPanel";
import type { DashboardWidget, WidgetType } from "../api/types";

import "react-grid-layout/css/styles.css";
import "react-resizable/css/styles.css";

// Grid layout configuration
const GRID_ROW_HEIGHT = 280;
const GRID_MARGIN: [number, number] = [16, 16];

// Widget component mapper
const widgetComponents: Record<WidgetType, React.FC<any>> = {
  capacity: CapacityWidget,
  servers: ServersWidget,
  drives: DrivesWidget,
  buckets: BucketsWidget,
  api_errors: ApiErrorsWidget,
  throughput: ThroughputWidget,
  encryption: EncryptionWidget,
  pools: PoolsWidget,
  quick_actions: QuickActionsWidget,
};

const DashboardPage: React.FC = () => {
  const dispatch = useAppDispatch();
  const { t } = useTranslation();

  // Redux state
  const boards = useAppSelector(selectBoards);
  const activeBoard = useAppSelector(selectActiveBoard);
  const activeBoardData = useAppSelector(selectActiveBoardData);
  const widgets = useAppSelector(selectActiveWidgets);
  const editMode = useAppSelector(selectEditMode);
  const loading = useAppSelector(selectDashboardLoading);
  const savingWidgets = useAppSelector(selectSavingWidgets);

  // Grid container width
  const { width: gridWidth, containerRef } = useContainerWidth();

  // Local state
  const [createBoardDialogOpen, setCreateBoardDialogOpen] = useState(false);
  const [newBoardName, setNewBoardName] = useState("");
  const [renameBoardDialogOpen, setRenameBoardDialogOpen] = useState(false);
  const [boardMenuAnchor, setBoardMenuAnchor] = useState<null | HTMLElement>(null);
  const [selectedWidget, setSelectedWidget] = useState<DashboardWidget | null>(null);
  const [settingsPanelOpen, setSettingsPanelOpen] = useState(false);
  const [addWidgetPanelOpen, setAddWidgetPanelOpen] = useState(false);
  const [snackbar, setSnackbar] = useState<{ open: boolean; message: string; severity: "success" | "error" }>({
    open: false,
    message: "",
    severity: "success",
  });

  // Initialize dashboard on mount
  useEffect(() => {
    dispatch(initializeDashboard());
  }, [dispatch]);

  // Create new board
  const handleCreateBoard = useCallback(async () => {
    if (!newBoardName.trim()) return;

    await dispatch(createBoard({ name: newBoardName, order_index: boards.length }));
    setNewBoardName("");
    setCreateBoardDialogOpen(false);
    setSnackbar({ open: true, message: "Board created successfully", severity: "success" });
  }, [dispatch, newBoardName, boards.length]);

  // Rename board
  const handleRenameBoard = useCallback(async () => {
    if (!activeBoard || !newBoardName.trim()) return;

    await dispatch(updateBoard({ id: activeBoard, request: { name: newBoardName } }));
    setNewBoardName("");
    setRenameBoardDialogOpen(false);
    setBoardMenuAnchor(null);
    setSnackbar({ open: true, message: "Board renamed successfully", severity: "success" });
  }, [dispatch, activeBoard, newBoardName]);

  // Delete board
  const handleDeleteBoard = useCallback(async () => {
    if (!activeBoard || boards.length <= 1) return;

    await dispatch(deleteBoard(activeBoard));
    setBoardMenuAnchor(null);
    setSnackbar({ open: true, message: "Board deleted successfully", severity: "success" });
  }, [dispatch, activeBoard, boards.length]);

  // Toggle edit mode
  const handleToggleEditMode = useCallback(() => {
    dispatch(setEditMode(!editMode));
  }, [dispatch, editMode]);

  // Save widgets
  const handleSaveWidgets = useCallback(async () => {
    if (!activeBoard) return;

    await dispatch(saveWidgets({ boardId: activeBoard, widgets }));
    dispatch(setEditMode(false));
    setSnackbar({ open: true, message: "Dashboard saved successfully", severity: "success" });
  }, [dispatch, activeBoard, widgets]);

  // Cancel edit mode
  const handleCancelEdit = useCallback(() => {
    if (activeBoard) {
      dispatch(fetchWidgets(activeBoard));
    }
    dispatch(setEditMode(false));
  }, [dispatch, activeBoard]);

  // Toggle widget visibility
  const handleToggleWidgetVisibility = useCallback(
    (widgetId: string) => {
      if (!activeBoard) return;

      const updatedWidgets = widgets.map((w) =>
        w.id === widgetId ? { ...w, visible: !w.visible } : w
      );
      dispatch(updateLocalWidgets({ boardId: activeBoard, widgets: updatedWidgets }));
    },
    [activeBoard, widgets, dispatch]
  );

  // Open widget settings
  const handleOpenWidgetSettings = useCallback((widget: DashboardWidget) => {
    setSelectedWidget(widget);
    setSettingsPanelOpen(true);
  }, []);

  // Update widget settings
  const handleUpdateWidgetSettings = useCallback(
    (updatedWidget: DashboardWidget) => {
      if (!activeBoard) return;

      const updatedWidgets = widgets.map((w) =>
        w.id === updatedWidget.id ? updatedWidget : w
      );
      dispatch(updateLocalWidgets({ boardId: activeBoard, widgets: updatedWidgets }));
      setSettingsPanelOpen(false);
      setSelectedWidget(null);
    },
    [activeBoard, widgets, dispatch]
  );

  // Add new widget
  const handleAddWidget = useCallback(
    (widgetType: WidgetType) => {
      if (!activeBoard) return;

      // Calculate position for new widget (find next available slot)
      const maxY = widgets.reduce((max, w) => Math.max(max, w.position.y + w.size.h), 0);
      const newWidget: DashboardWidget = {
        id: `widget-${Date.now()}`,
        board_id: activeBoard,
        widget_type: widgetType,
        position: { x: 0, y: maxY },
        size: { w: 1, h: 1 },
        visible: true,
        refresh_interval: 30,
        settings: {},
        order_index: widgets.length,
      };

      dispatch(updateLocalWidgets({ boardId: activeBoard, widgets: [...widgets, newWidget] }));
      setAddWidgetPanelOpen(false);
      setSnackbar({ open: true, message: `${widgetType} widget added`, severity: "success" });
    },
    [activeBoard, widgets, dispatch]
  );

  // Delete widget
  const handleDeleteWidget = useCallback(
    (widgetId: string) => {
      if (!activeBoard) return;

      const updatedWidgets = widgets.filter((w) => w.id !== widgetId);
      dispatch(updateLocalWidgets({ boardId: activeBoard, widgets: updatedWidgets }));
      setSnackbar({ open: true, message: "Widget deleted", severity: "success" });
    },
    [activeBoard, widgets, dispatch]
  );

  // Render widget component
  const renderWidget = useCallback(
    (widget: DashboardWidget) => {
      const WidgetComponent = widgetComponents[widget.widget_type];
      if (!WidgetComponent) return null;

      return (
        <WidgetComponent
          widget={widget}
          editMode={editMode}
          onSettingsClick={() => handleOpenWidgetSettings(widget)}
          onVisibilityToggle={() => handleToggleWidgetVisibility(widget.id)}
          onDelete={() => handleDeleteWidget(widget.id)}
        />
      );
    },
    [editMode, handleOpenWidgetSettings, handleToggleWidgetVisibility, handleDeleteWidget]
  );

  // Generate layout for react-grid-layout
  const visibleWidgets = useMemo(() => widgets.filter((w) => w.visible), [widgets]);

  const gridLayout = useMemo((): LayoutItem[] => {
    return visibleWidgets.map((widget, index) => ({
      i: widget.id,
      x: widget.position?.x ?? (index % 4),
      y: widget.position?.y ?? Math.floor(index / 4),
      w: widget.size?.w ?? 1,
      h: widget.size?.h ?? 1,
      minW: 1,
      minH: 1,
      maxW: 4,
      maxH: 2,
    }));
  }, [visibleWidgets]);

  // Handle layout change after drag/resize completes
  const handleDragResizeStop = useCallback(
    (layout: LayoutItem[]) => {
      if (!activeBoard || !editMode) return;

      const updatedWidgets = widgets.map((widget) => {
        const layoutItem = layout.find((l) => l.i === widget.id);
        if (layoutItem) {
          return {
            ...widget,
            position: { x: layoutItem.x, y: layoutItem.y },
            size: { w: layoutItem.w, h: layoutItem.h },
          };
        }
        return widget;
      });

      dispatch(updateLocalWidgets({ boardId: activeBoard, widgets: updatedWidgets }));
    },
    [activeBoard, editMode, widgets, dispatch]
  );

  // Refresh all widgets
  const handleRefreshAll = useCallback(() => {
    if (activeBoard) {
      dispatch(fetchWidgets(activeBoard));
    }
  }, [dispatch, activeBoard]);

  if (loading && boards.length === 0) {
    return <Loader message={t("dashboard.loading") || "Loading dashboard..."} />;
  }

  return (
    <Box sx={{ p: 3 }}>
      {/* Header */}
      <Box sx={{ display: "flex", justifyContent: "space-between", alignItems: "center", mb: 3 }}>
        <Typography variant="h4" fontWeight="bold">
          {t("dashboard.title") || "Dashboard"}
        </Typography>
        <Box sx={{ display: "flex", gap: 1 }}>
          <Tooltip title="Refresh">
            <IconButton onClick={handleRefreshAll}>
              <Refresh />
            </IconButton>
          </Tooltip>
        </Box>
      </Box>

      {/* Toolbar */}
      <Box
        sx={{
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
          mb: 3,
          gap: 2,
          flexWrap: "wrap",
        }}
      >
        {/* Board Chips Navigation */}
        <Box sx={{ display: "flex", alignItems: "center", gap: 1, flexWrap: "wrap" }}>
          {boards.map((board) => (
            <Chip
              key={board.id}
              label={board.name}
              icon={<DashboardIcon />}
              onClick={() => {
                dispatch(setActiveBoard(board.id));
                dispatch(fetchWidgets(board.id));
              }}
              color={activeBoard === board.id ? "primary" : "default"}
              variant={activeBoard === board.id ? "filled" : "outlined"}
            />
          ))}

          {/* Add Board Button */}
          <Tooltip title="New Board">
            <IconButton size="small" onClick={() => setCreateBoardDialogOpen(true)}>
              <Add />
            </IconButton>
          </Tooltip>

          {/* Board Menu */}
          {activeBoard && (
            <>
              <IconButton size="small" onClick={(e) => setBoardMenuAnchor(e.currentTarget)}>
                <MoreVert />
              </IconButton>
              <Menu
                anchorEl={boardMenuAnchor}
                open={Boolean(boardMenuAnchor)}
                onClose={() => setBoardMenuAnchor(null)}
              >
                <MenuItem
                  onClick={() => {
                    setNewBoardName(activeBoardData?.name || "");
                    setRenameBoardDialogOpen(true);
                  }}
                >
                  Rename Board
                </MenuItem>
                <MenuItem
                  onClick={handleDeleteBoard}
                  disabled={boards.length <= 1}
                  sx={{ color: "error.main" }}
                >
                  Delete Board
                </MenuItem>
              </Menu>
            </>
          )}
        </Box>

        {/* Action Buttons */}
        <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
          {editMode ? (
            <>
              <Button
                variant="outlined"
                size="small"
                startIcon={<Add />}
                onClick={() => setAddWidgetPanelOpen(true)}
              >
                Add Widget
              </Button>
              <Button
                variant="outlined"
                size="small"
                startIcon={<Close />}
                onClick={handleCancelEdit}
              >
                Cancel
              </Button>
              <Button
                variant="contained"
                size="small"
                startIcon={<Save />}
                onClick={handleSaveWidgets}
                disabled={savingWidgets}
              >
                {savingWidgets ? "Saving..." : "Save"}
              </Button>
            </>
          ) : (
            <Tooltip title="Customize Layout">
              <IconButton onClick={handleToggleEditMode}>
                <Tune />
              </IconButton>
            </Tooltip>
          )}
        </Box>
      </Box>

      {/* Hidden Widgets Panel (Edit Mode) */}
      {editMode && widgets.filter((w) => !w.visible).length > 0 && (
        <Box
          sx={{
            mb: 3,
            p: 2,
            display: "flex",
            alignItems: "center",
            gap: 2,
            bgcolor: (theme) => alpha(theme.palette.warning.main, 0.15),
            border: (theme) => `1px solid ${alpha(theme.palette.warning.main, 0.3)}`,
            borderRadius: 1,
          }}
        >
          <Typography variant="caption" fontWeight="bold" color="warning.main" sx={{ textTransform: "uppercase" }}>
            Hidden
          </Typography>
          <Box sx={{ display: "flex", gap: 1, flexWrap: "wrap" }}>
            {widgets.filter((w) => !w.visible).map((widget) => (
              <Chip
                key={widget.id}
                label={widget.widget_type.replace("_", " ")}
                size="small"
                icon={<Visibility />}
                onClick={() => handleToggleWidgetVisibility(widget.id)}
                onDelete={() => handleDeleteWidget(widget.id)}
                deleteIcon={<Delete fontSize="small" />}
                sx={{ cursor: "pointer", textTransform: "capitalize" }}
              />
            ))}
          </Box>
        </Box>
      )}

      {/* Widget Grid */}
      <div ref={containerRef}>
        <Responsive
          className="layout"
          layouts={{ lg: gridLayout, md: gridLayout, sm: gridLayout, xs: gridLayout }}
          breakpoints={{ lg: 1200, md: 996, sm: 768, xs: 480 }}
          cols={{ lg: 4, md: 3, sm: 2, xs: 1 }}
          rowHeight={GRID_ROW_HEIGHT}
          width={gridWidth}
          margin={GRID_MARGIN}
          containerPadding={[0, 0]}
          dragConfig={{ enabled: editMode, handle: ".drag-handle" }}
          resizeConfig={{ enabled: editMode }}
          onDragStop={(_layout, _oldItem, _newItem, _placeholder, _e, _element) => {
            // Cast layout properly for our handler
            handleDragResizeStop(_layout as unknown as LayoutItem[]);
          }}
          onResizeStop={(_layout, _oldItem, _newItem, _placeholder, _e, _element) => {
            handleDragResizeStop(_layout as unknown as LayoutItem[]);
          }}
        >
          {visibleWidgets.map((widget) => (
            <div
              key={widget.id}
              style={{ height: "100%" }}
            >
              {renderWidget(widget)}
            </div>
          ))}
        </Responsive>
      </div>

      {/* Create Board Dialog */}
      <Dialog open={createBoardDialogOpen} onClose={() => setCreateBoardDialogOpen(false)}>
        <DialogTitle>Create New Board</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Board Name"
            fullWidth
            value={newBoardName}
            onChange={(e) => setNewBoardName(e.target.value)}
            onKeyPress={(e) => e.key === "Enter" && handleCreateBoard()}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setCreateBoardDialogOpen(false)}>Cancel</Button>
          <Button onClick={handleCreateBoard} variant="contained" disabled={!newBoardName.trim()}>
            Create
          </Button>
        </DialogActions>
      </Dialog>

      {/* Rename Board Dialog */}
      <Dialog open={renameBoardDialogOpen} onClose={() => setRenameBoardDialogOpen(false)}>
        <DialogTitle>Rename Board</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Board Name"
            fullWidth
            value={newBoardName}
            onChange={(e) => setNewBoardName(e.target.value)}
            onKeyPress={(e) => e.key === "Enter" && handleRenameBoard()}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setRenameBoardDialogOpen(false)}>Cancel</Button>
          <Button onClick={handleRenameBoard} variant="contained" disabled={!newBoardName.trim()}>
            Rename
          </Button>
        </DialogActions>
      </Dialog>

      {/* Widget Settings Panel */}
      {selectedWidget && (
        <WidgetSettingsPanel
          open={settingsPanelOpen}
          widget={selectedWidget}
          onClose={() => {
            setSettingsPanelOpen(false);
            setSelectedWidget(null);
          }}
          onSave={handleUpdateWidgetSettings}
          mode="edit"
        />
      )}

      {/* Add Widget Panel */}
      <WidgetSettingsPanel
        open={addWidgetPanelOpen}
        widget={null}
        onClose={() => setAddWidgetPanelOpen(false)}
        onSave={() => {}}
        onAddWidget={handleAddWidget}
        existingWidgetTypes={widgets.map((w) => w.widget_type)}
        mode="add"
      />

      {/* Snackbar */}
      <Snackbar
        open={snackbar.open}
        autoHideDuration={3000}
        onClose={() => setSnackbar({ ...snackbar, open: false })}
        anchorOrigin={{ vertical: "bottom", horizontal: "right" }}
      >
        <Alert severity={snackbar.severity} onClose={() => setSnackbar({ ...snackbar, open: false })}>
          {snackbar.message}
        </Alert>
      </Snackbar>
    </Box>
  );
};

export default DashboardPage;
