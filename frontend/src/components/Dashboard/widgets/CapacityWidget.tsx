import React, { useCallback } from "react";
import { Box, Typography, useTheme, alpha } from "@mui/material";
import { Storage } from "@mui/icons-material";
import WidgetWrapper from "./WidgetWrapper";
import CapacityPieChart from "../../Charts/CapacityPieChart";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, SystemStats } from "../../../api/types";

interface CapacityWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  onClick?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const formatBytes = (bytes: number, decimals: number = 2): string => {
  if (bytes === 0) return "0 Bytes";
  const k = 1024;
  const dm = decimals < 0 ? 0 : decimals;
  const sizes = ["Bytes", "KB", "MB", "GB", "TB", "PB", "EB"];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + " " + sizes[i];
};

const formatBytesToEiB = (bytes: number): string => {
  const eib = bytes / (1024 ** 6);
  return eib.toFixed(2);
};

const CapacityWidget: React.FC<CapacityWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const [data, setData] = React.useState<SystemStats | null>(null);

  const fetchData = useCallback(async () => {
    const stats = await apiClient.getSystemStats();
    setData(stats);
  }, []);

  const { loading, error, lastRefresh, refresh, nextRefreshIn } = useWidgetRefresh({
    interval: widget.refresh_interval,
    enabled: widget.visible,
    onRefresh: fetchData,
  });

  const totalStorage = data?.storage_total ?? 0;
  const usedStorage = data?.storage_used ?? 0;
  const availableStorage = data?.storage_available ?? 0;

  return (
    <WidgetWrapper
      widget={widget}
      title="Capacity"
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
      <Box
        sx={{
          flex: 1,
          minHeight: 0,
          display: "flex",
          flexDirection: "column",
          gap: 1,
        }}
      >
        <Box sx={{ flexShrink: 0, display: "flex", alignItems: "center", justifyContent: "space-between" }}>
          <Typography variant="body2" sx={{ fontWeight: 600 }} noWrap>
            {formatBytesToEiB(totalStorage)} EiB
          </Typography>
        </Box>

        <Box sx={{ flex: "1 1 0%", minHeight: 120, width: "100%", overflow: "hidden" }}>
          <CapacityPieChart
            available={availableStorage}
            used={usedStorage}
            formatBytes={formatBytes}
          />
        </Box>

        <Box sx={{ flexShrink: 0, pt: 0.5 }}>
          <Box sx={{ display: "flex", justifyContent: "space-between", mb: 0.75, gap: 1 }}>
            <Box sx={{ display: "flex", alignItems: "center", gap: 1, minWidth: 0 }}>
              <Box
                sx={{
                  width: 10,
                  height: 10,
                  flexShrink: 0,
                  bgcolor: theme.palette.primary.main,
                  borderRadius: "50%",
                }}
              />
              <Typography variant="body2" noWrap>
                Object data
              </Typography>
            </Box>
            <Typography variant="body2" sx={{ fontWeight: 600 }} noWrap>
              {formatBytesToEiB(usedStorage)} EiB
            </Typography>
          </Box>
          <Box sx={{ display: "flex", justifyContent: "space-between", gap: 1 }}>
            <Box sx={{ display: "flex", alignItems: "center", gap: 1, minWidth: 0 }}>
              <Box
                sx={{
                  width: 10,
                  height: 10,
                  flexShrink: 0,
                  bgcolor: alpha(theme.palette.primary.main, 0.3),
                  borderRadius: "50%",
                }}
              />
              <Typography variant="body2" noWrap>
                Available
              </Typography>
            </Box>
            <Typography variant="body2" sx={{ fontWeight: 600 }} noWrap>
              {formatBytesToEiB(availableStorage)} EiB
            </Typography>
          </Box>
        </Box>
      </Box>
    </WidgetWrapper>
  );
};

export default CapacityWidget;
