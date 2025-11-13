import React, { useEffect, useState } from "react";
import {
  Box,
  Grid,
  Typography,
  Card,
  CardContent,
  useTheme,
  alpha,
  LinearProgress,
  Chip,
} from "@mui/material";
import {
  Storage,
  Computer,
  Folder,
  Description,
  ArrowForward,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";
import { useAppDispatch } from "../hooks/useAppDispatch";
import { useAppSelector } from "../hooks/useAppSelector";
import { fetchBuckets, selectBuckets } from "../store/bucketsSlice";
import {
  fetchAllStats,
  selectSystemStats,
  selectActivityStats,
  selectStatsLoading,
} from "../store/statsSlice";
import Loader from "../components/Common/Loader";
import CapacityPieChart from "../components/Charts/CapacityPieChart";
import DataThroughputChart from "../components/Charts/DataThroughputChart";
import ApiErrorsChart from "../components/Charts/ApiErrorsChart";

// Helper function to format bytes
const formatBytes = (bytes: number, decimals: number = 2): string => {
  if (bytes === 0) return "0 Bytes";
  const k = 1024;
  const dm = decimals < 0 ? 0 : decimals;
  const sizes = ["Bytes", "KB", "MB", "GB", "TB", "PB", "EB"];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + " " + sizes[i];
};

// Helper to format bytes to EiB
const formatBytesToEiB = (bytes: number): string => {
  const eib = bytes / (1024 ** 6);
  return eib.toFixed(2);
};

const DashboardPageNew: React.FC = () => {
  const dispatch = useAppDispatch();
  const theme = useTheme();
  const { t } = useTranslation();

  // Redux state
  const buckets = useAppSelector(selectBuckets);
  const systemStats = useAppSelector(selectSystemStats);
  const activityStats = useAppSelector(selectActivityStats);
  const statsLoading = useAppSelector(selectStatsLoading);

  const [loading, setLoading] = useState(true);
  const [serverStats, setServerStats] = useState<any>(null);
  const [driveStats, setDriveStats] = useState<any>(null);
  const [poolStats, setPoolStats] = useState<any[]>([]);
  const [apiErrorsData, setApiErrorsData] = useState<any[]>([]);
  const [throughputData, setThroughputData] = useState<any[]>([]);

  useEffect(() => {
    const loadData = async () => {
      try {
        await Promise.all([
          dispatch(fetchBuckets()),
          dispatch(fetchAllStats()),
        ]);

        // Load additional stats (mock data for now since backend endpoints don't exist yet)
        loadMockStats();
      } finally {
        setLoading(false);
      }
    };
    loadData();
  }, [dispatch]);

  const loadMockStats = () => {
    // Mock server stats
    setServerStats({
      online_count: 10,
      offline_count: 7,
      total_count: 20,
    });

    // Mock drive stats
    setDriveStats({
      online_count: 1900,
      offline_count: 100,
      total_count: 2000,
    });

    // Mock pool stats
    setPoolStats([
      {
        id: "pool1",
        name: "Pool 1",
        capacity: 5.25 * (1024 ** 6),
        available: 1.22 * (1024 ** 6),
        used: 4.03 * (1024 ** 6),
        drives_count: 90,
        online_drives: 80,
        offline_drives: 10,
      },
      {
        id: "pool2",
        name: "Pool 2",
        capacity: 5.25 * (1024 ** 6),
        available: 1.46 * (1024 ** 6),
        used: 3.79 * (1024 ** 6),
        drives_count: 90,
        online_drives: 80,
        offline_drives: 10,
      },
    ]);

    // Mock API errors data (last 24 hours)
    const now = Date.now();
    const mockApiErrors = [];
    for (let i = 23; i >= 0; i--) {
      const time = new Date(now - i * 3600000).toLocaleTimeString("en-US", {
        hour: "2-digit",
        minute: "2-digit",
        hour12: false,
      });
      mockApiErrors.push({
        time,
        count: Math.floor(Math.random() * 25),
        error_4xx: Math.floor(Math.random() * 15),
        error_5xx: Math.floor(Math.random() * 10),
      });
    }
    setApiErrorsData(mockApiErrors);

    // Mock throughput data (last 24 hours)
    const mockThroughput = [];
    for (let i = 23; i >= 0; i--) {
      const time = new Date(now - i * 3600000).toLocaleTimeString("en-US", {
        hour: "2-digit",
        minute: "2-digit",
        hour12: false,
      });
      const readBytes = Math.floor(Math.random() * 1000 * 1024 * 1024 * 1024);
      const writeBytes = Math.floor(Math.random() * 800 * 1024 * 1024 * 1024);
      mockThroughput.push({
        time,
        read_bytes: readBytes,
        write_bytes: writeBytes,
        total_bytes: readBytes + writeBytes,
      });
    }
    setThroughputData(mockThroughput);
  };

  if (loading || statsLoading) {
    return <Loader message={t("dashboard.loading")} />;
  }

  // Calculate stats
  const totalObjects = systemStats?.objects || 1263;
  const totalSize = systemStats?.storage_used || 7.82 * (1024 ** 6);
  const totalBuckets = systemStats?.buckets || buckets.length || 7;
  const availableStorage = systemStats?.storage_available || 2.68 * (1024 ** 6);
  const totalStorage = systemStats?.storage_total || 10.50 * (1024 ** 6);

  const totalApiErrors = apiErrorsData.reduce((sum, item) => sum + item.count, 0);
  const totalDataTransfer =
    throughputData.reduce((sum, item) => sum + item.total_bytes, 0) / (1024 ** 3);

  return (
    <Box
      sx={{
        bgcolor: theme.palette.mode === "dark" ? "#0F172A" : "#F8FAFC",
        minHeight: "100vh",
        p: 3,
      }}
    >
      {/* Header */}
      <Box sx={{ mb: 3 }}>
        <Typography
          variant="h4"
          gutterBottom
          sx={{ fontWeight: 700, color: theme.palette.text.primary }}
        >
          {t("dashboard.title") || "Object Storage Dashboard"}
        </Typography>
        <Typography variant="body1" color="text.secondary">
          {t("dashboard.subtitle") || "System Overview and Metrics"}
        </Typography>
      </Box>

      <Grid container spacing={3}>
        {/* Capacity Card */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              bgcolor:
                theme.palette.mode === "dark"
                  ? alpha("#1E293B", 0.8)
                  : "#FFFFFF",
              borderRadius: 2,
              border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
            }}
          >
            <CardContent>
              <Box sx={{ display: "flex", alignItems: "center", mb: 2 }}>
                <Storage sx={{ color: theme.palette.primary.main, mr: 1 }} />
                <Typography variant="h6" sx={{ fontWeight: 600 }}>
                  Capacity
                </Typography>
                <Typography
                  variant="body2"
                  sx={{ ml: "auto", fontWeight: 600 }}
                >
                  {formatBytesToEiB(totalStorage)} EiB
                </Typography>
              </Box>

              <CapacityPieChart
                available={availableStorage}
                used={totalSize}
                formatBytes={formatBytes}
              />

              {/* Storage breakdown */}
              <Box sx={{ mt: 2 }}>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                    mb: 1,
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Box
                      sx={{
                        width: 12,
                        height: 12,
                        bgcolor: theme.palette.primary.main,
                        borderRadius: "50%",
                      }}
                    />
                    <Typography variant="body2">Object data</Typography>
                  </Box>
                  <Typography variant="body2" sx={{ fontWeight: 600 }}>
                    {formatBytesToEiB(totalSize)} EiB
                  </Typography>
                </Box>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Box
                      sx={{
                        width: 12,
                        height: 12,
                        bgcolor: alpha(theme.palette.primary.main, 0.3),
                        borderRadius: "50%",
                      }}
                    />
                    <Typography variant="body2">Available</Typography>
                  </Box>
                  <Typography variant="body2" sx={{ fontWeight: 600 }}>
                    {formatBytesToEiB(availableStorage)} EiB
                  </Typography>
                </Box>
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Servers Card */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              bgcolor:
                theme.palette.mode === "dark"
                  ? alpha("#1E293B", 0.8)
                  : "#FFFFFF",
              borderRadius: 2,
              border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
            }}
          >
            <CardContent>
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "space-between",
                  mb: 3,
                }}
              >
                <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                  <Computer sx={{ color: theme.palette.primary.main }} />
                  <Typography variant="h6" sx={{ fontWeight: 600 }}>
                    Servers
                  </Typography>
                </Box>
                <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                  <Typography variant="h4" sx={{ fontWeight: 700 }}>
                    {serverStats?.total_count || 20}
                  </Typography>
                  <ArrowForward sx={{ color: theme.palette.text.secondary }} />
                </Box>
              </Box>

              <Box sx={{ mb: 2 }}>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                    mb: 1,
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Chip
                      label="Online"
                      size="small"
                      sx={{
                        bgcolor: alpha("#10B981", 0.1),
                        color: "#10B981",
                        fontWeight: 600,
                      }}
                    />
                    <Typography variant="body2">
                      {serverStats?.online_count || 10}
                    </Typography>
                  </Box>
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Chip
                      label="Offline"
                      size="small"
                      sx={{
                        bgcolor: alpha("#EF4444", 0.1),
                        color: "#EF4444",
                        fontWeight: 600,
                      }}
                    />
                    <Typography variant="body2">
                      {serverStats?.offline_count || 7}
                    </Typography>
                  </Box>
                </Box>

                <LinearProgress
                  variant="determinate"
                  value={
                    ((serverStats?.online_count || 10) /
                      (serverStats?.total_count || 20)) *
                    100
                  }
                  sx={{
                    height: 8,
                    borderRadius: 1,
                    bgcolor: alpha("#EF4444", 0.2),
                    "& .MuiLinearProgress-bar": {
                      bgcolor: "#10B981",
                      borderRadius: 1,
                    },
                  }}
                />
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Drives Card */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              bgcolor:
                theme.palette.mode === "dark"
                  ? alpha("#1E293B", 0.8)
                  : "#FFFFFF",
              borderRadius: 2,
              border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
            }}
          >
            <CardContent>
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "space-between",
                  mb: 3,
                }}
              >
                <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                  <Storage sx={{ color: theme.palette.primary.main }} />
                  <Typography variant="h6" sx={{ fontWeight: 600 }}>
                    Drives
                  </Typography>
                </Box>
                <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                  <Typography variant="h4" sx={{ fontWeight: 700 }}>
                    {driveStats?.total_count || 2000}
                  </Typography>
                  <ArrowForward sx={{ color: theme.palette.text.secondary }} />
                </Box>
              </Box>

              <Box sx={{ mb: 2 }}>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                    mb: 1,
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Chip
                      label="Online"
                      size="small"
                      sx={{
                        bgcolor: alpha("#10B981", 0.1),
                        color: "#10B981",
                        fontWeight: 600,
                      }}
                    />
                    <Typography variant="body2">
                      {driveStats?.online_count || 1900}
                    </Typography>
                  </Box>
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Chip
                      label="Offline"
                      size="small"
                      sx={{
                        bgcolor: alpha("#EF4444", 0.1),
                        color: "#EF4444",
                        fontWeight: 600,
                      }}
                    />
                    <Typography variant="body2">
                      {driveStats?.offline_count || 100}
                    </Typography>
                  </Box>
                </Box>

                <LinearProgress
                  variant="determinate"
                  value={
                    ((driveStats?.online_count || 1900) /
                      (driveStats?.total_count || 2000)) *
                    100
                  }
                  sx={{
                    height: 8,
                    borderRadius: 1,
                    bgcolor: alpha("#EF4444", 0.2),
                    "& .MuiLinearProgress-bar": {
                      bgcolor: "#10B981",
                      borderRadius: 1,
                    },
                  }}
                />
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Buckets Card */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              bgcolor:
                theme.palette.mode === "dark"
                  ? alpha("#1E293B", 0.8)
                  : "#FFFFFF",
              borderRadius: 2,
              border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
            }}
          >
            <CardContent>
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "space-between",
                  mb: 2,
                }}
              >
                <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                  <Folder sx={{ color: theme.palette.primary.main }} />
                  <Typography variant="h6" sx={{ fontWeight: 600 }}>
                    Buckets
                  </Typography>
                </Box>
                <ArrowForward sx={{ color: theme.palette.text.secondary }} />
              </Box>

              <Box sx={{ mb: 2 }}>
                <Typography variant="h3" sx={{ fontWeight: 700, mb: 0.5 }}>
                  {totalBuckets}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  Buckets
                </Typography>
              </Box>

              <Box sx={{ mb: 2 }}>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                    mb: 0.5,
                  }}
                >
                  <Typography variant="body2" color="text.secondary">
                    Objects
                  </Typography>
                  <Typography variant="body2" sx={{ fontWeight: 600 }}>
                    {totalObjects.toLocaleString()}
                  </Typography>
                </Box>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                  }}
                >
                  <Typography variant="body2" color="text.secondary">
                    Size
                  </Typography>
                  <Typography variant="body2" sx={{ fontWeight: 600 }}>
                    {formatBytesToEiB(totalSize)} EiB
                  </Typography>
                </Box>
              </Box>

              {/* Recent Activity */}
              <Typography
                variant="caption"
                sx={{ fontWeight: 600, color: theme.palette.text.secondary }}
              >
                Recent Activity
              </Typography>
              <Box sx={{ mt: 1, maxHeight: 120, overflowY: "auto" }}>
                {activityStats?.recent_buckets
                  ?.slice(0, 4)
                  .map((bucket) => (
                    <Box
                      key={bucket.name}
                      sx={{
                        py: 0.5,
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "space-between",
                      }}
                    >
                      <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                        <Folder
                          sx={{ fontSize: 16, color: theme.palette.primary.main }}
                        />
                        <Typography variant="caption">{bucket.name}</Typography>
                      </Box>
                      <Typography variant="caption" color="text.secondary">
                        {bucket.objects.toLocaleString()}
                      </Typography>
                    </Box>
                  ))}
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* API Errors Chart */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              bgcolor:
                theme.palette.mode === "dark"
                  ? alpha("#1E293B", 0.8)
                  : "#FFFFFF",
              borderRadius: 2,
              border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
            }}
          >
            <CardContent>
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "space-between",
                  mb: 2,
                }}
              >
                <Typography variant="h6" sx={{ fontWeight: 600 }}>
                  API Errors (24 hrs)
                </Typography>
                <ArrowForward sx={{ color: theme.palette.text.secondary }} />
              </Box>

              <Box sx={{ mb: 2 }}>
                <Typography variant="h4" sx={{ fontWeight: 700 }}>
                  Total {totalApiErrors}
                </Typography>
              </Box>

              <ApiErrorsChart data={apiErrorsData} />
            </CardContent>
          </Card>
        </Grid>

        {/* Data Throughput Chart */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              bgcolor:
                theme.palette.mode === "dark"
                  ? alpha("#1E293B", 0.8)
                  : "#FFFFFF",
              borderRadius: 2,
              border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
            }}
          >
            <CardContent>
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "space-between",
                  mb: 2,
                }}
              >
                <Typography variant="h6" sx={{ fontWeight: 600 }}>
                  Data (24 hrs)
                </Typography>
                <ArrowForward sx={{ color: theme.palette.text.secondary }} />
              </Box>

              <Box sx={{ mb: 2 }}>
                <Typography variant="h4" sx={{ fontWeight: 700 }}>
                  Total {totalDataTransfer.toFixed(2)} GiB
                </Typography>
                <Typography variant="caption" color="text.secondary">
                  TTFB 567 ms
                </Typography>
              </Box>

              <DataThroughputChart
                data={throughputData}
                formatBytes={formatBytes}
              />
            </CardContent>
          </Card>
        </Grid>

        {/* Pool 1 */}
        {poolStats.map((pool) => (
          <Grid item xs={12} md={6} lg={6} key={pool.id}>
            <Card
              sx={{
                height: "100%",
                bgcolor:
                  theme.palette.mode === "dark"
                    ? alpha("#1E293B", 0.8)
                    : "#FFFFFF",
                borderRadius: 2,
                border: `1px solid ${alpha(theme.palette.divider, 0.1)}`,
              }}
            >
              <CardContent>
                <Box
                  sx={{
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "space-between",
                    mb: 3,
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Storage sx={{ color: theme.palette.primary.main }} />
                    <Typography variant="h6" sx={{ fontWeight: 600 }}>
                      {pool.name}
                    </Typography>
                  </Box>
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <Description sx={{ color: theme.palette.primary.main }} />
                    <ArrowForward sx={{ color: theme.palette.text.secondary }} />
                  </Box>
                </Box>

                <Grid container spacing={2}>
                  <Grid item xs={6}>
                    <Typography variant="body2" sx={{ fontWeight: 600, mb: 1 }}>
                      Capacity
                    </Typography>
                    <Box sx={{ mb: 2 }}>
                      <Typography variant="h4" sx={{ fontWeight: 700 }}>
                        {formatBytesToEiB(pool.available)}
                      </Typography>
                      <Typography variant="body2" color="text.secondary">
                        EiB Available
                      </Typography>
                    </Box>
                    <Box
                      sx={{
                        display: "flex",
                        justifyContent: "space-between",
                        mb: 0.5,
                      }}
                    >
                      <Typography variant="caption" color="text.secondary">
                        Object data
                      </Typography>
                      <Typography variant="caption" sx={{ fontWeight: 600 }}>
                        {formatBytesToEiB(pool.used)} EiB
                      </Typography>
                    </Box>
                  </Grid>

                  <Grid item xs={6}>
                    <Typography variant="body2" sx={{ fontWeight: 600, mb: 1 }}>
                      Drives
                    </Typography>
                    <Typography variant="h4" sx={{ fontWeight: 700, mb: 0.5 }}>
                      {pool.drives_count}
                    </Typography>
                    <Box
                      sx={{
                        display: "flex",
                        justifyContent: "space-between",
                        mb: 1,
                      }}
                    >
                      <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                        <Chip
                          label="Online"
                          size="small"
                          sx={{
                            bgcolor: alpha("#10B981", 0.1),
                            color: "#10B981",
                            fontWeight: 600,
                          }}
                        />
                        <Typography variant="caption">
                          {pool.online_drives}
                        </Typography>
                      </Box>
                      <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                        <Chip
                          label="Offline"
                          size="small"
                          sx={{
                            bgcolor: alpha("#EF4444", 0.1),
                            color: "#EF4444",
                            fontWeight: 600,
                          }}
                        />
                        <Typography variant="caption">
                          {pool.offline_drives}
                        </Typography>
                      </Box>
                    </Box>
                    <LinearProgress
                      variant="determinate"
                      value={(pool.online_drives / pool.drives_count) * 100}
                      sx={{
                        height: 6,
                        borderRadius: 1,
                        bgcolor: alpha("#EF4444", 0.2),
                        "& .MuiLinearProgress-bar": {
                          bgcolor: "#10B981",
                          borderRadius: 1,
                        },
                      }}
                    />
                  </Grid>
                </Grid>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>
    </Box>
  );
};

export default DashboardPageNew;
