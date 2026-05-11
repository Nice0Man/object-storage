import React, { useCallback } from "react";
import { Box, Typography, Chip, LinearProgress, Grid, alpha, useTheme } from "@mui/material";
import { Storage, ArrowForward } from "@mui/icons-material";
import WidgetWrapper from "./WidgetWrapper";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, PoolInfo } from "../../../api/types";

interface PoolsWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  onClick?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const formatBytesToEiB = (bytes: number): string => {
  const eib = bytes / (1024 ** 6);
  return eib.toFixed(2);
};

const PoolsWidget: React.FC<PoolsWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const [pools, setPools] = React.useState<PoolInfo[]>([]);

  const fetchData = useCallback(async () => {
    const data = await apiClient.getPoolStats();
    setPools(Array.isArray(data) ? data : []);
  }, []);

  const { loading, error, lastRefresh, refresh, nextRefreshIn } = useWidgetRefresh({
    interval: widget.refresh_interval,
    enabled: widget.visible,
    onRefresh: fetchData,
  });

  if (pools.length === 0) {
    return (
      <WidgetWrapper
        widget={widget}
        title="Storage Pools"
        icon={<Storage />}
        loading={loading}
        error={error}
        lastRefresh={lastRefresh}
        nextRefreshIn={nextRefreshIn}
        editMode={editMode}
        onRefresh={refresh}
        onSettingsClick={onSettingsClick}
        onVisibilityToggle={onVisibilityToggle}
        dragHandleProps={dragHandleProps}
      >
        <Box sx={{ display: "flex", alignItems: "center", justifyContent: "center", height: "100%" }}>
          <Typography variant="body2" color="text.secondary">
            No storage pools configured
          </Typography>
        </Box>
      </WidgetWrapper>
    );
  }

  // Show first pool in widget, click for details
  const pool = pools[0];

  return (
    <WidgetWrapper
      widget={widget}
      title={pool.name}
      icon={<Storage />}
      loading={loading}
      error={error}
      lastRefresh={lastRefresh}
      nextRefreshIn={nextRefreshIn}
      editMode={editMode}
      onRefresh={refresh}
      onSettingsClick={onSettingsClick}
      onVisibilityToggle={onVisibilityToggle}
      onDelete={onDelete}
      onClick={onClick}
      dragHandleProps={dragHandleProps}
    >
      <Box sx={{ flex: 1, minHeight: 0, overflow: "auto" }}>
      <Grid container spacing={2}>
        <Grid item xs={6}>
          <Typography variant="body2" sx={{ fontWeight: 600, mb: 1 }}>
            Capacity
          </Typography>
          <Box sx={{ mb: 2 }}>
            <Typography sx={{ fontWeight: 700, fontSize: "clamp(1.25rem, 2.5vmin, 2.125rem)" }}>
              {formatBytesToEiB(pool.available)}
            </Typography>
            <Typography variant="body2" color="text.secondary">
              EiB Available
            </Typography>
          </Box>
          <Box sx={{ display: "flex", justifyContent: "space-between", mb: 0.5 }}>
            <Typography variant="caption" color="text.secondary">Object data</Typography>
            <Typography variant="caption" sx={{ fontWeight: 600 }}>
              {formatBytesToEiB(pool.used)} EiB
            </Typography>
          </Box>
        </Grid>

        <Grid item xs={6}>
          <Typography variant="body2" sx={{ fontWeight: 600, mb: 1 }}>
            Drives
          </Typography>
          <Typography sx={{ fontWeight: 700, mb: 0.5, fontSize: "clamp(1.25rem, 2.5vmin, 2.125rem)" }}>
            {pool.drives_count}
          </Typography>
          <Box sx={{ display: "flex", justifyContent: "space-between", mb: 1 }}>
            <Box sx={{ display: "flex", alignItems: "center", gap: 0.5 }}>
              <Chip
                label="Online"
                size="small"
                sx={{
                  bgcolor: alpha("#10B981", 0.1),
                  color: "#10B981",
                  fontWeight: 600,
                  height: 20,
                  fontSize: "0.65rem",
                }}
              />
              <Typography variant="caption">{pool.online_drives}</Typography>
            </Box>
            <Box sx={{ display: "flex", alignItems: "center", gap: 0.5 }}>
              <Chip
                label="Off"
                size="small"
                sx={{
                  bgcolor: alpha("#EF4444", 0.1),
                  color: "#EF4444",
                  fontWeight: 600,
                  height: 20,
                  fontSize: "0.65rem",
                }}
              />
              <Typography variant="caption">{pool.offline_drives}</Typography>
            </Box>
          </Box>
          <LinearProgress
            variant="determinate"
            value={(pool.online_drives / pool.drives_count) * 100}
            sx={{
              height: 6,
              borderRadius: 1,
              bgcolor: alpha("#EF4444", 0.2),
              "& .MuiLinearProgress-bar": {
                bgcolor: "#10B981",
                borderRadius: 1,
              },
            }}
          />
        </Grid>
      </Grid>

      {pools.length > 1 && (
        <Box sx={{ mt: 2, display: "flex", alignItems: "center", justifyContent: "center" }}>
          <Typography variant="caption" color="text.secondary">
            +{pools.length - 1} more pools
          </Typography>
          <ArrowForward sx={{ fontSize: 14, color: theme.palette.text.secondary, ml: 0.5 }} />
        </Box>
      )}
      </Box>
    </WidgetWrapper>
  );
};

export default PoolsWidget;
