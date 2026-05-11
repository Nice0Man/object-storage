import React from 'react';
import {
  Box,
  Container,
  Toolbar,
  Typography,
  Button,
  Drawer,
  List,
  ListItem,
  ListItemIcon,
  ListItemText,
  ListItemButton,
  IconButton,
  Divider,
  Tooltip,
  Avatar,
  Stack,
  Chip,
  useTheme,
  alpha,
} from '@mui/material';
import {
  Menu as MenuIcon,
  Storage as StorageIcon,
  Folder as FolderIcon,
  People as PeopleIcon,
  ExitToApp as LogoutIcon,
  Dashboard as DashboardIcon,
  MonitorHeart as MonitorHeartIcon,
  Person as PersonIcon,
  ChevronLeft as ChevronLeftIcon,
} from '@mui/icons-material';
import { useNavigate, useLocation } from 'react-router-dom';
import { useAppDispatch } from '../../hooks/useAppDispatch';
import { useAppSelector } from '../../hooks/useAppSelector';
import { logout, selectCapabilities, selectUsername, selectRole } from '../../store/authSlice';
import { useTranslation } from 'react-i18next';
import ThemeToggle from '../Settings/ThemeToggle';
import LanguageSelector from '../Settings/LanguageSelector';
import { Outlet } from 'react-router-dom';
import { can, type Capability } from '../../auth/capabilities';

const drawerWidthExpanded = 240;
const drawerWidthCollapsed = 72;

interface MainLayoutProps {
  children?: React.ReactNode;
}

const MainLayout: React.FC<MainLayoutProps> = ({ children }) => {
  const theme = useTheme();
  const [mobileOpen, setMobileOpen] = React.useState(false);
  const [collapsed, setCollapsed] = React.useState(false);
  const navigate = useNavigate();
  const location = useLocation();
  const dispatch = useAppDispatch();
  const username = useAppSelector(selectUsername);
  const role = useAppSelector(selectRole);
  const capabilities = useAppSelector(selectCapabilities);
  const { t } = useTranslation();

  const roleLabel = React.useMemo(() => {
    const r = (role || 'viewer').toLowerCase();
    if (r === 'admin') return t('nav.role_admin');
    if (r === 'editor') return t('nav.role_editor');
    if (r === 'viewer') return t('nav.role_viewer');
    return t('nav.role_other', { role });
  }, [role, t]);

  const drawerWidth = collapsed ? drawerWidthCollapsed : drawerWidthExpanded;

  const handleDrawerToggle = () => {
    setMobileOpen(!mobileOpen);
  };

  const handleCollapse = () => {
    setCollapsed(!collapsed);
  };

  const handleLogout = async () => {
    await dispatch(logout());
    navigate('/login');
  };

  const menuItems: Array<{ text: string; icon: React.ReactElement; path: string; capability: Capability }> = [
    { text: 'nav.dashboard', icon: <DashboardIcon />, path: '/', capability: 'dashboard.view' },
    { text: 'nav.buckets', icon: <StorageIcon />, path: '/buckets', capability: 'buckets.view' },
    { text: 'nav.objects', icon: <FolderIcon />, path: '/objects', capability: 'objects.view' },
    { text: 'nav.users', icon: <PeopleIcon />, path: '/users', capability: 'users.view' },
    { text: 'nav.system', icon: <MonitorHeartIcon />, path: '/system-health', capability: 'system.view' },
    { text: 'nav.profile', icon: <PersonIcon />, path: '/profile', capability: 'profile.view' },
  ];
  const availableItems = menuItems.filter((item) => can(capabilities, item.capability));
  const unavailableItems = menuItems.filter((item) => !can(capabilities, item.capability));

  const renderMenuButton = (
    item: { text: string; icon: React.ReactElement; path: string; capability: Capability },
    disabled = false
  ) => (
    <ListItem key={disabled ? `${item.text}-disabled` : item.text} disablePadding>
      <Tooltip title={t(item.text)} placement="right">
        <span style={{ width: '100%' }}>
          <ListItemButton
            selected={!disabled && location.pathname === item.path}
            onClick={
              disabled
                ? undefined
                : () => {
                    navigate(item.path);
                    setMobileOpen(false);
                  }
            }
            disabled={disabled}
            sx={{
              borderRadius: 2,
              mx: 1,
              my: 0.5,
              minHeight: 44,
              justifyContent: collapsed ? 'center' : 'flex-start',
              px: collapsed ? 1 : 1.5,
              '&.Mui-selected': {
                bgcolor: 'primary.main',
                color: 'primary.contrastText',
                '&:hover': {
                  bgcolor: 'primary.dark',
                },
                '& .MuiListItemIcon-root': {
                  color: 'primary.contrastText',
                },
              },
              ...(disabled
                ? {
                    opacity: 0.52,
                    color: 'text.disabled',
                  }
                : {}),
            }}
          >
            <ListItemIcon sx={{ minWidth: collapsed ? 'auto' : 40, justifyContent: 'center' }}>
              {item.icon}
            </ListItemIcon>
            {!collapsed && <ListItemText primary={t(item.text)} />}
          </ListItemButton>
        </span>
      </Tooltip>
    </ListItem>
  );

  const drawer = (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
      <Toolbar
        sx={{
          background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
          justifyContent: collapsed ? 'center' : 'space-between',
          px: 1.5,
        }}
      >
        {!collapsed && (
          <Typography variant="subtitle1" noWrap component="div" sx={{ color: 'white', fontWeight: 700 }}>
            {t('app.title')}
          </Typography>
        )}
        <IconButton
          onClick={handleCollapse}
          aria-label={collapsed ? 'Expand sidebar' : 'Collapse sidebar'}
          sx={{
            color: 'white',
            ml: collapsed ? 'auto' : 0,
            mr: collapsed ? 'auto' : 0,
            bgcolor: collapsed ? 'rgba(255,255,255,0.14)' : 'transparent',
            '&:hover': {
              bgcolor: collapsed ? 'rgba(255,255,255,0.22)' : 'rgba(255,255,255,0.08)',
            },
          }}
        >
          {collapsed ? <StorageIcon fontSize="small" /> : <ChevronLeftIcon />}
        </IconButton>
      </Toolbar>
      <Divider />

      <List sx={{ flexGrow: 1, pt: 1 }}>
        {availableItems.map((item) => renderMenuButton(item))}
        {unavailableItems.length > 0 && (
          <>
            {!collapsed && (
              <Typography
                variant="caption"
                color="text.secondary"
                sx={{ px: 2.5, pt: 1.5, pb: 0.5, textTransform: 'uppercase', letterSpacing: 0.8 }}
              >
                Недоступно
              </Typography>
            )}
            {unavailableItems.map((item) => renderMenuButton(item, true))}
          </>
        )}
      </List>

      <Divider />

      <Box
        sx={{
          px: 1.5,
          pt: 2,
          pb: 1.5,
          display: 'flex',
          justifyContent: collapsed ? 'center' : 'stretch',
          borderTop: 1,
          borderColor: 'divider',
          bgcolor: alpha(theme.palette.action.hover, theme.palette.mode === 'dark' ? 0.12 : 0.5),
        }}
      >
        {!collapsed ? (
          <Stack spacing={1.5} sx={{ width: '100%' }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.25, minWidth: 0 }}>
              <Avatar sx={{ width: 40, height: 40, bgcolor: 'primary.main', flexShrink: 0 }}>
                {(username ?? '?').charAt(0).toUpperCase()}
              </Avatar>
              <Box sx={{ minWidth: 0, flex: 1 }}>
                <Typography variant="subtitle2" noWrap sx={{ fontWeight: 600, lineHeight: 1.3 }}>
                  {username ?? 'User'}
                </Typography>
                {roleLabel.trim().toLowerCase() !== (username ?? '').trim().toLowerCase() && (
                  <Chip
                    label={roleLabel}
                    size="small"
                    variant="outlined"
                    color="primary"
                    sx={{ mt: 0.5, height: 22, '& .MuiChip-label': { px: 1, fontSize: '0.7rem' } }}
                  />
                )}
              </Box>
            </Box>

            <Stack direction="row" spacing={0.5} justifyContent="flex-start" alignItems="center" sx={{ pl: 0.25 }}>
              <ThemeToggle />
              <LanguageSelector />
            </Stack>

            <Button
              fullWidth
              variant="outlined"
              color="error"
              size="small"
              startIcon={<LogoutIcon />}
              onClick={handleLogout}
              sx={{ textTransform: 'none', borderRadius: 1.5 }}
            >
              {t('auth.logout')}
            </Button>
          </Stack>
        ) : (
          <Stack sx={{ alignItems: 'center', gap: 0.75, width: '100%' }}>
            <Tooltip title={username} placement="right">
              <Avatar sx={{ width: 40, height: 40, bgcolor: 'primary.main' }}>
                {(username ?? '?').charAt(0).toUpperCase()}
              </Avatar>
            </Tooltip>
            <ThemeToggle />
            <LanguageSelector />
            <Tooltip title={t('auth.logout')} placement="right">
              <IconButton color="error" onClick={handleLogout}>
                <LogoutIcon />
              </IconButton>
            </Tooltip>
          </Stack>
        )}
      </Box>
    </Box>
  );

  return (
    <Box sx={{ display: 'flex', minHeight: '100vh' }}>
      {/* Mobile burger menu button */}
          <IconButton
            color="inherit"
            aria-label="open drawer"
            edge="start"
            onClick={handleDrawerToggle}
        sx={{
          position: 'fixed',
          top: 16,
          left: 16,
          zIndex: 1300,
          display: { sm: 'none' },
          bgcolor: 'background.paper',
          boxShadow: 2,
          '&:hover': {
            bgcolor: 'background.paper',
          },
        }}
          >
            <MenuIcon />
          </IconButton>

      <Box component="nav" sx={{ width: { sm: drawerWidth }, flexShrink: { sm: 0 } }}>
        <Drawer
          variant="temporary"
          open={mobileOpen}
          onClose={handleDrawerToggle}
          ModalProps={{
            keepMounted: true,
          }}
          sx={{
            display: { xs: 'block', sm: 'none' },
            '& .MuiDrawer-paper': { boxSizing: 'border-box', width: drawerWidthExpanded },
          }}
        >
          {drawer}
        </Drawer>
        <Drawer
          variant="permanent"
          sx={{
            display: { xs: 'none', sm: 'block' },
            '& .MuiDrawer-paper': {
              boxSizing: 'border-box',
              width: drawerWidth,
              transition: 'width 0.3s ease',
            },
          }}
          open
        >
          {drawer}
        </Drawer>
      </Box>
      <Box
        component="main"
        sx={{
          flexGrow: 1,
          display: 'flex',
          flexDirection: 'column',
          p: 3,
          width: { sm: `calc(100% - ${drawerWidth}px)` },
          transition: 'width 0.3s ease, margin 0.3s ease',
          pt: { xs: 8, sm: 3 },
        }}
      >
        <Container maxWidth="xl" sx={{ flexGrow: 1, display: 'flex', flexDirection: 'column' }}>
          {children || <Outlet />}
        </Container>
      </Box>
    </Box>
  );
};

export default MainLayout;
