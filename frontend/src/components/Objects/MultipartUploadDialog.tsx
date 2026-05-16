import React, { useState, useCallback } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  LinearProgress,
  List,
  ListItem,
  ListItemIcon,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
  Alert,
  Chip,
  Paper,
} from "@mui/material";
import {
  CloudUpload,
  InsertDriveFile,
  Delete,
  CheckCircle,
  Error as ErrorIcon,
  Pending,
} from "@mui/icons-material";
import { useDropzone } from "react-dropzone";
import apiClient from "../../api/client";
import { widgetScrollSx } from "../../theme/widgetStyles";

interface FileUploadState {
  file: File;
  progress: number;
  status: "pending" | "uploading" | "completed" | "error";
  error?: string;
}

interface MultipartUploadDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  currentPrefix: string;
  onSuccess?: () => void;
}

const CHUNK_SIZE = 10 * 1024 * 1024; // 10MB chunks
const MULTIPART_THRESHOLD = 100 * 1024 * 1024; // 100MB - use multipart for files larger than this

const MultipartUploadDialog: React.FC<MultipartUploadDialogProps> = ({
  open,
  onClose,
  bucketName,
  currentPrefix,
  onSuccess,
}) => {
  const [files, setFiles] = useState<FileUploadState[]>([]);
  const [uploading, setUploading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const onDrop = useCallback((acceptedFiles: File[]) => {
    const newFiles = acceptedFiles.map((file) => ({
      file,
      progress: 0,
      status: "pending" as const,
    }));
    setFiles((prev) => [...prev, ...newFiles]);
  }, []);

  const { getRootProps, getInputProps, isDragActive } = useDropzone({
    onDrop,
    disabled: uploading,
  });

  const removeFile = (index: number) => {
    setFiles((prev) => prev.filter((_, i) => i !== index));
  };

  const updateFileProgress = (index: number, progress: number) => {
    setFiles((prev) =>
      prev.map((f, i) => (i === index ? { ...f, progress } : f))
    );
  };

  const updateFileStatus = (
    index: number,
    status: FileUploadState["status"],
    error?: string
  ) => {
    setFiles((prev) =>
      prev.map((f, i) => (i === index ? { ...f, status, error } : f))
    );
  };

  const uploadFile = async (fileState: FileUploadState, index: number) => {
    const { file } = fileState;
    const key = currentPrefix ? `${currentPrefix}${file.name}` : file.name;

    updateFileStatus(index, "uploading");

    try {
      if (file.size > MULTIPART_THRESHOLD) {
        // Use multipart upload for large files
        await apiClient.uploadLargeFile(
          bucketName,
          key,
          file,
          (progress) => updateFileProgress(index, progress),
          CHUNK_SIZE
        );
      } else {
        // Use simple upload for smaller files
        await apiClient.uploadObject(
          bucketName,
          key,
          file,
          undefined, // contentType - auto-detect
          undefined, // sseCustomerKey - none
          (progress) => updateFileProgress(index, progress)
        );
      }
      updateFileStatus(index, "completed");
      updateFileProgress(index, 100);
    } catch (err: any) {
      updateFileStatus(
        index,
        "error",
        err.response?.data?.error || err.message || "Upload failed"
      );
    }
  };

  const handleUpload = async () => {
    if (files.length === 0) return;

    setUploading(true);
    setError(null);

    // Upload files sequentially to avoid overwhelming the server
    for (let i = 0; i < files.length; i++) {
      if (files[i].status === "pending") {
        await uploadFile(files[i], i);
      }
    }

    setUploading(false);

    // Check if all uploads completed successfully
    const allCompleted = files.every(
      (f) => f.status === "completed" || f.status === "error"
    );
    const anyError = files.some((f) => f.status === "error");

    if (allCompleted && !anyError) {
      onSuccess?.();
    }
  };

  const handleClose = () => {
    if (!uploading) {
      setFiles([]);
      setError(null);
      onClose();
    }
  };

  const formatSize = (bytes: number) => {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + " " + sizes[i];
  };

  const totalSize = files.reduce((acc, f) => acc + f.file.size, 0);
  const uploadedSize = files.reduce(
    (acc, f) => acc + (f.file.size * f.progress) / 100,
    0
  );
  const overallProgress = totalSize > 0 ? (uploadedSize / totalSize) * 100 : 0;

  const getStatusIcon = (status: FileUploadState["status"]) => {
    switch (status) {
      case "completed":
        return <CheckCircle color="success" />;
      case "error":
        return <ErrorIcon color="error" />;
      case "uploading":
        return <CloudUpload color="primary" />;
      default:
        return <Pending color="disabled" />;
    }
  };

  return (
    <Dialog open={open} onClose={handleClose} maxWidth="md" fullWidth>
      <DialogTitle sx={{ display: "flex", alignItems: "center", gap: 1 }}>
        <CloudUpload color="primary" />
        Upload Files
        {files.length > 0 && (
          <Chip
            label={`${files.length} file${files.length > 1 ? "s" : ""}`}
            size="small"
            sx={{ ml: 1 }}
          />
        )}
      </DialogTitle>
      <DialogContent>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
          Destination: <strong>{bucketName}/{currentPrefix || ""}</strong>
        </Typography>

        {/* Drop zone */}
        <Paper
          {...getRootProps()}
          variant="outlined"
          sx={{
            p: 4,
            textAlign: "center",
            cursor: uploading ? "not-allowed" : "pointer",
            bgcolor: isDragActive ? "action.hover" : "background.paper",
            borderStyle: "dashed",
            borderColor: isDragActive ? "primary.main" : "divider",
            mb: 2,
            transition: "all 0.2s ease",
            "&:hover": {
              borderColor: uploading ? "divider" : "primary.main",
              bgcolor: uploading ? "background.paper" : "action.hover",
            },
          }}
        >
          <input {...getInputProps()} />
          <CloudUpload sx={{ fontSize: 48, color: "text.secondary", mb: 1 }} />
          <Typography variant="body1">
            {isDragActive
              ? "Drop files here..."
              : "Drag & drop files here, or click to select"}
          </Typography>
          <Typography variant="caption" color="text.secondary">
            Large files (100MB+) will use multipart upload automatically
          </Typography>
        </Paper>

        {/* File list */}
        {files.length > 0 && (
          <>
            <Box sx={{ mb: 2 }}>
              <Box sx={{ display: "flex", justifyContent: "space-between", mb: 1 }}>
                <Typography variant="body2">
                  Overall Progress: {Math.round(overallProgress)}%
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  {formatSize(uploadedSize)} / {formatSize(totalSize)}
                </Typography>
              </Box>
              <LinearProgress
                variant="determinate"
                value={overallProgress}
                sx={{ height: 8, borderRadius: 1 }}
              />
            </Box>

            <List
              dense
              sx={(theme) => ({
                maxHeight: 280,
                overflowY: "auto",
                overflowX: "hidden",
                border: 1,
                borderColor: "divider",
                borderRadius: 1,
                ...widgetScrollSx(theme),
              })}
            >
              {files.map((fileState, index) => (
                <ListItem
                  key={`${fileState.file.name}-${index}`}
                  sx={{
                    bgcolor:
                      fileState.status === "error"
                        ? "error.light"
                        : fileState.status === "completed"
                        ? "success.light"
                        : "transparent",
                    borderRadius: 1,
                    mb: 0.5,
                  }}
                >
                  <ListItemIcon>{getStatusIcon(fileState.status)}</ListItemIcon>
                  <ListItemText
                    primary={
                      <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                        <InsertDriveFile fontSize="small" />
                        <Typography variant="body2" noWrap sx={{ maxWidth: 300 }}>
                          {fileState.file.name}
                        </Typography>
                        {fileState.file.size > MULTIPART_THRESHOLD && (
                          <Chip
                            label="Multipart"
                            size="small"
                            variant="outlined"
                            color="info"
                          />
                        )}
                      </Box>
                    }
                    secondary={
                      <Box sx={{ mt: 0.5 }}>
                        <Typography variant="caption" color="text.secondary">
                          {formatSize(fileState.file.size)}
                        </Typography>
                        {fileState.status === "uploading" && (
                          <LinearProgress
                            variant="determinate"
                            value={fileState.progress}
                            sx={{ mt: 0.5, height: 4, borderRadius: 1 }}
                          />
                        )}
                        {fileState.error && (
                          <Typography variant="caption" color="error">
                            {fileState.error}
                          </Typography>
                        )}
                      </Box>
                    }
                  />
                  <ListItemSecondaryAction>
                    {fileState.status === "pending" && !uploading && (
                      <IconButton size="small" onClick={() => removeFile(index)}>
                        <Delete fontSize="small" />
                      </IconButton>
                    )}
                    {fileState.status === "uploading" && (
                      <Typography variant="body2" color="primary">
                        {fileState.progress}%
                      </Typography>
                    )}
                  </ListItemSecondaryAction>
                </ListItem>
              ))}
            </List>
          </>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={handleClose} disabled={uploading}>
          {uploading ? "Uploading..." : "Close"}
        </Button>
        <Button
          variant="contained"
          onClick={handleUpload}
          disabled={uploading || files.length === 0 || files.every((f) => f.status !== "pending")}
          startIcon={<CloudUpload />}
        >
          {uploading ? "Uploading..." : `Upload ${files.filter((f) => f.status === "pending").length} File(s)`}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

export default MultipartUploadDialog;
