import React, { useEffect, useState } from "react";
import {
  Box,
  Grid,
  Paper,
  Typography,
  Chip,
  Card,
  CardContent,
  IconButton,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  useTheme,
  alpha,
  CircularProgress,
} from "@mui/material";
import {
  CheckCircle,
  Error as ErrorIcon,
  Refresh,
  Info,
  Cloud,
  Storage,
  Computer,
  Speed,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";
import apiClient from "../api/client";
import type { HealthResponse, VersionResponse } from "../api/types";

const SystemHealthPage: React.FC = () => {
  const theme = useTheme();
  const { t } = useTranslation();
  const [health, setHealth] = useState<HealthResponse | null>(null);
  const [version, setVersion] = useState<VersionResponse | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [lastCheck, setLastCheck] = useState<Date>(new Date());

  const loadHealthData = async () => {
    setLoading(true);
    setError(null);
    try {
      const [healthData, versionData] = await Promise.all([
        apiClient.getHealth(),
        apiClient.getVersion(),
      ]);
      setHealth(healthData);
      setVersion(versionData);
      setLastCheck(new Date());
    } catch (err: any) {
      setError(err.response?.data?.message || "Failed to load system health");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadHealthData();
    // Auto-refresh every 30 seconds
    const interval = setInterval(loadHealthData, 30000);
    return () => clearInterval(interval);
  }, []);

  const getStatusColor = (status: string) => {
    switch (status.toLowerCase()) {
      case "healthy":
      case "ok":
      case "online":
        return theme.palette.success.main;
      case "degraded":
      case "warning":
        return theme.palette.warning.main;
      case "unhealthy":
      case "error":
      case "offline":
        return theme.palette.error.main;
      default:
        return theme.palette.grey[500];
    }
  };

  const getStatusIcon = (status: string) => {
    switch (status.toLowerCase()) {
      case "healthy":
      case "ok":
      case "online":
        return <CheckCircle />;
      case "degraded":
      case "warning":
        return <ErrorIcon />;
      case "unhealthy":
      case "error":
      case "offline":
        return <ErrorIcon />;
      default:
        return <Info />;
    }
  };

  if (error) {
    return (
      <Box>
        <Paper sx={{ p: 3, bgcolor: alpha(theme.palette.error.main, 0.1) }}>
          <Box sx={{ display: "flex", alignItems: "center", gap: 2 }}>
            <ErrorIcon color="error" sx={{ fontSize: 40 }} />
            <Box>
              <Typography variant="h6" color="error">
                {t("system.health.error")}
              </Typography>
              <Typography variant="body2" color="text.secondary">
                {error}
              </Typography>
            </Box>
            <Box sx={{ ml: "auto" }}>
              <IconButton onClick={loadHealthData} color="primary">
                <Refresh />
              </IconButton>
            </Box>
          </Box>
        </Paper>
      </Box>
    );
  }

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', flex: 1 }}>
      {/* Header */}
      <Box
        sx={{
          display: "flex",
          justifyContent: "space-between",
          alignItems: "center",
          mb: 4,
        }}
      >
        <Box>
          <Typography variant="h4" sx={{ fontWeight: 700, mb: 1 }}>
            {t("system.health.title")}
          </Typography>
          <Typography variant="body2" color="text.secondary">
            {t("system.health.lastCheck")}: {lastCheck.toLocaleString()}
          </Typography>
        </Box>
        <IconButton
          onClick={loadHealthData}
          disabled={loading}
          color="primary"
          size="large"
        >
          <Refresh />
        </IconButton>
      </Box>

      {loading ? (
        <Box sx={{ display: "flex", justifyContent: "center", py: 8 }}>
          <CircularProgress />
        </Box>
      ) : (
        <>
          {/* Overall Status */}
          <Paper
            sx={{
              p: 3,
              mb: 3,
              background: `linear-gradient(135deg, ${alpha(
                getStatusColor(health?.status || ""),
                0.1,
              )}, ${alpha(getStatusColor(health?.status || ""), 0.05)})`,
              border: `2px solid ${getStatusColor(health?.status || "")}`,
            }}
          >
            <Box sx={{ display: "flex", alignItems: "center", gap: 2 }}>
              <Box
                sx={{
                  color: getStatusColor(health?.status || ""),
                  fontSize: 60,
                }}
              >
                {getStatusIcon(health?.status || "")}
              </Box>
              <Box>
                <Typography variant="h4" sx={{ fontWeight: 700, mb: 1 }}>
                  {health?.status?.toUpperCase()}
                </Typography>
                <Typography variant="body1" color="text.secondary">
                  {health?.service} •{" "}
                  {health?.timestamp
                    ? new Date(typeof health.timestamp === 'number' && health.timestamp < 10000000000 ? health.timestamp * 1000 : health.timestamp).toLocaleString()
                    : ""}
                </Typography>
              </Box>
            </Box>
          </Paper>

          {/* System Components */}
          <Grid container spacing={3} sx={{ mb: 3 }}>
            {health?.checks &&
              Object.entries(health.checks).map(([component, status]) => (
                <Grid item xs={12} sm={6} md={3} key={component}>
                  <Card
                    sx={{
                      height: "100%",
                      borderLeft: `4px solid ${getStatusColor(status as string)}`,
                      transition: "all 0.3s",
                      "&:hover": {
                        transform: "translateY(-4px)",
                        boxShadow: theme.shadows[8],
                      },
                    }}
                  >
                    <CardContent>
                      <Box
                        sx={{ display: "flex", alignItems: "center", mb: 2 }}
                      >
                        <Box
                          sx={{
                            color: getStatusColor(status as string),
                            backgroundColor: alpha(
                              getStatusColor(status as string),
                              0.1,
                            ),
                            borderRadius: 2,
                            p: 1,
                            mr: 2,
                          }}
                        >
                          {component === "s3" && <Cloud />}
                          {component === "storage" && <Storage />}
                          {component === "database" && <Computer />}
                          {!["s3", "storage", "database"].includes(
                            component,
                          ) && <Speed />}
                        </Box>
                        <Box sx={{ color: getStatusColor(status as string) }}>
                          {getStatusIcon(status as string)}
                        </Box>
                      </Box>
                      <Typography
                        variant="h6"
                        sx={{ textTransform: "capitalize", mb: 1 }}
                      >
                        {component}
                      </Typography>
                      <Chip
                        label={String(status)}
                        size="small"
                        sx={{
                          bgcolor: alpha(getStatusColor(status as string), 0.2),
                          color: getStatusColor(status as string),
                          fontWeight: 600,
                          textTransform: "uppercase",
                        }}
                      />
                    </CardContent>
                  </Card>
                </Grid>
              ))}
          </Grid>

          {/* Version Information */}
          {version && (
            <TableContainer component={Paper}>
              <Table>
                <TableHead>
                  <TableRow>
                    <TableCell colSpan={2}>
                      <Typography variant="h6" sx={{ fontWeight: 600 }}>
                        {t("system.health.versionInfo")}
                      </Typography>
                    </TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  <TableRow hover>
                    <TableCell sx={{ fontWeight: 600, width: "200px" }}>
                      {t("system.health.version")}
                    </TableCell>
                    <TableCell>{version.version}</TableCell>
                  </TableRow>
                  <TableRow hover>
                    <TableCell sx={{ fontWeight: 600 }}>
                      {t("system.health.apiVersion")}
                    </TableCell>
                    <TableCell>{version.api_version}</TableCell>
                  </TableRow>
                  <TableRow hover>
                    <TableCell sx={{ fontWeight: 600 }}>
                      {t("system.health.buildDate")}
                    </TableCell>
                    <TableCell>
                      {version.build_date} {version.build_time}
                    </TableCell>
                  </TableRow>
                  <TableRow hover>
                    <TableCell sx={{ fontWeight: 600 }}>
                      {t("system.health.compiler")}
                    </TableCell>
                    <TableCell>{version.compiler}</TableCell>
                  </TableRow>
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </>
      )}
    </Box>
  );
};

export default SystemHealthPage;
