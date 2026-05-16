export type ChartTimeRange = "1h" | "6h" | "24h" | "7d";

export function normalizeChartTimestamp(ts?: number): number | undefined {
  if (ts == null || Number.isNaN(ts)) {
    return undefined;
  }
  return ts > 1e12 ? Math.floor(ts / 1000) : ts;
}

/** Client-side filter when API returns a wider window than the selected range. */
export function filterChartDataByTimeRange<T extends { timestamp?: number }>(
  data: T[],
  range: ChartTimeRange,
): T[] {
  if (!data?.length) {
    return [];
  }

  const normalized = data.map((point) => ({
    ...point,
    timestamp: normalizeChartTimestamp(point.timestamp),
  }));

  const nowSec = Math.floor(Date.now() / 1000);
  const rangeSeconds: Record<ChartTimeRange, number> = {
    "1h": 3600,
    "6h": 6 * 3600,
    "24h": 24 * 3600,
    "7d": 7 * 24 * 3600,
  };

  const cutoff = nowSec - rangeSeconds[range];
  const withTs = normalized.filter((p) => (p.timestamp ?? 0) >= cutoff);
  if (withTs.length > 0) {
    return withTs;
  }

  const pointCount: Record<ChartTimeRange, number> = {
    "1h": 1,
    "6h": 6,
    "24h": normalized.length,
    "7d": normalized.length,
  };
  return normalized.slice(-pointCount[range]);
}
