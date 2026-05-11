import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Card,
  CardContent,
  Grid,
  TextField,
  Button,
  Alert,
  CircularProgress,
  Divider,
  List,
  ListItem,
  ListItemText,
  Chip,
} from '@mui/material';
import { Person, Lock, Save } from '@mui/icons-material';
import { useTranslation } from 'react-i18next';
import apiClient from '../api/client';

const ProfilePage: React.FC = () => {
  const { t } = useTranslation();
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  // User info
  const [userInfo, setUserInfo] = useState<{
    username: string;
    access_key: string;
    is_admin: boolean;
  } | null>(null);

  // Password change
  const [oldPassword, setOldPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');

  useEffect(() => {
    loadUserInfo();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const loadUserInfo = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await apiClient.getCurrentUser();
      setUserInfo(response);
    } catch (err: any) {
      setError(err.response?.data?.error || t('profile.loadError'));
    } finally {
      setLoading(false);
    }
  };

  const handleChangePassword = async () => {
    // Validation
    if (!oldPassword || !newPassword || !confirmPassword) {
      setError(t('profile.fillAllFields'));
      return;
    }

    if (newPassword !== confirmPassword) {
      setError(t('profile.passwordsDontMatch'));
      return;
    }

    if (newPassword.length < 8) {
      setError(t('profile.passwordTooShort'));
      return;
    }

    try {
      setSaving(true);
      setError(null);
      await apiClient.changePassword({
        old_password: oldPassword,
        new_password: newPassword,
      });
      setSuccess(t('profile.passwordChanged'));
      setOldPassword('');
      setNewPassword('');
      setConfirmPassword('');
    } catch (err: any) {
      setError(err.response?.data?.error || t('profile.changePasswordError'));
    } finally {
      setSaving(false);
    }
  };

  if (loading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="60vh">
        <CircularProgress />
      </Box>
    );
  }

  if (!userInfo) {
    return (
      <Box>
        <Alert severity="error">{error || t('profile.noUser')}</Alert>
      </Box>
    );
  }

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', minHeight: '70vh' }}>
      <Box sx={{ display: 'flex', alignItems: 'center', mb: 3 }}>
        <Person sx={{ fontSize: 40, mr: 2 }} />
        <Box>
          <Typography variant="h4">{t('profile.title')}</Typography>
          <Typography variant="body2" color="textSecondary">
            {t('profile.subtitle')}
          </Typography>
        </Box>
      </Box>

      {error && (
        <Alert severity="error" sx={{ mb: 3 }} onClose={() => setError(null)}>
          {error}
        </Alert>
      )}

      {success && (
        <Alert severity="success" sx={{ mb: 3 }} onClose={() => setSuccess(null)}>
          {success}
        </Alert>
      )}

      <Grid container spacing={3}>
        {/* User Information */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                <Person sx={{ verticalAlign: 'middle', mr: 1 }} />
                {t('profile.userInformation')}
              </Typography>
              <Divider sx={{ my: 2 }} />
              <List>
                <ListItem>
                  <ListItemText
                    primary={t('profile.username')}
                    secondary={userInfo.username}
                  />
                </ListItem>
                <ListItem>
                  <ListItemText
                    primary={t('profile.accessKey')}
                    secondary={userInfo.access_key}
                  />
                </ListItem>
                <ListItem>
                  <ListItemText
                    primary={t('profile.role')}
                    secondary={userInfo.is_admin ? t('profile.admin') : t('profile.user')}
                  />
                  <Chip
                    label={userInfo.is_admin ? t('profile.admin') : t('profile.user')}
                    color={userInfo.is_admin ? 'primary' : 'default'}
                    size="small"
                  />
                </ListItem>
              </List>
            </CardContent>
          </Card>
        </Grid>

        {/* Change Password */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                <Lock sx={{ verticalAlign: 'middle', mr: 1 }} />
                {t('profile.changePassword')}
              </Typography>
              <Divider sx={{ my: 2 }} />
              <Box component="form" noValidate autoComplete="off">
                <TextField
                  fullWidth
                  label={t('profile.oldPassword')}
                  type="password"
                  value={oldPassword}
                  onChange={(e) => setOldPassword(e.target.value)}
                  margin="normal"
                  disabled={saving}
                />
                <TextField
                  fullWidth
                  label={t('profile.newPassword')}
                  type="password"
                  value={newPassword}
                  onChange={(e) => setNewPassword(e.target.value)}
                  margin="normal"
                  disabled={saving}
                  helperText={t('profile.passwordMinLength')}
                />
                <TextField
                  fullWidth
                  label={t('profile.confirmPassword')}
                  type="password"
                  value={confirmPassword}
                  onChange={(e) => setConfirmPassword(e.target.value)}
                  margin="normal"
                  disabled={saving}
                />
                <Button
                  fullWidth
                  variant="contained"
                  startIcon={saving ? <CircularProgress size={20} /> : <Save />}
                  onClick={handleChangePassword}
                  disabled={saving || !oldPassword || !newPassword || !confirmPassword}
                  sx={{ mt: 2 }}
                >
                  {saving ? t('common.saving') : t('profile.updatePassword')}
                </Button>
              </Box>
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default ProfilePage;
