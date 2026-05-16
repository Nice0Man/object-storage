import { createTheme, ThemeOptions, alpha } from "@mui/material/styles";

export const lightPalette = {
  primary: {
    main: "#2563eb",
    light: "#60a5fa",
    dark: "#1e40af",
    contrastText: "#ffffff",
  },
  secondary: {
    main: "#64748b",
    light: "#94a3b8",
    dark: "#475569",
    contrastText: "#ffffff",
  },
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
  background: {
    default: "#f1f5f9",
    paper: "#ffffff",
  },
  text: {
    primary: "#0f172a",
    secondary: "#64748b",
  },
};

export const darkPalette = {
  primary: {
    main: "#3b82f6",
    light: "#60a5fa",
    dark: "#2563eb",
    contrastText: "#ffffff",
  },
  secondary: {
    main: "#94a3b8",
    light: "#cbd5e1",
    dark: "#64748b",
    contrastText: "#0f172a",
  },
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
  background: {
    default: "#0f172a",
    paper: "#1e293b",
  },
  text: {
    primary: "#f1f5f9",
    secondary: "#94a3b8",
  },
};

const getDesignTokens = (mode: "light" | "dark"): ThemeOptions => ({
  palette: {
    mode,
    ...(mode === "light" ? lightPalette : darkPalette),
  },
  typography: {
    fontFamily: '"Inter", "Roboto", "Helvetica", "Arial", sans-serif',
    h1: { fontSize: "2rem", fontWeight: 600, letterSpacing: "-0.02em" },
    h2: { fontSize: "1.75rem", fontWeight: 600, letterSpacing: "-0.01em" },
    h3: { fontSize: "1.5rem", fontWeight: 600 },
    h4: { fontSize: "1.25rem", fontWeight: 600 },
    h5: { fontSize: "1.125rem", fontWeight: 600 },
    h6: { fontSize: "1rem", fontWeight: 600 },
    button: { textTransform: "none", fontWeight: 500 },
  },
  shape: {
    borderRadius: 8,
  },
  transitions: {
    duration: {
      shortest: 0,
      shorter: 0,
      short: 0,
    },
  },
  components: {
    MuiCssBaseline: {
      styleOverrides: {
        "@media (prefers-reduced-motion: reduce)": {
          "*": {
            animationDuration: "0.01ms !important",
            animationIterationCount: "1 !important",
            transitionDuration: "0.01ms !important",
          },
        },
      },
    },
    MuiButton: {
      styleOverrides: {
        root: {
          borderRadius: 8,
          padding: "8px 20px",
          fontSize: "0.9375rem",
          fontWeight: 500,
          boxShadow: "none",
          "&:hover": {
            boxShadow: "none",
          },
        },
        contained: {
          "&:hover": {
            boxShadow: "none",
          },
        },
      },
    },
    MuiPaper: {
      styleOverrides: {
        root: {
          backgroundImage: "none",
        },
        outlined: {
          borderColor:
            mode === "light"
              ? alpha("#0f172a", 0.12)
              : alpha("#f1f5f9", 0.12),
        },
      },
    },
    MuiCard: {
      styleOverrides: {
        root: {
          borderRadius: 8,
          boxShadow: "none",
          border: `1px solid ${mode === "light" ? alpha("#0f172a", 0.08) : alpha("#f1f5f9", 0.08)}`,
        },
      },
    },
    MuiTextField: {
      styleOverrides: {
        root: {
          "& .MuiOutlinedInput-root": {
            borderRadius: 8,
            "&:hover .MuiOutlinedInput-notchedOutline": {
              borderColor:
                mode === "light"
                  ? alpha("#2563eb", 0.4)
                  : alpha("#3b82f6", 0.4),
            },
            "&.Mui-focused .MuiOutlinedInput-notchedOutline": {
              borderWidth: 1,
            },
          },
        },
      },
    },
    MuiChip: {
      styleOverrides: {
        root: {
          borderRadius: 6,
          fontWeight: 500,
        },
      },
    },
    MuiTableRow: {
      styleOverrides: {
        root: {
          "&:hover": {
            backgroundColor:
              mode === "light"
                ? alpha("#0f172a", 0.04)
                : alpha("#f1f5f9", 0.04),
          },
        },
      },
    },
    MuiTableCell: {
      styleOverrides: {
        root: {
          borderBottom:
            mode === "light"
              ? "1px solid rgba(0, 0, 0, 0.08)"
              : "1px solid rgba(255, 255, 255, 0.08)",
        },
        head: {
          fontWeight: 600,
          backgroundColor:
            mode === "light" ? alpha("#2563eb", 0.04) : alpha("#3b82f6", 0.08),
        },
      },
    },
    MuiDrawer: {
      styleOverrides: {
        paper: {
          borderRight: `1px solid ${mode === "light" ? alpha("#0f172a", 0.08) : alpha("#f1f5f9", 0.08)}`,
        },
      },
    },
    MuiListItemButton: {
      styleOverrides: {
        root: {
          borderRadius: 8,
          "&.Mui-selected": {
            backgroundColor: alpha(mode === "light" ? "#2563eb" : "#3b82f6", 0.12),
            color: mode === "light" ? "#1e40af" : "#93c5fd",
            "&:hover": {
              backgroundColor: alpha(mode === "light" ? "#2563eb" : "#3b82f6", 0.16),
            },
            "& .MuiListItemIcon-root": {
              color: mode === "light" ? "#2563eb" : "#60a5fa",
            },
          },
        },
      },
    },
  },
});

export const createAppTheme = (mode: "light" | "dark") => createTheme(getDesignTokens(mode));
