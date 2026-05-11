import React from "react";
import { Box, CircularProgress, Typography } from "@mui/material";

interface LoaderProps {
  message?: string;
  size?: number;
}

const Loader: React.FC<LoaderProps> = ({
  message = "Loading...",
  size = 40,
}) => {
  return (
    <Box
      sx={{
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        justifyContent: "center",
        minHeight: "200px",
        gap: 2,
      }}
    >
      <CircularProgress size={size} />
      {message && <Typography variant="body2">{message}</Typography>}
    </Box>
  );
};

export default Loader;
