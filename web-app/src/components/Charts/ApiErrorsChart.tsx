import React from "react";
import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
} from "recharts";
import { Box, Typography, useTheme, alpha } from "@mui/material";

interface ApiErrorData {
  time: string;
  count: number;
  error_4xx: number;
  error_5xx: number;
}

interface ApiErrorsChartProps {
  data: ApiErrorData[];
}

const ApiErrorsChart: React.FC<ApiErrorsChartProps> = ({ data }) => {
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
                  bgcolor: entry.fill,
                  borderRadius: 1,
                }}
              />
              <Typography variant="caption" color="text.secondary">
                {entry.name}: {entry.value}
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
      <BarChart data={data} margin={{ top: 10, right: 10, left: 0, bottom: 0 }}>
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
        />
        <Tooltip content={<CustomTooltip />} />
        <Bar
          dataKey="error_4xx"
          name="4xx Errors"
          stackId="a"
          fill={theme.palette.mode === "dark" ? "#F59E0B" : "#FCD34D"}
          radius={[0, 0, 0, 0]}
        />
        <Bar
          dataKey="error_5xx"
          name="5xx Errors"
          stackId="a"
          fill={theme.palette.mode === "dark" ? "#EF4444" : "#FCA5A5"}
          radius={[4, 4, 0, 0]}
        />
      </BarChart>
    </ResponsiveContainer>
  );
};

export default ApiErrorsChart;
