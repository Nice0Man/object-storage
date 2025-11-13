import React, { useState, useEffect } from "react";
import {
  Box,
  TextField,
  Button,
  Typography,
  Container,
  InputAdornment,
  IconButton,
  Card,
  CardContent,
  Fade,
  Zoom,
} from "@mui/material";
import {
  Visibility,
  VisibilityOff,
  Lock,
  Person,
  Storage,
} from "@mui/icons-material";
import { useNavigate, useLocation } from "react-router-dom";
import { useTranslation } from "react-i18next";
import { useAppDispatch } from "../hooks/useAppDispatch";
import { useAppSelector } from "../hooks/useAppSelector";
import { login, selectAuth, clearError } from "../store/authSlice";
import ErrorAlert from "../components/Common/ErrorAlert";
import Loader from "../components/Common/Loader";
import ThemeToggle from "../components/Settings/ThemeToggle";
import LanguageSelector from "../components/Settings/LanguageSelector";

const LoginPage: React.FC = () => {
  const [accessKey, setAccessKey] = useState("");
  const [secretKey, setSecretKey] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const navigate = useNavigate();
  const location = useLocation();
  const dispatch = useAppDispatch();
  const { t } = useTranslation();
  const { isAuthenticated, loading, error } = useAppSelector(selectAuth);

  const from = (location.state as any)?.from?.pathname || "/";

  useEffect(() => {
    if (isAuthenticated) {
      navigate(from, { replace: true });
    }
  }, [isAuthenticated, navigate, from]);

  useEffect(() => {
    return () => {
      dispatch(clearError());
    };
  }, [dispatch]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();

    if (!accessKey || !secretKey) {
      return;
    }

    try {
      await dispatch(login({ accessKey, secretKey })).unwrap();
    } catch (err) {
      // Error is handled by the slice
    }
  };

  const handleClickShowPassword = () => {
    setShowPassword(!showPassword);
  };

  if (loading) {
    return (
      <Container maxWidth="sm">
        <Box sx={{ mt: 8 }}>
          <Loader message={t("common.loading")} />
        </Box>
      </Container>
    );
  }

  return (
    <Box
      sx={{
        minHeight: "100vh",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        background: (theme) =>
          theme.palette.mode === "light"
            ? "linear-gradient(135deg, #667eea 0%, #764ba2 100%)"
            : "linear-gradient(135deg, #1e3a8a 0%, #312e81 100%)",
        position: "relative",
      }}
    >
      {/* Controls in top-right corner */}
      <Box
        sx={{
          position: "absolute",
          top: 16,
          right: 16,
          display: "flex",
          gap: 1,
        }}
      >
        <ThemeToggle />
        <LanguageSelector />
      </Box>

      <Container maxWidth="sm">
        <Fade in timeout={800}>
          <Card
            elevation={24}
            sx={{
              borderRadius: 4,
              overflow: "hidden",
              backdropFilter: "blur(10px)",
              bgcolor: (theme) =>
                theme.palette.mode === "light"
                  ? "rgba(255, 255, 255, 0.95)"
                  : "rgba(30, 41, 59, 0.95)",
            }}
          >
            <Box
              sx={{
                p: 2,
                background: (theme) =>
                  theme.palette.mode === "light"
                    ? "linear-gradient(135deg, #667eea 0%, #764ba2 100%)"
                    : "linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%)",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              }}
            >
              <Zoom in timeout={600}>
                <Storage sx={{ fontSize: 48, color: "white", mr: 2 }} />
              </Zoom>
              <Box>
                <Typography
                  variant="h5"
                  sx={{ color: "white", fontWeight: 700 }}
                >
                  {t("app.title")}
                </Typography>
                <Typography
                  variant="body2"
                  sx={{ color: "rgba(255, 255, 255, 0.9)" }}
                >
                  {t("app.subtitle")}
                </Typography>
              </Box>
            </Box>

            <CardContent sx={{ p: 4 }}>
              <Box sx={{ mb: 3, textAlign: "center" }}>
                <Typography variant="h6" gutterBottom fontWeight={600}>
                  {t("auth.loginTitle")}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  {t("auth.loginDescription")}
                </Typography>
              </Box>

              <ErrorAlert
                error={error}
                onClose={() => dispatch(clearError())}
              />

              <Box
                component="form"
                onSubmit={handleSubmit}
                noValidate
                sx={{ mt: 2 }}
              >
                <TextField
                  margin="normal"
                  required
                  fullWidth
                  id="accessKey"
                  label={t("auth.accessKey")}
                  name="accessKey"
                  autoComplete="username"
                  autoFocus
                  value={accessKey}
                  onChange={(e) => setAccessKey(e.target.value)}
                  InputProps={{
                    startAdornment: (
                      <InputAdornment position="start">
                        <Person color="action" />
                      </InputAdornment>
                    ),
                  }}
                  sx={{
                    "& .MuiOutlinedInput-root": {
                      transition: "all 0.3s",
                      "&:hover": {
                        transform: "translateY(-2px)",
                      },
                    },
                  }}
                />
                <TextField
                  margin="normal"
                  required
                  fullWidth
                  name="secretKey"
                  label={t("auth.secretKey")}
                  type={showPassword ? "text" : "password"}
                  id="secretKey"
                  autoComplete="current-password"
                  value={secretKey}
                  onChange={(e) => setSecretKey(e.target.value)}
                  InputProps={{
                    startAdornment: (
                      <InputAdornment position="start">
                        <Lock color="action" />
                      </InputAdornment>
                    ),
                    endAdornment: (
                      <InputAdornment position="end">
                        <IconButton
                          aria-label="toggle password visibility"
                          onClick={handleClickShowPassword}
                          edge="end"
                        >
                          {showPassword ? <VisibilityOff /> : <Visibility />}
                        </IconButton>
                      </InputAdornment>
                    ),
                  }}
                  sx={{
                    "& .MuiOutlinedInput-root": {
                      transition: "all 0.3s",
                      "&:hover": {
                        transform: "translateY(-2px)",
                      },
                    },
                  }}
                />
                <Button
                  type="submit"
                  fullWidth
                  variant="contained"
                  size="large"
                  disabled={!accessKey || !secretKey}
                  sx={{
                    mt: 3,
                    mb: 2,
                    py: 1.5,
                    fontSize: "1.1rem",
                    fontWeight: 600,
                    background: (theme) =>
                      theme.palette.mode === "light"
                        ? "linear-gradient(135deg, #667eea 0%, #764ba2 100%)"
                        : "linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%)",
                    transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                    "&:hover": {
                      transform: "translateY(-2px)",
                      boxShadow: "0 8px 16px rgba(102, 126, 234, 0.4)",
                    },
                    "&:disabled": {
                      background: (theme) =>
                        theme.palette.action.disabledBackground,
                      transform: "none",
                    },
                  }}
                >
                  {t("auth.login")}
                </Button>
              </Box>
            </CardContent>
          </Card>
        </Fade>

        {/* Footer */}
        <Fade in timeout={1200}>
          <Typography
            variant="body2"
            align="center"
            sx={{
              mt: 3,
              color: "white",
              textShadow: "0 2px 4px rgba(0,0,0,0.3)",
            }}
          >
            © 2025 Object Storage Console. Built with ❤️
          </Typography>
        </Fade>
      </Container>
    </Box>
  );
};

export default LoginPage;
