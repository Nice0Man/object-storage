import { alpha } from "@mui/material/styles";
import type { Theme } from "@mui/material/styles";

/** Flat dashboard surfaces — no gradients or hover elevation. */
export const getWidgetSurfaceSx = (theme: Theme) => ({
  bgcolor: "background.paper",
  border: `1px solid ${theme.palette.divider}`,
  borderRadius: 1,
  boxShadow: "none",
});

export const getChipStyles = (theme: Theme, isActive: boolean) => ({
  bgcolor: isActive
    ? alpha(theme.palette.primary.main, 0.1)
    : theme.palette.action.hover,
  color: isActive ? theme.palette.primary.main : theme.palette.text.primary,
  borderColor: isActive ? alpha(theme.palette.primary.main, 0.35) : theme.palette.divider,
});
