import type { AxiosInstance } from "axios";

export interface TerminalStatusResponse {
  enabled: boolean;
  allowed: boolean;
  shell: string;
  max_sessions: number;
  ws_path: string;
}

export function createSystemApi(client: AxiosInstance) {
  return {
    async getTerminalStatus(): Promise<TerminalStatusResponse> {
      const response = await client.get<TerminalStatusResponse>(
        "/api/v1/system/terminal/status",
      );
      return response.data;
    },
  };
}
