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
        backgroundColor: (theme) => theme.palette.background.default,
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
        <Card
            elevation={3}
            sx={{
              borderRadius: 2,
              overflow: "hidden",
              bgcolor: "background.paper",
            }}
          >
            <Box
              sx={{
                p: 2,
                borderBottom: (theme) => `1px solid ${theme.palette.divider}`,
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              }}
            >
              <Storage sx={{ fontSize: 40, color: "primary.main", mr: 2 }} />
              <Box>
                <Typography
                  variant="h5"
                  sx={{ color: "text.primary", fontWeight: 700 }}
                >
                  {t("app.title")}
                </Typography>
                <Typography
                  variant="body2"
                  sx={{ color: "text.secondary" }}
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
                      transition: "border-color 0.2s ease",
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
                      transition: "border-color 0.2s ease",
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
                    transition: "background-color 0.2s ease, box-shadow 0.2s ease",
                    "&:hover": {
                      boxShadow: 2,
                    },
                    "&:disabled": {
                      background: (theme) =>
                        theme.palette.action.disabledBackground,
                    },
                  }}
                >
                  {t("auth.login")}
                </Button>
              </Box>
            </CardContent>
          </Card>

        <Typography
          variant="body2"
          align="center"
          sx={{
            mt: 3,
            color: "text.secondary",
          }}
        >
          © 2025 Object Storage Console
        </Typography>
      </Container>
    </Box>
  );
};

export default LoginPage;
