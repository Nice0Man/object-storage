import React from "react";
import { Box, Card, CardContent, Typography, alpha, useTheme } from "@mui/material";
import { CloudUpload, Folder, People, FlashOn } from "@mui/icons-material";
import { useNavigate } from "react-router-dom";
import WidgetWrapper from "./WidgetWrapper";
import type { DashboardWidget } from "../../../api/types";

interface QuickActionsWidgetProps {
  widget: DashboardWidget;
  editMode?: boolean;
  onSettingsClick?: () => void;
  onVisibilityToggle?: () => void;
  onDelete?: () => void;
  dragHandleProps?: Record<string, unknown>;
}

const QuickActionsWidget: React.FC<QuickActionsWidgetProps> = ({
  widget,
  editMode = false,
  onSettingsClick,
  onVisibilityToggle,
  onDelete,
  dragHandleProps,
}) => {
  const theme = useTheme();
  const navigate = useNavigate();

  const actions = [
    {
      title: "Create Bucket",
      description: "Add a new storage bucket",
      icon: CloudUpload,
      color: theme.palette.primary.main,
      path: "/buckets",
    },
    {
      title: "Upload Objects",
      description: "Upload files to storage",
      icon: Folder,
      color: "#10B981",
      path: "/objects",
    },
    {
      title: "Manage Users",
      description: "Configure access and policies",
      icon: People,
      color: "#F59E0B",
      path: "/users",
    },
  ];

  return (
    <WidgetWrapper
      widget={widget}
      title="Quick Actions"
      icon={<FlashOn />}
      editMode={editMode}
      onSettingsClick={onSettingsClick}
      onVisibilityToggle={onVisibilityToggle}
      onDelete={onDelete}
      dragHandleProps={dragHandleProps}
    >
      <Box
        sx={{
          display: "flex",
          gap: 2,
          flexWrap: "wrap",
          flex: 1,
          minHeight: 0,
          alignContent: "flex-start",
        }}
      >
        {actions.map((action) => (
          <Card
            key={action.title}
            onClick={() => !editMode && navigate(action.path)}
            sx={{
              flex: "1 1 140px",
              minWidth: 0,
              maxWidth: "100%",
              cursor: editMode ? "default" : "pointer",
              transition: "all 0.3s",
              borderRadius: 2,
              background: `linear-gradient(135deg, ${alpha(action.color, 0.1)} 0%, ${alpha(action.color, 0.05)} 100%)`,
              "&:hover": !editMode ? {
                transform: "translateY(-2px)",
                boxShadow: theme.shadows[4],
              } : {},
            }}
          >
            <CardContent sx={{ py: 1.5, px: 2 }}>
              <Box sx={{ display: "flex", alignItems: "center", gap: 1.5 }}>
                <action.icon sx={{ fontSize: 32, color: action.color }} />
                <Box>
                  <Typography variant="subtitle2" sx={{ fontWeight: 600 }}>
                    {action.title}
                  </Typography>
                  <Typography variant="caption" color="text.secondary">
                    {action.description}
                  </Typography>
                </Box>
              </Box>
            </CardContent>
          </Card>
        ))}
      </Box>
    </WidgetWrapper>
  );
};

export default QuickActionsWidget;
