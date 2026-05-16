import type { Theme } from "@mui/material/styles";
import { alpha } from "@mui/material/styles";

export const widgetScrollSx = (theme: Theme) => ({
  overflowY: "auto" as const,
  overflowX: "hidden" as const,
  scrollbarWidth: "thin" as const,
  scrollbarColor: `${alpha(theme.palette.text.primary, 0.35)} transparent`,
  "&::-webkit-scrollbar": {
    width: 8,
  },
  "&::-webkit-scrollbar-track": {
    bgcolor: "transparent",
  },
  "&::-webkit-scrollbar-thumb": {
    borderRadius: 4,
    bgcolor: alpha(
      theme.palette.text.primary,
      theme.palette.mode === "dark" ? 0.28 : 0.22,
    ),
  },
  "&::-webkit-scrollbar-thumb:hover": {
    bgcolor: alpha(theme.palette.text.primary, 0.42),
  },
});
