import { lightPalette, darkPalette } from "../../theme/theme";

export const dashboardTheme = {
  primary: {
    main: darkPalette.primary.main,
    light: darkPalette.primary.light,
    dark: darkPalette.primary.dark,
  },
  secondary: {
    main: lightPalette.secondary.main,
    light: lightPalette.secondary.light,
    dark: lightPalette.secondary.dark,
  },
  accent: {
    main: "#06b6d4",
    light: "#22d3ee",
    dark: "#0891b2",
  },
  success: lightPalette.success,
  warning: lightPalette.warning,
  error: lightPalette.error,
  background: {
    dark: {
      primary: "#0c1222",
      secondary: "#1a1f35",
      tertiary: darkPalette.background.default,
      card: darkPalette.background.paper,
      cardHover: "#334155",
    },
    light: {
      primary: lightPalette.background.default,
      secondary: "#e2e8f0",
      tertiary: "#f1f5f9",
      card: lightPalette.background.paper,
      cardHover: lightPalette.background.default,
    },
  },
  gradients: {
    primary: `linear-gradient(135deg, ${darkPalette.primary.main} 0%, ${lightPalette.secondary.main} 100%)`,
    primaryHover: `linear-gradient(135deg, ${lightPalette.primary.main} 0%, ${lightPalette.secondary.dark} 100%)`,
    accent: `linear-gradient(90deg, ${darkPalette.primary.main} 0%, ${lightPalette.secondary.main} 50%, #06b6d4 100%)`,
    background: {
      dark: "linear-gradient(135deg, #0c1222 0%, #1a1f35 50%, #0f172a 100%)",
      light: `linear-gradient(135deg, ${lightPalette.background.default} 0%, #e2e8f0 50%, #f1f5f9 100%)`,
    },
    card: {
      dark: `linear-gradient(145deg, rgba(30, 41, 59, 0.95) 0%, rgba(15, 23, 42, 0.9) 100%)`,
      light: `linear-gradient(145deg, rgba(255, 255, 255, 0.95) 0%, rgba(248, 250, 252, 0.9) 100%)`,
    },
  },
  shadows: {
    card: {
      dark: "0 4px 20px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255, 255, 255, 0.05)",
      light: "0 4px 20px rgba(0, 0, 0, 0.05), inset 0 1px 0 rgba(255, 255, 255, 0.8)",
    },
    cardHover: {
      dark: `0 12px 40px rgba(59, 130, 246, 0.2), inset 0 1px 0 rgba(255, 255, 255, 0.08)`,
      light: `0 12px 40px rgba(59, 130, 246, 0.15), inset 0 1px 0 rgba(255, 255, 255, 1)`,
    },
    button: `0 4px 15px rgba(59, 130, 246, 0.3)`,
    menu: {
      dark: "0 20px 40px rgba(0, 0, 0, 0.4)",
      light: "0 20px 40px rgba(0, 0, 0, 0.1)",
    },
  },
  borderRadius: {
    sm: 2,
    md: 3,
    lg: 4,
    chip: "10px",
  },
  opacity: {
    active: 0.25,
    hover: 0.15,
    subtle: 0.08,
    divider: 0.15,
    border: 0.1,
  },
} as const;

export const getThemeValue = <T>(isDark: boolean, darkValue: T, lightValue: T): T =>
  isDark ? darkValue : lightValue;

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
