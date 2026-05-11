import { createTheme, ThemeOptions, alpha } from "@mui/material/styles";

export const lightPalette = {
  primary: {
    main: "#2563eb", // Modern blue
    light: "#60a5fa",
    dark: "#1e40af",
    contrastText: "#ffffff",
  },
  secondary: {
    main: "#8b5cf6", // Purple accent
    light: "#a78bfa",
    dark: "#6d28d9",
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
    default: "#f8fafc",
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
    main: "#a78bfa",
    light: "#c4b5fd",
    dark: "#8b5cf6",
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
    h1: {
      fontSize: "2.5rem",
      fontWeight: 700,
      letterSpacing: "-0.02em",
    },
    h2: {
      fontSize: "2rem",
      fontWeight: 700,
      letterSpacing: "-0.01em",
    },
    h3: {
      fontSize: "1.75rem",
      fontWeight: 600,
      letterSpacing: "-0.01em",
    },
    h4: {
      fontSize: "1.5rem",
      fontWeight: 600,
    },
    h5: {
      fontSize: "1.25rem",
      fontWeight: 600,
    },
    h6: {
      fontSize: "1rem",
      fontWeight: 600,
    },
    button: {
      textTransform: "none",
      fontWeight: 500,
    },
  },
  shape: {
    borderRadius: 12, // Smooth rounded corners
  },
  components: {
    MuiButton: {
      styleOverrides: {
        root: {
          borderRadius: 10,
          padding: "10px 24px",
          fontSize: "0.95rem",
          fontWeight: 500,
          boxShadow: "none",
          transition: "all 0.2s cubic-bezier(0.4, 0, 0.2, 1)",
          "&:hover": {
            boxShadow:
              mode === "light"
                ? "0 4px 12px rgba(0, 0, 0, 0.15)"
                : "0 4px 12px rgba(0, 0, 0, 0.5)",
            transform: "translateY(-1px)",
          },
        },
        contained: {
          "&:hover": {
            boxShadow:
              mode === "light"
                ? "0 6px 16px rgba(0, 0, 0, 0.2)"
                : "0 6px 16px rgba(0, 0, 0, 0.6)",
          },
        },
      },
    },
    MuiPaper: {
      styleOverrides: {
        root: {
          backgroundImage: "none",
          transition: "box-shadow 0.2s cubic-bezier(0.4, 0, 0.2, 1)",
        },
        elevation1: {
          boxShadow:
            mode === "light"
              ? "0 1px 3px rgba(0, 0, 0, 0.12), 0 1px 2px rgba(0, 0, 0, 0.08)"
              : "0 1px 3px rgba(0, 0, 0, 0.5), 0 1px 2px rgba(0, 0, 0, 0.4)",
        },
      },
    },
    MuiCard: {
      styleOverrides: {
        root: {
          borderRadius: 16,
          boxShadow:
            mode === "light"
              ? "0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06)"
              : "0 4px 6px -1px rgba(0, 0, 0, 0.5), 0 2px 4px -1px rgba(0, 0, 0, 0.4)",
          transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
          "&:hover": {
            boxShadow:
              mode === "light"
                ? "0 10px 15px -3px rgba(0, 0, 0, 0.15), 0 4px 6px -2px rgba(0, 0, 0, 0.08)"
                : "0 10px 15px -3px rgba(0, 0, 0, 0.6), 0 4px 6px -2px rgba(0, 0, 0, 0.5)",
            transform: "translateY(-2px)",
          },
        },
      },
    },
    MuiTextField: {
      styleOverrides: {
        root: {
          "& .MuiOutlinedInput-root": {
            borderRadius: 10,
            transition: "all 0.2s cubic-bezier(0.4, 0, 0.2, 1)",
            "&:hover": {
              "& .MuiOutlinedInput-notchedOutline": {
                borderColor:
                  mode === "light"
                    ? alpha("#2563eb", 0.5)
                    : alpha("#3b82f6", 0.5),
              },
            },
            "&.Mui-focused": {
              boxShadow:
                mode === "light"
                  ? "0 0 0 3px rgba(37, 99, 235, 0.1)"
                  : "0 0 0 3px rgba(59, 130, 246, 0.2)",
            },
          },
        },
      },
    },
    MuiChip: {
      styleOverrides: {
        root: {
          borderRadius: 8,
          fontWeight: 500,
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
            mode === "light" ? alpha("#2563eb", 0.05) : alpha("#3b82f6", 0.1),
        },
      },
    },
    MuiAppBar: {
      styleOverrides: {
        root: {
          boxShadow:
            mode === "light"
              ? "0 1px 3px rgba(0, 0, 0, 0.12), 0 1px 2px rgba(0, 0, 0, 0.08)"
              : "0 1px 3px rgba(0, 0, 0, 0.5), 0 1px 2px rgba(0, 0, 0, 0.4)",
        },
      },
    },
  },
});

export const createAppTheme = (mode: "light" | "dark") => {
  return createTheme(getDesignTokens(mode));
};
