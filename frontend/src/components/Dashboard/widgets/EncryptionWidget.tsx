import React, { useCallback } from "react";
import { Box, Typography, Chip, alpha, useTheme } from "@mui/material";
import { Lock, LockOpen, ArrowForward } from "@mui/icons-material";
import WidgetWrapper from "./WidgetWrapper";
import { useWidgetRefresh } from "../../../hooks/useWidgetRefresh";
import { apiClient } from "../../../api/client";
import type { DashboardWidget, EncryptionStats } from "../../../api/types";

interface EncryptionWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  onClick?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const EncryptionWidget: React.FC<EncryptionWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  onClick,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const [data, setData] = React.useState<EncryptionStats | null>(null);

  const fetchData = useCallback(async () => {
    const stats = await apiClient.getEncryptionStats();
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
      title="Encryption"
      icon={<Lock />}
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
      <Box sx={{ flex: 1, minHeight: 0, display: "flex", flexDirection: "column", gap: 1.5 }}>
      <Box sx={{ display: "flex", alignItems: "center", justifyContent: "space-between", flexShrink: 0 }}>
        <Box sx={{ minWidth: 0 }}>
          <Typography
            sx={{
              fontWeight: 700,
              fontSize: "clamp(1.35rem, 2.8vmin, 2.125rem)",
              lineHeight: 1.2,
            }}
          >
            {data?.encryption_percentage?.toFixed(1) ?? 0}%
          </Typography>
          <Typography variant="body2" color="text.secondary">
            Objects Encrypted
          </Typography>
        </Box>
        <ArrowForward sx={{ color: theme.palette.text.secondary, flexShrink: 0 }} />
      </Box>

      <Box sx={{ flexShrink: 0 }}>
        <Box sx={{ display: "flex", justifyContent: "space-between", mb: 1 }}>
          <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
            <Lock sx={{ fontSize: 16, color: "#10B981" }} />
            <Typography variant="body2">Encrypted</Typography>
          </Box>
          <Typography variant="body2" sx={{ fontWeight: 600 }}>
            {data?.encrypted_objects ?? 0}
          </Typography>
        </Box>
        <Box sx={{ display: "flex", justifyContent: "space-between", mb: 1 }}>
          <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
            <LockOpen sx={{ fontSize: 16, color: "#F59E0B" }} />
            <Typography variant="body2">Unencrypted</Typography>
          </Box>
          <Typography variant="body2" sx={{ fontWeight: 600 }}>
            {data?.unencrypted_objects ?? 0}
          </Typography>
        </Box>
      </Box>

      <Box sx={{ flex: "1 1 auto", minHeight: 0, overflow: "auto" }}>
        <Typography
          variant="caption"
          sx={{ fontWeight: 600, color: theme.palette.text.secondary, mb: 1, display: "block" }}
        >
          Encryption Types
        </Typography>
        <Box sx={{ display: "flex", gap: 1, flexWrap: "wrap" }}>
          <Chip
            label={`SSE-S3: ${data?.sse_s3_count ?? 0}`}
            size="small"
            sx={{
              bgcolor: alpha("#3B82F6", 0.1),
              color: "#3B82F6",
              fontWeight: 600,
            }}
          />
          <Chip
            label={`SSE-C: ${data?.sse_c_count ?? 0}`}
            size="small"
            sx={{
              bgcolor: alpha("#8B5CF6", 0.1),
              color: "#8B5CF6",
              fontWeight: 600,
            }}
          />
        </Box>
      </Box>
      </Box>
    </WidgetWrapper>
  );
};

export default EncryptionWidget;
