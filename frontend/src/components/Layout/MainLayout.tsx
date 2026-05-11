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
  ChevronRight as ChevronRightIcon,
} from '@mui/icons-material';
import { useNavigate, useLocation } from 'react-router-dom';
import { useAppDispatch } from '../../hooks/useAppDispatch';
import { useAppSelector } from '../../hooks/useAppSelector';
import { logout, selectUsername } from '../../store/authSlice';
import { useTranslation } from 'react-i18next';
import ThemeToggle from '../Settings/ThemeToggle';
import LanguageSelector from '../Settings/LanguageSelector';
import { Outlet } from 'react-router-dom';

const drawerWidthExpanded = 240;
const drawerWidthCollapsed = 72;

interface MainLayoutProps {
  children?: React.ReactNode;
}

const MainLayout: React.FC<MainLayoutProps> = ({ children }) => {
  const [mobileOpen, setMobileOpen] = React.useState(false);
  const [collapsed, setCollapsed] = React.useState(false);
  const navigate = useNavigate();
  const location = useLocation();
  const dispatch = useAppDispatch();
  const username = useAppSelector(selectUsername);
  const { t } = useTranslation();

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

  const menuItems = [
    { text: 'nav.dashboard', icon: <DashboardIcon />, path: '/' },
    { text: 'nav.buckets', icon: <StorageIcon />, path: '/buckets' },
    { text: 'nav.objects', icon: <FolderIcon />, path: '/objects' },
    { text: 'nav.users', icon: <PeopleIcon />, path: '/users' },
    { text: 'nav.system', icon: <MonitorHeartIcon />, path: '/system-health' },
    { text: 'nav.profile', icon: <PersonIcon />, path: '/profile' },
  ];

  const drawer = (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
      {/* Header with collapse button */}
      <Toolbar sx={{ background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)', justifyContent: 'space-between' }}>
        {!collapsed && (
        <Typography variant="h6" noWrap component="div" sx={{ color: 'white', fontWeight: 600 }}>
          {t('app.title')}
        </Typography>
        )}
        <IconButton
          onClick={handleCollapse}
          sx={{
            color: 'white',
            ml: collapsed ? 'auto' : 0,
            mr: collapsed ? 'auto' : 0,
          }}
        >
          {collapsed ? <ChevronRightIcon /> : <ChevronLeftIcon />}
        </IconButton>
      </Toolbar>
      <Divider />

      {/* Main menu */}
      <List sx={{ flexGrow: 1 }}>
        {menuItems.map((item) => (
          <ListItem key={item.text} disablePadding>
            <Tooltip title={collapsed ? t(item.text) : ''} placement="right">
            <ListItemButton
              selected={location.pathname === item.path}
              onClick={() => {
                navigate(item.path);
                setMobileOpen(false);
              }}
              sx={{
                borderRadius: '10px',
                mx: 1,
                my: 0.5,
                  justifyContent: collapsed ? 'center' : 'flex-start',
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
              }}
            >
                <ListItemIcon sx={{ minWidth: collapsed ? 'auto' : 40, justifyContent: 'center' }}>
                  {item.icon}
                </ListItemIcon>
                {!collapsed && <ListItemText primary={t(item.text)} />}
            </ListItemButton>
            </Tooltip>
          </ListItem>
        ))}
      </List>

      <Divider />

      {/* Bottom section with user info and settings */}
      <Box sx={{ p: 2 }}>
        {!collapsed ? (
          <Box>
            {/* User info */}
            <Box sx={{ display: 'flex', alignItems: 'center', mb: 2, px: 1 }}>
              <Avatar sx={{ width: 32, height: 32, bgcolor: 'primary.main', mr: 1 }}>
                {username?.charAt(0).toUpperCase()}
              </Avatar>
              <Typography variant="body2" noWrap sx={{ fontWeight: 600 }}>
                {username}
              </Typography>
            </Box>

            {/* Theme and Language */}
            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-around', mb: 2 }}>
              <ThemeToggle />
              <LanguageSelector />
            </Box>

            {/* Logout button */}
            <Button
              fullWidth
              variant="outlined"
              color="error"
              startIcon={<LogoutIcon />}
              onClick={handleLogout}
            >
              {t('auth.logout')}
            </Button>
          </Box>
        ) : (
          <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 1 }}>
            <Tooltip title={username} placement="right">
              <Avatar sx={{ width: 40, height: 40, bgcolor: 'primary.main' }}>
                {username?.charAt(0).toUpperCase()}
              </Avatar>
            </Tooltip>
            <ThemeToggle />
            <LanguageSelector />
            <Tooltip title={t('auth.logout')} placement="right">
              <IconButton color="error" onClick={handleLogout}>
                <LogoutIcon />
              </IconButton>
            </Tooltip>
          </Box>
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
