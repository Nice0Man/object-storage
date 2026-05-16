import React, { useCallback, useEffect, useRef, useState } from "react";
import {
  Alert,
  Box,
  Button,
  Chip,
  Paper,
  Tooltip,
  Typography,
} from "@mui/material";
import { useTranslation } from "react-i18next";
import { Terminal as TerminalIcon, Refresh } from "@mui/icons-material";
import { Terminal } from "@xterm/xterm";
import { FitAddon } from "@xterm/addon-fit";
import "@xterm/xterm/css/xterm.css";
import apiClient from "../../api/client";
import type { TerminalStatusResponse } from "../../api/types";
import { getApiUrlFromEnv } from "../../env";

function buildTerminalWebSocketUrl(wsPath: string): string {
  const token = localStorage.getItem("auth_token") ?? "";
  const envApi = getApiUrlFromEnv();
  let host: string;
  let protocol: string;

  if (envApi) {
    const parsed = new URL(envApi, window.location.origin);
    host = parsed.host;
    protocol = parsed.protocol === "https:" ? "wss:" : "ws:";
  } else {
    host = window.location.host;
    protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  }

  const path = wsPath.startsWith("/") ? wsPath : `/${wsPath}`;
  return `${protocol}//${host}${path}?token=${encodeURIComponent(token)}`;
}

const AdminTerminalPanel: React.FC = () => {
  const { t } = useTranslation();
  const containerRef = useRef<HTMLDivElement>(null);
  const termRef = useRef<Terminal | null>(null);
  const fitRef = useRef<FitAddon | null>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const [status, setStatus] = useState<TerminalStatusResponse | null>(null);
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [loadingStatus, setLoadingStatus] = useState(true);

  const loadStatus = useCallback(async () => {
    setLoadingStatus(true);
    try {
      const data = await apiClient.getTerminalStatus();
      setStatus(data);
      if (!data.allowed) {
        setError(
          data.enabled ? t("system.terminal.errorAdminOnly") : t("system.terminal.errorDisabled"),
        );
      } else {
        setError(null);
      }
    } catch {
      setError(t("system.terminal.errorLoad"));
      setStatus(null);
    } finally {
      setLoadingStatus(false);
    }
  }, [t]);

  useEffect(() => {
    loadStatus();
  }, [loadStatus]);

  const disconnect = useCallback(() => {
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
    setConnected(false);
  }, []);

  const sendResize = useCallback(() => {
    const term = termRef.current;
    const ws = wsRef.current;
    if (!term || !ws || ws.readyState !== WebSocket.OPEN) {
      return;
    }
    ws.send(
      JSON.stringify({
        type: "resize",
        cols: term.cols,
        rows: term.rows,
      }),
    );
  }, []);

  const connect = useCallback(() => {
    if (!status?.allowed || !containerRef.current) {
      return;
    }

    disconnect();

    const term = new Terminal({
      cursorBlink: true,
      fontSize: 14,
      fontFamily: '"JetBrains Mono", "Fira Code", monospace',
      theme: {
        background: "#0f172a",
        foreground: "#e2e8f0",
        cursor: "#93c5fd",
      },
      convertEol: true,
    });
    const fitAddon = new FitAddon();
    term.loadAddon(fitAddon);
    term.open(containerRef.current);
    fitAddon.fit();

    termRef.current = term;
    fitRef.current = fitAddon;

    const wsUrl = buildTerminalWebSocketUrl(status.ws_path || "/api/v1/ws/terminal");
    const ws = new WebSocket(wsUrl);
    ws.binaryType = "arraybuffer";
    wsRef.current = ws;

    ws.onopen = () => {
      setConnected(true);
      setError(null);
      term.writeln("\r\n\x1b[32m[connected]\x1b[0m Admin shell session\r\n");
      sendResize();
    };

    ws.onmessage = (event) => {
      if (typeof event.data === "string") {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === "error") {
            setError(msg.message || "Terminal error");
            term.writeln(`\r\n\x1b[31m${msg.message}\x1b[0m`);
          }
        } catch {
          term.write(event.data);
        }
        return;
      }
      if (event.data instanceof ArrayBuffer) {
        term.write(new Uint8Array(event.data));
      }
    };

    ws.onerror = () => {
      setError(t("system.terminal.errorWebSocket"));
    };

    ws.onclose = () => {
      setConnected(false);
      term.writeln("\r\n\x1b[33m[disconnected]\x1b[0m");
    };

    term.onData((data) => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(new TextEncoder().encode(data));
      }
    });

    const onResize = () => {
      fitAddon.fit();
      sendResize();
    };
    window.addEventListener("resize", onResize);

    return () => {
      window.removeEventListener("resize", onResize);
    };
  }, [status, disconnect, sendResize, t]);

  useEffect(() => {
    return () => {
      disconnect();
      termRef.current?.dispose();
      termRef.current = null;
    };
  }, [disconnect]);

  if (loadingStatus) {
    return (
      <Paper variant="outlined" sx={{ p: 3 }}>
        <Typography color="text.secondary">{t("system.terminal.loading")}</Typography>
      </Paper>
    );
  }

  return (
    <Paper variant="outlined" sx={{ p: 2 }}>
      <Box sx={{ display: "flex", alignItems: "center", gap: 1, mb: 2, flexWrap: "wrap" }}>
        <TerminalIcon color="primary" />
        <Typography variant="h6" sx={{ fontWeight: 600, flex: 1 }}>
          {t("system.terminal.title")}
        </Typography>
        <Chip
          size="small"
          label={connected ? t("system.terminal.connected") : t("system.terminal.disconnected")}
          color={connected ? "success" : "default"}
          variant="outlined"
        />
        {status?.shell && (
          <Chip size="small" label={status.shell} variant="outlined" />
        )}
        <Tooltip title={t("system.terminal.refreshTooltip")}>
          <span>
            <Button
              size="small"
              startIcon={<Refresh />}
              onClick={loadStatus}
              disabled={loadingStatus}
            >
              {t("common.refresh")}
            </Button>
          </span>
        </Tooltip>
        {status?.allowed && (
          <Button
            size="small"
            variant="contained"
            onClick={connected ? disconnect : connect}
          >
            {connected ? t("system.terminal.disconnect") : t("system.terminal.connect")}
          </Button>
        )}
      </Box>

      {error && (
        <Alert severity="warning" sx={{ mb: 2 }}>
          {error}
        </Alert>
      )}

      {!status?.enabled && (
        <Alert severity="info" sx={{ mb: 2 }}>
          {t("system.terminal.disabledHint")}
        </Alert>
      )}

      <Box
        ref={containerRef}
        sx={{
          height: 420,
          bgcolor: "#0f172a",
          borderRadius: 1,
          border: 1,
          borderColor: "divider",
          overflow: "hidden",
          opacity: status?.allowed ? 1 : 0.5,
          pointerEvents: status?.allowed ? "auto" : "none",
        }}
      />
    </Paper>
  );
};

export default AdminTerminalPanel;
