import React, { useCallback } from "react";
import { Box, Typography, alpha, useTheme } from "@mui/material";
import { Folder, ArrowForward } from "@mui/icons-material";
import { useNavigate } from "react-router-dom";
import WidgetWrapper from "./WidgetWrapper";
import { widgetScrollSx } from "../../../theme/widgetStyles";
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
      <Box
        sx={{
          flex: 1,
          minHeight: 0,
          display: "flex",
          flexDirection: "column",
          gap: 1,
        }}
      >
      <Box sx={{ display: "flex", alignItems: "center", justifyContent: "space-between", flexShrink: 0 }}>
        <Typography
          sx={{
            fontWeight: 700,
            fontSize: "clamp(1.5rem, 4vmin, 3rem)",
            lineHeight: 1.15,
            color: "text.primary",
          }}
        >
          {systemData?.buckets ?? 0}
        </Typography>
        <ArrowForward sx={{ color: theme.palette.text.secondary, flexShrink: 0 }} />
      </Box>

      <Box sx={{ flexShrink: 0 }}>
        <Box sx={{ display: "flex", justifyContent: "space-between", mb: 0.5, gap: 1 }}>
          <Typography variant="body2" color="text.secondary" noWrap>Objects</Typography>
          <Typography variant="body2" sx={{ fontWeight: 600 }} noWrap>
            {(systemData?.objects ?? 0).toLocaleString()}
          </Typography>
        </Box>
        <Box sx={{ display: "flex", justifyContent: "space-between", gap: 1 }}>
          <Typography variant="body2" color="text.secondary" noWrap>Size</Typography>
          <Typography variant="body2" sx={{ fontWeight: 600 }} noWrap>
            {formatBytesToEiB(systemData?.storage_used ?? 0)} EiB
          </Typography>
        </Box>
      </Box>

      <Typography variant="caption" sx={{ fontWeight: 600, color: theme.palette.text.secondary, flexShrink: 0 }}>
        Recent Activity
      </Typography>
      <Box sx={{ flex: "1 1 0%", minHeight: 0, mt: 0.5, ...widgetScrollSx(theme) }}>
        {activityData?.recent_buckets?.slice(0, 8).map((bucket) => (
          <Box
            key={bucket.name}
            onClick={(e) => {
              e.stopPropagation();
              if (!editMode) {
                navigate(`/objects?bucket=${encodeURIComponent(bucket.name)}`);
              }
            }}
            sx={{
              py: 0.75,
              px: 1,
              mb: 0.25,
              display: "flex",
              alignItems: "center",
              justifyContent: "space-between",
              gap: 1,
              cursor: editMode ? "default" : "pointer",
              borderRadius: 1,
              bgcolor: alpha(theme.palette.action.hover, theme.palette.mode === "dark" ? 0.15 : 0.04),
              "&:hover": !editMode
                ? {
                    bgcolor: alpha(theme.palette.primary.main, theme.palette.mode === "dark" ? 0.22 : 0.1),
                  }
                : {},
            }}
          >
            <Box sx={{ display: "flex", alignItems: "center", gap: 1, minWidth: 0, flex: 1 }}>
              <Folder sx={{ fontSize: 16, color: "primary.main", flexShrink: 0 }} />
              <Typography variant="caption" color="text.primary" noWrap title={bucket.name}>
                {bucket.name}
              </Typography>
            </Box>
            <Typography variant="caption" color="text.secondary" sx={{ flexShrink: 0, fontWeight: 600 }}>
              {bucket.objects.toLocaleString()}
            </Typography>
          </Box>
        ))}
      </Box>
      </Box>
    </WidgetWrapper>
  );
};

export default BucketsWidget;
