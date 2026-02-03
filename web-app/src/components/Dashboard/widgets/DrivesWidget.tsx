import React, { useCallback } from "react";
import { Box, Typography, Chip, LinearProgress, alpha, useTheme } from "@mui/material";
import { Storage, ArrowForward } from "@mui/icons-material";
import WidgetWrapper from "./WidgetWrapper";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, DriveStats } from "../../../api/types";

interface DrivesWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  onClick?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const DrivesWidget: React.FC<DrivesWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const [data, setData] = React.useState<DriveStats | null>(null);

  const fetchData = useCallback(async () => {
    const stats = await apiClient.getDriveStats();
    setData(stats);
  }, []);

  const { loading, error, lastRefresh, refresh, nextRefreshIn } = useWidgetRefresh({
    interval: widget.refresh_interval,
    enabled: widget.visible,
    onRefresh: fetchData,
  });

  return (
    <WidgetWrapper
      widget={widget}
      title="Drives"
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
      <Box sx={{ display: "flex", alignItems: "center", justifyContent: "space-between", mb: 3 }}>
        <Typography variant="h4" sx={{ fontWeight: 700 }}>
          {data?.total_count ?? 0}
        </Typography>
        <ArrowForward sx={{ color: theme.palette.text.secondary }} />
      </Box>

      <Box sx={{ mb: 2 }}>
        <Box sx={{ display: "flex", justifyContent: "space-between", mb: 1 }}>
          <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
            <Chip
              label="Online"
              size="small"
              sx={{
                bgcolor: alpha("#10B981", 0.1),
                color: "#10B981",
                fontWeight: 600,
              }}
            />
            <Typography variant="body2">{data?.online_count ?? 0}</Typography>
          </Box>
          <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
            <Chip
              label="Offline"
              size="small"
              sx={{
                bgcolor: alpha("#EF4444", 0.1),
                color: "#EF4444",
                fontWeight: 600,
              }}
            />
            <Typography variant="body2">{data?.offline_count ?? 0}</Typography>
          </Box>
        </Box>

        <LinearProgress
          variant="determinate"
          value={
            data?.total_count
              ? ((data?.online_count ?? 0) / data.total_count) * 100
              : 0
          }
          sx={{
            height: 8,
            borderRadius: 1,
            bgcolor: alpha("#EF4444", 0.2),
            "& .MuiLinearProgress-bar": {
              bgcolor: "#10B981",
              borderRadius: 1,
            },
          }}
        />
      </Box>
    </WidgetWrapper>
  );
};

export default DrivesWidget;
