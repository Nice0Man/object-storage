import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  TextField,
  Alert,
  CircularProgress,
  Box,
  Typography,
} from '@mui/material';
import { useTranslation } from 'react-i18next';
import apiClient from '../../api/client';

interface BucketPolicyDialogProps {
  open: boolean;
  onClose: () => void;
  bucketName: string;
}

const BucketPolicyDialog: React.FC<BucketPolicyDialogProps> = ({
  open,
  onClose,
  bucketName,
}) => {
  const { t } = useTranslation();
  const [policy, setPolicy] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState(false);

  useEffect(() => {
    if (open) {
      loadPolicy();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open, bucketName]);

  const loadPolicy = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await apiClient.getBucketPolicy(bucketName);
      setPolicy(response.policy || '');
    } catch (err: any) {
      if (err.response?.status === 404) {
        setPolicy(''); // No policy set
      } else {
        setError(err.response?.data?.error || t('buckets.policy.loadError'));
      }
    } finally {
      setLoading(false);
    }
  };

  const handleSave = async () => {
    if (!policy.trim()) {
      setError(t('buckets.policy.emptyPolicy'));
      return;
    }

    // Validate JSON
    try {
      JSON.parse(policy);
    } catch {
      setError(t('buckets.policy.invalidJson'));
      return;
    }

    try {
      setLoading(true);
      setError(null);
      await apiClient.setBucketPolicy(bucketName, { policy });
      setSuccess(true);
      setTimeout(() => {
        setSuccess(false);
        onClose();
      }, 1500);
    } catch (err: any) {
      setError(err.response?.data?.error || t('buckets.policy.saveError'));
    } finally {
      setLoading(false);
    }
  };

  const examplePolicy = `{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": "*",
      "Action": ["s3:GetObject"],
      "Resource": ["arn:aws:s3:::${bucketName}/*"]
    }
  ]
}`;

  const handleSetExample = () => {
    setPolicy(examplePolicy);
    setError(null);
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle>
        {t('buckets.policy.title')} - {bucketName}
      </DialogTitle>
      <DialogContent dividers>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}
        {success && (
          <Alert severity="success" sx={{ mb: 2 }}>
            {t('buckets.policy.saved')}
          </Alert>
        )}

        {loading && !policy ? (
          <Box display="flex" justifyContent="center" p={3}>
            <CircularProgress />
          </Box>
        ) : (
          <>
            <Typography variant="body2" color="textSecondary" paragraph>
              {t('buckets.policy.description')}
            </Typography>

            <TextField
              fullWidth
              multiline
              rows={16}
              value={policy}
              onChange={(e) => setPolicy(e.target.value)}
              placeholder={examplePolicy}
              variant="outlined"
              sx={{
                fontFamily: 'monospace',
                '& .MuiInputBase-input': {
                  fontFamily: 'monospace',
                  fontSize: '0.875rem',
                },
              }}
            />

            <Box display="flex" justifyContent="flex-end" mt={1}>
              <Button size="small" onClick={handleSetExample}>
                {t('buckets.policy.useExample')}
              </Button>
            </Box>
          </>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose} disabled={loading}>
          {t('common.cancel')}
        </Button>
        <Button
          onClick={handleSave}
          variant="contained"
          disabled={loading || !policy.trim()}
        >
          {loading ? t('common.saving') : t('common.save')}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

export default BucketPolicyDialog;

