import React, { useEffect, useState } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  TextField,
  Switch,
  FormControlLabel,
  Alert,
  CircularProgress,
  Paper,
  Divider,
  Chip,
} from "@mui/material";
import { Lock, Gavel, CalendarMonth } from "@mui/icons-material";
import apiClient from "../../api/client";
import type { ObjectRetention } from "../../api/types";

interface ObjectRetentionDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
  objectKey: string;
  onSuccess?: () => void;
}

const ObjectRetentionDialog: React.FC<ObjectRetentionDialogProps> = ({
  open,
  onClose,
  bucketName,
  objectKey,
  onSuccess,
}) => {
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  // Retention
  const [retentionMode, setRetentionMode] = useState<"GOVERNANCE" | "COMPLIANCE">(
    "GOVERNANCE"
  );
  const [retainUntilDate, setRetainUntilDate] = useState<string>("");
  const [hasRetention, setHasRetention] = useState(false);

  // Legal Hold
  const [legalHoldEnabled, setLegalHoldEnabled] = useState(false);

  const fetchSettings = async () => {
    if (!bucketName || !objectKey) return;

    setLoading(true);
    setError(null);

    try {
      // Fetch retention
      try {
        const retention = await apiClient.getObjectRetention(bucketName, objectKey);
        if (retention.mode) {
          setRetentionMode(retention.mode);
          setRetainUntilDate(
            new Date(retention.retain_until_date * 1000).toISOString().split("T")[0]
          );
          setHasRetention(true);
        }
      } catch (e) {
        // No retention set
        setHasRetention(false);
      }

      // Fetch legal hold
      try {
        const legalHold = await apiClient.getObjectLegalHold(bucketName, objectKey);
        setLegalHoldEnabled(legalHold.status);
      } catch (e) {
        // No legal hold
        setLegalHoldEnabled(false);
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
  }, [open, bucketName, objectKey]);

  const handleSaveRetention = async () => {
    if (!retainUntilDate) {
      setError("Please select a retention date");
      return;
    }

    setSaving(true);
    setError(null);
    try {
      const retention: ObjectRetention = {
        mode: retentionMode,
        retain_until_date: Math.floor(new Date(retainUntilDate).getTime() / 1000),
      };
      await apiClient.setObjectRetention(bucketName, objectKey, retention);
      setSuccess("Retention settings saved");
      setHasRetention(true);
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to save retention");
    } finally {
      setSaving(false);
    }
  };

  const handleSaveLegalHold = async () => {
    setSaving(true);
    setError(null);
    try {
      await apiClient.setObjectLegalHold(bucketName, objectKey, legalHoldEnabled);
      setSuccess("Legal hold settings saved");
      onSuccess?.();
    } catch (err: any) {
      setError(err.response?.data?.error || "Failed to save legal hold");
    } finally {
      setSaving(false);
    }
  };

  const getMinDate = () => {
    const tomorrow = new Date();
    tomorrow.setDate(tomorrow.getDate() + 1);
    return tomorrow.toISOString().split("T")[0];
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle sx={{ display: "flex", alignItems: "center", gap: 1 }}>
        <Lock color="primary" />
        Object Lock & Retention
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
            {/* Retention Section */}
            <Paper variant="outlined" sx={{ p: 2, mb: 2 }}>
              <Box sx={{ display: "flex", alignItems: "center", gap: 1, mb: 2 }}>
                <CalendarMonth color="primary" />
                <Typography variant="subtitle1" fontWeight="medium">
                  Retention Policy
                </Typography>
                {hasRetention && (
                  <Chip label="Active" size="small" color="success" />
                )}
              </Box>

              <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                Prevents object deletion until the retention date expires.
              </Typography>

              <FormControl fullWidth size="small" sx={{ mb: 2 }}>
                <InputLabel>Retention Mode</InputLabel>
                <Select
                  value={retentionMode}
                  label="Retention Mode"
                  onChange={(e) =>
                    setRetentionMode(e.target.value as "GOVERNANCE" | "COMPLIANCE")
                  }
                >
                  <MenuItem value="GOVERNANCE">
                    <Box>
                      <Typography variant="body2">Governance</Typography>
                      <Typography variant="caption" color="text.secondary">
                        Can be overridden by users with special permissions
                      </Typography>
                    </Box>
                  </MenuItem>
                  <MenuItem value="COMPLIANCE">
                    <Box>
                      <Typography variant="body2">Compliance</Typography>
                      <Typography variant="caption" color="text.secondary">
                        Cannot be overridden by anyone, including root user
                      </Typography>
                    </Box>
                  </MenuItem>
                </Select>
              </FormControl>

              <TextField
                fullWidth
                size="small"
                type="date"
                label="Retain Until Date"
                value={retainUntilDate}
                onChange={(e) => setRetainUntilDate(e.target.value)}
                InputLabelProps={{ shrink: true }}
                inputProps={{ min: getMinDate() }}
                sx={{ mb: 2 }}
              />

              <Alert severity="warning" sx={{ mb: 2 }}>
                {retentionMode === "COMPLIANCE" ? (
                  <>
                    <strong>Warning:</strong> Compliance mode retention cannot be
                    shortened or removed. Make sure this is what you want.
                  </>
                ) : (
                  <>
                    Governance mode can be overridden by users with
                    s3:BypassGovernanceRetention permission.
                  </>
                )}
              </Alert>

              <Button
                variant="contained"
                onClick={handleSaveRetention}
                disabled={saving}
                fullWidth
              >
                {saving ? "Saving..." : "Set Retention"}
              </Button>
            </Paper>

            <Divider sx={{ my: 2 }} />

            {/* Legal Hold Section */}
            <Paper variant="outlined" sx={{ p: 2 }}>
              <Box sx={{ display: "flex", alignItems: "center", gap: 1, mb: 2 }}>
                <Gavel color="primary" />
                <Typography variant="subtitle1" fontWeight="medium">
                  Legal Hold
                </Typography>
                {legalHoldEnabled && (
                  <Chip label="Active" size="small" color="error" />
                )}
              </Box>

              <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                Legal hold prevents object deletion regardless of retention settings.
                Use for legal or compliance purposes.
              </Typography>

              <FormControlLabel
                control={
                  <Switch
                    checked={legalHoldEnabled}
                    onChange={(e) => setLegalHoldEnabled(e.target.checked)}
                    color={legalHoldEnabled ? "error" : "default"}
                  />
                }
                label={
                  <Box>
                    <Typography variant="body2">
                      {legalHoldEnabled ? "Legal Hold Enabled" : "Legal Hold Disabled"}
                    </Typography>
                    {legalHoldEnabled && (
                      <Typography variant="caption" color="error">
                        Object cannot be deleted
                      </Typography>
                    )}
                  </Box>
                }
              />

              <Box sx={{ mt: 2 }}>
                <Button
                  variant="contained"
                  onClick={handleSaveLegalHold}
                  disabled={saving}
                  color={legalHoldEnabled ? "error" : "primary"}
                  fullWidth
                >
                  {saving
                    ? "Saving..."
                    : legalHoldEnabled
                    ? "Enable Legal Hold"
                    : "Disable Legal Hold"}
                </Button>
              </Box>
            </Paper>
          </>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Close</Button>
      </DialogActions>
    </Dialog>
  );
};

export default ObjectRetentionDialog;
