import React, { useEffect, useState } from "react";
import {
  Box,
  Typography,
  Button,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Paper,
  IconButton,
  Checkbox,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Breadcrumbs,
  Link,
  Chip,
  Tooltip,
} from "@mui/material";
import {
  CloudUpload,
  Refresh,
  Folder,
  InsertDriveFile,
  Home,
  DeleteSweep,
  Lock,
  LockOpen,
} from "@mui/icons-material";
import ObjectActionsMenu from "../components/Objects/ObjectActionsMenu";
import {
  ObjectInfoDialog,
  ObjectTagsDialog,
  CopyObjectDialog,
  PresignedUrlDialog,
} from "../components/Objects/ObjectDialogs";
import ObjectVersionsDialog from "../components/Objects/ObjectVersionsDialog";
import ObjectRetentionDialog from "../components/Objects/ObjectRetentionDialog";
import MultipartUploadDialog from "../components/Objects/MultipartUploadDialog";
import EncryptedUploadDialog from "../components/Objects/EncryptedUploadDialog";
import EncryptedDownloadDialog from "../components/Objects/EncryptedDownloadDialog";
import FilePreview from "../components/Objects/FilePreview";
import apiClient from "../api/client";
import { useSearchParams } from "react-router-dom";
import { useAppDispatch } from "../hooks/useAppDispatch";
import { useAppSelector } from "../hooks/useAppSelector";
import {
  fetchObjects,
  uploadObject,
  deleteObject,
  downloadObject,
  batchDeleteObjects,
  setCurrentPrefix,
  selectObjects,
  selectPrefixes,
  selectObjectsLoading,
  selectObjectsError,
  selectUploadProgress,
  clearError,
} from "../store/objectsSlice";
import { fetchBuckets, selectBuckets } from "../store/bucketsSlice";
import { selectCan } from "../store/authSlice";
import Loader from "../components/Common/Loader";
import ErrorAlert from "../components/Common/ErrorAlert";
import FileUploader from "../components/Common/FileUploader";

const ObjectsPage: React.FC = () => {
  const [searchParams, setSearchParams] = useSearchParams();
  const dispatch = useAppDispatch();
  const objects = useAppSelector(selectObjects);
  const prefixes = useAppSelector(selectPrefixes);
  const loading = useAppSelector(selectObjectsLoading);
  const error = useAppSelector(selectObjectsError);
  const uploadProgress = useAppSelector(selectUploadProgress);
  const buckets = useAppSelector(selectBuckets);
  const canManageObjects = useAppSelector(selectCan("objects.manage"));

  const bucketFromUrl = searchParams.get("bucket") || "";
  const [selectedBucket, setSelectedBucket] = useState("");
  const [currentPrefix, setCurrentPrefixState] = useState(
    searchParams.get("prefix") || ""
  );

  // Validate bucket from URL against available buckets
  useEffect(() => {
    if (bucketFromUrl) {
      const bucketExists = buckets.some((b) => b.name === bucketFromUrl);
      if (bucketExists) {
        setSelectedBucket(bucketFromUrl);
      } else if (buckets.length > 0) {
        // Bucket doesn't exist, clear selection
        setSelectedBucket("");
        setSearchParams({ bucket: "", prefix: "" });
      }
    }
  }, [bucketFromUrl, buckets, setSearchParams]);
  const [uploadDialogOpen, setUploadDialogOpen] = useState(false);
  const [selectedKeys, setSelectedKeys] = useState<Set<string>>(new Set());
  const [deleteDialogOpen, setDeleteDialogOpen] = useState(false);
  const [selectedObjectKey, setSelectedObjectKey] = useState<string>("");
  const [infoDialogOpen, setInfoDialogOpen] = useState(false);
  const [tagsDialogOpen, setTagsDialogOpen] = useState(false);
  const [copyDialogOpen, setCopyDialogOpen] = useState(false);
  const [presignedUrlDialogOpen, setPresignedUrlDialogOpen] = useState(false);
  const [versionsDialogOpen, setVersionsDialogOpen] = useState(false);
  const [retentionDialogOpen, setRetentionDialogOpen] = useState(false);
  const [multipartUploadOpen, setMultipartUploadOpen] = useState(false);
  const [encryptedUploadOpen, setEncryptedUploadOpen] = useState(false);
  const [encryptedDownloadOpen, setEncryptedDownloadOpen] = useState(false);
  const [previewOpen, setPreviewOpen] = useState(false);
  const [previewBlobUrl, setPreviewBlobUrl] = useState<string | null>(null);
  const [previewLoading, setPreviewLoading] = useState(false);
  const [previewObject, setPreviewObject] = useState<{
    url: string;
    name: string;
    size?: number;
    isEncrypted?: boolean;
    encryptionType?: string;
  } | null>(null);

  const revokePreviewBlob = () => {
    if (previewBlobUrl) {
      URL.revokeObjectURL(previewBlobUrl);
      setPreviewBlobUrl(null);
    }
  };

  const handlePreviewClose = () => {
    setPreviewOpen(false);
    revokePreviewBlob();
    setPreviewObject(null);
    setPreviewLoading(false);
  };

  useEffect(() => {
    dispatch(fetchBuckets());
  }, [dispatch]);

  useEffect(() => {
    if (selectedBucket) {
      dispatch(
        fetchObjects({
          bucketName: selectedBucket,
          params: { prefix: currentPrefix },
        }),
      );
      dispatch(setCurrentPrefix(currentPrefix));
      setSearchParams({ bucket: selectedBucket, prefix: currentPrefix || "" });
    }
  }, [selectedBucket, currentPrefix, dispatch]);

  const handleBucketChange = (bucket: string) => {
    setSelectedBucket(bucket);
    setCurrentPrefixState("");
    setSelectedKeys(new Set());
  };

  const handlePrefixClick = (prefix: string) => {
    setCurrentPrefixState(prefix);
    setSelectedKeys(new Set());
  };

  const handleUploadFiles = async (files: File[]) => {
    if (!selectedBucket) return;

    for (const file of files) {
      const key = currentPrefix ? `${currentPrefix}${file.name}` : file.name;
      await dispatch(uploadObject({ bucketName: selectedBucket, key, file }));
    }
    setUploadDialogOpen(false);
  };

  const handleDownload = async (key: string, sseCustomerKey?: string) => {
    if (!selectedBucket) return;
    await dispatch(downloadObject({ bucketName: selectedBucket, key, sseCustomerKey }));
  };

  const handleDownloadWithEncryptionCheck = (key: string) => {
    if (!selectedBucket) return;
    // Find the object to check if it's SSE-C encrypted
    const object = objects.find((obj) => obj.key === key);
    if (object?.sse_type === "SSE-C") {
      setSelectedObjectKey(key);
      setEncryptedDownloadOpen(true);
    } else {
      handleDownload(key);
    }
  };

  const handleEncryptedDownload = async (sseCustomerKey: string) => {
    await handleDownload(selectedObjectKey, sseCustomerKey);
  };

  const handlePreview = async (key: string) => {
    if (!selectedBucket) return;
    const object = objects.find((obj) => obj.key === key);
    if (!object) return;

    setSelectedObjectKey(key);
    revokePreviewBlob();

    const displayName = key.split("/").pop() || key;
    const basePreview = {
      name: displayName,
      size: object.size,
      isEncrypted: object.encrypted || !!object.sse_type,
      encryptionType: object.sse_type,
    };

    if (object.sse_type === "SSE-C") {
      setPreviewObject({ url: "", ...basePreview, isEncrypted: true });
      setPreviewOpen(true);
      return;
    }

    setPreviewLoading(true);

    try {
      const blob = await apiClient.downloadObject(selectedBucket, key);
      const url = URL.createObjectURL(blob);
      setPreviewBlobUrl(url);
      setPreviewObject({ url, ...basePreview });
      setPreviewOpen(true);
    } catch (error) {
      console.error("Failed to load preview:", error);
      setPreviewObject({ url: "", ...basePreview });
      setPreviewOpen(true);
    } finally {
      setPreviewLoading(false);
    }
  };

  const handleDelete = async (key: string) => {
    if (!selectedBucket) return;
    await dispatch(deleteObject({ bucketName: selectedBucket, key }));
  };

  const handleObjectAction = (key: string, action: string) => {
    setSelectedObjectKey(key);
    switch (action) {
      case "preview":
        handlePreview(key);
        break;
      case "info":
        setInfoDialogOpen(true);
        break;
      case "download":
        handleDownloadWithEncryptionCheck(key);
        break;
      case "copy":
        setCopyDialogOpen(true);
        break;
      case "tags":
        setTagsDialogOpen(true);
        break;
      case "presignedUrl":
        setPresignedUrlDialogOpen(true);
        break;
      case "versions":
        setVersionsDialogOpen(true);
        break;
      case "retention":
        setRetentionDialogOpen(true);
        break;
      case "delete":
        handleDelete(key);
        break;
    }
  };

  const handleDialogSuccess = () => {
    if (selectedBucket) {
      dispatch(
        fetchObjects({
          bucketName: selectedBucket,
          params: { prefix: currentPrefix },
        }),
      );
    }
  };

  const handleBatchDelete = async () => {
    if (!selectedBucket || selectedKeys.size === 0) return;
    await dispatch(
      batchDeleteObjects({
        bucketName: selectedBucket,
        keys: Array.from(selectedKeys),
      }),
    );
    setSelectedKeys(new Set());
    setDeleteDialogOpen(false);
  };

  const handleSelectAll = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (event.target.checked) {
      const newSelected = new Set((objects || []).map((obj) => obj.key));
      setSelectedKeys(newSelected);
    } else {
      setSelectedKeys(new Set());
    }
  };

  const handleSelectOne = (key: string) => {
    const newSelected = new Set(selectedKeys);
    if (newSelected.has(key)) {
      newSelected.delete(key);
    } else {
      newSelected.add(key);
    }
    setSelectedKeys(newSelected);
  };

  const formatSize = (bytes: number) => {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + " " + sizes[i];
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString();
  };

  const getBreadcrumbs = () => {
    if (!currentPrefix) return [];
    const parts = currentPrefix.split("/").filter((p) => p);
    return parts.map((part, index) => ({
      label: part,
      path: parts.slice(0, index + 1).join("/") + "/",
    }));
  };

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', flex: 1 }}>
      <Box
        sx={{
          display: "flex",
          justifyContent: "space-between",
          alignItems: "center",
          mb: 3,
        }}
      >
        <Typography variant="h4">Objects</Typography>
        <Box>
          <IconButton
            onClick={() =>
              selectedBucket &&
              dispatch(fetchObjects({ bucketName: selectedBucket }))
            }
            sx={{ mr: 1 }}
          >
            <Refresh />
          </IconButton>
          {selectedKeys.size > 0 && (
            <Button
              variant="outlined"
              color="error"
              startIcon={<DeleteSweep />}
              onClick={() => setDeleteDialogOpen(true)}
              sx={{ mr: 1 }}
            >
              Delete Selected ({selectedKeys.size})
            </Button>
          )}
          <Button
            variant="outlined"
            startIcon={<CloudUpload />}
            onClick={() => setUploadDialogOpen(true)}
            disabled={!selectedBucket || !canManageObjects}
            sx={{ mr: 1 }}
          >
            Quick Upload
          </Button>
          <Button
            variant="outlined"
            startIcon={<Lock />}
            onClick={() => setEncryptedUploadOpen(true)}
            disabled={!selectedBucket || !canManageObjects}
            sx={{ mr: 1 }}
            color="warning"
          >
            Encrypted Upload
          </Button>
          <Button
            variant="contained"
            startIcon={<CloudUpload />}
            onClick={() => setMultipartUploadOpen(true)}
            disabled={!selectedBucket || !canManageObjects}
          >
            Upload Large Files
          </Button>
        </Box>
      </Box>

      <ErrorAlert error={error} onClose={() => dispatch(clearError())} />

      <Paper sx={{ p: 2, mb: 2 }}>
        <FormControl fullWidth>
          <InputLabel>Select Bucket</InputLabel>
          <Select
            value={selectedBucket}
            onChange={(e) => handleBucketChange(e.target.value)}
            label="Select Bucket"
          >
            <MenuItem value="">
              <em>Select a bucket</em>
            </MenuItem>
            {buckets.map((bucket) => (
              <MenuItem key={bucket.name} value={bucket.name}>
                {bucket.name}
              </MenuItem>
            ))}
          </Select>
        </FormControl>

        {selectedBucket && (
          <Box sx={{ mt: 2 }}>
            <Breadcrumbs>
              <Link
                component="button"
                variant="body2"
                onClick={() => setCurrentPrefixState("")}
                sx={{ display: "flex", alignItems: "center" }}
              >
                <Home sx={{ mr: 0.5, fontSize: 20 }} />
                {selectedBucket}
              </Link>
              {getBreadcrumbs().map((crumb, index) => (
                <Link
                  key={index}
                  component="button"
                  variant="body2"
                  onClick={() => setCurrentPrefixState(crumb.path)}
                >
                  {crumb.label}
                </Link>
              ))}
            </Breadcrumbs>
          </Box>
        )}
      </Paper>

      {!selectedBucket ? (
        <Box sx={{ textAlign: "center", py: 8 }}>
          <Folder sx={{ fontSize: 100, color: "text.secondary", mb: 2 }} />
          <Typography variant="h6" color="text.secondary">
            Select a bucket to browse objects
          </Typography>
        </Box>
      ) : loading ? (
        <Loader message="Loading objects..." />
      ) : (
        <TableContainer component={Paper}>
          <Table>
            <TableHead>
              <TableRow>
                <TableCell padding="checkbox">
                  <Checkbox
                    indeterminate={
                      selectedKeys.size > 0 &&
                      selectedKeys.size < (objects || []).length
                    }
                    checked={
                      (objects || []).length > 0 &&
                      selectedKeys.size === (objects || []).length
                    }
                    onChange={handleSelectAll}
                  />
                </TableCell>
                <TableCell>Name</TableCell>
                <TableCell>Size</TableCell>
                <TableCell>Last Modified</TableCell>
                <TableCell>Type</TableCell>
                <TableCell align="center">Encryption</TableCell>
                <TableCell align="right">Actions</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {(prefixes || []).map((prefix) => (
                <TableRow
                  key={prefix}
                  hover
                  onClick={() => handlePrefixClick(prefix)}
                  sx={{ cursor: "pointer" }}
                >
                  <TableCell padding="checkbox"></TableCell>
                  <TableCell>
                    <Box sx={{ display: "flex", alignItems: "center" }}>
                      <Folder sx={{ mr: 1, color: "primary.main" }} />
                      {prefix
                        .split("/")
                        .filter((p) => p)
                        .pop()}
                    </Box>
                  </TableCell>
                  <TableCell>-</TableCell>
                  <TableCell>-</TableCell>
                  <TableCell>
                    <Chip label="Folder" size="small" />
                  </TableCell>
                  <TableCell align="center">-</TableCell>
                  <TableCell></TableCell>
                </TableRow>
              ))}
              {(objects || []).map((object) => (
                <TableRow key={object.key} hover>
                  <TableCell padding="checkbox">
                    <Checkbox
                      checked={selectedKeys.has(object.key)}
                      onChange={() => handleSelectOne(object.key)}
                    />
                  </TableCell>
                  <TableCell>
                    <Box sx={{ display: "flex", alignItems: "center" }}>
                      <InsertDriveFile
                        sx={{ mr: 1, color: "text.secondary" }}
                      />
                      {object.key.split("/").pop()}
                    </Box>
                  </TableCell>
                  <TableCell>{formatSize(object.size)}</TableCell>
                  <TableCell>{formatDate(object.last_modified)}</TableCell>
                  <TableCell>
                    <Chip
                      label={object.content_type || "unknown"}
                      size="small"
                    />
                  </TableCell>
                  <TableCell align="center">
                    {object.encrypted ? (
                      <Tooltip
                        title={`${object.sse_type} (${object.encryption_algorithm})`}
                        arrow
                      >
                        <Lock
                          fontSize="small"
                          color={object.sse_type === "SSE-C" ? "warning" : "success"}
                        />
                      </Tooltip>
                    ) : (
                      <Tooltip title="Not encrypted" arrow>
                        <LockOpen fontSize="small" color="disabled" />
                      </Tooltip>
                    )}
                  </TableCell>
                  <TableCell align="right">
                    <ObjectActionsMenu
                      onPreview={() => handleObjectAction(object.key, "preview")}
                      onInfo={() => handleObjectAction(object.key, "info")}
                      onDownload={() =>
                        handleObjectAction(object.key, "download")
                      }
                      onCopy={() => handleObjectAction(object.key, "copy")}
                      onTags={() => handleObjectAction(object.key, "tags")}
                      onPresignedUrl={() =>
                        handleObjectAction(object.key, "presignedUrl")
                      }
                      onVersions={() => handleObjectAction(object.key, "versions")}
                      onRetention={() => handleObjectAction(object.key, "retention")}
                      onDelete={() => handleObjectAction(object.key, "delete")}
                      readOnly={!canManageObjects}
                    />
                  </TableCell>
                </TableRow>
              ))}
              {(objects || []).length === 0 &&
                (prefixes || []).length === 0 && (
                  <TableRow>
                    <TableCell colSpan={7} align="center" sx={{ py: 4 }}>
                      <Typography variant="body2" color="text.secondary">
                        No objects found
                      </Typography>
                    </TableCell>
                  </TableRow>
                )}
            </TableBody>
          </Table>
        </TableContainer>
      )}

      {/* Upload Dialog */}
      <Dialog
        open={uploadDialogOpen}
        onClose={() => setUploadDialogOpen(false)}
        maxWidth="md"
        fullWidth
      >
        <DialogTitle>Upload Files</DialogTitle>
        <DialogContent>
          <FileUploader
            onFilesSelected={handleUploadFiles}
            uploadProgress={uploadProgress}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setUploadDialogOpen(false)}>Close</Button>
        </DialogActions>
      </Dialog>

      {/* Delete Confirmation Dialog */}
      <Dialog
        open={deleteDialogOpen}
        onClose={() => setDeleteDialogOpen(false)}
      >
        <DialogTitle>Delete Objects</DialogTitle>
        <DialogContent>
          <Typography>
            Are you sure you want to delete {selectedKeys.size} object(s)?
          </Typography>
          <Typography color="error" sx={{ mt: 2 }}>
            This action cannot be undone.
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setDeleteDialogOpen(false)}>Cancel</Button>
          <Button onClick={handleBatchDelete} color="error" variant="contained">
            Delete
          </Button>
        </DialogActions>
      </Dialog>

      {/* Object Info Dialog */}
      <ObjectInfoDialog
        open={infoDialogOpen}
        onClose={() => setInfoDialogOpen(false)}
        bucketName={selectedBucket}
        objectKey={selectedObjectKey}
      />

      {/* Object Tags Dialog */}
      <ObjectTagsDialog
        open={tagsDialogOpen}
        onClose={() => setTagsDialogOpen(false)}
        bucketName={selectedBucket}
        objectKey={selectedObjectKey}
        onSuccess={handleDialogSuccess}
      />

      {/* Copy Object Dialog */}
      <CopyObjectDialog
        open={copyDialogOpen}
        onClose={() => setCopyDialogOpen(false)}
        sourceBucket={selectedBucket}
        sourceKey={selectedObjectKey}
        availableBuckets={buckets.map((b) => b.name)}
        onSuccess={handleDialogSuccess}
      />

      {/* Presigned URL Dialog */}
      <PresignedUrlDialog
        open={presignedUrlDialogOpen}
        onClose={() => setPresignedUrlDialogOpen(false)}
        bucketName={selectedBucket}
        objectKey={selectedObjectKey}
      />

      {/* Object Versions Dialog */}
      <ObjectVersionsDialog
        open={versionsDialogOpen}
        onClose={() => setVersionsDialogOpen(false)}
        bucketName={selectedBucket}
        objectKey={selectedObjectKey}
        onSuccess={handleDialogSuccess}
      />

      {/* Object Retention Dialog */}
      <ObjectRetentionDialog
        open={retentionDialogOpen}
        onClose={() => setRetentionDialogOpen(false)}
        bucketName={selectedBucket}
        objectKey={selectedObjectKey}
        onSuccess={handleDialogSuccess}
      />

      {/* Multipart Upload Dialog */}
      <MultipartUploadDialog
        open={multipartUploadOpen}
        onClose={() => setMultipartUploadOpen(false)}
        bucketName={selectedBucket}
        currentPrefix={currentPrefix}
        onSuccess={handleDialogSuccess}
      />

      {/* Encrypted Upload Dialog */}
      <EncryptedUploadDialog
        open={encryptedUploadOpen}
        onClose={() => setEncryptedUploadOpen(false)}
        bucketName={selectedBucket}
        currentPrefix={currentPrefix}
        onSuccess={handleDialogSuccess}
      />

      {/* Encrypted Download Dialog */}
      <EncryptedDownloadDialog
        open={encryptedDownloadOpen}
        onClose={() => setEncryptedDownloadOpen(false)}
        bucketName={selectedBucket}
        objectKey={selectedObjectKey}
        onDownload={handleEncryptedDownload}
      />

      {/* File Preview Dialog */}
      {previewObject && (
        <FilePreview
          open={previewOpen}
          onClose={handlePreviewClose}
          fileUrl={previewLoading ? "" : previewObject.url}
          fileName={previewObject.name}
          fileSize={previewObject.size}
          isEncrypted={previewObject.isEncrypted}
          encryptionType={previewObject.encryptionType}
          onDownload={() => handleDownloadWithEncryptionCheck(selectedObjectKey)}
        />
      )}
    </Box>
  );
};

export default ObjectsPage;
