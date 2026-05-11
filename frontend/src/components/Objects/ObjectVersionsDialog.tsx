import React, { useEffect, useState } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Paper,
  IconButton,
  Chip,
  Typography,
  Box,
  CircularProgress,
  Tooltip,
  Alert,
} from "@mui/material";
import {
  Download,
  RestoreFromTrash,
  Delete,
  History,
  CheckCircle,
} from "@mui/icons-material";
import apiClient from "../../api/client";
import type { ObjectVersion } from "../../api/types";

interface ObjectVersionsDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  objectKey: string;
  onSuccess?: () => void;
}

const ObjectVersionsDialog: React.FC<ObjectVersionsDialogProps> = ({
  open,
  onClose,
  bucketName,
  objectKey,
  onSuccess,
}) => {
  const [versions, setVersions] = useState<ObjectVersion[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [actionLoading, setActionLoading] = useState<string | null>(null);

  const fetchVersions = async () => {
    if (!bucketName || !objectKey) return;

    setLoading(true);
    setError(null);
    try {
      const response = await apiClient.listObjectVersions(bucketName, objectKey);
      setVersions(response.versions || []);
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to fetch versions");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (open) {
      fetchVersions();
    }
  }, [open, bucketName, objectKey]);

  const handleDownloadVersion = async (versionId: string) => {
    setActionLoading(versionId);
    try {
      const blob = await apiClient.getObjectVersion(bucketName, objectKey, versionId);
      const url = window.URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.href = url;
      link.download = `${objectKey.split("/").pop()}_${versionId}`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      window.URL.revokeObjectURL(url);
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to download version");
    } finally {
      setActionLoading(null);
    }
  };

  const handleRestoreVersion = async (versionId: string) => {
    setActionLoading(versionId);
    try {
      await apiClient.restoreObjectVersion(bucketName, objectKey, versionId);
      await fetchVersions();
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to restore version");
    } finally {
      setActionLoading(null);
    }
  };

  const handleDeleteVersion = async (versionId: string) => {
    if (!window.confirm(`Delete version ${versionId}? This cannot be undone.`)) {
      return;
    }

    setActionLoading(versionId);
    try {
      await apiClient.deleteObjectVersion(bucketName, objectKey, versionId);
      await fetchVersions();
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to delete version");
    } finally {
      setActionLoading(null);
    }
  };

  const formatDate = (timestamp: number) => {
    return new Date(timestamp * 1000).toLocaleString();
  };

  const formatSize = (bytes: number) => {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + " " + sizes[i];
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle sx={{ display: "flex", alignItems: "center", gap: 1 }}>
        <History color="primary" />
        Version History
      </DialogTitle>
      <DialogContent>
        <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
          Object: <strong>{objectKey}</strong>
        </Typography>

        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        {loading ? (
          <Box sx={{ display: "flex", justifyContent: "center", py: 4 }}>
            <CircularProgress />
          </Box>
        ) : versions.length === 0 ? (
          <Typography color="text.secondary" sx={{ textAlign: "center", py: 4 }}>
            No versions found. Enable versioning on the bucket to track object versions.
          </Typography>
        ) : (
          <TableContainer component={Paper} variant="outlined">
            <Table size="small">
              <TableHead>
                <TableRow>
                  <TableCell>Version ID</TableCell>
                  <TableCell>Last Modified</TableCell>
                  <TableCell>Size</TableCell>
                  <TableCell>Status</TableCell>
                  <TableCell align="right">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {versions.map((version) => (
                  <TableRow
                    key={version.version_id}
                    sx={{ bgcolor: version.is_latest ? "action.selected" : "inherit" }}
                  >
                    <TableCell>
                      <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                        <Typography variant="body2" sx={{ fontFamily: "monospace" }}>
                          {version.version_id.substring(0, 16)}...
                        </Typography>
                        {version.is_latest && (
                          <Chip
                            label="Latest"
                            size="small"
                            color="primary"
                            icon={<CheckCircle />}
                          />
                        )}
                      </Box>
                    </TableCell>
                    <TableCell>{formatDate(version.last_modified)}</TableCell>
                    <TableCell>{formatSize(version.size)}</TableCell>
                    <TableCell>
                      {version.is_delete_marker ? (
                        <Chip label="Delete Marker" size="small" color="error" />
                      ) : (
                        <Chip label="Active" size="small" color="success" />
                      )}
                    </TableCell>
                    <TableCell align="right">
                      {!version.is_delete_marker && (
                        <>
                          <Tooltip title="Download">
                            <IconButton
                              size="small"
                              onClick={() => handleDownloadVersion(version.version_id)}
                              disabled={actionLoading === version.version_id}
                            >
                              {actionLoading === version.version_id ? (
                                <CircularProgress size={16} />
                              ) : (
                                <Download fontSize="small" />
                              )}
                            </IconButton>
                          </Tooltip>
                          {!version.is_latest && (
                            <Tooltip title="Restore as Latest">
                              <IconButton
                                size="small"
                                onClick={() => handleRestoreVersion(version.version_id)}
                                disabled={actionLoading === version.version_id}
                                color="primary"
                              >
                                <RestoreFromTrash fontSize="small" />
                              </IconButton>
                            </Tooltip>
                          )}
                        </>
                      )}
                      {!version.is_latest && (
                        <Tooltip title="Delete Version">
                          <IconButton
                            size="small"
                            onClick={() => handleDeleteVersion(version.version_id)}
                            disabled={actionLoading === version.version_id}
                            color="error"
                          >
                            <Delete fontSize="small" />
                          </IconButton>
                        </Tooltip>
                      )}
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Close</Button>
      </DialogActions>
    </Dialog>
  );
};

export default ObjectVersionsDialog;
