import React, { useState, useMemo } from "react";
import {
    XAxis,
    YAxis,
    CartesianGrid,
    Tooltip,
    ResponsiveContainer,
    Area,
    AreaChart,
    Legend,
} from "recharts";
import {
    Box,
    Typography,
    useTheme,
    alpha,
    ToggleButton,
    ToggleButtonGroup,
    Stack,
    Chip,
} from "@mui/material";
import {
    TrendingUp,
    TrendingDown,
    CloudDownload,
    CloudUpload,
    ShowChart,
} from "@mui/icons-material";

interface DataPoint {
    time: string;
    timestamp?: number;
    read_bytes: number;
    write_bytes: number;
    total_bytes: number;
}

type TimeRange = "1h" | "6h" | "24h" | "7d";
type ChartMode = "stacked" | "lines" | "total";

interface DataThroughputChartProps {
    data: DataPoint[];
    formatBytes: (bytes: number) => string;
    showModeSelector?: boolean;
    showChartModeSelector?: boolean;
    defaultTimeRange?: TimeRange;
    defaultChartMode?: ChartMode;
    onTimeRangeChange?: (range: TimeRange) => void;
    /** When true, chart fills parent flex height (dashboard widgets). */
    fillParent?: boolean;
}

const DataThroughputChart: React.FC<DataThroughputChartProps> = ({
    data,
    formatBytes,
    showModeSelector = true,
    showChartModeSelector = true,
    defaultTimeRange = "24h",
    defaultChartMode = "stacked",
    onTimeRangeChange,
    fillParent = false,
}) => {
    const theme = useTheme();
    const [timeRange, setTimeRange] = useState<TimeRange>(defaultTimeRange);
    const [chartMode, setChartMode] = useState<ChartMode>(defaultChartMode);
    const [activeArea, setActiveArea] = useState<string | null>(null);

    // Colors based on theme
    const colors = useMemo(() => ({
        read: theme.palette.mode === "dark" ? "#8B5CF6" : "#7C3AED",
        readLight: theme.palette.mode === "dark" ? "#A78BFA" : "#A78BFA",
        write: theme.palette.mode === "dark" ? "#3B82F6" : "#2563EB",
        writeLight: theme.palette.mode === "dark" ? "#60A5FA" : "#60A5FA",
        total: theme.palette.mode === "dark" ? "#10B981" : "#059669",
        totalLight: theme.palette.mode === "dark" ? "#34D399" : "#34D399",
        grid: alpha(theme.palette.divider, 0.15),
        text: theme.palette.text.secondary,
    }), [theme.palette.mode, theme.palette.divider, theme.palette.text.secondary]);

    // Process data
    const processedData = useMemo(() => {
        if (!data || data.length === 0) {
            // Generate sample empty data
            return Array.from({ length: 12 }, (_, i) => ({
                time: `${String(i * 2).padStart(2, "0")}:00`,
                read_bytes: 0,
                write_bytes: 0,
                total_bytes: 0,
            }));
        }
        return data;
    }, [data]);

    // Calculate statistics
    const stats = useMemo(() => {
        const totalRead = processedData.reduce((sum, d) => sum + d.read_bytes, 0);
        const totalWrite = processedData.reduce((sum, d) => sum + d.write_bytes, 0);
        const total = totalRead + totalWrite;
        const avgRead = totalRead / processedData.length;
        const avgWrite = totalWrite / processedData.length;

        // Calculate trend (last half vs first half)
        const mid = Math.floor(processedData.length / 2);
        const firstHalf = processedData.slice(0, mid);
        const secondHalf = processedData.slice(mid);
        const firstHalfTotal = firstHalf.reduce((sum, d) => sum + d.read_bytes + d.write_bytes, 0);
        const secondHalfTotal = secondHalf.reduce((sum, d) => sum + d.read_bytes + d.write_bytes, 0);
        const trend = firstHalfTotal > 0
            ? ((secondHalfTotal - firstHalfTotal) / firstHalfTotal * 100).toFixed(1)
            : "0";
        const trendUp = parseFloat(trend) > 0;

        return { totalRead, totalWrite, total, avgRead, avgWrite, trend, trendUp };
    }, [processedData]);

    const handleTimeRangeChange = (event: React.MouseEvent<HTMLElement>, newRange: TimeRange | null) => {
        event.stopPropagation(); // Prevent parent card click
        if (newRange) {
            setTimeRange(newRange);
            onTimeRangeChange?.(newRange);
        }
    };

    const handleChartModeChange = (event: React.MouseEvent<HTMLElement>, newMode: ChartMode | null) => {
        event.stopPropagation(); // Prevent parent card click
        if (newMode) {
            setChartMode(newMode);
        }
    };

    const CustomTooltip = ({ active, payload, label }: any) => {
        if (active && payload && payload.length) {
            return (
                <Box
                    sx={{
                        bgcolor: alpha(theme.palette.background.paper, 0.98),
                        p: 2,
                        border: `1px solid ${theme.palette.divider}`,
                        borderRadius: 2,
                        boxShadow: theme.shadows[8],
                        minWidth: 180,
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
                                        bgcolor: entry.stroke,
                                        borderRadius: "50%",
                                    }}
                                />
                                <Typography variant="body2" color="text.secondary">
                                    {entry.name}
                                </Typography>
                            </Box>
                            <Typography variant="body2" sx={{ fontWeight: 600 }}>
                                {formatBytes(entry.value)}
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
                        <Typography variant="body2" sx={{ fontWeight: 700 }}>
                            {formatBytes(payload.reduce((sum: number, entry: any) => sum + entry.value, 0))}
                        </Typography>
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
                        opacity: activeArea && activeArea !== entry.dataKey ? 0.4 : 1,
                        transition: "opacity 0.2s ease",
                    }}
                    onMouseEnter={() => setActiveArea(entry.dataKey)}
                    onMouseLeave={() => setActiveArea(null)}
                >
                    {entry.dataKey === "read_bytes" && <CloudDownload sx={{ fontSize: 14, color: entry.color }} />}
                    {entry.dataKey === "write_bytes" && <CloudUpload sx={{ fontSize: 14, color: entry.color }} />}
                    {entry.dataKey === "total_bytes" && <ShowChart sx={{ fontSize: 14, color: entry.color }} />}
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
    if (!data || data.length === 0 || stats.total === 0) {
        return (
            <Box sx={rootLayout}>
                {showModeSelector && (
                    <Box sx={{ display: "flex", justifyContent: "flex-end", mb: 2 }}>
                        <ToggleButtonGroup
                            value={timeRange}
                            exclusive
                            onChange={handleTimeRangeChange}
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
                    <ShowChart sx={{ fontSize: 48, color: colors.write, mb: 1, opacity: 0.5 }} />
                    <Typography variant="body2" color="text.secondary">
                        No throughput data available for this period
                    </Typography>
                </Box>
            </Box>
        );
    }

    return (
        <Box sx={rootLayout}>
            {/* Header with stats and mode selectors */}
            <Box sx={{ display: "flex", justifyContent: "space-between", alignItems: "flex-start", mb: 2, flexShrink: 0 }}>
                <Stack direction="row" spacing={1.5} alignItems="center">
                    <Chip
                        icon={stats.trendUp ? <TrendingUp /> : <TrendingDown />}
                        label={`${stats.trendUp ? "+" : ""}${stats.trend}%`}
                        size="small"
                        sx={{
                            bgcolor: alpha(stats.trendUp ? colors.total : colors.read, 0.1),
                            color: stats.trendUp ? colors.total : colors.read,
                            fontWeight: 600,
                            "& .MuiChip-icon": { color: "inherit" },
                        }}
                    />
                    <Typography variant="caption" color="text.secondary">
                        {formatBytes(stats.total)} total
                    </Typography>
                </Stack>
                <Stack direction="row" spacing={1}>
                    {showChartModeSelector && (
                        <ToggleButtonGroup
                            value={chartMode}
                            exclusive
                            onChange={handleChartModeChange}
                            onMouseDown={(event) => event.stopPropagation()}
                            onClick={(event) => event.stopPropagation()}
                            size="small"
                            aria-label="chart mode"
                            sx={{
                                bgcolor: alpha(theme.palette.background.paper, 0.5),
                                borderRadius: 1,
                                "& .MuiToggleButton-root": {
                                    px: 1,
                                    py: 0.5,
                                    fontSize: "0.7rem",
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
                            <ToggleButton value="stacked">Stacked</ToggleButton>
                            <ToggleButton value="lines">Lines</ToggleButton>
                            <ToggleButton value="total">Total</ToggleButton>
                        </ToggleButtonGroup>
                    )}
                    {showModeSelector && (
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
                            <ToggleButton value="1h">1H</ToggleButton>
                            <ToggleButton value="6h">6H</ToggleButton>
                            <ToggleButton value="24h">24H</ToggleButton>
                            <ToggleButton value="7d">7D</ToggleButton>
                        </ToggleButtonGroup>
                    )}
                </Stack>
            </Box>

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
                              minHeight: showModeSelector || showChartModeSelector ? 210 : 240,
                          }
                }
                onMouseDown={(event) => event.stopPropagation()}
                onClick={(event) => event.stopPropagation()}
            >
            <ResponsiveContainer
                width="100%"
                height={fillParent ? "100%" : (showModeSelector || showChartModeSelector ? 210 : 240)}
            >
                <AreaChart
                    data={processedData}
                    margin={{ top: 5, right: 5, left: -10, bottom: 5 }}
                >
                    <defs>
                        <linearGradient id="colorRead" x1="0" y1="0" x2="0" y2="1">
                            <stop offset="5%" stopColor={colors.read} stopOpacity={0.4} />
                            <stop offset="95%" stopColor={colors.read} stopOpacity={0.05} />
                        </linearGradient>
                        <linearGradient id="colorWrite" x1="0" y1="0" x2="0" y2="1">
                            <stop offset="5%" stopColor={colors.write} stopOpacity={0.4} />
                            <stop offset="95%" stopColor={colors.write} stopOpacity={0.05} />
                        </linearGradient>
                        <linearGradient id="colorTotal" x1="0" y1="0" x2="0" y2="1">
                            <stop offset="5%" stopColor={colors.total} stopOpacity={0.4} />
                            <stop offset="95%" stopColor={colors.total} stopOpacity={0.05} />
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
                        tickFormatter={(value) => {
                            const formatted = formatBytes(value);
                            return formatted.split(" ")[0];
                        }}
                    />
                    <Tooltip
                        content={<CustomTooltip />}
                        cursor={{ stroke: alpha(theme.palette.primary.main, 0.3), strokeWidth: 1 }}
                    />
                    <Legend content={<CustomLegend />} />

                    {chartMode === "total" ? (
                        <Area
                            type="monotone"
                            dataKey="total_bytes"
                            name="Total"
                            stroke={colors.total}
                            strokeWidth={2}
                            fill="url(#colorTotal)"
                            animationDuration={800}
                            animationEasing="ease-out"
                            dot={false}
                            activeDot={{ r: 6, fill: colors.total, stroke: "#fff", strokeWidth: 2 }}
                        />
                    ) : (
                        <>
                            <Area
                                type="monotone"
                                dataKey="write_bytes"
                                name="Write"
                                stroke={colors.write}
                                strokeWidth={2}
                                fill={chartMode === "stacked" ? "url(#colorWrite)" : "transparent"}
                                stackId={chartMode === "stacked" ? "1" : undefined}
                                animationDuration={800}
                                animationEasing="ease-out"
                                dot={false}
                                activeDot={{ r: 5, fill: colors.write, stroke: "#fff", strokeWidth: 2 }}
                                opacity={activeArea === null || activeArea === "write_bytes" ? 1 : 0.3}
                            />
                            <Area
                                type="monotone"
                                dataKey="read_bytes"
                                name="Read"
                                stroke={colors.read}
                                strokeWidth={2}
                                fill={chartMode === "stacked" ? "url(#colorRead)" : "transparent"}
                                stackId={chartMode === "stacked" ? "1" : undefined}
                                animationDuration={800}
                                animationEasing="ease-out"
                                animationBegin={200}
                                dot={false}
                                activeDot={{ r: 5, fill: colors.read, stroke: "#fff", strokeWidth: 2 }}
                                opacity={activeArea === null || activeArea === "read_bytes" ? 1 : 0.3}
                            />
                        </>
                    )}
                </AreaChart>
            </ResponsiveContainer>
            </Box>
        </Box>
    );
};

export default DataThroughputChart;
