import React, { useEffect, useState } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  Tabs,
  Tab,
  TextField,
  Switch,
  FormControlLabel,
  IconButton,
  Paper,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  Alert,
  CircularProgress,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Chip,
  Divider,
} from "@mui/material";
import {
  Settings,
  LocalOffer,
  Lock,
  Schedule,
  Delete,
  Add,
  Security,
  History,
} from "@mui/icons-material";
import apiClient from "../../api/client";
import type {
  BucketEncryptionConfig,
  LifecycleConfiguration,
  LifecycleRule,
} from "../../api/types";

interface TabPanelProps {
  children?: React.ReactNode;
  index: number;
  value: number;
}

const TabPanel: React.FC<TabPanelProps> = ({ children, value, index }) => (
  <div hidden={value !== index} style={{ paddingTop: 16 }}>
    {value === index && children}
  </div>
);

interface BucketSettingsDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  onSuccess?: () => void;
}

const BucketSettingsDialog: React.FC<BucketSettingsDialogProps> = ({
  open,
  onClose,
  bucketName,
  onSuccess,
}) => {
  const [tabValue, setTabValue] = useState(0);
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  // Versioning
  const [versioningEnabled, setVersioningEnabled] = useState(false);

  // Tags
  const [tags, setTags] = useState<{ [key: string]: string }>({});
  const [newTagKey, setNewTagKey] = useState("");
  const [newTagValue, setNewTagValue] = useState("");

  // Encryption
  const [encryptionConfig, setEncryptionConfig] = useState<BucketEncryptionConfig>({
    enabled: false,
    algorithm: "AES256",
  });

  // Lifecycle
  const [lifecycleConfig, setLifecycleConfig] = useState<LifecycleConfiguration>({
    rules: [],
  });
  const [newRuleId, setNewRuleId] = useState("");
  const [newRulePrefix, setNewRulePrefix] = useState("");
  const [newRuleDays, setNewRuleDays] = useState(30);

  const fetchSettings = async () => {
    if (!bucketName) return;

    setLoading(true);
    setError(null);

    try {
      // Fetch versioning
      try {
        const versioningData = await apiClient.getBucketVersioning(bucketName);
        setVersioningEnabled(versioningData.enabled);
      } catch (e) {
        console.warn("Failed to fetch versioning:", e);
      }

      // Fetch tags
      try {
        const tagsData = await apiClient.getBucketTags(bucketName);
        setTags(tagsData.tags || {});
      } catch (e) {
        console.warn("Failed to fetch tags:", e);
      }

      // Fetch encryption
      try {
        const encryptionData = await apiClient.getBucketEncryption(bucketName);
        setEncryptionConfig(encryptionData);
      } catch (e) {
        console.warn("Failed to fetch encryption:", e);
      }

      // Fetch lifecycle
      try {
        const lifecycleData = await apiClient.getBucketLifecycle(bucketName);
        setLifecycleConfig(lifecycleData);
      } catch (e) {
        console.warn("Failed to fetch lifecycle:", e);
      }
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to load settings");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (open) {
      fetchSettings();
    }
  }, [open, bucketName]);

  const handleSaveVersioning = async () => {
    setSaving(true);
    try {
      await apiClient.setBucketVersioning(bucketName, versioningEnabled);
      setSuccess("Versioning settings saved");
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to save versioning");
    } finally {
      setSaving(false);
    }
  };

  const handleAddTag = () => {
    if (newTagKey && newTagValue) {
      setTags((prev) => ({ ...prev, [newTagKey]: newTagValue }));
      setNewTagKey("");
      setNewTagValue("");
    }
  };

  const handleRemoveTag = (key: string) => {
    setTags((prev) => {
      const newTags = { ...prev };
      delete newTags[key];
      return newTags;
    });
  };

  const handleSaveTags = async () => {
    setSaving(true);
    try {
      await apiClient.setBucketTags(bucketName, tags);
      setSuccess("Tags saved successfully");
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to save tags");
    } finally {
      setSaving(false);
    }
  };

  const handleSaveEncryption = async () => {
    setSaving(true);
    try {
      await apiClient.setBucketEncryption(bucketName, encryptionConfig);
      setSuccess("Encryption settings saved");
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to save encryption");
    } finally {
      setSaving(false);
    }
  };

  const handleAddLifecycleRule = () => {
    if (newRuleId) {
      const newRule: LifecycleRule = {
        id: newRuleId,
        status: "Enabled",
        filter: { prefix: newRulePrefix },
        actions: [{ type: "Expiration", days: newRuleDays }],
      };
      setLifecycleConfig((prev) => ({
        rules: [...prev.rules, newRule],
      }));
      setNewRuleId("");
      setNewRulePrefix("");
      setNewRuleDays(30);
    }
  };

  const handleRemoveLifecycleRule = (ruleId: string) => {
    setLifecycleConfig((prev) => ({
      rules: prev.rules.filter((r) => r.id !== ruleId),
    }));
  };

  const handleSaveLifecycle = async () => {
    setSaving(true);
    try {
      await apiClient.setBucketLifecycle(bucketName, lifecycleConfig);
      setSuccess("Lifecycle rules saved");
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to save lifecycle rules");
    } finally {
      setSaving(false);
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle sx={{ display: "flex", alignItems: "center", gap: 1 }}>
        <Settings color="primary" />
        Bucket Settings: {bucketName}
      </DialogTitle>
      <DialogContent>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}
        {success && (
          <Alert severity="success" sx={{ mb: 2 }} onClose={() => setSuccess(null)}>
            {success}
          </Alert>
        )}

        {loading ? (
          <Box sx={{ display: "flex", justifyContent: "center", py: 4 }}>
            <CircularProgress />
          </Box>
        ) : (
          <>
            <Tabs value={tabValue} onChange={(_, v) => setTabValue(v)}>
              <Tab icon={<History />} label="Versioning" />
              <Tab icon={<LocalOffer />} label="Tags" />
              <Tab icon={<Lock />} label="Encryption" />
              <Tab icon={<Schedule />} label="Lifecycle" />
            </Tabs>

            {/* Versioning Tab */}
            <TabPanel value={tabValue} index={0}>
              <Paper variant="outlined" sx={{ p: 3 }}>
                <Typography variant="h6" gutterBottom>
                  Object Versioning
                </Typography>
                <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                  When enabled, multiple versions of objects are kept. You can restore
                  or delete specific versions.
                </Typography>
                <FormControlLabel
                  control={
                    <Switch
                      checked={versioningEnabled}
                      onChange={(e) => setVersioningEnabled(e.target.checked)}
                    />
                  }
                  label={versioningEnabled ? "Enabled" : "Disabled"}
                />
                <Box sx={{ mt: 2 }}>
                  <Button
                    variant="contained"
                    onClick={handleSaveVersioning}
                    disabled={saving}
                  >
                    {saving ? "Saving..." : "Save Versioning"}
                  </Button>
                </Box>
              </Paper>
            </TabPanel>

            {/* Tags Tab */}
            <TabPanel value={tabValue} index={1}>
              <Paper variant="outlined" sx={{ p: 3 }}>
                <Typography variant="h6" gutterBottom>
                  Bucket Tags
                </Typography>
                <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                  Add tags to organize and categorize your bucket. Maximum 50 tags.
                </Typography>

                <Box sx={{ display: "flex", gap: 1, mb: 2 }}>
                  <TextField
                    label="Key"
                    size="small"
                    value={newTagKey}
                    onChange={(e) => setNewTagKey(e.target.value)}
                    sx={{ flex: 1 }}
                  />
                  <TextField
                    label="Value"
                    size="small"
                    value={newTagValue}
                    onChange={(e) => setNewTagValue(e.target.value)}
                    sx={{ flex: 1 }}
                  />
                  <Button
                    variant="outlined"
                    onClick={handleAddTag}
                    disabled={!newTagKey || !newTagValue}
                    startIcon={<Add />}
                  >
                    Add
                  </Button>
                </Box>

                <List dense>
                  {Object.entries(tags).map(([key, value]) => (
                    <ListItem key={key}>
                      <ListItemText
                        primary={
                          <Box sx={{ display: "flex", gap: 1 }}>
                            <Chip label={key} size="small" />
                            <Typography variant="body2">=</Typography>
                            <Chip label={value} size="small" variant="outlined" />
                          </Box>
                        }
                      />
                      <ListItemSecondaryAction>
                        <IconButton size="small" onClick={() => handleRemoveTag(key)}>
                          <Delete fontSize="small" />
                        </IconButton>
                      </ListItemSecondaryAction>
                    </ListItem>
                  ))}
                </List>

                <Box sx={{ mt: 2 }}>
                  <Button
                    variant="contained"
                    onClick={handleSaveTags}
                    disabled={saving}
                  >
                    {saving ? "Saving..." : "Save Tags"}
                  </Button>
                </Box>
              </Paper>
            </TabPanel>

            {/* Encryption Tab */}
            <TabPanel value={tabValue} index={2}>
              <Paper variant="outlined" sx={{ p: 3 }}>
                <Typography variant="h6" gutterBottom>
                  <Security sx={{ verticalAlign: "middle", mr: 1 }} />
                  Server-Side Encryption
                </Typography>
                <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                  Enable encryption at rest for all objects in this bucket.
                </Typography>

                <FormControlLabel
                  control={
                    <Switch
                      checked={encryptionConfig.enabled}
                      onChange={(e) =>
                        setEncryptionConfig((prev) => ({
                          ...prev,
                          enabled: e.target.checked,
                        }))
                      }
                    />
                  }
                  label="Enable Default Encryption"
                />

                {encryptionConfig.enabled && (
                  <Box sx={{ mt: 2 }}>
                    <FormControl fullWidth size="small">
                      <InputLabel>Algorithm</InputLabel>
                      <Select
                        value={encryptionConfig.algorithm}
                        label="Algorithm"
                        onChange={(e) =>
                          setEncryptionConfig((prev) => ({
                            ...prev,
                            algorithm: e.target.value as "AES256" | "aws:kms",
                          }))
                        }
                      >
                        <MenuItem value="AES256">AES-256 (SSE-S3)</MenuItem>
                        <MenuItem value="aws:kms">AWS KMS (SSE-KMS)</MenuItem>
                      </Select>
                    </FormControl>
                  </Box>
                )}

                <Box sx={{ mt: 2 }}>
                  <Button
                    variant="contained"
                    onClick={handleSaveEncryption}
                    disabled={saving}
                  >
                    {saving ? "Saving..." : "Save Encryption"}
                  </Button>
                </Box>
              </Paper>
            </TabPanel>

            {/* Lifecycle Tab */}
            <TabPanel value={tabValue} index={3}>
              <Paper variant="outlined" sx={{ p: 3 }}>
                <Typography variant="h6" gutterBottom>
                  Lifecycle Rules
                </Typography>
                <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                  Automatically manage objects based on age. Rules run daily.
                </Typography>

                <Divider sx={{ my: 2 }} />

                <Typography variant="subtitle2" gutterBottom>
                  Add New Rule
                </Typography>
                <Box sx={{ display: "flex", gap: 1, mb: 2, flexWrap: "wrap" }}>
                  <TextField
                    label="Rule ID"
                    size="small"
                    value={newRuleId}
                    onChange={(e) => setNewRuleId(e.target.value)}
                    sx={{ width: 150 }}
                  />
                  <TextField
                    label="Prefix (optional)"
                    size="small"
                    value={newRulePrefix}
                    onChange={(e) => setNewRulePrefix(e.target.value)}
                    sx={{ width: 150 }}
                  />
                  <TextField
                    label="Expire after (days)"
                    size="small"
                    type="number"
                    value={newRuleDays}
                    onChange={(e) => setNewRuleDays(parseInt(e.target.value) || 30)}
                    sx={{ width: 150 }}
                  />
                  <Button
                    variant="outlined"
                    onClick={handleAddLifecycleRule}
                    disabled={!newRuleId}
                    startIcon={<Add />}
                  >
                    Add Rule
                  </Button>
                </Box>

                <List dense>
                  {lifecycleConfig.rules.map((rule) => (
                    <ListItem key={rule.id}>
                      <ListItemText
                        primary={
                          <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                            <Typography variant="body2" fontWeight="medium">
                              {rule.id}
                            </Typography>
                            <Chip
                              label={rule.status}
                              size="small"
                              color={rule.status === "Enabled" ? "success" : "default"}
                            />
                          </Box>
                        }
                        secondary={
                          <>
                            {rule.filter.prefix && `Prefix: ${rule.filter.prefix} | `}
                            {rule.actions.map((a, i) => (
                              <span key={i}>
                                {a.type}: {a.days || a.noncurrent_days} days
                              </span>
                            ))}
                          </>
                        }
                      />
                      <ListItemSecondaryAction>
                        <IconButton
                          size="small"
                          onClick={() => handleRemoveLifecycleRule(rule.id)}
                        >
                          <Delete fontSize="small" />
                        </IconButton>
                      </ListItemSecondaryAction>
                    </ListItem>
                  ))}
                </List>

                <Box sx={{ mt: 2 }}>
                  <Button
                    variant="contained"
                    onClick={handleSaveLifecycle}
                    disabled={saving}
                  >
                    {saving ? "Saving..." : "Save Lifecycle Rules"}
                  </Button>
                </Box>
              </Paper>
            </TabPanel>
          </>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Close</Button>
      </DialogActions>
    </Dialog>
  );
};

export default BucketSettingsDialog;
