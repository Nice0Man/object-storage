import { useEffect, useRef, useState, useCallback } from "react";

interface UseWidgetRefreshOptions {
  /** Refresh interval in seconds, 0 means disabled */
  interval: number;
  /** Whether the widget is visible */
  enabled?: boolean;
  /** Callback to fetch data */
  onRefresh: () => Promise<void>;
  /** Initial fetch on mount */
  fetchOnMount?: boolean;
}

interface UseWidgetRefreshReturn {
  /** Whether the widget is currently loading */
  loading: boolean;
  /** Last error if any */
  error: string | null;
  /** Last successful refresh timestamp */
  lastRefresh: number | null;
  /** Manually trigger refresh */
  refresh: () => Promise<void>;
  /** Time until next refresh in seconds */
  nextRefreshIn: number | null;
}

/**
 * Hook for independent widget data refresh with configurable intervals
 * Each widget can have its own refresh cycle without affecting others
 */
export function useWidgetRefresh({
  interval,
  enabled = true,
  onRefresh,
  fetchOnMount = true,
}: UseWidgetRefreshOptions): UseWidgetRefreshReturn {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [lastRefresh, setLastRefresh] = useState<number | null>(null);
  const [nextRefreshIn, setNextRefreshIn] = useState<number | null>(null);

  const intervalRef = useRef<NodeJS.Timeout | null>(null);
  const countdownRef = useRef<NodeJS.Timeout | null>(null);
  const mountedRef = useRef(true);
  const onRefreshRef = useRef(onRefresh);

  // Keep onRefresh ref updated
  useEffect(() => {
    onRefreshRef.current = onRefresh;
  }, [onRefresh]);

  // Cleanup on unmount
  useEffect(() => {
    mountedRef.current = true;
    return () => {
      mountedRef.current = false;
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
      if (countdownRef.current) {
        clearInterval(countdownRef.current);
      }
    };
  }, []);

  // Refresh function
  const refresh = useCallback(async () => {
    if (!mountedRef.current) return;

    setLoading(true);
    setError(null);

    try {
      await onRefreshRef.current();
      if (mountedRef.current) {
        setLastRefresh(Date.now());
        // Reset countdown
        if (interval > 0) {
          setNextRefreshIn(interval);
        }
      }
    } catch (err) {
      if (mountedRef.current) {
        setError(err instanceof Error ? err.message : "Refresh failed");
      }
    } finally {
      if (mountedRef.current) {
        setLoading(false);
      }
    }
  }, [interval]);

  // Ref for refresh to avoid dependency cycle
  const refreshRef = useRef(refresh);
  useEffect(() => {
    refreshRef.current = refresh;
  }, [refresh]);

  // Initial fetch on mount - only run once
  const initialFetchDone = useRef(false);
  useEffect(() => {
    if (fetchOnMount && enabled && !initialFetchDone.current) {
      initialFetchDone.current = true;
      refreshRef.current();
    }
  }, [fetchOnMount, enabled]);

  // Set up interval for auto-refresh
  useEffect(() => {
    // Clear existing intervals
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    if (countdownRef.current) {
      clearInterval(countdownRef.current);
      countdownRef.current = null;
    }

    // Don't set up interval if disabled or interval is 0
    if (!enabled || interval <= 0) {
      setNextRefreshIn(null);
      return;
    }

    // Initialize countdown
    setNextRefreshIn(interval);

    // Countdown timer - updates every second
    countdownRef.current = setInterval(() => {
      if (mountedRef.current) {
        setNextRefreshIn((prev) => {
          if (prev === null || prev <= 1) return interval;
          return prev - 1;
        });
      }
    }, 1000);

    // Set up refresh interval
    intervalRef.current = setInterval(() => {
      if (mountedRef.current) {
        refreshRef.current();
      }
    }, interval * 1000);

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
      if (countdownRef.current) {
        clearInterval(countdownRef.current);
      }
    };
  }, [enabled, interval]);

  return {
    loading,
    error,
    lastRefresh,
    refresh,
    nextRefreshIn,
  };
}

export default useWidgetRefresh;
