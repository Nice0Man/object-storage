import React, { useState, useCallback } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Paper,
  Typography,
  TextField,
  FormControlLabel,
  Switch,
  Alert,
  LinearProgress,
  List,
  ListItem,
  ListItemText,
  IconButton,
  Tooltip,
  InputAdornment,
} from "@mui/material";
import {
  CloudUpload,
  Delete,
  Lock,
  Visibility,
  VisibilityOff,
  Refresh,
} from "@mui/icons-material";
import apiClient from "../../api/client";

interface EncryptedUploadDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  currentPrefix: string;
  onSuccess?: () => void;
}

const EncryptedUploadDialog: React.FC<EncryptedUploadDialogProps> = ({
  open,
  onClose,
  bucketName,
  currentPrefix,
  onSuccess,
}) => {
  const [selectedFiles, setSelectedFiles] = useState<File[]>([]);
  const [useSseC, setUseSseC] = useState(false);
  const [customerKey, setCustomerKey] = useState("");
  const [showKey, setShowKey] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [uploadProgress, setUploadProgress] = useState<{ [key: string]: number }>({});
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [dragActive, setDragActive] = useState(false);
  const fileInputRef = React.useRef<HTMLInputElement>(null);

  const handleDrag = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    if (e.type === "dragenter" || e.type === "dragover") {
      setDragActive(true);
    } else if (e.type === "dragleave") {
      setDragActive(false);
    }
  }, []);

  const handleDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setDragActive(false);

    if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      const files = Array.from(e.dataTransfer.files);
      setSelectedFiles(files);
    }
  }, []);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files.length > 0) {
      const files = Array.from(e.target.files);
      setSelectedFiles(files);
    }
  };

  const handleClick = () => {
    fileInputRef.current?.click();
  };

  const handleRemoveFile = (index: number) => {
    setSelectedFiles(selectedFiles.filter((_, i) => i !== index));
  };

  const generateRandomKey = () => {
    const array = new Uint8Array(32);
    crypto.getRandomValues(array);
    const hexKey = Array.from(array)
      .map((b) => b.toString(16).padStart(2, "0"))
      .join("");
    setCustomerKey(hexKey);
  };

  const validateKey = () => {
    if (!useSseC) return true;
    if (customerKey.length !== 64) return false;
    return /^[0-9a-fA-F]{64}$/.test(customerKey);
  };

  const handleUpload = async () => {
    if (selectedFiles.length === 0) {
      setError("Please select files to upload");
      return;
    }

    if (useSseC && !validateKey()) {
      setError("Invalid encryption key. Must be 64 hexadecimal characters (32 bytes).");
      return;
    }

    setUploading(true);
    setError(null);
    setSuccess(null);

    try {
      for (const file of selectedFiles) {
        const key = currentPrefix ? `${currentPrefix}${file.name}` : file.name;

        setUploadProgress((prev) => ({ ...prev, [file.name]: 10 }));

        // Read file as ArrayBuffer
        const arrayBuffer = await file.arrayBuffer();
        const data = new Uint8Array(arrayBuffer);

        setUploadProgress((prev) => ({ ...prev, [file.name]: 50 }));

        // Upload with or without SSE-C
        await apiClient.uploadObject(
          bucketName,
          key,
          data,
          file.type || "application/octet-stream",
          useSseC ? customerKey : undefined
        );

        setUploadProgress((prev) => ({ ...prev, [file.name]: 100 }));
      }

      setSuccess(`Successfully uploaded ${selectedFiles.length} file(s)${useSseC ? " with SSE-C encryption" : ""}`);
      onSuccess?.();

      // Reset form after success
      setTimeout(() => {
        setSelectedFiles([]);
        setUploadProgress({});
        setSuccess(null);
      }, 2000);
    } catch (err: any) {
      setError(err.response?.data?.message || "Failed to upload files");
    } finally {
      setUploading(false);
    }
  };

  const handleClose = () => {
    if (!uploading) {
      setSelectedFiles([]);
      setUploadProgress({});
      setError(null);
      setSuccess(null);
      setUseSseC(false);
      setCustomerKey("");
      onClose();
    }
  };

  return (
    <Dialog open={open} onClose={handleClose} maxWidth="md" fullWidth>
      <DialogTitle sx={{ display: "flex", alignItems: "center", gap: 1 }}>
        <CloudUpload color="primary" />
        Upload Files
        {useSseC && <Lock fontSize="small" color="warning" />}
      </DialogTitle>
      <DialogContent>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}
        {success && (
          <Alert severity="success" sx={{ mb: 2 }}>
            {success}
          </Alert>
        )}

        {/* Drop Zone */}
        <Paper
          sx={{
            p: 4,
            textAlign: "center",
            border: "2px dashed",
            borderColor: dragActive ? "primary.main" : "grey.400",
            bgcolor: dragActive ? "action.hover" : "background.paper",
            cursor: "pointer",
            transition: "all 0.3s",
            "&:hover": {
              borderColor: "primary.main",
              bgcolor: "action.hover",
            },
            mb: 3,
          }}
          onDragEnter={handleDrag}
          onDragLeave={handleDrag}
          onDragOver={handleDrag}
          onDrop={handleDrop}
          onClick={handleClick}
        >
          <input
            ref={fileInputRef}
            type="file"
            multiple
            onChange={handleChange}
            style={{ display: "none" }}
          />
          <CloudUpload sx={{ fontSize: 48, color: "primary.main", mb: 2 }} />
          <Typography variant="h6" gutterBottom>
            Drag & Drop files here
          </Typography>
          <Typography variant="body2" color="text.secondary">
            or click to browse
          </Typography>
        </Paper>

        {/* SSE-C Encryption Options */}
        <Paper variant="outlined" sx={{ p: 2, mb: 3 }}>
          <FormControlLabel
            control={
              <Switch
                checked={useSseC}
                onChange={(e) => setUseSseC(e.target.checked)}
                disabled={uploading}
              />
            }
            label={
              <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                <Lock fontSize="small" />
                <Typography>Use Customer-Provided Encryption Key (SSE-C)</Typography>
              </Box>
            }
          />

          {useSseC && (
            <Box sx={{ mt: 2 }}>
              <Alert severity="warning" sx={{ mb: 2 }}>
                <Typography variant="body2">
                  <strong>Important:</strong> Save this key! You will need it to download or access this file.
                  The server does NOT store this key.
                </Typography>
              </Alert>

              <TextField
                fullWidth
                label="Encryption Key (64 hex characters)"
                value={customerKey}
                onChange={(e) => setCustomerKey(e.target.value.toLowerCase())}
                type={showKey ? "text" : "password"}
                placeholder="0123456789abcdef..."
                error={customerKey.length > 0 && !validateKey()}
                helperText={
                  customerKey.length > 0 && !validateKey()
                    ? "Key must be exactly 64 hexadecimal characters"
                    : "256-bit AES key in hexadecimal format"
                }
                disabled={uploading}
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
                      <Tooltip title="Generate random key">
                        <IconButton
                          onClick={generateRandomKey}
                          edge="end"
                          disabled={uploading}
                        >
                          <Refresh />
                        </IconButton>
                      </Tooltip>
                    </InputAdornment>
                  ),
                  sx: { fontFamily: "monospace" },
                }}
              />
            </Box>
          )}
        </Paper>

        {/* Selected Files */}
        {selectedFiles.length > 0 && (
          <Box>
            <Typography variant="subtitle2" gutterBottom>
              Selected Files ({selectedFiles.length}):
            </Typography>
            <List dense>
              {selectedFiles.map((file, index) => {
                const progress = uploadProgress[file.name] || 0;
                return (
                  <ListItem
                    key={index}
                    secondaryAction={
                      !uploading && (
                        <IconButton
                          edge="end"
                          onClick={() => handleRemoveFile(index)}
                        >
                          <Delete />
                        </IconButton>
                      )
                    }
                    sx={{ flexDirection: "column", alignItems: "flex-start" }}
                  >
                    <ListItemText
                      primary={file.name}
                      secondary={`${(file.size / 1024 / 1024).toFixed(2)} MB`}
                    />
                    {progress > 0 && (
                      <Box sx={{ width: "100%", mt: 1 }}>
                        <LinearProgress variant="determinate" value={progress} />
                        <Typography variant="caption" sx={{ mt: 0.5 }}>
                          {progress}%
                        </Typography>
                      </Box>
                    )}
                  </ListItem>
                );
              })}
            </List>
          </Box>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={handleClose} disabled={uploading}>
          Cancel
        </Button>
        <Button
          onClick={handleUpload}
          variant="contained"
          disabled={uploading || selectedFiles.length === 0 || (useSseC && !validateKey())}
          startIcon={useSseC ? <Lock /> : <CloudUpload />}
        >
          {uploading ? "Uploading..." : useSseC ? "Upload Encrypted" : "Upload"}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

export default EncryptedUploadDialog;
