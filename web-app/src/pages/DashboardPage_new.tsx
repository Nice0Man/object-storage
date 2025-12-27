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
  useTheme,
  alpha,
  Menu,
  MenuItem,
  Snackbar,
  Alert,
  Chip,
  Fade,
} from "@mui/material";
import {
  Add,
  Save,
  Close,
  MoreVert,
  Visibility,
  Dashboard as DashboardIcon,
  Tune,
} from "@mui/icons-material";
import { ResponsiveGridLayout, useContainerWidth } from "react-grid-layout";
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
import { dashboardTheme, getThemeValue } from "../components/Dashboard/theme";
import type { DashboardWidget, WidgetType } from "../api/types";

import "react-grid-layout/css/styles.css";
import "react-resizable/css/styles.css";

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
  static?: boolean;
}

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

const DashboardPageNew: React.FC = () => {
  const dispatch = useAppDispatch();
  const theme = useTheme();
  const { t } = useTranslation();

  // Redux state
  const boards = useAppSelector(selectBoards);
  const activeBoard = useAppSelector(selectActiveBoard);
  const activeBoardData = useAppSelector(selectActiveBoardData);
  const widgets = useAppSelector(selectActiveWidgets);
  const editMode = useAppSelector(selectEditMode);
  const loading = useAppSelector(selectDashboardLoading);
  const savingWidgets = useAppSelector(selectSavingWidgets);

  // Grid width hook for responsive layout
  const { width: containerWidth, containerRef, mounted: gridMounted } = useContainerWidth();

  // Local state
  const [createBoardDialogOpen, setCreateBoardDialogOpen] = useState(false);
  const [newBoardName, setNewBoardName] = useState("");
  const [renameBoardDialogOpen, setRenameBoardDialogOpen] = useState(false);
  const [boardMenuAnchor, setBoardMenuAnchor] = useState<null | HTMLElement>(null);
  const [selectedWidget, setSelectedWidget] = useState<DashboardWidget | null>(null);
  const [settingsPanelOpen, setSettingsPanelOpen] = useState(false);
  const [snackbar, setSnackbar] = useState<{ open: boolean; message: string; severity: "success" | "error" }>({
    open: false,
    message: "",
    severity: "success",
  });

  // Initialize dashboard on mount
  useEffect(() => {
    dispatch(initializeDashboard());
  }, [dispatch]);

  // Generate layout from widgets
  const layout = useMemo(() => {
    return widgets
      .filter((w) => w.visible)
      .map((widget) => ({
        i: widget.id,
        x: widget.position.x,
        y: widget.position.y,
        w: widget.size.w,
        h: widget.size.h,
        minW: 2,
        minH: 1,
        static: !editMode,
      }));
  }, [widgets, editMode]);

  // Handle layout change
  const handleLayoutChange = useCallback(
    (newLayout: LayoutItem[]) => {
      if (!editMode || !activeBoard) return;

      const updatedWidgets = widgets.map((widget) => {
        const layoutItem = newLayout.find((l) => l.i === widget.id);
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
    [editMode, activeBoard, widgets, dispatch]
  );


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
        />
      );
    },
    [editMode, handleOpenWidgetSettings, handleToggleWidgetVisibility]
  );

  if (loading && boards.length === 0) {
    return <Loader message={t("dashboard.loading") || "Loading dashboard..."} />;
  }

  const isDark = theme.palette.mode === "dark";
  const { primary, secondary, background, gradients, shadows, opacity } = dashboardTheme;

  return (
    <Box
      sx={{
        minHeight: "100vh",
        background: getThemeValue(isDark, gradients.background.dark, gradients.background.light),
        position: "relative",
        overflow: "hidden",
      }}
    >
      {/* Subtle background pattern */}
      <Box
        sx={{
          position: "absolute",
          inset: 0,
          backgroundImage: isDark
            ? `radial-gradient(circle at 25% 25%, ${alpha(primary.main, 0.03)} 0%, transparent 50%),
               radial-gradient(circle at 75% 75%, ${alpha(secondary.main, 0.03)} 0%, transparent 50%)`
            : `radial-gradient(circle at 25% 25%, ${alpha(primary.main, 0.04)} 0%, transparent 50%),
               radial-gradient(circle at 75% 75%, ${alpha(secondary.main, 0.04)} 0%, transparent 50%)`,
          pointerEvents: "none",
        }}
      />

      <Box sx={{ position: "relative", zIndex: 1, p: { xs: 2, md: 3 } }}>
        {/* Compact Toolbar */}
        <Fade in timeout={400}>
          <Box
            sx={{
              display: "flex",
              alignItems: "center",
              justifyContent: "space-between",
              mb: 2.5,
              gap: 2,
            }}
          >
            {/* Board Chips Navigation */}
            <Box
              sx={{
                display: "flex",
                alignItems: "center",
                gap: 1,
                flexWrap: "wrap",
                flex: 1,
              }}
            >
              {boards.map((board) => (
                <Chip
                  key={board.id}
                  label={board.name}
                  icon={<DashboardIcon sx={{ fontSize: "1rem !important" }} />}
                  onClick={() => {
                    dispatch(setActiveBoard(board.id));
                    dispatch(fetchWidgets(board.id));
                  }}
                  sx={{
                    fontWeight: 600,
                    fontSize: "0.85rem",
                    height: 36,
                    px: 0.5,
                    borderRadius: dashboardTheme.borderRadius.chip,
                    transition: "all 0.2s ease",
                    bgcolor: activeBoard === board.id
                      ? alpha(primary.main, isDark ? opacity.active : opacity.hover)
                      : alpha(isDark ? background.dark.card : background.light.card, isDark ? 0.6 : 0.8),
                    color: activeBoard === board.id
                      ? getThemeValue(isDark, primary.light, primary.dark)
                      : theme.palette.text.primary,
                    border: activeBoard === board.id
                      ? `1px solid ${alpha(primary.main, isDark ? 0.4 : 0.3)}`
                      : `1px solid ${alpha(theme.palette.divider, opacity.divider)}`,
                    backdropFilter: "blur(8px)",
                    "&:hover": {
                      bgcolor: activeBoard === board.id
                        ? alpha(primary.main, isDark ? 0.3 : 0.2)
                        : alpha(isDark ? background.dark.card : background.light.card, isDark ? 0.8 : 1),
                      transform: "translateY(-1px)",
                    },
                    "& .MuiChip-icon": {
                      color: activeBoard === board.id
                        ? getThemeValue(isDark, primary.light, primary.dark)
                        : theme.palette.text.secondary,
                    },
                  }}
                />
              ))}

              {/* Add Board Button */}
              <Tooltip title="New Board" arrow>
                <IconButton
                  size="small"
                  onClick={() => setCreateBoardDialogOpen(true)}
                  sx={{
                    width: 36,
                    height: 36,
                    bgcolor: alpha(getThemeValue(isDark, background.dark.card, background.light.card), isDark ? 0.6 : 0.8),
                    border: `1px dashed ${alpha(theme.palette.divider, 0.3)}`,
                    backdropFilter: "blur(8px)",
                    "&:hover": {
                      bgcolor: alpha(getThemeValue(isDark, background.dark.card, background.light.card), isDark ? 0.9 : 1),
                      borderColor: primary.main,
                    },
                  }}
                >
                  <Add sx={{ fontSize: 18 }} />
                </IconButton>
              </Tooltip>

              {/* Board Menu */}
              {activeBoard && (
                <>
                  <IconButton
                    size="small"
                    onClick={(e) => setBoardMenuAnchor(e.currentTarget)}
                    sx={{
                      width: 36,
                      height: 36,
                      bgcolor: alpha(getThemeValue(isDark, background.dark.card, background.light.card), isDark ? 0.6 : 0.8),
                      border: `1px solid ${alpha(theme.palette.divider, opacity.divider)}`,
                      backdropFilter: "blur(8px)",
                    }}
                  >
                    <MoreVert sx={{ fontSize: 18 }} />
                  </IconButton>
                  <Menu
                    anchorEl={boardMenuAnchor}
                    open={Boolean(boardMenuAnchor)}
                    onClose={() => setBoardMenuAnchor(null)}
                    PaperProps={{
                      sx: {
                        bgcolor: getThemeValue(isDark, background.dark.card, background.light.card),
                        backdropFilter: "blur(20px)",
                        borderRadius: dashboardTheme.borderRadius.sm,
                        border: `1px solid ${alpha(theme.palette.divider, opacity.border)}`,
                        boxShadow: getThemeValue(isDark, shadows.menu.dark, shadows.menu.light),
                      },
                    }}
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
                      sx={{ color: theme.palette.error.main }}
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
                    startIcon={<Close />}
                    onClick={handleCancelEdit}
                    sx={{
                      borderRadius: dashboardTheme.borderRadius.sm,
                      textTransform: "none",
                      fontWeight: 600,
                      borderColor: alpha(theme.palette.divider, 0.3),
                    }}
                  >
                    Cancel
                  </Button>
                  <Button
                    variant="contained"
                    size="small"
                    startIcon={<Save />}
                    onClick={handleSaveWidgets}
                    disabled={savingWidgets}
                    sx={{
                      borderRadius: dashboardTheme.borderRadius.sm,
                      textTransform: "none",
                      fontWeight: 600,
                      background: gradients.primary,
                      boxShadow: shadows.button,
                      "&:hover": {
                        background: gradients.primaryHover,
                      },
                    }}
                  >
                    {savingWidgets ? "Saving..." : "Save"}
                  </Button>
                </>
              ) : (
                <Tooltip title="Customize Layout" arrow>
                  <IconButton
                    onClick={handleToggleEditMode}
                    sx={{
                      width: 36,
                      height: 36,
                      bgcolor: alpha(getThemeValue(isDark, background.dark.card, background.light.card), isDark ? 0.6 : 0.8),
                      border: `1px solid ${alpha(theme.palette.divider, opacity.divider)}`,
                      backdropFilter: "blur(8px)",
                      "&:hover": {
                        bgcolor: alpha(primary.main, 0.1),
                        borderColor: primary.main,
                      },
                    }}
                  >
                    <Tune sx={{ fontSize: 18 }} />
                  </IconButton>
                </Tooltip>
              )}
            </Box>
          </Box>
        </Fade>

        {/* Hidden Widgets Panel (Edit Mode) */}
        {editMode && widgets.filter((w) => !w.visible).length > 0 && (
          <Fade in timeout={300}>
            <Box
              sx={{
                mb: 2.5,
                p: 2,
                display: "flex",
                alignItems: "center",
                gap: 2,
                bgcolor: alpha(dashboardTheme.warning.main, opacity.subtle),
                borderRadius: dashboardTheme.borderRadius.md,
                border: `1px solid ${alpha(dashboardTheme.warning.main, 0.2)}`,
              }}
            >
              <Box
                sx={{
                  fontSize: "0.75rem",
                  fontWeight: 600,
                  color: dashboardTheme.warning.main,
                  textTransform: "uppercase",
                  letterSpacing: "0.05em",
                  whiteSpace: "nowrap",
                }}
              >
                Hidden
              </Box>
              <Box sx={{ display: "flex", gap: 1, flexWrap: "wrap" }}>
                {widgets.filter((w) => !w.visible).map((widget) => (
                  <Chip
                    key={widget.id}
                    label={widget.widget_type.replace("_", " ")}
                    size="small"
                    icon={<Visibility sx={{ fontSize: "0.9rem !important" }} />}
                    onClick={() => handleToggleWidgetVisibility(widget.id)}
                    sx={{
                      height: 28,
                      borderRadius: dashboardTheme.borderRadius.sm,
                      bgcolor: alpha(getThemeValue(isDark, background.dark.card, background.light.card), isDark ? 0.8 : 0.9),
                      border: `1px solid ${alpha(theme.palette.divider, 0.2)}`,
                      cursor: "pointer",
                      transition: "all 0.2s ease",
                      "&:hover": {
                        bgcolor: alpha(primary.main, 0.1),
                        borderColor: primary.main,
                      },
                    }}
                  />
                ))}
              </Box>
            </Box>
          </Fade>
        )}

        {/* Widget Grid */}
        <div ref={containerRef as React.RefObject<HTMLDivElement>} style={{ width: "100%" }}>
          {gridMounted && (
            <ResponsiveGridLayout
              className="layout"
              layouts={{ lg: layout, md: layout, sm: layout }}
              breakpoints={{ lg: 1200, md: 996, sm: 768 }}
              cols={{ lg: 12, md: 12, sm: 6 }}
              rowHeight={120}
              width={containerWidth}
              onLayoutChange={(newLayout) => handleLayoutChange(newLayout as unknown as LayoutItem[])}
              margin={[16, 16]}
            >
              {widgets.filter((w) => w.visible).map((widget) => (
                <Box
                  key={widget.id}
                  sx={{ height: "100%" }}
                  className={editMode ? "drag-handle" : ""}
                >
                  {renderWidget(widget)}
                </Box>
              ))}
            </ResponsiveGridLayout>
          )}
        </div>
      </Box>

      {/* Create Board Dialog */}
      <Dialog
        open={createBoardDialogOpen}
        onClose={() => setCreateBoardDialogOpen(false)}
        PaperProps={{
          sx: {
            bgcolor: getThemeValue(isDark, background.dark.card, background.light.card),
            borderRadius: dashboardTheme.borderRadius.md,
            minWidth: 360,
          },
        }}
      >
        <DialogTitle sx={{ fontWeight: 600 }}>Create New Board</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Board Name"
            fullWidth
            value={newBoardName}
            onChange={(e) => setNewBoardName(e.target.value)}
            onKeyPress={(e) => e.key === "Enter" && handleCreateBoard()}
            sx={{
              "& .MuiOutlinedInput-root": {
                borderRadius: dashboardTheme.borderRadius.sm,
              },
            }}
          />
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button
            onClick={() => setCreateBoardDialogOpen(false)}
            sx={{ borderRadius: dashboardTheme.borderRadius.sm, textTransform: "none" }}
          >
            Cancel
          </Button>
          <Button
            onClick={handleCreateBoard}
            variant="contained"
            disabled={!newBoardName.trim()}
            sx={{
              borderRadius: dashboardTheme.borderRadius.sm,
              textTransform: "none",
              fontWeight: 600,
              background: gradients.primary,
            }}
          >
            Create
          </Button>
        </DialogActions>
      </Dialog>

      {/* Rename Board Dialog */}
      <Dialog
        open={renameBoardDialogOpen}
        onClose={() => setRenameBoardDialogOpen(false)}
        PaperProps={{
          sx: {
            bgcolor: getThemeValue(isDark, background.dark.card, background.light.card),
            borderRadius: dashboardTheme.borderRadius.md,
            minWidth: 360,
          },
        }}
      >
        <DialogTitle sx={{ fontWeight: 600 }}>Rename Board</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Board Name"
            fullWidth
            value={newBoardName}
            onChange={(e) => setNewBoardName(e.target.value)}
            onKeyPress={(e) => e.key === "Enter" && handleRenameBoard()}
            sx={{
              "& .MuiOutlinedInput-root": {
                borderRadius: dashboardTheme.borderRadius.sm,
              },
            }}
          />
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button
            onClick={() => setRenameBoardDialogOpen(false)}
            sx={{ borderRadius: dashboardTheme.borderRadius.sm, textTransform: "none" }}
          >
            Cancel
          </Button>
          <Button
            onClick={handleRenameBoard}
            variant="contained"
            disabled={!newBoardName.trim()}
            sx={{
              borderRadius: dashboardTheme.borderRadius.sm,
              textTransform: "none",
              fontWeight: 600,
              background: gradients.primary,
            }}
          >
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
        />
      )}

      {/* Snackbar */}
      <Snackbar
        open={snackbar.open}
        autoHideDuration={3000}
        onClose={() => setSnackbar({ ...snackbar, open: false })}
        anchorOrigin={{ vertical: "bottom", horizontal: "right" }}
      >
        <Alert
          severity={snackbar.severity}
          onClose={() => setSnackbar({ ...snackbar, open: false })}
          sx={{ borderRadius: dashboardTheme.borderRadius.sm }}
        >
          {snackbar.message}
        </Alert>
      </Snackbar>
    </Box>
  );
};

export default DashboardPageNew;
