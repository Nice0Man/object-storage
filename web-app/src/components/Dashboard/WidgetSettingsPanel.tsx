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
  List,
  ListItem,
  ListItemIcon,
  ListItemText,
  ListItemButton,
} from "@mui/material";
import {
  Close,
  Save,
  Storage,
  Dns,
  SdStorage,
  Inventory2,
  ErrorOutline,
  ShowChart,
  Security,
  Pool,
  FlashOn,
} from "@mui/icons-material";
import type { DashboardWidget, WidgetSettings, WidgetType } from "../../api/types";

// Available widget types with metadata
const AVAILABLE_WIDGETS: { type: WidgetType; name: string; description: string; icon: React.ReactNode }[] = [
  { type: "capacity", name: "Capacity", description: "Storage capacity overview", icon: <Storage /> },
  { type: "servers", name: "Servers", description: "Server status monitoring", icon: <Dns /> },
  { type: "drives", name: "Drives", description: "Drive health and status", icon: <SdStorage /> },
  { type: "buckets", name: "Buckets", description: "Bucket statistics", icon: <Inventory2 /> },
  { type: "api_errors", name: "API Errors", description: "API error tracking", icon: <ErrorOutline /> },
  { type: "throughput", name: "Throughput", description: "Data throughput charts", icon: <ShowChart /> },
  { type: "encryption", name: "Encryption", description: "Encryption status", icon: <Security /> },
  { type: "pools", name: "Storage Pools", description: "Pool management", icon: <Pool /> },
  { type: "quick_actions", name: "Quick Actions", description: "Common actions shortcuts", icon: <FlashOn /> },
];

interface WidgetSettingsPanelProps {
  open: boolean;
  widget: DashboardWidget | null;
  onClose: () => void;
  onSave: (widget: DashboardWidget) => void;
  onAddWidget?: (widgetType: WidgetType) => void;
  existingWidgetTypes?: WidgetType[];
  mode?: "edit" | "add";
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
  onAddWidget,
  existingWidgetTypes = [],
  mode = "edit",
}) => {
  const theme = useTheme();
  const [refreshInterval, setRefreshInterval] = useState(widget?.refresh_interval ?? 30);
  const [visible, setVisible] = useState(widget?.visible ?? true);
  const [settings, setSettings] = useState<WidgetSettings>(widget?.settings || {});

  useEffect(() => {
    if (widget) {
      setRefreshInterval(widget.refresh_interval);
      setVisible(widget.visible);
      setSettings(widget.settings || {});
    }
  }, [widget]);

  const handleSave = () => {
    if (!widget) return;
    onSave({
      ...widget,
      refresh_interval: refreshInterval,
      visible,
      settings,
    });
  };

  const getWidgetTypeName = (type: string) => {
    const widgetInfo = AVAILABLE_WIDGETS.find((w) => w.type === type);
    return widgetInfo?.name || type;
  };

  const hasChartSettings = widget ? ["api_errors", "throughput"].includes(widget.widget_type) : false;

  // Available widgets that are not already on the dashboard
  const availableToAdd = AVAILABLE_WIDGETS.filter(
    (w) => !existingWidgetTypes.includes(w.type)
  );

  // Render Add Widget mode
  if (mode === "add") {
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
              Add Widget
            </Typography>
            <IconButton onClick={onClose} size="small">
              <Close />
            </IconButton>
          </Box>

          <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
            Select a widget to add to your dashboard
          </Typography>

          {/* Available Widgets List */}
          <Box sx={{ flex: 1, overflow: "auto" }}>
            <List disablePadding>
              {availableToAdd.length > 0 ? (
                availableToAdd.map((widgetInfo) => (
                  <ListItem key={widgetInfo.type} disablePadding sx={{ mb: 1 }}>
                    <ListItemButton
                      onClick={() => onAddWidget?.(widgetInfo.type)}
                      sx={{
                        borderRadius: 1,
                        border: `1px solid ${alpha(theme.palette.divider, 0.3)}`,
                        "&:hover": {
                          bgcolor: alpha(theme.palette.primary.main, 0.1),
                          borderColor: theme.palette.primary.main,
                        },
                      }}
                    >
                      <ListItemIcon
                        sx={{
                          minWidth: 40,
                          color: theme.palette.primary.main,
                        }}
                      >
                        {widgetInfo.icon}
                      </ListItemIcon>
                      <ListItemText
                        primary={widgetInfo.name}
                        secondary={widgetInfo.description}
                        primaryTypographyProps={{ fontWeight: 600 }}
                        secondaryTypographyProps={{ variant: "caption" }}
                      />
                    </ListItemButton>
                  </ListItem>
                ))
              ) : (
                <Box sx={{ textAlign: "center", py: 4 }}>
                  <Typography variant="body2" color="text.secondary">
                    All widgets are already added to this dashboard
                  </Typography>
                </Box>
              )}
            </List>
          </Box>

          {/* Close Button */}
          <Box sx={{ mt: 3 }}>
            <Button variant="outlined" onClick={onClose} fullWidth>
              Close
            </Button>
          </Box>
        </Box>
      </Drawer>
    );
  }

  // Render Edit Widget mode
  if (!widget) return null;

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
