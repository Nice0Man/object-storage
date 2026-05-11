import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  IconButton,
  TextField,
  Box,
  Typography,
  Alert,
  CircularProgress,
  Accordion,
  AccordionSummary,
  AccordionDetails,
} from '@mui/material';
import DeleteIcon from '@mui/icons-material/Delete';
import AddIcon from '@mui/icons-material/Add';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import { useTranslation } from 'react-i18next';
import apiClient from '../../api/client';

export interface Policy {
  name: string;
  policy: string;
}

interface UserPoliciesDialogProps {
  open: boolean;
  onClose: () => void;
  accessKey: string;
  username: string;
}

const UserPoliciesDialog: React.FC<UserPoliciesDialogProps> = ({
  open,
  onClose,
  accessKey,
  username,
}) => {
  const { t } = useTranslation();
  const [policies, setPolicies] = useState<Policy[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [newPolicyName, setNewPolicyName] = useState('');
  const [newPolicyDoc, setNewPolicyDoc] = useState('');
  const [addingPolicy, setAddingPolicy] = useState(false);

  useEffect(() => {
    if (open) {
      loadPolicies();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open, accessKey]);

  const loadPolicies = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await apiClient.getUserPolicies(accessKey);
      setPolicies((response.policies || []) as unknown as Policy[]);
    } catch (err: any) {
      setError(err.response?.data?.error || t('users.policies.loadError'));
    } finally {
      setLoading(false);
    }
  };

  const handleAddPolicy = async () => {
    if (!newPolicyName.trim() || !newPolicyDoc.trim()) {
      setError(t('users.policies.fillAllFields'));
      return;
    }

    // Validate JSON
    try {
      JSON.parse(newPolicyDoc);
    } catch {
      setError(t('users.policies.invalidJson'));
      return;
    }

    try {
      setAddingPolicy(true);
      setError(null);
      await apiClient.assignUserPolicy(accessKey, newPolicyName, {
        policy: newPolicyDoc,
      });
      setNewPolicyName('');
      setNewPolicyDoc('');
      await loadPolicies();
    } catch (err: any) {
      setError(err.response?.data?.error || t('users.policies.addError'));
    } finally {
      setAddingPolicy(false);
    }
  };

  const handleDeletePolicy = async (policyName: string) => {
    if (!window.confirm(t('users.policies.confirmDelete', { name: policyName }))) {
      return;
    }

    try {
      setError(null);
      await apiClient.removeUserPolicy(accessKey, policyName);
      await loadPolicies();
    } catch (err: any) {
      setError(err.response?.data?.error || t('users.policies.deleteError'));
    }
  };

  const renderPolicyDocument = (policyStr: string) => {
    try {
      const policy = JSON.parse(policyStr);
      return (
        <Box sx={{ mt: 1 }}>
          <pre
            style={{
              fontSize: '0.75rem',
              backgroundColor: '#f5f5f5',
              padding: '8px',
              borderRadius: '4px',
              overflow: 'auto',
              maxHeight: '200px',
            }}
          >
            {JSON.stringify(policy, null, 2)}
          </pre>
        </Box>
      );
    } catch {
      return (
        <Typography variant="caption" color="error">
          {t('users.policies.invalidFormat')}
        </Typography>
      );
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle>
        {t('users.policies.title')} - {username}
      </DialogTitle>
      <DialogContent dividers>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        {loading ? (
          <Box display="flex" justifyContent="center" p={3}>
            <CircularProgress />
          </Box>
        ) : (
          <>
            {/* Existing Policies */}
            <Typography variant="h6" gutterBottom>
              {t('users.policies.existing')}
            </Typography>
            {policies.length === 0 ? (
              <Typography color="textSecondary" sx={{ mb: 3 }}>
                {t('users.policies.noPolicies')}
              </Typography>
            ) : (
              <Box sx={{ mb: 3 }}>
                {policies.map((policy) => (
                  <Accordion key={policy.name}>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                      <Box display="flex" alignItems="center" width="100%">
                        <Typography sx={{ flexGrow: 1 }}>{policy.name}</Typography>
                        <IconButton
                          size="small"
                          color="error"
                          onClick={(e) => {
                            e.stopPropagation();
                            handleDeletePolicy(policy.name);
                          }}
                        >
                          <DeleteIcon />
                        </IconButton>
                      </Box>
                    </AccordionSummary>
                    <AccordionDetails>
                      {renderPolicyDocument(policy.policy)}
                    </AccordionDetails>
                  </Accordion>
                ))}
              </Box>
            )}

            {/* Add New Policy */}
            <Typography variant="h6" gutterBottom>
              {t('users.policies.addNew')}
            </Typography>
            <TextField
              fullWidth
              label={t('users.policies.policyName')}
              value={newPolicyName}
              onChange={(e) => setNewPolicyName(e.target.value)}
              margin="normal"
              placeholder="ReadOnlyAccess"
            />
            <TextField
              fullWidth
              label={t('users.policies.policyDocument')}
              value={newPolicyDoc}
              onChange={(e) => setNewPolicyDoc(e.target.value)}
              margin="normal"
              multiline
              rows={8}
              placeholder={`{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": ["s3:GetObject"],
      "Resource": ["arn:aws:s3:::mybucket/*"]
    }
  ]
}`}
            />
            <Button
              variant="contained"
              startIcon={<AddIcon />}
              onClick={handleAddPolicy}
              disabled={addingPolicy || !newPolicyName.trim() || !newPolicyDoc.trim()}
              sx={{ mt: 2 }}
            >
              {addingPolicy ? t('common.adding') : t('users.policies.add')}
            </Button>
          </>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>{t('common.close')}</Button>
      </DialogActions>
    </Dialog>
  );
};

export default UserPoliciesDialog;
