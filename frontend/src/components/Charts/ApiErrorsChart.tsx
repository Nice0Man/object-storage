import React, { useState, useMemo } from "react";
import {
    BarChart,
    Bar,
    XAxis,
    YAxis,
    CartesianGrid,
    Tooltip,
    ResponsiveContainer,
    Legend,
    Cell,
    ReferenceLine,
} from "recharts";
import {
    Box,
    Typography,
    useTheme,
    alpha,
    ToggleButton,
    ToggleButtonGroup,
    Chip,
    Stack,
} from "@mui/material";
import { CheckCircle, Warning, Error as ErrorIcon } from "@mui/icons-material";

interface ApiErrorData {
    time: string;
    timestamp?: number;
    requests?: number;
    count?: number;
    error_4xx: number;
    error_5xx: number;
    success?: number;
}

type TimeRange = "1h" | "6h" | "24h" | "7d";

interface ApiErrorsChartProps {
    data: ApiErrorData[];
    showModeSelector?: boolean;
    defaultMode?: TimeRange;
    onModeChange?: (mode: TimeRange) => void;
    fillParent?: boolean;
}

const ApiErrorsChart: React.FC<ApiErrorsChartProps> = ({
    data,
    showModeSelector = true,
    defaultMode = "24h",
    onModeChange,
    fillParent = false,
}) => {
    const theme = useTheme();
    const [timeRange, setTimeRange] = useState<TimeRange>(defaultMode);
    const [activeBar, setActiveBar] = useState<string | null>(null);

    // Colors based on theme
    const colors = useMemo(() => ({
        success: theme.palette.mode === "dark" ? "#10B981" : "#059669",
        warning: theme.palette.mode === "dark" ? "#F59E0B" : "#D97706",
        error: theme.palette.mode === "dark" ? "#EF4444" : "#DC2626",
        grid: alpha(theme.palette.divider, 0.15),
        text: theme.palette.text.secondary,
    }), [theme.palette.mode, theme.palette.divider, theme.palette.text.secondary]);

    // Process data based on time range
    const processedData = useMemo(() => {
        if (!data || data.length === 0) {
            // Generate sample empty data
            return Array.from({ length: 12 }, (_, i) => ({
                time: `${String(i * 2).padStart(2, "0")}:00`,
                error_4xx: 0,
                error_5xx: 0,
                success: 0,
                total: 0,
            }));
        }

        return data.map((item) => ({
            ...item,
            success: (item.requests || item.count || 0) - item.error_4xx - item.error_5xx,
            total: item.requests || item.count || (item.error_4xx + item.error_5xx),
        }));
    }, [data]);

    // Calculate statistics
    const stats = useMemo(() => {
        const total4xx = processedData.reduce((sum, d) => sum + d.error_4xx, 0);
        const total5xx = processedData.reduce((sum, d) => sum + d.error_5xx, 0);
        const totalRequests = processedData.reduce((sum, d) => sum + (d.total || 0), 0);
        const totalErrors = total4xx + total5xx;
        const successRate = totalRequests > 0
            ? ((totalRequests - totalErrors) / totalRequests * 100).toFixed(1)
            : "100.0";

        return { total4xx, total5xx, totalErrors, totalRequests, successRate };
    }, [processedData]);

    const handleTimeRangeChange = (event: React.MouseEvent<HTMLElement>, newRange: TimeRange | null) => {
        event.stopPropagation(); // Prevent parent card click
        if (newRange) {
            setTimeRange(newRange);
            onModeChange?.(newRange);
        }
    };

    const CustomTooltip = ({ active, payload, label }: any) => {
        if (active && payload && payload.length) {
            const total = payload.reduce((sum: number, entry: any) => sum + (entry.value || 0), 0);
            return (
                <Box
                    sx={{
                        bgcolor: alpha(theme.palette.background.paper, 0.98),
                        p: 2,
                        border: `1px solid ${theme.palette.divider}`,
                        borderRadius: 2,
                        boxShadow: theme.shadows[8],
                        minWidth: 160,
                    }}
                >
                    <Typography variant="subtitle2" sx={{ fontWeight: 700, mb: 1.5, color: theme.palette.text.primary }}>
                        {label}
                    </Typography>
                    {payload.map((entry: any, index: number) => (
                        <Box
                            key={index}
                            sx={{
                                display: "flex",
                                alignItems: "center",
                                justifyContent: "space-between",
                                gap: 2,
                                py: 0.5,
                            }}
                        >
                            <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
                                <Box
                                    sx={{
                                        width: 10,
                                        height: 10,
                                        bgcolor: entry.fill,
                                        borderRadius: 0.5,
                                    }}
                                />
                                <Typography variant="body2" color="text.secondary">
                                    {entry.name}
                                </Typography>
                            </Box>
                            <Typography variant="body2" sx={{ fontWeight: 600 }}>
                                {entry.value}
                            </Typography>
                        </Box>
                    ))}
                    <Box
                        sx={{
                            display: "flex",
                            justifyContent: "space-between",
                            mt: 1.5,
                            pt: 1.5,
                            borderTop: `1px solid ${theme.palette.divider}`,
                        }}
                    >
                        <Typography variant="body2" sx={{ fontWeight: 600 }}>Total</Typography>
                        <Typography variant="body2" sx={{ fontWeight: 700 }}>{total}</Typography>
                    </Box>
                </Box>
            );
        }
        return null;
    };

    const CustomLegend = ({ payload }: any) => (
        <Box sx={{ display: "flex", justifyContent: "center", gap: 3, mt: 1 }}>
            {payload?.map((entry: any, index: number) => (
                <Box
                    key={index}
                    sx={{
                        display: "flex",
                        alignItems: "center",
                        gap: 0.75,
                        cursor: "pointer",
                        opacity: activeBar && activeBar !== entry.dataKey ? 0.4 : 1,
                        transition: "opacity 0.2s ease",
                    }}
                    onMouseEnter={() => setActiveBar(entry.dataKey)}
                    onMouseLeave={() => setActiveBar(null)}
                >
                    <Box
                        sx={{
                            width: 12,
                            height: 12,
                            bgcolor: entry.color,
                            borderRadius: 0.5,
                        }}
                    />
                    <Typography variant="caption" sx={{ fontWeight: 500, color: theme.palette.text.secondary }}>
                        {entry.value}
                    </Typography>
                </Box>
            ))}
        </Box>
    );

    const rootLayout = fillParent
        ? { height: "100%", minHeight: 160, display: "flex", flexDirection: "column" as const }
        : { height: 280, display: "flex", flexDirection: "column" as const };

    // Empty state
    if (!data || data.length === 0 || stats.totalRequests === 0) {
        return (
            <Box sx={rootLayout}>
                {showModeSelector && (
                    <Box sx={{ display: "flex", justifyContent: "space-between", alignItems: "center", mb: 2 }}>
                        <Stack direction="row" spacing={1} alignItems="center">
                            <CheckCircle sx={{ color: colors.success, fontSize: 20 }} />
                            <Typography variant="body2" sx={{ fontWeight: 600, color: colors.success }}>
                                No errors recorded
                            </Typography>
                        </Stack>
                        <ToggleButtonGroup
                            value={timeRange}
                            exclusive
                            onChange={handleTimeRangeChange}
                            onMouseDown={(event) => event.stopPropagation()}
                            onClick={(event) => event.stopPropagation()}
                            size="small"
                            aria-label="time range"
                            sx={{
                                bgcolor: alpha(theme.palette.background.paper, 0.5),
                                borderRadius: 1,
                                "& .MuiToggleButton-root": {
                                    px: 1.5,
                                    py: 0.5,
                                    fontSize: "0.75rem",
                                    fontWeight: 500,
                                    color: theme.palette.text.secondary,
                                    border: "none",
                                    borderRadius: "4px !important",
                                    mx: 0.25,
                                    "&.Mui-selected": {
                                        bgcolor: alpha(theme.palette.primary.main, 0.15),
                                        color: theme.palette.primary.main,
                                        fontWeight: 600,
                                    },
                                },
                            }}
                        >
                            <ToggleButton value="1h">1H</ToggleButton>
                            <ToggleButton value="6h">6H</ToggleButton>
                            <ToggleButton value="24h">24H</ToggleButton>
                            <ToggleButton value="7d">7D</ToggleButton>
                        </ToggleButtonGroup>
                    </Box>
                )}
                <Box
                    sx={{
                        flex: 1,
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        justifyContent: "center",
                        color: theme.palette.text.secondary,
                    }}
                >
                    <CheckCircle sx={{ fontSize: 48, color: colors.success, mb: 1, opacity: 0.5 }} />
                    <Typography variant="body2" color="text.secondary">
                        No error data available for this period
                    </Typography>
                </Box>
            </Box>
        );
    }

    return (
        <Box sx={rootLayout}>
            {/* Header with stats and mode selector */}
            {showModeSelector && (
                <Box sx={{ display: "flex", justifyContent: "space-between", alignItems: "flex-start", mb: 2, flexShrink: 0 }}>
                    <Stack direction="row" spacing={1.5} alignItems="center">
                        <Chip
                            icon={parseFloat(stats.successRate) >= 99 ? <CheckCircle /> : parseFloat(stats.successRate) >= 95 ? <Warning /> : <ErrorIcon />}
                            label={`${stats.successRate}% Success`}
                            size="small"
                            sx={{
                                bgcolor: alpha(
                                    parseFloat(stats.successRate) >= 99 ? colors.success :
                                    parseFloat(stats.successRate) >= 95 ? colors.warning : colors.error,
                                    0.1
                                ),
                                color: parseFloat(stats.successRate) >= 99 ? colors.success :
                                       parseFloat(stats.successRate) >= 95 ? colors.warning : colors.error,
                                fontWeight: 600,
                                "& .MuiChip-icon": { color: "inherit" },
                            }}
                        />
                        <Typography variant="caption" color="text.secondary">
                            {stats.totalErrors} errors / {stats.totalRequests} requests
                        </Typography>
                    </Stack>
                    <ToggleButtonGroup
                        value={timeRange}
                        exclusive
                        onChange={handleTimeRangeChange}
                        onMouseDown={(event) => event.stopPropagation()}
                        onClick={(event) => event.stopPropagation()}
                        size="small"
                        aria-label="time range"
                        sx={{
                            bgcolor: alpha(theme.palette.background.paper, 0.5),
                            borderRadius: 1,
                            "& .MuiToggleButton-root": {
                                px: 1.5,
                                py: 0.5,
                                fontSize: "0.75rem",
                                fontWeight: 500,
                                color: theme.palette.text.secondary,
                                border: "none",
                                borderRadius: "4px !important",
                                mx: 0.25,
                                transition: "all 0.2s ease",
                                "&:hover": {
                                    bgcolor: alpha(theme.palette.primary.main, 0.08),
                                },
                                "&.Mui-selected": {
                                    bgcolor: alpha(theme.palette.primary.main, 0.15),
                                    color: theme.palette.primary.main,
                                    fontWeight: 600,
                                    "&:hover": {
                                        bgcolor: alpha(theme.palette.primary.main, 0.2),
                                    },
                                },
                            },
                        }}
                    >
                        <ToggleButton value="1h" aria-label="1 hour">1H</ToggleButton>
                        <ToggleButton value="6h" aria-label="6 hours">6H</ToggleButton>
                        <ToggleButton value="24h" aria-label="24 hours">24H</ToggleButton>
                        <ToggleButton value="7d" aria-label="7 days">7D</ToggleButton>
                    </ToggleButtonGroup>
                </Box>
            )}

            <Box
                sx={
                    fillParent
                        ? {
                              flex: 1,
                              minHeight: 0,
                              width: "100%",
                              maxWidth: "100%",
                              overflow: "hidden",
                              position: "relative",
                              isolation: "isolate",
                          }
                        : {
                              width: "100%",
                              maxWidth: "100%",
                              overflow: "hidden",
                              position: "relative",
                              minHeight: showModeSelector ? 210 : 240,
                          }
                }
                onMouseDown={(event) => event.stopPropagation()}
                onClick={(event) => event.stopPropagation()}
            >
            <ResponsiveContainer
                width="100%"
                height={fillParent ? "100%" : (showModeSelector ? 210 : 240)}
            >
                <BarChart
                    data={processedData}
                    margin={{ top: 5, right: 5, left: -10, bottom: 5 }}
                    barGap={0}
                    barCategoryGap="20%"
                >
                    <defs>
                        <linearGradient id="gradient4xx" x1="0" y1="0" x2="0" y2="1">
                            <stop offset="0%" stopColor={colors.warning} stopOpacity={1} />
                            <stop offset="100%" stopColor={colors.warning} stopOpacity={0.7} />
                        </linearGradient>
                        <linearGradient id="gradient5xx" x1="0" y1="0" x2="0" y2="1">
                            <stop offset="0%" stopColor={colors.error} stopOpacity={1} />
                            <stop offset="100%" stopColor={colors.error} stopOpacity={0.7} />
                        </linearGradient>
                    </defs>
                    <CartesianGrid
                        strokeDasharray="3 3"
                        stroke={colors.grid}
                        vertical={false}
                    />
                    <XAxis
                        dataKey="time"
                        stroke={colors.text}
                        style={{ fontSize: "11px" }}
                        tickLine={false}
                        axisLine={{ stroke: colors.grid }}
                        tick={{ fill: colors.text }}
                    />
                    <YAxis
                        stroke={colors.text}
                        style={{ fontSize: "11px" }}
                        tickLine={false}
                        axisLine={false}
                        tick={{ fill: colors.text }}
                        allowDecimals={false}
                    />
                    <Tooltip
                        content={<CustomTooltip />}
                        cursor={{ fill: alpha(theme.palette.action.hover, 0.1) }}
                    />
                    <Legend content={<CustomLegend />} />
                    <Bar
                        dataKey="error_4xx"
                        name="4xx Client Errors"
                        stackId="errors"
                        fill="url(#gradient4xx)"
                        radius={[0, 0, 0, 0]}
                        animationDuration={800}
                        animationEasing="ease-out"
                    >
                        {processedData.map((_, index) => (
                            <Cell
                                key={`cell-4xx-${index}`}
                                opacity={activeBar === null || activeBar === "error_4xx" ? 1 : 0.3}
                            />
                        ))}
                    </Bar>
                    <Bar
                        dataKey="error_5xx"
                        name="5xx Server Errors"
                        stackId="errors"
                        fill="url(#gradient5xx)"
                        radius={[4, 4, 0, 0]}
                        animationDuration={800}
                        animationEasing="ease-out"
                        animationBegin={200}
                    >
                        {processedData.map((_, index) => (
                            <Cell
                                key={`cell-5xx-${index}`}
                                opacity={activeBar === null || activeBar === "error_5xx" ? 1 : 0.3}
                            />
                        ))}
                    </Bar>
                    {/* Average line */}
                    {stats.totalErrors > 0 && (
                        <ReferenceLine
                            y={stats.totalErrors / processedData.length}
                            stroke={colors.error}
                            strokeDasharray="5 5"
                            strokeOpacity={0.5}
                        />
                    )}
                </BarChart>
            </ResponsiveContainer>
            </Box>
        </Box>
    );
};

export default ApiErrorsChart;
