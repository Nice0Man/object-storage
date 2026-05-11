import React from "react";
import { IconButton, Tooltip } from "@mui/material";
import { Brightness4, Brightness7 } from "@mui/icons-material";
import { useTheme } from "../../contexts/ThemeContext";
import { useTranslation } from "react-i18next";

const ThemeToggle: React.FC = () => {
  const { mode, toggleTheme } = useTheme();
  const { t } = useTranslation();

  return (
    <Tooltip
      title={
        mode === "light" ? t("settings.darkMode") : t("settings.lightMode")
      }
    >
      <IconButton onClick={toggleTheme} color="inherit" sx={{ ml: 1 }}>
        {mode === "light" ? <Brightness4 /> : <Brightness7 />}
      </IconButton>
    </Tooltip>
  );
};

export default ThemeToggle;
