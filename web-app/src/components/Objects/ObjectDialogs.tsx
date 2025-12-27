import React, { useState, useEffect } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  TextField,
  Table,
  TableBody,
  TableRow,
  TableCell,
  Box,
  Typography,
  Chip,
  IconButton,
  Alert,
  CircularProgress,
  InputAdornment,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Stack,
} from "@mui/material";
import { ContentCopy, Close, Add, Delete, Lock, LockOpen } from "@mui/icons-material";
import { useTranslation } from "react-i18next";
import apiClient from "../../api/client";
import type { ObjectInfo } from "../../api/types";

// Object Info Dialog
interface ObjectInfoDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  objectKey: string;
}

export const ObjectInfoDialog: React.FC<ObjectInfoDialogProps> = ({
  open,
  onClose,
  bucketName,
  objectKey,
}) => {
  const { t } = useTranslation();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [info, setInfo] = useState<ObjectInfo | null>(null);

  useEffect(() => {
    if (open && bucketName && objectKey) {
      loadObjectInfo();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open, bucketName, objectKey]);

  const loadObjectInfo = async () => {
    setLoading(true);
    setError(null);
    try {
      const data = await apiClient.getObjectInfo(bucketName, objectKey);
      setInfo(data);
    } catch (err: any) {
      setError(err.response?.data?.message || "Failed to load object info");
    } finally {
      setLoading(false);
    }
  };

  const formatSize = (bytes: number) => {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return `${(bytes / Math.pow(k, i)).toFixed(2)} ${sizes[i]}`;
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle>
        {t("objects.info.title")}
        <IconButton
          onClick={onClose}
          sx={{ position: "absolute", right: 8, top: 8 }}
        >
          <Close />
        </IconButton>
      </DialogTitle>
      <DialogContent dividers>
        {loading ? (
          <Box sx={{ display: "flex", justifyContent: "center", py: 4 }}>
            <CircularProgress />
          </Box>
        ) : error ? (
          <Alert severity="error">{error}</Alert>
        ) : info ? (
          <Table>
            <TableBody>
              <TableRow>
                <TableCell sx={{ fontWeight: 600 }}>
                  {t("objects.info.key")}
                </TableCell>
                <TableCell>{info.key}</TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ fontWeight: 600 }}>
                  {t("objects.info.size")}
                </TableCell>
                <TableCell>{formatSize(info.size)}</TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ fontWeight: 600 }}>
                  {t("objects.info.lastModified")}
                </TableCell>
                <TableCell>
                  {new Date(info.last_modified).toLocaleString()}
                </TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ fontWeight: 600 }}>
                  {t("objects.info.contentType")}
                </TableCell>
                <TableCell>{info.content_type || "unknown"}</TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ fontWeight: 600 }}>
                  {t("objects.info.etag")}
                </TableCell>
                <TableCell
                  sx={{ fontFamily: "monospace", fontSize: "0.875rem" }}
                >
                  {info.etag}
                </TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ fontWeight: 600 }}>
                  Encryption
                </TableCell>
                <TableCell>
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    {info.encrypted ? (
                      <>
                        <Lock fontSize="small" color={info.sse_type === "SSE-C" ? "warning" : "success"} />
                        <Chip
                          label={info.sse_type || "Encrypted"}
                          size="small"
                          color={info.sse_type === "SSE-C" ? "warning" : "success"}
                        />
                        <Typography variant="body2" color="text.secondary">
                          ({info.encryption_algorithm})
                        </Typography>
                        {info.original_size && info.original_size !== info.size && (
                          <Typography variant="body2" color="text.secondary">
                            Original: {formatSize(info.original_size)}
                          </Typography>
                        )}
                      </>
                    ) : (
                      <>
                        <LockOpen fontSize="small" color="disabled" />
                        <Typography variant="body2" color="text.secondary">
                          Not encrypted
                        </Typography>
                      </>
                    )}
                  </Box>
                </TableCell>
              </TableRow>
              {info.encrypted && info.sse_type === "SSE-C" && (
                <TableRow>
                  <TableCell sx={{ fontWeight: 600 }}>
                    Customer Key MD5
                  </TableCell>
                  <TableCell
                    sx={{ fontFamily: "monospace", fontSize: "0.875rem" }}
                  >
                    {info.sse_customer_key_md5}
                  </TableCell>
                </TableRow>
              )}
              {info.metadata && Object.keys(info.metadata).length > 0 && (
                <TableRow>
                  <TableCell sx={{ fontWeight: 600 }}>
                    {t("objects.info.metadata")}
                  </TableCell>
                  <TableCell>
                    <Box sx={{ display: "flex", flexWrap: "wrap", gap: 1 }}>
                      {Object.entries(info.metadata).map(([key, value]) => (
                        <Chip
                          key={key}
                          label={`${key}: ${value}`}
                          size="small"
                        />
                      ))}
                    </Box>
                  </TableCell>
                </TableRow>
              )}
            </TableBody>
          </Table>
        ) : null}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>{t("common.close")}</Button>
      </DialogActions>
    </Dialog>
  );
};

// Object Tags Dialog
interface ObjectTagsDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  objectKey: string;
  onSuccess?: () => void;
}

export const ObjectTagsDialog: React.FC<ObjectTagsDialogProps> = ({
  open,
  onClose,
  bucketName,
  objectKey,
  onSuccess,
}) => {
  const { t } = useTranslation();
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [tags, setTags] = useState<{ [key: string]: string }>({});
  const [newKey, setNewKey] = useState("");
  const [newValue, setNewValue] = useState("");

  useEffect(() => {
    if (open && bucketName && objectKey) {
      loadTags();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open, bucketName, objectKey]);

  const loadTags = async () => {
    setLoading(true);
    setError(null);
    try {
      const data = await apiClient.getObjectTags(bucketName, objectKey);
      setTags(data.tags || {});
    } catch (err: any) {
      setError(err.response?.data?.message || "Failed to load tags");
    } finally {
      setLoading(false);
    }
  };

  const handleAddTag = () => {
    if (newKey && newValue) {
      setTags({ ...tags, [newKey]: newValue });
      setNewKey("");
      setNewValue("");
    }
  };

  const handleRemoveTag = (key: string) => {
    const newTags = { ...tags };
    delete newTags[key];
    setTags(newTags);
  };

  const handleSave = async () => {
    setSaving(true);
    setError(null);
    try {
      await apiClient.setObjectTags(bucketName, objectKey, tags);
      onSuccess?.();
      onClose();
    } catch (err: any) {
      setError(err.response?.data?.message || "Failed to save tags");
    } finally {
      setSaving(false);
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>
        {t("objects.tags.title")}
        <IconButton
          onClick={onClose}
          sx={{ position: "absolute", right: 8, top: 8 }}
        >
          <Close />
        </IconButton>
      </DialogTitle>
      <DialogContent dividers>
        {loading ? (
          <Box sx={{ display: "flex", justifyContent: "center", py: 4 }}>
            <CircularProgress />
          </Box>
        ) : (
          <>
            {error && (
              <Alert severity="error" sx={{ mb: 2 }}>
                {error}
              </Alert>
            )}

            {/* Existing Tags */}
            <Box sx={{ mb: 3 }}>
              <Typography variant="subtitle2" sx={{ mb: 1 }}>
                {t("objects.tags.existing")}
              </Typography>
              {Object.keys(tags).length === 0 ? (
                <Typography variant="body2" color="text.secondary">
                  {t("objects.tags.noTags")}
                </Typography>
              ) : (
                <Stack spacing={1}>
                  {Object.entries(tags).map(([key, value]) => (
                    <Box
                      key={key}
                      sx={{
                        display: "flex",
                        alignItems: "center",
                        gap: 1,
                        p: 1,
                        border: "1px solid",
                        borderColor: "divider",
                        borderRadius: 1,
                      }}
                    >
                      <Typography variant="body2" sx={{ flex: 1 }}>
                        <strong>{key}:</strong> {value}
                      </Typography>
                      <IconButton
                        size="small"
                        onClick={() => handleRemoveTag(key)}
                      >
                        <Delete fontSize="small" />
                      </IconButton>
                    </Box>
                  ))}
                </Stack>
              )}
            </Box>

            {/* Add New Tag */}
            <Box>
              <Typography variant="subtitle2" sx={{ mb: 1 }}>
                {t("objects.tags.addNew")}
              </Typography>
              <Box sx={{ display: "flex", gap: 1, mb: 1 }}>
                <TextField
                  size="small"
                  placeholder={t("objects.tags.key")}
                  value={newKey}
                  onChange={(e) => setNewKey(e.target.value)}
                  fullWidth
                />
                <TextField
                  size="small"
                  placeholder={t("objects.tags.value")}
                  value={newValue}
                  onChange={(e) => setNewValue(e.target.value)}
                  fullWidth
                />
                <IconButton
                  onClick={handleAddTag}
                  disabled={!newKey || !newValue}
                  color="primary"
                >
                  <Add />
                </IconButton>
              </Box>
            </Box>
          </>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>{t("common.cancel")}</Button>
        <Button onClick={handleSave} variant="contained" disabled={saving}>
          {saving ? <CircularProgress size={20} /> : t("common.save")}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

// Copy Object Dialog
interface CopyObjectDialogProps {
  open: boolean;
  onClose: () => void;
  sourceBucket: string;
  sourceKey: string;
  availableBuckets: string[];
  onSuccess?: () => void;
}

export const CopyObjectDialog: React.FC<CopyObjectDialogProps> = ({
  open,
  onClose,
  sourceBucket,
  sourceKey,
  availableBuckets,
  onSuccess,
}) => {
  const { t } = useTranslation();
  const [destBucket, setDestBucket] = useState("");
  const [destKey, setDestKey] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    if (open) {
      setDestBucket(sourceBucket);
      setDestKey(sourceKey);
      setError(null);
    }
  }, [open, sourceBucket, sourceKey]);

  const handleCopy = async () => {
    if (!destBucket || !destKey) {
      setError(t("objects.copy.fillAllFields"));
      return;
    }

    setLoading(true);
    setError(null);
    try {
      await apiClient.copyObject({
        source_bucket: sourceBucket,
        source_key: sourceKey,
        destination_bucket: destBucket,
        destination_key: destKey,
      });
      onSuccess?.();
      onClose();
    } catch (err: any) {
      setError(err.response?.data?.message || "Failed to copy object");
    } finally {
      setLoading(false);
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>
        {t("objects.copy.title")}
        <IconButton
          onClick={onClose}
          sx={{ position: "absolute", right: 8, top: 8 }}
        >
          <Close />
        </IconButton>
      </DialogTitle>
      <DialogContent dividers>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }}>
            {error}
          </Alert>
        )}

        {/* Source */}
        <Box sx={{ mb: 3 }}>
          <Typography variant="subtitle2" sx={{ mb: 1 }}>
            {t("objects.copy.source")}
          </Typography>
          <Typography variant="body2" color="text.secondary">
            {sourceBucket}/{sourceKey}
          </Typography>
        </Box>

        {/* Destination */}
        <Box>
          <Typography variant="subtitle2" sx={{ mb: 1 }}>
            {t("objects.copy.destination")}
          </Typography>
          <FormControl fullWidth sx={{ mb: 2 }}>
            <InputLabel>{t("objects.copy.bucket")}</InputLabel>
            <Select
              value={destBucket}
              onChange={(e) => setDestBucket(e.target.value)}
              label={t("objects.copy.bucket")}
            >
              {availableBuckets.map((bucket) => (
                <MenuItem key={bucket} value={bucket}>
                  {bucket}
                </MenuItem>
              ))}
            </Select>
          </FormControl>
          <TextField
            fullWidth
            label={t("objects.copy.objectKey")}
            value={destKey}
            onChange={(e) => setDestKey(e.target.value)}
            placeholder={t("objects.copy.objectKeyPlaceholder")}
          />
        </Box>
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>{t("common.cancel")}</Button>
        <Button onClick={handleCopy} variant="contained" disabled={loading}>
          {loading ? <CircularProgress size={20} /> : t("objects.copy.copy")}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

// Presigned URL Dialog
interface PresignedUrlDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  objectKey: string;
}

export const PresignedUrlDialog: React.FC<PresignedUrlDialogProps> = ({
  open,
  onClose,
  bucketName,
  objectKey,
}) => {
  const { t } = useTranslation();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [url, setUrl] = useState("");
  const [expiresAt, setExpiresAt] = useState("");
  const [expiresIn, setExpiresIn] = useState(3600); // 1 hour default
  const [copied, setCopied] = useState(false);

  const generateUrl = async () => {
    setLoading(true);
    setError(null);
    try {
      const data = await apiClient.getPresignedUrl(
        bucketName,
        objectKey,
        expiresIn,
      );
      setUrl(data.url);
      setExpiresAt(data.expires_at);
    } catch (err: any) {
      setError(
        err.response?.data?.message || "Failed to generate presigned URL",
      );
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (open && bucketName && objectKey) {
      generateUrl();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open, bucketName, objectKey, expiresIn]);

  const handleCopy = () => {
    navigator.clipboard.writeText(url);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle>
        {t("objects.presignedUrl.title")}
        <IconButton
          onClick={onClose}
          sx={{ position: "absolute", right: 8, top: 8 }}
        >
          <Close />
        </IconButton>
      </DialogTitle>
      <DialogContent dividers>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }}>
            {error}
          </Alert>
        )}

        <FormControl fullWidth sx={{ mb: 3 }}>
          <InputLabel>{t("objects.presignedUrl.expiresIn")}</InputLabel>
          <Select
            value={expiresIn}
            onChange={(e) => setExpiresIn(Number(e.target.value))}
            label={t("objects.presignedUrl.expiresIn")}
          >
            <MenuItem value={300}>
              5 {t("objects.presignedUrl.minutes")}
            </MenuItem>
            <MenuItem value={900}>
              15 {t("objects.presignedUrl.minutes")}
            </MenuItem>
            <MenuItem value={1800}>
              30 {t("objects.presignedUrl.minutes")}
            </MenuItem>
            <MenuItem value={3600}>1 {t("objects.presignedUrl.hour")}</MenuItem>
            <MenuItem value={7200}>
              2 {t("objects.presignedUrl.hours")}
            </MenuItem>
            <MenuItem value={86400}>
              24 {t("objects.presignedUrl.hours")}
            </MenuItem>
          </Select>
        </FormControl>

        {loading ? (
          <Box sx={{ display: "flex", justifyContent: "center", py: 4 }}>
            <CircularProgress />
          </Box>
        ) : url ? (
          <>
            <TextField
              fullWidth
              multiline
              rows={4}
              value={url}
              InputProps={{
                readOnly: true,
                endAdornment: (
                  <InputAdornment position="end">
                    <IconButton onClick={handleCopy}>
                      <ContentCopy />
                    </IconButton>
                  </InputAdornment>
                ),
              }}
              sx={{ mb: 2 }}
            />
            {copied && (
              <Alert severity="success" sx={{ mb: 2 }}>
                {t("objects.presignedUrl.copied")}
              </Alert>
            )}
            <Typography variant="body2" color="text.secondary">
              {t("objects.presignedUrl.expiresAt")}:{" "}
              {new Date(expiresAt).toLocaleString()}
            </Typography>
          </>
        ) : null}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>{t("common.close")}</Button>
      </DialogActions>
    </Dialog>
  );
};
