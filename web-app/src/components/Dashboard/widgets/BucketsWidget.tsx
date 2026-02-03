import React, { useCallback } from "react";
import { Box, Typography, alpha, useTheme } from "@mui/material";
import { Folder, ArrowForward } from "@mui/icons-material";
import { useNavigate } from "react-router-dom";
import WidgetWrapper from "./WidgetWrapper";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, SystemStats, ActivityStats } from "../../../api/types";

interface BucketsWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const formatBytesToEiB = (bytes: number): string => {
  const eib = bytes / (1024 ** 6);
  return eib.toFixed(2);
};

const BucketsWidget: React.FC<BucketsWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const navigate = useNavigate();
  const [systemData, setSystemData] = React.useState<SystemStats | null>(null);
  const [activityData, setActivityData] = React.useState<ActivityStats | null>(null);

  const fetchData = useCallback(async () => {
    const [system, activity] = await Promise.all([
      apiClient.getSystemStats(),
      apiClient.getActivityStats(),
    ]);
    setSystemData(system);
    setActivityData(activity);
  }, []);

  const { loading, error, lastRefresh, refresh, nextRefreshIn } = useWidgetRefresh({
    interval: widget.refresh_interval,
    enabled: widget.visible,
    onRefresh: fetchData,
  });

  const handleClick = () => {
    if (!editMode) {
      navigate("/buckets");
    }
  };

  return (
    <WidgetWrapper
      widget={widget}
      title="Buckets"
      icon={<Folder />}
      loading={loading}
      error={error}
      lastRefresh={lastRefresh}
      nextRefreshIn={nextRefreshIn}
      editMode={editMode}
      onRefresh={refresh}
      onSettingsClick={onSettingsClick}
      onVisibilityToggle={onVisibilityToggle}
      onDelete={onDelete}
      onClick={handleClick}
      dragHandleProps={dragHandleProps}
    >
      <Box sx={{ display: "flex", alignItems: "center", justifyContent: "space-between", mb: 2 }}>
        <Typography variant="h3" sx={{ fontWeight: 700 }}>
          {systemData?.buckets ?? 0}
        </Typography>
        <ArrowForward sx={{ color: theme.palette.text.secondary }} />
      </Box>

      <Box sx={{ mb: 2 }}>
        <Box sx={{ display: "flex", justifyContent: "space-between", mb: 0.5 }}>
          <Typography variant="body2" color="text.secondary">Objects</Typography>
          <Typography variant="body2" sx={{ fontWeight: 600 }}>
            {(systemData?.objects ?? 0).toLocaleString()}
          </Typography>
        </Box>
        <Box sx={{ display: "flex", justifyContent: "space-between" }}>
          <Typography variant="body2" color="text.secondary">Size</Typography>
          <Typography variant="body2" sx={{ fontWeight: 600 }}>
            {formatBytesToEiB(systemData?.storage_used ?? 0)} EiB
          </Typography>
        </Box>
      </Box>

      {/* Recent Activity */}
      <Typography variant="caption" sx={{ fontWeight: 600, color: theme.palette.text.secondary }}>
        Recent Activity
      </Typography>
      <Box sx={{ mt: 1, maxHeight: 120, overflowY: "auto" }}>
        {activityData?.recent_buckets?.slice(0, 4).map((bucket) => (
          <Box
            key={bucket.name}
            onClick={(e) => {
              e.stopPropagation();
              if (!editMode) {
                navigate(`/objects?bucket=${encodeURIComponent(bucket.name)}`);
              }
            }}
            sx={{
              py: 0.5,
              px: 1,
              display: "flex",
              alignItems: "center",
              justifyContent: "space-between",
              cursor: editMode ? "default" : "pointer",
              borderRadius: 1,
              "&:hover": !editMode ? {
                bgcolor: alpha(theme.palette.primary.main, 0.1),
              } : {},
            }}
          >
            <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
              <Folder sx={{ fontSize: 16, color: theme.palette.primary.main }} />
              <Typography variant="caption">{bucket.name}</Typography>
            </Box>
            <Typography variant="caption" color="text.secondary">
              {bucket.objects.toLocaleString()}
            </Typography>
          </Box>
        ))}
      </Box>
    </WidgetWrapper>
  );
};

export default BucketsWidget;
