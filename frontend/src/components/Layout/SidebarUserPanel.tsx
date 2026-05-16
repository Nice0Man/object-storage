import React from 'react';
import {
  Avatar,
  Box,
  Button,
  Divider,
  IconButton,
  Stack,
  ToggleButton,
  ToggleButtonGroup,
  Tooltip,
  Typography,
  useTheme,
  alpha,
} from '@mui/material';
import {
  Brightness4,
  Brightness7,
  ExitToApp as LogoutIcon,
  Translate,
} from '@mui/icons-material';
import { useTranslation } from 'react-i18next';
import { useTheme as useAppTheme } from '../../contexts/ThemeContext';

export interface SidebarUserPanelProps {
  collapsed: boolean;
  username: string | null;
  role: string | null;
  onLogout: () => void;
}

const SidebarUserPanel: React.FC<SidebarUserPanelProps> = ({
  collapsed,
  username,
  role,
  onLogout,
}) => {
  const theme = useTheme();
  const { mode, toggleTheme } = useAppTheme();
  const { t, i18n } = useTranslation();

  const displayName = username?.trim() || t('nav.userPanel.defaultUser');
  const initial = displayName.charAt(0).toUpperCase();

  const normalizedRole = (role || 'viewer').toLowerCase();
  const roleLabel = (() => {
    if (normalizedRole === 'admin') return t('nav.role_admin');
    if (normalizedRole === 'editor') return t('nav.role_editor');
    if (normalizedRole === 'viewer') return t('nav.role_viewer');
    return t('nav.role_other', { role: role || 'viewer' });
  })();

  const roleColor = (() => {
    if (normalizedRole === 'admin') return theme.palette.primary.main;
    if (normalizedRole === 'editor') return theme.palette.info.main;
    return theme.palette.text.secondary;
  })();

  const currentLang = i18n.language.startsWith('ru') ? 'ru' : 'en';

  const handleLanguageChange = (_: React.MouseEvent<HTMLElement>, value: string | null) => {
    if (value) {
      i18n.changeLanguage(value);
    }
  };

  const panelSurface = {
    width: '100%',
    boxSizing: 'border-box',
    borderRadius: 0,
    border: 0,
    borderTop: 0,
    bgcolor: theme.palette.background.paper,
    backgroundImage:
      theme.palette.mode === 'dark'
        ? `linear-gradient(145deg, ${alpha(theme.palette.primary.main, 0.12)} 0%, transparent 55%)`
        : `linear-gradient(145deg, ${alpha(theme.palette.primary.main, 0.06)} 0%, ${theme.palette.background.paper} 50%)`,
    boxShadow: 'none',
  };

  if (collapsed) {
    return (
      <Stack
        alignItems="center"
        spacing={1}
        sx={{ ...panelSurface, width: '100%', py: 1.5, px: 0.5 }}
      >
        <Tooltip title={`${displayName} · ${roleLabel}`} placement="right">
          <Avatar
            sx={{
              width: 40,
              height: 40,
              fontWeight: 700,
              bgcolor: alpha(roleColor, 0.15),
              color: roleColor,
              border: `2px solid ${alpha(roleColor, 0.45)}`,
            }}
          >
            {initial}
          </Avatar>
        </Tooltip>
        <Tooltip
          title={mode === 'light' ? t('settings.darkMode') : t('settings.lightMode')}
          placement="right"
        >
          <IconButton size="small" onClick={toggleTheme} sx={{ color: 'text.secondary' }}>
            {mode === 'light' ? <Brightness4 fontSize="small" /> : <Brightness7 fontSize="small" />}
          </IconButton>
        </Tooltip>
        <Tooltip title={t('settings.selectLanguage')} placement="right">
          <IconButton
            size="small"
            onClick={() => i18n.changeLanguage(currentLang === 'en' ? 'ru' : 'en')}
            sx={{ color: 'text.secondary' }}
          >
            <Translate fontSize="small" />
          </IconButton>
        </Tooltip>
        <Tooltip title={t('auth.logout')} placement="right">
          <IconButton
            size="small"
            onClick={onLogout}
            sx={{
              color: 'error.main',
              '&:hover': { bgcolor: alpha(theme.palette.error.main, 0.1) },
            }}
          >
            <LogoutIcon fontSize="small" />
          </IconButton>
        </Tooltip>
      </Stack>
    );
  }

  return (
    <Box sx={{ ...panelSurface, p: 1.5, flex: 1, alignSelf: 'stretch' }}>
      <Stack spacing={1.5}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.25, minWidth: 0 }}>
          <Avatar
            sx={{
              width: 44,
              height: 44,
              flexShrink: 0,
              fontWeight: 700,
              fontSize: '1.1rem',
              bgcolor: alpha(roleColor, 0.14),
              color: roleColor,
              border: `2px solid ${alpha(roleColor, 0.4)}`,
            }}
          >
            {initial}
          </Avatar>
          <Box sx={{ minWidth: 0, flex: 1 }}>
            <Typography variant="caption" color="text.secondary" sx={{ display: 'block', lineHeight: 1.2 }}>
              {t('nav.userPanel.signedInAs')}
            </Typography>
            <Typography variant="subtitle2" noWrap sx={{ fontWeight: 700, color: 'text.primary', lineHeight: 1.35 }}>
              {displayName}
            </Typography>
            <Typography
              component="span"
              variant="caption"
              sx={{
                display: 'inline-block',
                mt: 0.5,
                px: 1,
                py: 0.15,
                borderRadius: 1,
                fontWeight: 600,
                fontSize: '0.68rem',
                letterSpacing: 0.3,
                textTransform: 'uppercase',
                bgcolor: alpha(roleColor, theme.palette.mode === 'dark' ? 0.22 : 0.12),
                color: roleColor,
                border: `1px solid ${alpha(roleColor, 0.35)}`,
                maxWidth: '100%',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
                whiteSpace: 'nowrap',
              }}
            >
              {roleLabel}
            </Typography>
          </Box>
        </Box>

        <Divider flexItem sx={{ borderColor: alpha(theme.palette.divider, 0.8) }} />

        <Box>
          <Typography
            variant="overline"
            sx={{
              display: 'block',
              color: 'text.secondary',
              fontSize: '0.65rem',
              letterSpacing: 1,
              mb: 1,
              lineHeight: 1.2,
            }}
          >
            {t('nav.userPanel.appearance')}
          </Typography>

          <Stack spacing={1}>
            <Box
              sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                gap: 1,
                px: 1,
                py: 0.75,
                borderRadius: 1,
                bgcolor: alpha(theme.palette.action.hover, theme.palette.mode === 'dark' ? 0.2 : 0.5),
              }}
            >
              <Typography variant="body2" color="text.secondary" sx={{ fontSize: '0.8rem' }}>
                {t('nav.userPanel.theme')}
              </Typography>
              <Tooltip title={mode === 'light' ? t('settings.darkMode') : t('settings.lightMode')}>
                <IconButton
                  size="small"
                  onClick={toggleTheme}
                  sx={{
                    bgcolor: alpha(theme.palette.primary.main, 0.1),
                    color: 'primary.main',
                    '&:hover': { bgcolor: alpha(theme.palette.primary.main, 0.18) },
                  }}
                >
                  {mode === 'light' ? <Brightness4 fontSize="small" /> : <Brightness7 fontSize="small" />}
                </IconButton>
              </Tooltip>
            </Box>

            <Box
              sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                gap: 1,
                px: 1,
                py: 0.75,
                borderRadius: 1,
                bgcolor: alpha(theme.palette.action.hover, theme.palette.mode === 'dark' ? 0.2 : 0.5),
              }}
            >
              <Typography variant="body2" color="text.secondary" sx={{ fontSize: '0.8rem' }}>
                {t('nav.userPanel.language')}
              </Typography>
              <ToggleButtonGroup
                exclusive
                size="small"
                value={currentLang}
                onChange={handleLanguageChange}
                aria-label={t('settings.selectLanguage')}
                sx={{
                  '& .MuiToggleButton-root': {
                    px: 1.25,
                    py: 0.25,
                    fontSize: '0.7rem',
                    fontWeight: 600,
                    textTransform: 'none',
                    border: `1px solid ${theme.palette.divider}`,
                    color: 'text.secondary',
                    '&.Mui-selected': {
                      bgcolor: alpha(theme.palette.primary.main, 0.15),
                      color: 'primary.main',
                      borderColor: alpha(theme.palette.primary.main, 0.4),
                    },
                  },
                }}
              >
                <ToggleButton value="en">{t('settings.englishShort')}</ToggleButton>
                <ToggleButton value="ru">{t('settings.russianShort')}</ToggleButton>
              </ToggleButtonGroup>
            </Box>
          </Stack>
        </Box>

        <Button
          fullWidth
          variant="outlined"
          color="error"
          size="small"
          startIcon={<LogoutIcon fontSize="small" />}
          onClick={onLogout}
          sx={{
            textTransform: 'none',
            fontWeight: 600,
            borderRadius: 1.5,
            py: 0.75,
            borderColor: alpha(theme.palette.error.main, 0.45),
            '&:hover': {
              borderColor: 'error.main',
              bgcolor: alpha(theme.palette.error.main, 0.08),
            },
          }}
        >
          {t('auth.logout')}
        </Button>
      </Stack>
    </Box>
  );
};

export default SidebarUserPanel;
