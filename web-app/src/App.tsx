import React from 'react';
import { Routes, Route, Navigate } from 'react-router-dom';
import { Box, Container, AppBar, Toolbar, Typography } from '@mui/material';

function App() {
  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: '100vh' }}>
      <AppBar position="static">
        <Toolbar>
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            OpenMaxIO Object Browser
          </Typography>
        </Toolbar>
      </AppBar>

      <Container component="main" sx={{ mt: 4, mb: 4, flex: 1 }}>
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/login" element={<LoginPage />} />
          <Route path="/buckets" element={<BucketsPage />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </Container>

      <Box
        component="footer"
        sx={{
          py: 3,
          px: 2,
          mt: 'auto',
          backgroundColor: (theme) =>
            theme.palette.mode === 'light'
              ? theme.palette.grey[200]
              : theme.palette.grey[800],
        }}
      >
        <Container maxWidth="sm">
          <Typography variant="body2" color="text.secondary" align="center">
            {'OpenMaxIO Object Browser © '}
            {new Date().getFullYear()}
          </Typography>
        </Container>
      </Box>
    </Box>
  );
}

// Placeholder pages
const HomePage = () => (
  <Box>
    <Typography variant="h4" gutterBottom>
      Welcome to OpenMaxIO Object Browser
    </Typography>
    <Typography variant="body1">
      A modern S3-compatible object storage web console
    </Typography>
  </Box>
);

const LoginPage = () => (
  <Box>
    <Typography variant="h4" gutterBottom>
      Login
    </Typography>
    <Typography variant="body1">
      Login page will be implemented here
    </Typography>
  </Box>
);

const BucketsPage = () => (
  <Box>
    <Typography variant="h4" gutterBottom>
      Buckets
    </Typography>
    <Typography variant="body1">
      Buckets list will be displayed here
    </Typography>
  </Box>
);

export default App;
