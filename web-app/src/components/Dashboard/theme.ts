/**
 * Dashboard Theme Constants
 * Common color palette and styling for consistent dashboard UI
 */

export const dashboardTheme = {
  // Primary accent colors
  primary: {
    main: "#3b82f6",
    light: "#60a5fa",
    dark: "#2563eb",
  },
  // Secondary accent
  secondary: {
    main: "#8b5cf6",
    light: "#a78bfa",
    dark: "#7c3aed",
  },
  // Cyan accent for variety
  accent: {
    main: "#06b6d4",
    light: "#22d3ee",
    dark: "#0891b2",
  },
  // Status colors
  success: {
    main: "#10b981",
    light: "#34d399",
    dark: "#059669",
  },
  warning: {
    main: "#f59e0b",
    light: "#fbbf24",
    dark: "#d97706",
  },
  error: {
    main: "#ef4444",
    light: "#f87171",
    dark: "#dc2626",
  },
  // Background colors
  background: {
    dark: {
      primary: "#0c1222",
      secondary: "#1a1f35",
      tertiary: "#0f172a",
      card: "#1e293b",
      cardHover: "#334155",
    },
    light: {
      primary: "#f8fafc",
      secondary: "#e2e8f0",
      tertiary: "#f1f5f9",
      card: "#ffffff",
      cardHover: "#f8fafc",
    },
  },
  // Gradients
  gradients: {
    primary: "linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%)",
    primaryHover: "linear-gradient(135deg, #2563eb 0%, #7c3aed 100%)",
    accent: "linear-gradient(90deg, #3b82f6 0%, #8b5cf6 50%, #06b6d4 100%)",
    background: {
      dark: "linear-gradient(135deg, #0c1222 0%, #1a1f35 50%, #0f172a 100%)",
      light: "linear-gradient(135deg, #f8fafc 0%, #e2e8f0 50%, #f1f5f9 100%)",
    },
    card: {
      dark: "linear-gradient(145deg, rgba(30, 41, 59, 0.95) 0%, rgba(15, 23, 42, 0.9) 100%)",
      light: "linear-gradient(145deg, rgba(255, 255, 255, 0.95) 0%, rgba(248, 250, 252, 0.9) 100%)",
    },
  },
  // Shadows
  shadows: {
    card: {
      dark: "0 4px 20px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255, 255, 255, 0.05)",
      light: "0 4px 20px rgba(0, 0, 0, 0.05), inset 0 1px 0 rgba(255, 255, 255, 0.8)",
    },
    cardHover: {
      dark: "0 12px 40px rgba(59, 130, 246, 0.2), inset 0 1px 0 rgba(255, 255, 255, 0.08)",
      light: "0 12px 40px rgba(59, 130, 246, 0.15), inset 0 1px 0 rgba(255, 255, 255, 1)",
    },
    button: "0 4px 15px rgba(59, 130, 246, 0.3)",
    menu: {
      dark: "0 20px 40px rgba(0, 0, 0, 0.4)",
      light: "0 20px 40px rgba(0, 0, 0, 0.1)",
    },
  },
  // Border radius
  borderRadius: {
    sm: 2,
    md: 3,
    lg: 4,
    chip: "10px",
  },
  // Opacity values
  opacity: {
    active: 0.25,
    hover: 0.15,
    subtle: 0.08,
    divider: 0.15,
    border: 0.1,
  },
} as const;

// Helper function to get theme-aware value
export const getThemeValue = <T>(isDark: boolean, darkValue: T, lightValue: T): T =>
  isDark ? darkValue : lightValue;

// Common style helpers
export const getCardStyles = (isDark: boolean) => ({
  background: getThemeValue(isDark, dashboardTheme.gradients.card.dark, dashboardTheme.gradients.card.light),
  boxShadow: getThemeValue(isDark, dashboardTheme.shadows.card.dark, dashboardTheme.shadows.card.light),
  borderRadius: dashboardTheme.borderRadius.md,
});

export const getChipStyles = (isDark: boolean, isActive: boolean) => ({
  bgcolor: isActive
    ? `rgba(59, 130, 246, ${isDark ? 0.25 : 0.15})`
    : `rgba(${isDark ? "30, 41, 59" : "255, 255, 255"}, ${isDark ? 0.6 : 0.8})`,
  color: isActive
    ? getThemeValue(isDark, dashboardTheme.primary.light, dashboardTheme.primary.dark)
    : undefined,
  borderColor: isActive
    ? `rgba(59, 130, 246, ${isDark ? 0.4 : 0.3})`
    : undefined,
});

export const getButtonGradient = () => ({
  background: dashboardTheme.gradients.primary,
  boxShadow: dashboardTheme.shadows.button,
  "&:hover": {
    background: dashboardTheme.gradients.primaryHover,
  },
});
