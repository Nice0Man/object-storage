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

  const renderCustomLabel = (props: {
    cx?: number;
    cy?: number;
    outerRadius?: number | string;
  }) => {
    const cx = Number(props.cx ?? 0);
    const cy = Number(props.cy ?? 0);
    let outerRadius = typeof props.outerRadius === "number" ? props.outerRadius : Number(props.outerRadius);
    if (!Number.isFinite(outerRadius) || outerRadius <= 0) {
      outerRadius = 90;
    }
    const mainSize = Math.max(11, Math.min(26, Math.round(outerRadius * 0.3)));
    const subSize = Math.max(9, Math.min(14, Math.round(outerRadius * 0.14)));
    const capSize = Math.max(8, Math.min(12, Math.round(outerRadius * 0.11)));
    const parts = formatBytes(available).split(" ");

    return (
      <g>
        <text
          x={cx}
          y={cy - outerRadius * 0.12}
          fill={theme.palette.text.primary}
          textAnchor="middle"
          dominantBaseline="central"
          style={{
            fontSize: `${mainSize}px`,
            fontWeight: 700,
          }}
        >
          {parts[0] ?? ""}
        </text>
        <text
          x={cx}
          y={cy + outerRadius * 0.08}
          fill={theme.palette.text.secondary}
          textAnchor="middle"
          dominantBaseline="central"
          style={{
            fontSize: `${subSize}px`,
          }}
        >
          {parts[1] ?? ""}
        </text>
        <text
          x={cx}
          y={cy + outerRadius * 0.22}
          fill={theme.palette.text.secondary}
          textAnchor="middle"
          dominantBaseline="central"
          style={{
            fontSize: `${capSize}px`,
          }}
        >
          Available
        </text>
      </g>
    );
  };

  const CustomTooltip = ({ active, payload }: {
    active?: boolean;
    payload?: Array<{ name: string; value: number }>;
  }) => {
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
    <Box sx={{ width: "100%", height: "100%", minHeight: 100, flex: "1 1 0%" }}>
      <ResponsiveContainer width="100%" height="100%">
        <PieChart>
          <Pie
            data={data}
            cx="50%"
            cy="50%"
            labelLine={false}
            label={renderCustomLabel}
            innerRadius="52%"
            outerRadius="88%"
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
    </Box>
  );
};

export default CapacityPieChart;
