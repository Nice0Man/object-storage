/** Shared Recharts layout so axes and legend are not clipped in widgets. */
export const CHART_MARGIN = { top: 12, right: 16, left: 4, bottom: 28 } as const;

export const CHART_Y_AXIS_WIDTH = 56;

export const chartContainerSx = (fillParent: boolean) =>
  fillParent
    ? {
        flex: 1,
        minHeight: 0,
        width: "100%",
        maxWidth: "100%",
        overflow: "visible",
        position: "relative" as const,
      }
    : {
        width: "100%",
        maxWidth: "100%",
        overflow: "visible",
        position: "relative" as const,
        minHeight: 240,
      };
