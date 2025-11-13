import React from "react";
import { PieChart, Pie, Cell, ResponsiveContainer, Tooltip } from "recharts";
import { Box, Typography, useTheme } from "@mui/material";

interface CapacityData {
  name: string;
  value: number;
  color: string;
  [key: string]: string | number;
}

interface CapacityPieChartProps {
  available: number;
  used: number;
  formatBytes: (bytes: number) => string;
}

const CapacityPieChart: React.FC<CapacityPieChartProps> = ({
  available,
  used,
  formatBytes,
}) => {
  const theme = useTheme();

  const data: CapacityData[] = [
    {
      name: "Available",
      value: available,
      color: theme.palette.mode === "dark" ? "#3B82F6" : "#2563EB",
    },
    {
      name: "Object Data",
      value: used,
      color: theme.palette.mode === "dark" ? "#1E40AF" : "#1D4ED8",
    },
  ];

  const renderCustomLabel = ({
    cx,
    cy,
  }: any) => {
    return (
      <g>
        <text
          x={cx}
          y={cy - 10}
          fill={theme.palette.text.primary}
          textAnchor="middle"
          dominantBaseline="central"
          style={{
            fontSize: "32px",
            fontWeight: 700,
          }}
        >
          {formatBytes(available).split(" ")[0]}
        </text>
        <text
          x={cx}
          y={cy + 20}
          fill={theme.palette.text.secondary}
          textAnchor="middle"
          dominantBaseline="central"
          style={{
            fontSize: "14px",
          }}
        >
          {formatBytes(available).split(" ")[1]}
        </text>
        <text
          x={cx}
          y={cy + 40}
          fill={theme.palette.text.secondary}
          textAnchor="middle"
          dominantBaseline="central"
          style={{
            fontSize: "12px",
          }}
        >
          Available
        </text>
      </g>
    );
  };

  const CustomTooltip = ({ active, payload }: any) => {
    if (active && payload && payload.length) {
      return (
        <Box
          sx={{
            bgcolor: theme.palette.background.paper,
            p: 1.5,
            border: `1px solid ${theme.palette.divider}`,
            borderRadius: 1,
          }}
        >
          <Typography variant="body2" sx={{ fontWeight: 600 }}>
            {payload[0].name}
          </Typography>
          <Typography variant="body2" color="text.secondary">
            {formatBytes(payload[0].value)}
          </Typography>
        </Box>
      );
    }
    return null;
  };

  return (
    <ResponsiveContainer width="100%" height={240}>
      <PieChart>
        <Pie
          data={data}
          cx="50%"
          cy="50%"
          labelLine={false}
          label={renderCustomLabel}
          innerRadius={70}
          outerRadius={100}
          fill="#8884d8"
          dataKey="value"
          paddingAngle={2}
        >
          {data.map((entry, index) => (
            <Cell key={`cell-${index}`} fill={entry.color} />
          ))}
        </Pie>
        <Tooltip content={<CustomTooltip />} />
      </PieChart>
    </ResponsiveContainer>
  );
};

export default CapacityPieChart;
