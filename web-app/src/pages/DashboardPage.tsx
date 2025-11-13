import React, { useEffect } from "react";
import {
  Box,
  Grid,
  Typography,
  LinearProgress,
  Card,
  CardContent,
  useTheme,
  alpha,
  CircularProgress,
} from "@mui/material";
import {
  Storage,
  Folder,
  People,
  CloudUpload,
  TrendingUp,
  FiberManualRecord,
} from "@mui/icons-material";
import { useNavigate } from "react-router-dom";
import { useTranslation } from "react-i18next";
import { useAppDispatch } from "../hooks/useAppDispatch";
import { useAppSelector } from "../hooks/useAppSelector";
import { fetchBuckets, selectBuckets } from "../store/bucketsSlice";
import { fetchUsers, selectUsers } from "../store/usersSlice";
import {
  fetchAllStats,
  selectSystemStats,
  selectActivityStats,
  selectStatsLoading,
} from "../store/statsSlice";
import Loader from "../components/Common/Loader";

// Helper function to format bytes
const formatBytes = (bytes: number, decimals: number = 2): string => {
  if (bytes === 0) return "0 Bytes";
  const k = 1024;
  const dm = decimals < 0 ? 0 : decimals;
  const sizes = ["Bytes", "KB", "MB", "GB", "TB", "PB"];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + " " + sizes[i];
};

const DashboardPage: React.FC = () => {
  const navigate = useNavigate();
  const dispatch = useAppDispatch();
  const theme = useTheme();
  const { t } = useTranslation();

  // Redux state
  const buckets = useAppSelector(selectBuckets);
  const users = useAppSelector(selectUsers);
  const systemStats = useAppSelector(selectSystemStats);
  const activityStats = useAppSelector(selectActivityStats);
  const statsLoading = useAppSelector(selectStatsLoading);

  const [loading, setLoading] = React.useState(true);

  useEffect(() => {
    const loadData = async () => {
      try {
        await Promise.all([
          dispatch(fetchBuckets()),
          dispatch(fetchUsers()),
          dispatch(fetchAllStats()),
        ]);
      } finally {
        setLoading(false);
      }
    };
    loadData();
  }, [dispatch]);

  if (loading || statsLoading) {
    return <Loader message={t("dashboard.loading")} />;
  }

  // Calculate stats
  const totalObjects = systemStats?.objects || 0;
  const totalSize = systemStats?.storage_used || 0;
  const totalBuckets = systemStats?.buckets || buckets.length;
  const totalUsers = systemStats?.users || users.length;
  const availableStorage = systemStats?.storage_available || 0;
  const totalStorage = systemStats?.storage_total || 0;
  const usagePercent = totalStorage > 0 ? (totalSize / totalStorage) * 100 : 0;

  // Status indicators
  const stats = [
    {
      title: t("dashboard.stats.buckets"),
      value: totalBuckets,
      icon: <Storage sx={{ fontSize: 40 }} />,
      color: theme.palette.primary.main,
      action: () => navigate("/buckets"),
    },
    {
      title: t("dashboard.stats.objects"),
      value: totalObjects.toLocaleString(),
      icon: <Folder sx={{ fontSize: 40 }} />,
      color: theme.palette.success.main,
      action: () => navigate("/objects"),
    },
    {
      title: t("dashboard.stats.users"),
      value: totalUsers,
      icon: <People sx={{ fontSize: 40 }} />,
      color: theme.palette.warning.main,
      action: () => navigate("/users"),
    },
    {
      title: t("dashboard.stats.storage"),
      value: formatBytes(totalSize),
      icon: <TrendingUp sx={{ fontSize: 40 }} />,
      color: theme.palette.secondary.main,
    },
  ];

  return (
    <Box
      sx={{
        display: "flex",
        flexDirection: "column",
        height: "100%",
        minHeight: "70vh",
      }}
    >
      {/* Header */}
      <Box sx={{ mb: 4 }}>
        <Typography variant="h4" gutterBottom sx={{ fontWeight: 700 }}>
          {t("dashboard.title")}
        </Typography>
        <Typography variant="body1" color="text.secondary">
          {t("dashboard.subtitle")}
        </Typography>
      </Box>

      <Grid container spacing={3}>
        {/* Capacity Card (Large) */}
        <Grid item xs={12} md={6} lg={4}>
          <Card
            sx={{
              height: "100%",
              background: `linear-gradient(135deg, ${alpha(theme.palette.primary.main, 0.1)} 0%, ${alpha(theme.palette.primary.main, 0.05)} 100%)`,
              borderRadius: 3,
              border: `1px solid ${alpha(theme.palette.primary.main, 0.1)}`,
            }}
          >
            <CardContent>
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  mb: 2,
                  gap: 1,
                }}
              >
                <Storage color="primary" />
                <Typography variant="h6" sx={{ fontWeight: 600 }}>
                  {t("dashboard.capacity")}
                </Typography>
                <Typography
                  variant="body2"
                  sx={{ ml: "auto", fontWeight: 600 }}
                >
                  {formatBytes(totalStorage)}
                </Typography>
              </Box>

              {/* Circular Progress */}
              <Box
                sx={{
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "center",
                  my: 3,
                  position: "relative",
                }}
              >
                <CircularProgress
                  variant="determinate"
                  value={100}
                  size={180}
                  thickness={4}
                  sx={{
                    color: alpha(theme.palette.primary.main, 0.1),
                    position: "absolute",
                  }}
                />
                <CircularProgress
                  variant="determinate"
                  value={usagePercent}
                  size={180}
                  thickness={4}
                  sx={{
                    color: theme.palette.primary.main,
                    position: "absolute",
                  }}
                />
                <Box
                  sx={{
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                  }}
                >
                  <Typography variant="h3" sx={{ fontWeight: 700 }}>
                    {formatBytes(availableStorage).split(" ")[0]}
                  </Typography>
                  <Typography variant="body2" color="text.secondary">
                    {formatBytes(availableStorage).split(" ")[1]}
                  </Typography>
                  <Typography variant="caption" color="text.secondary">
                    {t("dashboard.available")}
                  </Typography>
                </Box>
              </Box>

              {/* Storage breakdown */}
              <Box sx={{ mt: 3 }}>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                    mb: 1,
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <FiberManualRecord
                      sx={{
                        fontSize: 12,
                        color: theme.palette.primary.main,
                      }}
                    />
                    <Typography variant="body2">
                      {t("dashboard.objectData")}
                    </Typography>
                  </Box>
                  <Typography variant="body2" sx={{ fontWeight: 600 }}>
                    {formatBytes(totalSize)}
                  </Typography>
                </Box>
                <Box
                  sx={{
                    display: "flex",
                    justifyContent: "space-between",
                  }}
                >
                  <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                    <FiberManualRecord
                      sx={{
                        fontSize: 12,
                        color: alpha(theme.palette.primary.main, 0.3),
                      }}
                    />
                    <Typography variant="body2">
                      {t("dashboard.available")}
                    </Typography>
                  </Box>
                  <Typography variant="body2" sx={{ fontWeight: 600 }}>
                    {formatBytes(availableStorage)}
                  </Typography>
                </Box>
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Stats Cards */}
        {stats.map((stat, index) => (
          <Grid item xs={12} sm={6} md={3} lg={2} key={index}>
            <Card
              onClick={stat.action}
              sx={{
                height: "100%",
                cursor: stat.action ? "pointer" : "default",
                transition: "all 0.3s ease",
                borderRadius: 3,
                "&:hover": stat.action
                  ? {
                      transform: "translateY(-4px)",
                      boxShadow: theme.shadows[8],
                    }
                  : {},
              }}
            >
              <CardContent>
                <Box sx={{ display: "flex", alignItems: "center", mb: 2 }}>
                  <Box
                    sx={{
                      p: 1.5,
                      borderRadius: 2,
                      bgcolor: alpha(stat.color, 0.1),
                      color: stat.color,
                      display: "flex",
                      alignItems: "center",
                      justifyContent: "center",
                    }}
                  >
                    {stat.icon}
                  </Box>
                </Box>
                <Typography variant="h4" sx={{ fontWeight: 700, mb: 0.5 }}>
                  {stat.value}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  {stat.title}
                </Typography>
              </CardContent>
            </Card>
          </Grid>
        ))}

        {/* Recent Activity */}
        <Grid item xs={12} md={6}>
          <Card sx={{ height: "100%", borderRadius: 3 }}>
            <CardContent>
              <Box sx={{ mb: 2 }}>
                <Typography variant="h6" sx={{ fontWeight: 600, mb: 0.5 }}>
                  {t("dashboard.recentActivity")}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  {t("dashboard.latestBuckets")}
                </Typography>
              </Box>
              <Box sx={{ maxHeight: 300, overflowY: "auto" }}>
                {activityStats?.recent_buckets &&
                activityStats.recent_buckets.length > 0 ? (
                  activityStats.recent_buckets.map((bucket, index) => (
                    <Box
                      key={bucket.name}
                      onClick={() =>
                        navigate(
                          `/objects?bucket=${encodeURIComponent(bucket.name)}`,
                        )
                      }
                      sx={{
                        py: 2,
                        px: 1,
                        borderBottom:
                          index < (activityStats.recent_buckets?.length || 0) - 1
                            ? `1px solid ${theme.palette.divider}`
                            : "none",
                        cursor: "pointer",
                        transition: "background 0.2s",
                        borderRadius: 1,
                        "&:hover": {
                          bgcolor: alpha(theme.palette.primary.main, 0.05),
                        },
                      }}
                    >
                      <Box
                        sx={{
                          display: "flex",
                          justifyContent: "space-between",
                          alignItems: "center",
                        }}
                      >
                        <Box sx={{ display: "flex", alignItems: "center", gap: 1.5 }}>
                          <Storage
                            sx={{
                              color: theme.palette.primary.main,
                              fontSize: 20,
                            }}
                          />
                          <Box>
                            <Typography
                              variant="body1"
                              sx={{ fontWeight: 600 }}
                            >
                              {bucket.name}
                            </Typography>
                            <Typography variant="caption" color="text.secondary">
                              {bucket.objects.toLocaleString()}{" "}
                              {t("dashboard.objects")}
                            </Typography>
                          </Box>
                        </Box>
                        <Typography
                          variant="body2"
                          sx={{
                            fontWeight: 600,
                            color: theme.palette.text.secondary,
                          }}
                        >
                          {formatBytes(bucket.size)}
                        </Typography>
                      </Box>
                    </Box>
                  ))
                ) : (
                  <Typography
                    variant="body2"
                    color="text.secondary"
                    sx={{ textAlign: "center", py: 4 }}
                  >
                    {t("dashboard.noActivity")}
                  </Typography>
                )}
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Top Buckets by Size */}
        <Grid item xs={12} md={6}>
          <Card sx={{ height: "100%", borderRadius: 3 }}>
            <CardContent>
              <Box sx={{ mb: 2 }}>
                <Typography variant="h6" sx={{ fontWeight: 600, mb: 0.5 }}>
                  {t("dashboard.topBuckets")}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  {t("dashboard.bySize")}
                </Typography>
              </Box>
              <Box sx={{ maxHeight: 300, overflowY: "auto" }}>
                {[...buckets]
                  .sort((a, b) => (b.size || 0) - (a.size || 0))
                  .slice(0, 5)
                  .map((bucket, index) => {
                    const bucketSize = bucket.size || 0;
                    const maxSize = Math.max(
                      ...[...buckets].map((b) => b.size || 0),
                    );
                    const percentage =
                      maxSize > 0 ? (bucketSize / maxSize) * 100 : 0;

                    return (
                      <Box
                        key={bucket.name}
                        onClick={() =>
                          navigate(
                            `/objects?bucket=${encodeURIComponent(bucket.name)}`,
                          )
                        }
                        sx={{
                          py: 2,
                          borderBottom:
                            index < 4
                              ? `1px solid ${theme.palette.divider}`
                              : "none",
                          cursor: "pointer",
                          transition: "background 0.2s",
                          borderRadius: 1,
                          "&:hover": {
                            bgcolor: alpha(theme.palette.primary.main, 0.05),
                          },
                        }}
                      >
                        <Box
                          sx={{
                            display: "flex",
                            justifyContent: "space-between",
                            alignItems: "center",
                            mb: 1,
                          }}
                        >
                          <Typography variant="body1" sx={{ fontWeight: 600 }}>
                            {bucket.name}
                          </Typography>
                          <Typography
                            variant="body2"
                            color="text.secondary"
                            sx={{ fontWeight: 600 }}
                          >
                            {formatBytes(bucketSize)}
                          </Typography>
                        </Box>
                        <Box
                          sx={{
                            display: "flex",
                            alignItems: "center",
                            gap: 2,
                          }}
                        >
                          <LinearProgress
                            variant="determinate"
                            value={percentage}
                            sx={{
                              flex: 1,
                              height: 6,
                              borderRadius: 3,
                              bgcolor: alpha(theme.palette.primary.main, 0.1),
                              "& .MuiLinearProgress-bar": {
                                borderRadius: 3,
                                background: `linear-gradient(90deg, ${theme.palette.primary.main} 0%, ${theme.palette.primary.light} 100%)`,
                              },
                            }}
                          />
                          <Typography
                            variant="caption"
                            color="text.secondary"
                            sx={{ minWidth: 80, textAlign: "right" }}
                          >
                            {(bucket.objects_count || 0).toLocaleString()}{" "}
                            {t("dashboard.objects")}
                          </Typography>
                        </Box>
                      </Box>
                    );
                  })}
                {buckets.length === 0 && (
                  <Typography
                    variant="body2"
                    color="text.secondary"
                    sx={{ textAlign: "center", py: 4 }}
                  >
                    {t("dashboard.noBuckets")}
                  </Typography>
                )}
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Quick Actions */}
        <Grid item xs={12}>
          <Card sx={{ borderRadius: 3 }}>
            <CardContent>
              <Typography variant="h6" sx={{ fontWeight: 600, mb: 2 }}>
                {t("dashboard.quickActions")}
              </Typography>
              <Box sx={{ display: "flex", gap: 2, flexWrap: "wrap" }}>
                <Card
                  onClick={() => navigate("/buckets")}
                  sx={{
                    flex: 1,
                    minWidth: 200,
                    cursor: "pointer",
                    transition: "all 0.3s",
                    borderRadius: 2,
                    background: `linear-gradient(135deg, ${alpha(theme.palette.primary.main, 0.1)} 0%, ${alpha(theme.palette.primary.main, 0.05)} 100%)`,
                    "&:hover": {
                      transform: "translateY(-2px)",
                      boxShadow: theme.shadows[4],
                    },
                  }}
                >
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2 }}>
                      <CloudUpload
                        sx={{ fontSize: 40, color: theme.palette.primary.main }}
                      />
                      <Box>
                        <Typography variant="h6" sx={{ fontWeight: 600 }}>
                          {t("dashboard.createBucket")}
                        </Typography>
                        <Typography variant="body2" color="text.secondary">
                          {t("dashboard.createBucketDesc")}
                        </Typography>
                      </Box>
                    </Box>
                  </CardContent>
                </Card>

                <Card
                  onClick={() => navigate("/objects")}
                  sx={{
                    flex: 1,
                    minWidth: 200,
                    cursor: "pointer",
                    transition: "all 0.3s",
                    borderRadius: 2,
                    background: `linear-gradient(135deg, ${alpha(theme.palette.success.main, 0.1)} 0%, ${alpha(theme.palette.success.main, 0.05)} 100%)`,
                    "&:hover": {
                      transform: "translateY(-2px)",
                      boxShadow: theme.shadows[4],
                    },
                  }}
                >
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2 }}>
                      <Folder
                        sx={{ fontSize: 40, color: theme.palette.success.main }}
                      />
                      <Box>
                        <Typography variant="h6" sx={{ fontWeight: 600 }}>
                          {t("dashboard.uploadObjects")}
                        </Typography>
                        <Typography variant="body2" color="text.secondary">
                          {t("dashboard.uploadObjectsDesc")}
                        </Typography>
                      </Box>
                    </Box>
                  </CardContent>
                </Card>

                <Card
                  onClick={() => navigate("/users")}
                  sx={{
                    flex: 1,
                    minWidth: 200,
                    cursor: "pointer",
                    transition: "all 0.3s",
                    borderRadius: 2,
                    background: `linear-gradient(135deg, ${alpha(theme.palette.warning.main, 0.1)} 0%, ${alpha(theme.palette.warning.main, 0.05)} 100%)`,
                    "&:hover": {
                      transform: "translateY(-2px)",
                      boxShadow: theme.shadows[4],
                    },
                  }}
                >
                  <CardContent>
                    <Box sx={{ display: "flex", alignItems: "center", gap: 2 }}>
                      <People
                        sx={{ fontSize: 40, color: theme.palette.warning.main }}
                      />
                      <Box>
                        <Typography variant="h6" sx={{ fontWeight: 600 }}>
                          {t("dashboard.manageUsers")}
                        </Typography>
                        <Typography variant="body2" color="text.secondary">
                          {t("dashboard.manageUsersDesc")}
                        </Typography>
                      </Box>
                    </Box>
                  </CardContent>
                </Card>
              </Box>
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default DashboardPage;
