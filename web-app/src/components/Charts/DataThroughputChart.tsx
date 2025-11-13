import React from "react";
import {
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Area,
  AreaChart,
} from "recharts";
import { Box, Typography, useTheme, alpha } from "@mui/material";

interface DataPoint {
  time: string;
  read_bytes: number;
  write_bytes: number;
  total_bytes: number;
}

interface DataThroughputChartProps {
  data: DataPoint[];
  formatBytes: (bytes: number) => string;
}

const DataThroughputChart: React.FC<DataThroughputChartProps> = ({
  data,
  formatBytes,
}) => {
  const theme = useTheme();

  const CustomTooltip = ({ active, payload, label }: any) => {
    if (active && payload && payload.length) {
      return (
        <Box
          sx={{
            bgcolor: alpha(theme.palette.background.paper, 0.95),
            p: 1.5,
            border: `1px solid ${theme.palette.divider}`,
            borderRadius: 1,
          }}
        >
          <Typography variant="caption" sx={{ fontWeight: 600, mb: 0.5 }}>
            {label}
          </Typography>
          {payload.map((entry: any, index: number) => (
            <Box
              key={index}
              sx={{ display: "flex", alignItems: "center", gap: 1, mt: 0.5 }}
            >
              <Box
                sx={{
                  width: 12,
                  height: 12,
                  bgcolor: entry.color,
                  borderRadius: "50%",
                }}
              />
              <Typography variant="caption" color="text.secondary">
                {entry.name}: {formatBytes(entry.value)}
              </Typography>
            </Box>
          ))}
        </Box>
      );
    }
    return null;
  };

  return (
    <ResponsiveContainer width="100%" height={240}>
      <AreaChart
        data={data}
        margin={{ top: 10, right: 10, left: 0, bottom: 0 }}
      >
        <defs>
          <linearGradient id="colorRead" x1="0" y1="0" x2="0" y2="1">
            <stop
              offset="5%"
              stopColor={theme.palette.mode === "dark" ? "#8B5CF6" : "#A78BFA"}
              stopOpacity={0.3}
            />
            <stop
              offset="95%"
              stopColor={theme.palette.mode === "dark" ? "#8B5CF6" : "#A78BFA"}
              stopOpacity={0}
            />
          </linearGradient>
          <linearGradient id="colorWrite" x1="0" y1="0" x2="0" y2="1">
            <stop
              offset="5%"
              stopColor={theme.palette.mode === "dark" ? "#3B82F6" : "#60A5FA"}
              stopOpacity={0.3}
            />
            <stop
              offset="95%"
              stopColor={theme.palette.mode === "dark" ? "#3B82F6" : "#60A5FA"}
              stopOpacity={0}
            />
          </linearGradient>
        </defs>
        <CartesianGrid
          strokeDasharray="3 3"
          stroke={alpha(theme.palette.divider, 0.2)}
          vertical={false}
        />
        <XAxis
          dataKey="time"
          stroke={theme.palette.text.secondary}
          style={{ fontSize: "12px" }}
          tickLine={false}
        />
        <YAxis
          stroke={theme.palette.text.secondary}
          style={{ fontSize: "12px" }}
          tickLine={false}
          tickFormatter={(value) => formatBytes(value).split(" ")[0]}
        />
        <Tooltip content={<CustomTooltip />} />
        <Area
          type="monotone"
          dataKey="write_bytes"
          name="Write"
          stroke={theme.palette.mode === "dark" ? "#3B82F6" : "#2563EB"}
          strokeWidth={2}
          fill="url(#colorWrite)"
        />
        <Area
          type="monotone"
          dataKey="read_bytes"
          name="Read"
          stroke={theme.palette.mode === "dark" ? "#8B5CF6" : "#7C3AED"}
          strokeWidth={2}
          fill="url(#colorRead)"
        />
      </AreaChart>
    </ResponsiveContainer>
  );
};

export default DataThroughputChart;
