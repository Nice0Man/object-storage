import React, { useState, useEffect } from "react";
import {
  Drawer,
  Box,
  Typography,
  IconButton,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  FormControlLabel,
  Switch,
  Button,
  Divider,
  useTheme,
  alpha,
} from "@mui/material";
import { Close, Save } from "@mui/icons-material";
import type { DashboardWidget, WidgetSettings } from "../../api/types";

interface WidgetSettingsPanelProps {
  open: boolean;
  widget: DashboardWidget;
  onClose: () => void;
  onSave: (widget: DashboardWidget) => void;
}

const refreshIntervalOptions = [
  { value: 0, label: "Disabled" },
  { value: 10, label: "10 seconds" },
  { value: 30, label: "30 seconds" },
  { value: 60, label: "1 minute" },
  { value: 120, label: "2 minutes" },
  { value: 300, label: "5 minutes" },
  { value: 600, label: "10 minutes" },
];

const timeRangeOptions = [
  { value: "1h", label: "1 Hour" },
  { value: "6h", label: "6 Hours" },
  { value: "24h", label: "24 Hours" },
  { value: "7d", label: "7 Days" },
];

const chartModeOptions = [
  { value: "stacked", label: "Stacked Area" },
  { value: "line", label: "Line" },
  { value: "bar", label: "Bar" },
];

const WidgetSettingsPanel: React.FC<WidgetSettingsPanelProps> = ({
  open,
  widget,
  onClose,
  onSave,
}) => {
  const theme = useTheme();
  const [refreshInterval, setRefreshInterval] = useState(widget.refresh_interval);
  const [visible, setVisible] = useState(widget.visible);
  const [settings, setSettings] = useState<WidgetSettings>(widget.settings || {});

  useEffect(() => {
    setRefreshInterval(widget.refresh_interval);
    setVisible(widget.visible);
    setSettings(widget.settings || {});
  }, [widget]);

  const handleSave = () => {
    onSave({
      ...widget,
      refresh_interval: refreshInterval,
      visible,
      settings,
    });
  };

  const getWidgetTypeName = (type: string) => {
    const names: Record<string, string> = {
      capacity: "Capacity",
      servers: "Servers",
      drives: "Drives",
      buckets: "Buckets",
      api_errors: "API Errors",
      throughput: "Throughput",
      encryption: "Encryption",
      pools: "Storage Pools",
      quick_actions: "Quick Actions",
    };
    return names[type] || type;
  };

  const hasChartSettings = ["api_errors", "throughput"].includes(widget.widget_type);

  return (
    <Drawer
      anchor="right"
      open={open}
      onClose={onClose}
      PaperProps={{
        sx: {
          width: 360,
          bgcolor: theme.palette.mode === "dark" ? "#1E293B" : "#FFFFFF",
        },
      }}
    >
      <Box sx={{ p: 3, height: "100%", display: "flex", flexDirection: "column" }}>
        {/* Header */}
        <Box sx={{ display: "flex", alignItems: "center", justifyContent: "space-between", mb: 3 }}>
          <Typography variant="h6" sx={{ fontWeight: 600 }}>
            Widget Settings
          </Typography>
          <IconButton onClick={onClose} size="small">
            <Close />
          </IconButton>
        </Box>

        {/* Widget Type */}
        <Box
          sx={{
            p: 2,
            mb: 3,
            borderRadius: 2,
            bgcolor: alpha(theme.palette.primary.main, 0.1),
          }}
        >
          <Typography variant="subtitle2" color="text.secondary">
            Widget Type
          </Typography>
          <Typography variant="h6" sx={{ fontWeight: 600 }}>
            {getWidgetTypeName(widget.widget_type)}
          </Typography>
        </Box>

        {/* Settings Form */}
        <Box sx={{ flex: 1, overflow: "auto" }}>
          {/* Visibility */}
          <FormControlLabel
            control={
              <Switch
                checked={visible}
                onChange={(e) => setVisible(e.target.checked)}
                color="primary"
              />
            }
            label="Visible on Dashboard"
            sx={{ mb: 3 }}
          />

          <Divider sx={{ my: 2 }} />

          {/* Refresh Interval */}
          <Typography variant="subtitle2" sx={{ mb: 1, fontWeight: 600 }}>
            Auto-Refresh Interval
          </Typography>
          <FormControl fullWidth sx={{ mb: 3 }}>
            <Select
              value={refreshInterval}
              onChange={(e) => setRefreshInterval(Number(e.target.value))}
            >
              {refreshIntervalOptions.map((option) => (
                <MenuItem key={option.value} value={option.value}>
                  {option.label}
                </MenuItem>
              ))}
            </Select>
          </FormControl>

          {refreshInterval > 0 && (
            <Typography variant="caption" color="text.secondary" sx={{ display: "block", mb: 3 }}>
              Widget data will refresh every {refreshInterval} seconds
            </Typography>
          )}

          {/* Chart-specific settings */}
          {hasChartSettings && (
            <>
              <Divider sx={{ my: 2 }} />

              <Typography variant="subtitle2" sx={{ mb: 2, fontWeight: 600 }}>
                Chart Settings
              </Typography>

              {/* Time Range */}
              <FormControl fullWidth sx={{ mb: 2 }}>
                <InputLabel>Time Range</InputLabel>
                <Select
                  value={settings.timeRange || "24h"}
                  onChange={(e) => setSettings({ ...settings, timeRange: e.target.value as WidgetSettings["timeRange"] })}
                  label="Time Range"
                >
                  {timeRangeOptions.map((option) => (
                    <MenuItem key={option.value} value={option.value}>
                      {option.label}
                    </MenuItem>
                  ))}
                </Select>
              </FormControl>

              {/* Chart Mode */}
              <FormControl fullWidth sx={{ mb: 2 }}>
                <InputLabel>Chart Style</InputLabel>
                <Select
                  value={settings.chartMode || "stacked"}
                  onChange={(e) => setSettings({ ...settings, chartMode: e.target.value as WidgetSettings["chartMode"] })}
                  label="Chart Style"
                >
                  {chartModeOptions.map((option) => (
                    <MenuItem key={option.value} value={option.value}>
                      {option.label}
                    </MenuItem>
                  ))}
                </Select>
              </FormControl>

              {/* Show Legend */}
              <FormControlLabel
                control={
                  <Switch
                    checked={settings.showLegend !== false}
                    onChange={(e) => setSettings({ ...settings, showLegend: e.target.checked })}
                    color="primary"
                  />
                }
                label="Show Legend"
              />
            </>
          )}
        </Box>

        {/* Actions */}
        <Box sx={{ mt: 3, display: "flex", gap: 2 }}>
          <Button variant="outlined" onClick={onClose} fullWidth>
            Cancel
          </Button>
          <Button
            variant="contained"
            onClick={handleSave}
            startIcon={<Save />}
            fullWidth
          >
            Apply
          </Button>
        </Box>
      </Box>
    </Drawer>
  );
};

export default WidgetSettingsPanel;
