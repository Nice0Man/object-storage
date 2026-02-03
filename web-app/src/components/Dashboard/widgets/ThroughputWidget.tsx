import React, { useCallback } from "react";
import { Box, useTheme } from "@mui/material";
import { Speed, ArrowForward } from "@mui/icons-material";
import WidgetWrapper from "./WidgetWrapper";
import DataThroughputChart from "../../Charts/DataThroughputChart";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, DataThroughputData } from "../../../api/types";

interface ThroughputWidgetProps {
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

const ThroughputWidget: React.FC<ThroughputWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const [data, setData] = React.useState<DataThroughputData[]>([]);

  const fetchData = useCallback(async () => {
    const response = await apiClient.getDataThroughputStats();
    if (response && response.data) {
      setData(response.data);
    }
  }, []);

  const { loading, error, lastRefresh, refresh, nextRefreshIn } = useWidgetRefresh({
    interval: widget.refresh_interval,
    enabled: widget.visible,
    onRefresh: fetchData,
  });

  const timeRange = widget.settings?.timeRange || "24h";
  const rawChartMode = widget.settings?.chartMode as string | undefined;
  // Ensure chartMode is a valid value for DataThroughputChart
  const chartMode: "stacked" | "lines" | "total" =
    rawChartMode === "lines" || rawChartMode === "total" ? rawChartMode : "stacked";

  return (
    <WidgetWrapper
      widget={widget}
      title="Throughput Over Time"
      icon={<Speed />}
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
      <Box sx={{ display: "flex", alignItems: "center", justifyContent: "flex-end", mb: 1 }}>
        <ArrowForward sx={{ color: theme.palette.text.secondary, fontSize: 18 }} />
      </Box>

      <DataThroughputChart
        data={data}
        formatBytes={formatBytes}
        showModeSelector={!editMode}
        showChartModeSelector={!editMode}
        defaultTimeRange={timeRange}
        defaultChartMode={chartMode}
      />
    </WidgetWrapper>
  );
};

export default ThroughputWidget;
