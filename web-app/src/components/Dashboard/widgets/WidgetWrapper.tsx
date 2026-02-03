import React from "react";
import {
  Card,
  CardContent,
  Box,
  Typography,
  IconButton,
  CircularProgress,
  Tooltip,
  useTheme,
  alpha,
} from "@mui/material";
import {
  Refresh,
  Settings,
  DragIndicator,
  VisibilityOff,
  Delete,
} from "@mui/icons-material";
import type { DashboardWidget } from "../../../api/types";

interface WidgetWrapperProps {
  widget: DashboardWidget;
  title: string;
  icon?: React.ReactNode;
  loading?: boolean;
  error?: string | null;
  lastRefresh?: number | null;
  nextRefreshIn?: number | null;
  editMode?: boolean;
  onRefresh?: () => void;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  onClick?: () => void;
  children: React.ReactNode;
  dragHandleProps?: Record<string, unknown>;
}

const WidgetWrapper: React.FC<WidgetWrapperProps> = ({
  widget: _widget,
  title,
  icon,
  loading = false,
  error = null,
  lastRefresh,
  nextRefreshIn,
  editMode = false,
  onRefresh,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  children,
  dragHandleProps: _dragHandleProps,
}) => {
  const theme = useTheme();

  // Memoize last refresh formatting to avoid re-renders
  const lastRefreshFormatted = React.useMemo(() => {
    if (!lastRefresh) return null;
    const seconds = Math.floor((Date.now() - lastRefresh) / 1000);
    if (seconds < 60) return `${seconds}s ago`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes}m ago`;
    return `${Math.floor(minutes / 60)}h ago`;
  }, [lastRefresh]);

  return (
    <Card
      onClick={!editMode ? onClick : undefined}
      sx={{
        height: "100%",
        display: "flex",
        flexDirection: "column",
        cursor: editMode ? "default" : onClick ? "pointer" : "default",
        transition: "box-shadow 0.2s ease, border-color 0.2s ease",
        "&:hover": !editMode && onClick
          ? { boxShadow: theme.shadows[6] }
          : {},
        ...(editMode && {
          border: `2px dashed ${alpha(theme.palette.primary.main, 0.5)}`,
          "&:hover": {
            borderColor: theme.palette.primary.main,
          },
        }),
      }}
    >
      {/* Header - Drag Handle Area */}
      <Box
        className={editMode ? "drag-handle" : undefined}
        sx={{
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
          px: 2,
          pt: 2,
          pb: 0.5,
          ...(editMode && {
            cursor: "grab",
            "&:active": { cursor: "grabbing" },
          }),
        }}
      >
        <Box sx={{ display: "flex", alignItems: "center", gap: 1.5, flex: 1, minWidth: 0 }}>
          {editMode && (
            <DragIndicator color="action" fontSize="small" sx={{ opacity: 0.6 }} />
          )}
          {icon && (
            <Box
              sx={{
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                width: 32,
                height: 32,
                borderRadius: 1,
                bgcolor: "primary.light",
                color: "primary.contrastText",
                "& svg": { fontSize: 18 },
              }}
            >
              {icon}
            </Box>
          )}
          <Typography
            variant="subtitle1"
            fontWeight={600}
            noWrap
          >
            {title}
          </Typography>
        </Box>

        <Box sx={{ display: "flex", alignItems: "center", gap: 0.5 }}>
          {loading && <CircularProgress size={14} sx={{ mr: 0.5 }} />}

          {!editMode && nextRefreshIn != null && nextRefreshIn > 0 && (
            <Tooltip title={`Next refresh in ${nextRefreshIn}s`}>
              <Typography variant="caption" color="text.secondary" sx={{ mr: 0.5 }}>
                {nextRefreshIn}s
              </Typography>
            </Tooltip>
          )}

          {!editMode && onRefresh && (
            <Tooltip title={lastRefreshFormatted ? `Last: ${lastRefreshFormatted}` : "Refresh"}>
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onRefresh();
                }}
                disabled={loading}
              >
                <Refresh fontSize="small" />
              </IconButton>
            </Tooltip>
          )}

          {editMode && onSettingsClick && (
            <Tooltip title="Widget Settings">
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onSettingsClick();
                }}
              >
                <Settings fontSize="small" />
              </IconButton>
            </Tooltip>
          )}

          {editMode && onVisibilityToggle && (
            <Tooltip title="Hide Widget">
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onVisibilityToggle();
                }}
              >
                <VisibilityOff fontSize="small" />
              </IconButton>
            </Tooltip>
          )}

          {editMode && onDelete && (
            <Tooltip title="Delete Widget">
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onDelete();
                }}
                color="error"
              >
                <Delete fontSize="small" />
              </IconButton>
            </Tooltip>
          )}
        </Box>
      </Box>

      {/* Content */}
      <CardContent sx={{ flex: 1, pt: 1.5, pb: 2, px: 2, overflow: "auto" }}>
        {error ? (
          <Box
            sx={{
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              height: "100%",
              flexDirection: "column",
              gap: 1,
            }}
          >
            <Typography variant="body2" color="error" textAlign="center">
              {error}
            </Typography>
          </Box>
        ) : (
          children
        )}
      </CardContent>
    </Card>
  );
};

export default WidgetWrapper;
