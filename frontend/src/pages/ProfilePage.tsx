import React, { useState, useEffect, useMemo, useCallback } from 'react';
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
  Chip,
  Stack,
  IconButton,
  Tooltip,
} from '@mui/material';
import { Person, Lock, Save, Refresh } from '@mui/icons-material';
import { useTranslation } from 'react-i18next';
import apiClient from '../api/client';
import type { CurrentUserResponse } from '../api/types';
import { buildCapabilities, type Capability } from '../auth/capabilities';

function formatSessionExpiry(iso: string | null, locale: string): string | null {
  if (!iso) return null;
  try {
    const d = new Date(iso);
    if (Number.isNaN(d.getTime())) return null;
    const loc = locale.startsWith('ru') ? 'ru-RU' : 'en-US';
    return d.toLocaleString(loc, { dateStyle: 'medium', timeStyle: 'short' });
  } catch {
    return null;
  }
}

const ProfilePage: React.FC = () => {
  const { t, i18n } = useTranslation();
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  const [userInfo, setUserInfo] = useState<CurrentUserResponse | null>(null);
  const [sessionExpiresAt, setSessionExpiresAt] = useState<string | null>(null);

  const [oldPassword, setOldPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');

  const loadUserInfo = useCallback(async (silent = false) => {
    try {
      if (silent) setRefreshing(true);
      else setLoading(true);
      setError(null);
      const [user, session] = await Promise.all([
        apiClient.getCurrentUser(),
        apiClient.getSession(),
      ]);
      setUserInfo(user);
      setSessionExpiresAt(session.authenticated ? session.expires_at : null);
    } catch (err: unknown) {
      const ax = err as { response?: { data?: { error?: string } } };
      setError(ax.response?.data?.error || t('profile.loadError'));
      setUserInfo(null);
      setSessionExpiresAt(null);
    } finally {
      if (silent) setRefreshing(false);
      else setLoading(false);
    }
  }, [t]);

  useEffect(() => {
    void loadUserInfo(false);
  }, [loadUserInfo]);

  const roleLabel = useMemo(() => {
    if (!userInfo) return '';
    const r = (
      userInfo.role ||
      (userInfo.is_admin ? 'admin' : 'viewer')
    ).toLowerCase();
    if (r === 'admin') return t('nav.role_admin');
    if (r === 'editor') return t('nav.role_editor');
    if (r === 'viewer') return t('nav.role_viewer');
    return t('nav.role_other', { role: userInfo.role || r });
  }, [userInfo, t]);

  const capabilityMap = useMemo(() => {
    if (!userInfo) return null;
    return buildCapabilities({
      role: userInfo.role || (userInfo.is_admin ? 'admin' : 'viewer'),
      isAdmin: userInfo.is_admin,
      policies: userInfo.policies ?? [],
    });
  }, [userInfo]);

  const enabledCapabilities = useMemo((): Capability[] => {
    if (!capabilityMap) return [];
    return (Object.keys(capabilityMap) as Capability[]).filter(
      (k) => capabilityMap[k],
    );
  }, [capabilityMap]);

  const sessionFormatted = useMemo(
    () => formatSessionExpiry(sessionExpiresAt, i18n.language),
    [sessionExpiresAt, i18n.language],
  );

  const handleChangePassword = async () => {
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
    } catch (err: unknown) {
      const ax = err as { response?: { data?: { error?: string } } };
      setError(ax.response?.data?.error || t('profile.changePasswordError'));
    } finally {
      setSaving(false);
    }
  };

  const handlePasswordSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    await handleChangePassword();
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

  const groups = userInfo.groups ?? [];
  const policies = userInfo.policies ?? [];

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', minHeight: '70vh' }}>
      <Box sx={{ display: 'flex', alignItems: 'center', mb: 3 }}>
        <Person sx={{ fontSize: 40, mr: 2 }} />
        <Box sx={{ flex: 1 }}>
          <Typography variant="h4">{t('profile.title')}</Typography>
          <Typography variant="body2" color="textSecondary">
            {t('profile.subtitle')}
          </Typography>
        </Box>
        <Tooltip title={t('common.refresh')}>
          <span>
            <IconButton
              aria-label={t('common.refresh')}
              onClick={() => void loadUserInfo(true)}
              disabled={refreshing}
            >
              {refreshing ? <CircularProgress color="inherit" size={22} /> : <Refresh />}
            </IconButton>
          </span>
        </Tooltip>
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
        <Grid item xs={12} md={7}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                <Person sx={{ verticalAlign: 'middle', mr: 1 }} />
                {t('profile.userInformation')}
              </Typography>
              <Divider sx={{ my: 2 }} />
              <Stack spacing={2}>
                <Box>
                  <Typography variant="caption" color="text.secondary" display="block">
                    {t('profile.username')}
                  </Typography>
                  <Typography variant="body1">{userInfo.username}</Typography>
                </Box>
                <Box>
                  <Typography variant="caption" color="text.secondary" display="block">
                    {t('profile.accessKey')}
                  </Typography>
                  <Typography variant="body2" sx={{ wordBreak: 'break-all', fontFamily: 'monospace' }}>
                    {userInfo.access_key}
                  </Typography>
                </Box>
                <Box>
                  <Typography variant="caption" color="text.secondary" display="block" gutterBottom>
                    {t('profile.role')}
                  </Typography>
                  <Stack direction="row" alignItems="center" spacing={1} flexWrap="wrap" useFlexGap>
                    <Chip
                      label={roleLabel}
                      color={userInfo.is_admin ? 'primary' : 'default'}
                      size="small"
                    />
                    {userInfo.role && (
                      <Typography variant="caption" color="text.secondary">
                        {t('profile.rawRole')}: {userInfo.role}
                      </Typography>
                    )}
                  </Stack>
                </Box>

                <Divider />

                <Box>
                  <Typography variant="caption" color="text.secondary" display="block">
                    {t('profile.sessionExpires')}
                  </Typography>
                  <Typography variant="body2">
                    {sessionFormatted ?? t('profile.sessionUnknown')}
                  </Typography>
                </Box>

                <Divider />

                <Box>
                  <Typography variant="subtitle2" gutterBottom>
                    {t('profile.groups')}
                  </Typography>
                  {groups.length === 0 ? (
                    <Typography variant="body2" color="text.secondary">
                      {t('profile.noGroups')}
                    </Typography>
                  ) : (
                    <Stack direction="row" flexWrap="wrap" useFlexGap spacing={0.5}>
                      {groups.map((g) => (
                        <Chip key={g} label={g} size="small" variant="outlined" />
                      ))}
                    </Stack>
                  )}
                </Box>

                <Box>
                  <Typography variant="subtitle2" gutterBottom>
                    {t('profile.policies')}
                  </Typography>
                  {policies.length === 0 ? (
                    <Typography variant="body2" color="text.secondary">
                      {t('profile.noPolicies')}
                    </Typography>
                  ) : (
                    <Stack direction="row" flexWrap="wrap" useFlexGap spacing={0.5}>
                      {policies.map((p) => (
                        <Chip key={p} label={p} size="small" variant="outlined" />
                      ))}
                    </Stack>
                  )}
                </Box>

                <Divider />

                <Box>
                  <Typography variant="subtitle2" gutterBottom>
                    {t('profile.capabilities')}
                  </Typography>
                  <Stack direction="row" flexWrap="wrap" useFlexGap spacing={0.5}>
                    {enabledCapabilities.map((c) => (
                      <Chip key={c} label={c} size="small" variant="outlined" color="secondary" />
                    ))}
                  </Stack>
                </Box>
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        <Grid item xs={12} md={5}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                <Lock sx={{ verticalAlign: 'middle', mr: 1 }} />
                {t('profile.changePassword')}
              </Typography>
              <Divider sx={{ my: 2 }} />
              <Box component="form" noValidate autoComplete="off" onSubmit={handlePasswordSubmit}>
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
                  type="submit"
                  startIcon={saving ? <CircularProgress size={20} /> : <Save />}
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
