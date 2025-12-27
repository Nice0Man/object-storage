import React from "react";
import {
  Card,
  CardContent,
  Box,
  Typography,
  IconButton,
  CircularProgress,
  Tooltip,
  alpha,
  useTheme,
} from "@mui/material";
import {
  Refresh,
  Settings,
  DragIndicator,
  VisibilityOff,
} from "@mui/icons-material";
import type { DashboardWidget } from "../../../api/types";
import { dashboardTheme, getThemeValue } from "../theme";

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
  onClick,
  children,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const isDark = theme.palette.mode === "dark";
  const { primary, secondary, background, gradients, shadows } = dashboardTheme;

  const formatLastRefresh = (timestamp: number | null) => {
    if (!timestamp) return null;
    const seconds = Math.floor((Date.now() - timestamp) / 1000);
    if (seconds < 60) return `${seconds}s ago`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes}m ago`;
    return `${Math.floor(minutes / 60)}h ago`;
  };

  return (
    <Card
      onClick={!editMode ? onClick : undefined}
      sx={{
        height: "100%",
        display: "flex",
        flexDirection: "column",
        background: getThemeValue(isDark, gradients.card.dark, gradients.card.light),
        borderRadius: dashboardTheme.borderRadius.md,
        border: `1px solid ${isDark ? alpha(primary.main, 0.1) : alpha("#e2e8f0", 0.8)}`,
        cursor: editMode ? "default" : onClick ? "pointer" : "default",
        transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
        position: "relative",
        overflow: "hidden",
        backdropFilter: "blur(12px)",
        boxShadow: getThemeValue(isDark, shadows.card.dark, shadows.card.light),
        "&:hover": !editMode && onClick
          ? {
              transform: "translateY(-4px)",
              boxShadow: getThemeValue(isDark, shadows.cardHover.dark, shadows.cardHover.light),
              borderColor: alpha(primary.main, isDark ? 0.3 : 0.2),
            }
          : {},
        ...(editMode && {
          border: `2px dashed ${alpha(primary.main, 0.5)}`,
          "&::after": {
            content: '""',
            position: "absolute",
            inset: 0,
            background: `repeating-linear-gradient(
              -45deg,
              transparent,
              transparent 8px,
              ${alpha(primary.main, 0.03)} 8px,
              ${alpha(primary.main, 0.03)} 16px
            )`,
            pointerEvents: "none",
          },
        }),
      }}
    >
      {/* Accent line at top */}
      <Box
        sx={{
          position: "absolute",
          top: 0,
          left: 0,
          right: 0,
          height: 2,
          background: gradients.accent,
          opacity: 0.7,
        }}
      />

      {/* Header */}
      <Box
        sx={{
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
          px: 2,
          pt: 2,
          pb: 0.5,
        }}
      >
        <Box sx={{ display: "flex", alignItems: "center", gap: 1.5, flex: 1, minWidth: 0 }}>
          {editMode && dragHandleProps && (
            <Box
              {...dragHandleProps}
              className="drag-handle"
              sx={{
                cursor: "grab",
                display: "flex",
                alignItems: "center",
                p: 0.5,
                borderRadius: 1,
                transition: "all 0.2s ease",
                "&:hover": {
                  bgcolor: alpha(theme.palette.primary.main, 0.1),
                },
                "&:active": {
                  cursor: "grabbing",
                },
              }}
            >
              <DragIndicator sx={{ color: theme.palette.text.secondary, fontSize: 18 }} />
            </Box>
          )}
          {icon && (
            <Box
              sx={{
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                width: 32,
                height: 32,
                borderRadius: dashboardTheme.borderRadius.sm,
                background: isDark
                  ? `linear-gradient(135deg, ${alpha(primary.main, 0.2)} 0%, ${alpha(secondary.main, 0.15)} 100%)`
                  : `linear-gradient(135deg, ${alpha(primary.main, 0.12)} 0%, ${alpha(secondary.main, 0.08)} 100%)`,
                color: getThemeValue(isDark, primary.light, primary.main),
                "& svg": {
                  fontSize: 18,
                },
              }}
            >
              {icon}
            </Box>
          )}
          <Typography
            variant="subtitle1"
            sx={{
              fontWeight: 600,
              fontSize: "0.875rem",
              color: theme.palette.text.primary,
              letterSpacing: "-0.01em",
              overflow: "hidden",
              textOverflow: "ellipsis",
              whiteSpace: "nowrap",
            }}
          >
            {title}
          </Typography>
        </Box>

        <Box sx={{ display: "flex", alignItems: "center", gap: 0.5 }}>
          {loading && (
            <CircularProgress
              size={14}
              thickness={5}
              sx={{
                mr: 0.5,
                color: getThemeValue(isDark, primary.light, primary.main),
              }}
            />
          )}

          {!editMode && nextRefreshIn != null && nextRefreshIn > 0 && (
            <Tooltip title={`Next refresh in ${nextRefreshIn}s`} arrow>
              <Typography
                variant="caption"
                sx={{
                  color: theme.palette.text.secondary,
                  mr: 0.5,
                  fontSize: "0.7rem",
                  opacity: 0.7,
                }}
              >
                {nextRefreshIn}s
              </Typography>
            </Tooltip>
          )}

          {!editMode && onRefresh && (
            <Tooltip title={lastRefresh ? `Last: ${formatLastRefresh(lastRefresh)}` : "Refresh"} arrow>
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onRefresh();
                }}
                disabled={loading}
                sx={{
                  width: 28,
                  height: 28,
                  transition: "all 0.2s ease",
                  "&:hover": {
                    bgcolor: alpha(theme.palette.primary.main, 0.1),
                    transform: "rotate(90deg)",
                  },
                }}
              >
                <Refresh sx={{ fontSize: 16 }} />
              </IconButton>
            </Tooltip>
          )}

          {editMode && onSettingsClick && (
            <Tooltip title="Widget Settings" arrow>
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onSettingsClick();
                }}
                sx={{
                  width: 28,
                  height: 28,
                  "&:hover": {
                    bgcolor: alpha(theme.palette.primary.main, 0.1),
                  },
                }}
              >
                <Settings sx={{ fontSize: 16 }} />
              </IconButton>
            </Tooltip>
          )}

          {editMode && onVisibilityToggle && (
            <Tooltip title="Hide Widget" arrow>
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onVisibilityToggle();
                }}
                sx={{
                  width: 28,
                  height: 28,
                  "&:hover": {
                    bgcolor: alpha(theme.palette.error.main, 0.1),
                    color: theme.palette.error.main,
                  },
                }}
              >
                <VisibilityOff sx={{ fontSize: 16 }} />
              </IconButton>
            </Tooltip>
          )}
        </Box>
      </Box>

      {/* Content */}
      <CardContent
        sx={{
          flex: 1,
          pt: 1.5,
          pb: 2,
          px: 2,
          overflow: "auto",
          "&::-webkit-scrollbar": {
            width: 4,
          },
          "&::-webkit-scrollbar-track": {
            bgcolor: "transparent",
          },
          "&::-webkit-scrollbar-thumb": {
            bgcolor: alpha(theme.palette.text.primary, 0.1),
            borderRadius: 2,
          },
        }}
      >
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
            <Box
              sx={{
                width: 40,
                height: 40,
                borderRadius: "50%",
                bgcolor: alpha(theme.palette.error.main, 0.1),
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                color: theme.palette.error.main,
              }}
            >
              !
            </Box>
            <Typography variant="body2" color="error" sx={{ textAlign: "center" }}>
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
