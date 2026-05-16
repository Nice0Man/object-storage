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
  LinearProgress,
  Tabs,
  Tab,
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
  Dns,
  SdStorage,
  Layers,
  Timeline,
  Warning,
  Terminal as TerminalIcon,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";
import { useAppSelector } from "../hooks/useAppSelector";
import { selectAuth } from "../store/authSlice";
import apiClient from "../api/client";
import AdminTerminalPanel from "../components/System/AdminTerminalPanel";
import type {
  HealthResponse,
  VersionResponse,
  ReadyResponse,
  LiveResponse,
  InfrastructureSummary,
  InfrastructureServersResponse,
  InfrastructureDrivesResponse,
  InfrastructureHealStatus,
} from "../api/types";
import ApiErrorsChart from "../components/Charts/ApiErrorsChart";
import DataThroughputChart from "../components/Charts/DataThroughputChart";

// Helper function to format bytes
const formatBytes = (bytes: number, decimals: number = 2): string => {
  if (bytes === 0) return "0 Bytes";
  const k = 1024;
  const dm = decimals < 0 ? 0 : decimals;
  const sizes = ["Bytes", "KB", "MB", "GB", "TB", "PB"];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + " " + sizes[i];
};

interface TabPanelProps {
  children?: React.ReactNode;
  index: number;
  value: number;
}

const TabPanel: React.FC<TabPanelProps> = ({ children, value, index }) => (
  <div hidden={value !== index} style={{ paddingTop: 16 }}>
    {value === index && children}
  </div>
);

const SystemHealthPage: React.FC = () => {
  const theme = useTheme();
  const { t } = useTranslation();
  const { isAdmin } = useAppSelector(selectAuth);
  const [tabValue, setTabValue] = useState(0);
  const [health, setHealth] = useState<HealthResponse | null>(null);
  const [ready, setReady] = useState<ReadyResponse | null>(null);
  const [live, setLive] = useState<LiveResponse | null>(null);
  const [version, setVersion] = useState<VersionResponse | null>(null);
  const [infraSummary, setInfraSummary] = useState<InfrastructureSummary | null>(null);
  const [infraServers, setInfraServers] = useState<InfrastructureServersResponse | null>(null);
  const [infraDrives, setInfraDrives] = useState<InfrastructureDrivesResponse | null>(null);
  const [infraHeal, setInfraHeal] = useState<InfrastructureHealStatus | null>(null);
  const [serverStats, setServerStats] = useState<any>(null);
  const [driveStats, setDriveStats] = useState<any>(null);
  const [poolStats, setPoolStats] = useState<any[]>([]);
  const [apiErrorsData, setApiErrorsData] = useState<any[]>([]);
  const [throughputData, setThroughputData] = useState<any[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [lastCheck, setLastCheck] = useState<Date>(new Date());

  const loadApiErrorsStats = async (range: "1h" | "6h" | "24h" | "7d") => {
    try {
      const apiErrorsResponse = await apiClient.getApiErrorStats(range);
      setApiErrorsData(apiErrorsResponse?.data || []);
    } catch (e) {
      console.warn("Failed to fetch API error stats:", e);
      setApiErrorsData([]);
    }
  };

  const loadThroughputStats = async (range: "1h" | "6h" | "24h" | "7d") => {
    try {
      const throughputResponse = await apiClient.getDataThroughputStats(range);
      setThroughputData(throughputResponse?.data || []);
    } catch (e) {
      console.warn("Failed to fetch throughput stats:", e);
      setThroughputData([]);
    }
  };

  const loadHealthData = async () => {
    setLoading(true);
    setError(null);
    try {
      const [healthData, versionData, readyData, liveData] = await Promise.all([
        apiClient.getHealth(),
        apiClient.getVersion(),
        apiClient.getReady(),
        apiClient.getLive(),
      ]);
      setHealth(healthData);
      setVersion(versionData);
      setReady(readyData);
      setLive(liveData);
      setLastCheck(new Date());

      try {
        const [summary, servers, drives, heal] = await Promise.all([
          apiClient.getInfrastructureSummary(),
          apiClient.listInfrastructureServers(),
          apiClient.listInfrastructureDrives(),
          apiClient.getInfrastructureHealStatus(),
        ]);
        setInfraSummary(summary);
        setInfraServers(servers);
        setInfraDrives(drives);
        setInfraHeal(heal);
      } catch (e) {
        console.warn("Failed to fetch infrastructure API:", e);
        setInfraSummary(null);
        setInfraServers(null);
        setInfraDrives(null);
        setInfraHeal(null);
      }

      // Load additional infrastructure stats
      try {
        const serverResponse = await apiClient.getServerStats();
        setServerStats(serverResponse);
      } catch (e) {
        console.warn("Failed to fetch server stats:", e);
        setServerStats({ servers: [], online_count: 0, offline_count: 0, total_count: 0 });
      }

      try {
        const driveResponse = await apiClient.getDriveStats();
        setDriveStats(driveResponse);
      } catch (e) {
        console.warn("Failed to fetch drive stats:", e);
        setDriveStats({ drives: [], online_count: 0, offline_count: 0, total_count: 0 });
      }

      try {
        const poolResponse = await apiClient.getPoolStats();
        setPoolStats(Array.isArray(poolResponse) ? poolResponse : poolResponse?.pools || []);
      } catch (e) {
        console.warn("Failed to fetch pool stats:", e);
        setPoolStats([]);
      }

      await loadApiErrorsStats("24h");
      await loadThroughputStats("24h");
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
          <Box sx={{ display: "flex", gap: 1, mt: 1, flexWrap: "wrap" }}>
            <Chip
              size="small"
              label={`Ready: ${ready?.ready ? "yes" : "no"}`}
              color={ready?.ready ? "success" : "warning"}
              variant="outlined"
            />
            <Chip
              size="small"
              label={`Live: ${live?.alive ? "yes" : "no"}`}
              color={live?.alive ? "success" : "error"}
              variant="outlined"
            />
          </Box>
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
              <Box sx={{ flex: 1 }}>
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
              {/* Quick Stats */}
              <Box sx={{ display: "flex", gap: 3 }}>
                <Box sx={{ textAlign: "center" }}>
                  <Typography variant="h4" sx={{ fontWeight: 700, color: theme.palette.primary.main }}>
                    {serverStats?.total_count || 0}
                  </Typography>
                  <Typography variant="caption" color="text.secondary">Servers</Typography>
                </Box>
                <Box sx={{ textAlign: "center" }}>
                  <Typography variant="h4" sx={{ fontWeight: 700, color: theme.palette.success.main }}>
                    {driveStats?.total_count || 0}
                  </Typography>
                  <Typography variant="caption" color="text.secondary">Drives</Typography>
                </Box>
                <Box sx={{ textAlign: "center" }}>
                  <Typography variant="h4" sx={{ fontWeight: 700, color: theme.palette.info.main }}>
                    {poolStats?.length || 0}
                  </Typography>
                  <Typography variant="caption" color="text.secondary">Pools</Typography>
                </Box>
              </Box>
            </Box>
          </Paper>

          {/* Tabs */}
          <Paper sx={{ mb: 3 }}>
            <Tabs value={tabValue} onChange={(_, v) => setTabValue(v)}>
              <Tab icon={<Speed />} label="Overview" iconPosition="start" />
              <Tab icon={<Dns />} label="Servers" iconPosition="start" />
              <Tab icon={<SdStorage />} label="Drives" iconPosition="start" />
              <Tab icon={<Layers />} label="Pools" iconPosition="start" />
              <Tab icon={<Timeline />} label="Metrics" iconPosition="start" />
              <Tab icon={<Storage />} label="Infrastructure" iconPosition="start" />
              {isAdmin && (
                <Tab icon={<TerminalIcon />} label="Terminal" iconPosition="start" />
              )}
            </Tabs>
          </Paper>

          {/* Overview Tab */}
          <TabPanel value={tabValue} index={0}>
            <Grid container spacing={3}>
          {/* System Components */}
            {health?.checks &&
              Object.entries(health.checks).map(([component, status]) => (
                <Grid item xs={12} sm={6} md={3} key={component}>
                  <Card
                    sx={{
                      height: "100%",
                      borderLeft: `4px solid ${getStatusColor(status as string)}`,
                    }}
                  >
                    <CardContent>
                        <Box sx={{ display: "flex", alignItems: "center", mb: 2 }}>
                        <Box
                          sx={{
                            color: getStatusColor(status as string),
                              backgroundColor: alpha(getStatusColor(status as string), 0.1),
                            borderRadius: 2,
                            p: 1,
                            mr: 2,
                          }}
                        >
                          {component === "s3" && <Cloud />}
                          {component === "storage" && <Storage />}
                          {component === "database" && <Computer />}
                            {!["s3", "storage", "database"].includes(component) && <Speed />}
                        </Box>
                        <Box sx={{ color: getStatusColor(status as string) }}>
                          {getStatusIcon(status as string)}
                        </Box>
                      </Box>
                        <Typography variant="h6" sx={{ textTransform: "capitalize", mb: 1 }}>
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

              {/* Version Info */}
              <Grid item xs={12}>
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
                          <TableCell sx={{ fontWeight: 600, width: "200px" }}>Version</TableCell>
                    <TableCell>{version.version}</TableCell>
                  </TableRow>
                  <TableRow hover>
                          <TableCell sx={{ fontWeight: 600 }}>API Version</TableCell>
                    <TableCell>{version.api_version}</TableCell>
                  </TableRow>
                  <TableRow hover>
                          <TableCell sx={{ fontWeight: 600 }}>Build Date</TableCell>
                          <TableCell>{version.build_date} {version.build_time}</TableCell>
                  </TableRow>
                  <TableRow hover>
                          <TableCell sx={{ fontWeight: 600 }}>Compiler</TableCell>
                    <TableCell>{version.compiler}</TableCell>
                  </TableRow>
                </TableBody>
              </Table>
            </TableContainer>
          )}
              </Grid>
            </Grid>
          </TabPanel>

          {/* Servers Tab */}
          <TabPanel value={tabValue} index={1}>
            <Grid container spacing={3}>
              {/* Server Summary */}
              <Grid item xs={12} md={4}>
                <Card>
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2, mb: 2 }}>
                      <Dns sx={{ fontSize: 40, color: theme.palette.primary.main }} />
                      <Box>
                        <Typography variant="h4" sx={{ fontWeight: 700 }}>
                          {serverStats?.total_count || 0}
                        </Typography>
                        <Typography variant="body2" color="text.secondary">Total Servers</Typography>
                      </Box>
                    </Box>
                    <Box sx={{ display: "flex", gap: 2, mb: 2 }}>
                      <Chip
                        label={`${serverStats?.online_count || 0} Online`}
                        color="success"
                        size="small"
                      />
                      <Chip
                        label={`${serverStats?.offline_count || 0} Offline`}
                        color="error"
                        size="small"
                      />
                    </Box>
                    <LinearProgress
                      variant="determinate"
                      value={serverStats?.total_count > 0 ? (serverStats.online_count / serverStats.total_count) * 100 : 0}
                      sx={{ height: 8, borderRadius: 1 }}
                    />
                  </CardContent>
                </Card>
              </Grid>

              {/* Server List */}
              <Grid item xs={12} md={8}>
                <TableContainer component={Paper}>
                  <Table>
                    <TableHead>
                      <TableRow>
                        <TableCell>Server Name</TableCell>
                        <TableCell>Endpoint</TableCell>
                        <TableCell>Status</TableCell>
                        <TableCell>Uptime</TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {(serverStats?.servers || []).length > 0 ? (
                        serverStats.servers.map((server: any) => (
                          <TableRow key={server.id} hover>
                            <TableCell>{server.name || server.id}</TableCell>
                            <TableCell sx={{ fontFamily: "monospace" }}>{server.endpoint}</TableCell>
                            <TableCell>
                              <Chip
                                label={server.status}
                                color={server.status === "online" ? "success" : "error"}
                                size="small"
                              />
                            </TableCell>
                            <TableCell>{server.uptime ? `${Math.floor(server.uptime / 3600)}h` : "-"}</TableCell>
                          </TableRow>
                        ))
                      ) : (
                        <TableRow>
                          <TableCell colSpan={4} align="center">No servers configured</TableCell>
                        </TableRow>
                      )}
                    </TableBody>
                  </Table>
                </TableContainer>
              </Grid>
            </Grid>
          </TabPanel>

          {/* Drives Tab */}
          <TabPanel value={tabValue} index={2}>
            <Grid container spacing={3}>
              {/* Drive Summary */}
              <Grid item xs={12} md={4}>
                <Card>
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2, mb: 2 }}>
                      <SdStorage sx={{ fontSize: 40, color: theme.palette.success.main }} />
                      <Box>
                        <Typography variant="h4" sx={{ fontWeight: 700 }}>
                          {driveStats?.total_count || 0}
                        </Typography>
                        <Typography variant="body2" color="text.secondary">Total Drives</Typography>
                      </Box>
                    </Box>
                    <Box sx={{ display: "flex", gap: 2, mb: 2 }}>
                      <Chip
                        label={`${driveStats?.online_count || 0} Online`}
                        color="success"
                        size="small"
                      />
                      <Chip
                        label={`${driveStats?.offline_count || 0} Offline`}
                        color="error"
                        size="small"
                      />
                    </Box>
                    <LinearProgress
                      variant="determinate"
                      value={driveStats?.total_count > 0 ? (driveStats.online_count / driveStats.total_count) * 100 : 0}
                      sx={{ height: 8, borderRadius: 1 }}
                    />
                  </CardContent>
                </Card>
              </Grid>

              {/* Drive List */}
              <Grid item xs={12} md={8}>
                <TableContainer component={Paper}>
                  <Table>
                    <TableHead>
                      <TableRow>
                        <TableCell>Drive Path</TableCell>
                        <TableCell>Status</TableCell>
                        <TableCell>Capacity</TableCell>
                        <TableCell>Used</TableCell>
                        <TableCell>Available</TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {(driveStats?.drives || []).length > 0 ? (
                        driveStats.drives.map((drive: any) => (
                          <TableRow key={drive.id} hover>
                            <TableCell sx={{ fontFamily: "monospace" }}>{drive.path}</TableCell>
                            <TableCell>
                              <Chip
                                label={drive.status}
                                color={drive.status === "online" ? "success" : "error"}
                                size="small"
                              />
                            </TableCell>
                            <TableCell>{formatBytes(drive.capacity || 0)}</TableCell>
                            <TableCell>{formatBytes(drive.used || 0)}</TableCell>
                            <TableCell>{formatBytes(drive.available || 0)}</TableCell>
                          </TableRow>
                        ))
                      ) : (
                        <TableRow>
                          <TableCell colSpan={5} align="center">No drives configured</TableCell>
                        </TableRow>
                      )}
                    </TableBody>
                  </Table>
                </TableContainer>
              </Grid>
            </Grid>
          </TabPanel>

          {/* Pools Tab */}
          <TabPanel value={tabValue} index={3}>
            <Grid container spacing={3}>
              {poolStats.length > 0 ? (
                poolStats.map((pool: any) => (
                  <Grid item xs={12} md={6} key={pool.id}>
                    <Card>
                      <CardContent>
                        <Box sx={{ display: "flex", alignItems: "center", gap: 2, mb: 2 }}>
                          <Layers sx={{ fontSize: 32, color: theme.palette.info.main }} />
                          <Typography variant="h6" sx={{ fontWeight: 600 }}>{pool.name}</Typography>
                        </Box>
                        <Grid container spacing={2}>
                          <Grid item xs={6}>
                            <Typography variant="body2" color="text.secondary">Capacity</Typography>
                            <Typography variant="h5" sx={{ fontWeight: 700 }}>
                              {formatBytes(pool.capacity || 0)}
                            </Typography>
                          </Grid>
                          <Grid item xs={6}>
                            <Typography variant="body2" color="text.secondary">Available</Typography>
                            <Typography variant="h5" sx={{ fontWeight: 700 }}>
                              {formatBytes(pool.available || 0)}
                            </Typography>
                          </Grid>
                          <Grid item xs={12}>
                            <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
                              Usage ({pool.capacity > 0 ? Math.round((pool.used / pool.capacity) * 100) : 0}%)
                            </Typography>
                            <LinearProgress
                              variant="determinate"
                              value={pool.capacity > 0 ? (pool.used / pool.capacity) * 100 : 0}
                              sx={{ height: 8, borderRadius: 1 }}
                            />
                          </Grid>
                          <Grid item xs={12}>
                            <Box sx={{ display: "flex", gap: 2 }}>
                              <Chip
                                label={`${pool.online_drives || 0} Online`}
                                color="success"
                                size="small"
                              />
                              <Chip
                                label={`${pool.offline_drives || 0} Offline`}
                                color="error"
                                size="small"
                              />
                              <Chip
                                label={`${pool.drives_count || 0} Total Drives`}
                                size="small"
                              />
                            </Box>
                          </Grid>
                        </Grid>
                      </CardContent>
                    </Card>
                  </Grid>
                ))
              ) : (
                <Grid item xs={12}>
                  <Paper sx={{ p: 4, textAlign: "center" }}>
                    <Layers sx={{ fontSize: 60, color: "text.disabled", mb: 2 }} />
                    <Typography variant="h6" color="text.secondary">No pools configured</Typography>
                  </Paper>
                </Grid>
              )}
            </Grid>
          </TabPanel>

          {/* Metrics Tab */}
          <TabPanel value={tabValue} index={4}>
            <Grid container spacing={3}>
              {/* API Errors */}
              <Grid item xs={12} md={6}>
                <Card>
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2, mb: 2 }}>
                      <Warning sx={{ color: theme.palette.error.main }} />
                      <Typography variant="h6" sx={{ fontWeight: 600 }}>API Errors (24h)</Typography>
                    </Box>
                    <Typography variant="h4" sx={{ fontWeight: 700, mb: 2 }}>
                      {apiErrorsData.reduce((sum, item) => sum + (item.count || 0), 0)}
                    </Typography>
                    <Box sx={{ height: 280 }}>
                      <ApiErrorsChart
                        data={apiErrorsData}
                        onModeChange={(range) => loadApiErrorsStats(range)}
                      />
                    </Box>
                  </CardContent>
                </Card>
              </Grid>

              {/* Data Throughput */}
              <Grid item xs={12} md={6}>
                <Card>
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2, mb: 2 }}>
                      <Timeline sx={{ color: theme.palette.info.main }} />
                      <Typography variant="h6" sx={{ fontWeight: 600 }}>Data Throughput (24h)</Typography>
                    </Box>
                    <Typography variant="h4" sx={{ fontWeight: 700, mb: 2 }}>
                      {formatBytes(throughputData.reduce((sum, item) => sum + (item.total_bytes || 0), 0))}
                    </Typography>
                    <Box sx={{ height: 280 }}>
                      <DataThroughputChart
                        data={throughputData}
                        formatBytes={formatBytes}
                        onTimeRangeChange={(range) => loadThroughputStats(range)}
                      />
                    </Box>
                  </CardContent>
                </Card>
              </Grid>
            </Grid>
          </TabPanel>

          <TabPanel value={tabValue} index={5}>
            <Grid container spacing={3}>
              <Grid item xs={12}>
                <Paper variant="outlined" sx={{ p: 2 }}>
                  <Typography variant="subtitle1" sx={{ fontWeight: 600, mb: 1 }}>
                    Summary (read-only)
                  </Typography>
                  {infraSummary ? (
                    <Typography variant="body2" component="pre" sx={{ m: 0, fontFamily: "monospace", fontSize: "0.8rem" }}>
                      {JSON.stringify(infraSummary, null, 2)}
                    </Typography>
                  ) : (
                    <Typography variant="body2" color="text.secondary">
                      Infrastructure API unavailable or insufficient permissions.
                    </Typography>
                  )}
                </Paper>
              </Grid>
              <Grid item xs={12} md={6}>
                <Typography variant="h6" sx={{ mb: 1 }}>Servers</Typography>
                <TableContainer component={Paper} variant="outlined">
                  <Table size="small">
                    <TableHead>
                      <TableRow>
                        <TableCell>ID</TableCell>
                        <TableCell>Status</TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {(infraServers?.servers ?? []).slice(0, 20).map((server, idx) => (
                        <TableRow key={String(server.id ?? idx)}>
                          <TableCell>{String(server.id ?? server.name ?? idx)}</TableCell>
                          <TableCell>{String(server.status ?? "—")}</TableCell>
                        </TableRow>
                      ))}
                      {!infraServers?.servers?.length && (
                        <TableRow>
                          <TableCell colSpan={2} align="center">No data</TableCell>
                        </TableRow>
                      )}
                    </TableBody>
                  </Table>
                </TableContainer>
              </Grid>
              <Grid item xs={12} md={6}>
                <Typography variant="h6" sx={{ mb: 1 }}>Drives</Typography>
                <TableContainer component={Paper} variant="outlined">
                  <Table size="small">
                    <TableHead>
                      <TableRow>
                        <TableCell>ID</TableCell>
                        <TableCell>Status</TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {(infraDrives?.drives ?? []).slice(0, 20).map((drive, idx) => (
                        <TableRow key={String(drive.id ?? idx)}>
                          <TableCell>{String(drive.id ?? idx)}</TableCell>
                          <TableCell>{String(drive.status ?? "—")}</TableCell>
                        </TableRow>
                      ))}
                      {!infraDrives?.drives?.length && (
                        <TableRow>
                          <TableCell colSpan={2} align="center">No data</TableCell>
                        </TableRow>
                      )}
                    </TableBody>
                  </Table>
                </TableContainer>
              </Grid>
              <Grid item xs={12}>
                <Paper variant="outlined" sx={{ p: 2 }}>
                  <Typography variant="subtitle1" sx={{ fontWeight: 600, mb: 1 }}>
                    Heal status
                  </Typography>
                  {infraHeal ? (
                    <Box sx={{ display: "flex", gap: 1, flexWrap: "wrap" }}>
                      <Chip label={`Status: ${infraHeal.status ?? "unknown"}`} size="small" />
                      {infraHeal.progress != null && (
                        <Chip label={`Progress: ${infraHeal.progress}%`} size="small" />
                      )}
                    </Box>
                  ) : (
                    <Typography variant="body2" color="text.secondary">No heal data</Typography>
                  )}
                </Paper>
              </Grid>
            </Grid>
          </TabPanel>

          {isAdmin && (
            <TabPanel value={tabValue} index={6}>
              <AdminTerminalPanel />
            </TabPanel>
          )}
        </>
      )}
    </Box>
  );
};

export default SystemHealthPage;
