import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
  TextField,
  Box,
  Typography,
  Alert,
  CircularProgress,
  InputAdornment,
} from '@mui/material';
import DeleteIcon from '@mui/icons-material/Delete';
import AddIcon from '@mui/icons-material/Add';
import GroupIcon from '@mui/icons-material/Group';
import { useTranslation } from 'react-i18next';
import apiClient from '../../api/client';

interface UserGroupsDialogProps {
  open: boolean;
  onClose: () => void;
  accessKey: string;
  username: string;
  groups?: string[];
  onUpdate?: () => void;
}

const UserGroupsDialog: React.FC<UserGroupsDialogProps> = ({
  open,
  onClose,
  accessKey,
  username,
  groups: initialGroups = [],
  onUpdate,
}) => {
  const { t } = useTranslation();
  const [groups, setGroups] = useState<string[]>(initialGroups);
  const [newGroupName, setNewGroupName] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    if (open) {
      setGroups(initialGroups);
      setError(null);
    }
  }, [open, initialGroups]);

  const handleAddGroup = async () => {
    if (!newGroupName.trim()) {
      setError(t('users.groups.enterName'));
      return;
    }

    if (groups.includes(newGroupName.trim())) {
      setError(t('users.groups.alreadyExists'));
      return;
    }

    try {
      setLoading(true);
      setError(null);
      await apiClient.addUserToGroup(accessKey, newGroupName.trim());
      setGroups([...groups, newGroupName.trim()]);
      setNewGroupName('');
      if (onUpdate) onUpdate();
    } catch (err: any) {
      setError(err.response?.data?.error || t('users.groups.addError'));
    } finally {
      setLoading(false);
    }
  };

  const handleRemoveGroup = async (groupName: string) => {
    if (!window.confirm(t('users.groups.confirmRemove', { name: groupName }))) {
      return;
    }

    try {
      setLoading(true);
      setError(null);
      await apiClient.removeUserFromGroup(accessKey, groupName);
      setGroups(groups.filter((g) => g !== groupName));
      if (onUpdate) onUpdate();
    } catch (err: any) {
      setError(err.response?.data?.error || t('users.groups.removeError'));
    } finally {
      setLoading(false);
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !loading) {
      handleAddGroup();
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>
        <Box display="flex" alignItems="center" gap={1}>
          <GroupIcon />
          {t('users.groups.title')} - {username}
        </Box>
      </DialogTitle>
      <DialogContent dividers>
        {error && (
          <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        {/* Add New Group */}
        <Box sx={{ mb: 3 }}>
          <TextField
            fullWidth
            label={t('users.groups.addNew')}
            value={newGroupName}
            onChange={(e) => setNewGroupName(e.target.value)}
            onKeyPress={handleKeyPress}
            disabled={loading}
            placeholder="developers"
            InputProps={{
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    onClick={handleAddGroup}
                    disabled={loading || !newGroupName.trim()}
                    color="primary"
                  >
                    <AddIcon />
                  </IconButton>
                </InputAdornment>
              ),
            }}
          />
        </Box>

        {/* Current Groups */}
        <Typography variant="subtitle2" color="textSecondary" gutterBottom>
          {t('users.groups.current')} ({groups.length})
        </Typography>

        {groups.length === 0 ? (
          <Box
            display="flex"
            justifyContent="center"
            alignItems="center"
            minHeight={100}
          >
            <Typography color="textSecondary">
              {t('users.groups.noGroups')}
            </Typography>
          </Box>
        ) : (
          <List>
            {groups.map((group) => (
              <ListItem
                key={group}
                sx={{
                  border: 1,
                  borderColor: 'divider',
                  borderRadius: 1,
                  mb: 1,
                }}
              >
                <ListItemText
                  primary={group}
                  primaryTypographyProps={{
                    fontWeight: 500,
                  }}
                />
                <ListItemSecondaryAction>
                  <IconButton
                    edge="end"
                    color="error"
                    onClick={() => handleRemoveGroup(group)}
                    disabled={loading}
                  >
                    <DeleteIcon />
                  </IconButton>
                </ListItemSecondaryAction>
              </ListItem>
            ))}
          </List>
        )}

        {loading && (
          <Box display="flex" justifyContent="center" mt={2}>
            <CircularProgress size={24} />
          </Box>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>{t('common.close')}</Button>
      </DialogActions>
    </Dialog>
  );
};

export default UserGroupsDialog;
