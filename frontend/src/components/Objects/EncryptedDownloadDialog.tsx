import React, { useState } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  TextField,
  Alert,
  InputAdornment,
  IconButton,
  Tooltip,
  CircularProgress,
} from "@mui/material";
import {
  Download,
  Lock,
  Visibility,
  VisibilityOff,
} from "@mui/icons-material";

interface EncryptedDownloadDialogProps {
  open: boolean;
  onClose: () => void;
  objectKey: string;
  bucketName: string;
  onDownload: (sseCustomerKey: string) => Promise<void>;
}

const EncryptedDownloadDialog: React.FC<EncryptedDownloadDialogProps> = ({
  open,
  onClose,
  objectKey,
  bucketName,
  onDownload,
}) => {
  const [customerKey, setCustomerKey] = useState("");
  const [showKey, setShowKey] = useState(false);
  const [downloading, setDownloading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const validateKey = () => {
    if (customerKey.length !== 64) return false;
    return /^[0-9a-fA-F]{64}$/.test(customerKey);
  };

  const handleDownload = async () => {
    if (!validateKey()) {
      setError("Invalid encryption key. Must be 64 hexadecimal characters (32 bytes).");
      return;
    }

    setDownloading(true);
    setError(null);

    try {
      await onDownload(customerKey);
      handleClose();
    } catch (err: any) {
      setError(err.message || "Failed to download object");
    } finally {
      setDownloading(false);
    }
  };

  const handleClose = () => {
    if (!downloading) {
      setCustomerKey("");
      setError(null);
      onClose();
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "Enter" && validateKey() && !downloading) {
      handleDownload();
    }
  };

  return (
    <Dialog open={open} onClose={handleClose} maxWidth="sm" fullWidth>
      <DialogTitle sx={{ display: "flex", alignItems: "center", gap: 1 }}>
        <Lock color="warning" />
        Encrypted Object Download
      </DialogTitle>
      <DialogContent>
        <Alert severity="info" sx={{ mb: 2 }}>
          <Typography variant="body2">
            This object is encrypted with SSE-C (Customer-Provided Key).
            Please enter the encryption key that was used when uploading this object.
          </Typography>
        </Alert>

        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        <Box sx={{ mb: 2 }}>
          <Typography variant="body2" color="text.secondary" gutterBottom>
            Object: <strong>{objectKey}</strong>
          </Typography>
          <Typography variant="body2" color="text.secondary">
            Bucket: <strong>{bucketName}</strong>
          </Typography>
        </Box>

        <TextField
          fullWidth
          label="Encryption Key (64 hex characters)"
          value={customerKey}
          onChange={(e) => setCustomerKey(e.target.value.toLowerCase())}
          onKeyDown={handleKeyDown}
          type={showKey ? "text" : "password"}
          placeholder="0123456789abcdef..."
          error={customerKey.length > 0 && !validateKey()}
          helperText={
            customerKey.length > 0 && !validateKey()
              ? "Key must be exactly 64 hexadecimal characters"
              : "Enter the 256-bit AES key in hexadecimal format"
          }
          disabled={downloading}
          autoFocus
          InputProps={{
            endAdornment: (
              <InputAdornment position="end">
                <Tooltip title={showKey ? "Hide key" : "Show key"}>
                  <IconButton
                    onClick={() => setShowKey(!showKey)}
                    edge="end"
                  >
                    {showKey ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </Tooltip>
              </InputAdornment>
            ),
            sx: { fontFamily: "monospace" },
          }}
        />
      </DialogContent>
      <DialogActions>
        <Button onClick={handleClose} disabled={downloading}>
          Cancel
        </Button>
        <Button
          onClick={handleDownload}
          variant="contained"
          disabled={downloading || !validateKey()}
          startIcon={downloading ? <CircularProgress size={20} /> : <Download />}
        >
          {downloading ? "Downloading..." : "Download"}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

export default EncryptedDownloadDialog;
