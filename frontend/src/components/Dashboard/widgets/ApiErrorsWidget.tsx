import React, { useCallback } from "react";
import { Box, useTheme } from "@mui/material";
import { Error as ErrorIcon, ArrowForward } from "@mui/icons-material";
import WidgetWrapper from "./WidgetWrapper";
import ApiErrorsChart from "../../Charts/ApiErrorsChart";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, ApiErrorData } from "../../../api/types";

interface ApiErrorsWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  onClick?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const ApiErrorsWidget: React.FC<ApiErrorsWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const [data, setData] = React.useState<ApiErrorData[]>([]);

  const fetchData = useCallback(async () => {
    const response = await apiClient.getApiErrorStats();
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

  return (
    <WidgetWrapper
      widget={widget}
      title="Error Distribution"
      icon={<ErrorIcon />}
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
          overflow: "hidden",
          maxWidth: "100%",
        }}
      >
      <Box sx={{ display: "flex", alignItems: "center", justifyContent: "flex-end", mb: 1, flexShrink: 0 }}>
        <ArrowForward sx={{ color: theme.palette.text.secondary, fontSize: 18 }} />
      </Box>

      <ApiErrorsChart
        data={data}
        showModeSelector={!editMode}
        defaultMode={timeRange}
        fillParent
      />
      </Box>
    </WidgetWrapper>
  );
};

export default ApiErrorsWidget;
