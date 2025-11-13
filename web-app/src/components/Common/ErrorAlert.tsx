import React from "react";
import { Alert, AlertTitle, Box } from "@mui/material";

interface ErrorAlertProps {
  error: string | null;
  onClose?: () => void;
}

const ErrorAlert: React.FC<ErrorAlertProps> = ({ error, onClose }) => {
  if (!error) return null;

  return (
    <Box sx={{ mb: 2 }}>
      <Alert severity="error" onClose={onClose}>
        <AlertTitle>Error</AlertTitle>
        {error}
      </Alert>
    </Box>
  );
};

export default ErrorAlert;
