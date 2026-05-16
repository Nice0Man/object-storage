import type { AxiosInstance } from "axios";
import type {
  HealthResponse,
  LiveResponse,
  ReadyResponse,
  VersionResponse,
} from "../../api/types";

export function createHealthApi(client: AxiosInstance) {
  return {
    async getHealth(): Promise<HealthResponse> {
      const response = await client.get<HealthResponse>("/api/v1/health");
      return response.data;
    },
    async getReady(): Promise<ReadyResponse> {
      const response = await client.get<ReadyResponse>("/api/v1/ready");
      return response.data;
    },
    async getLive(): Promise<LiveResponse> {
      const response = await client.get<LiveResponse>("/api/v1/live");
      return response.data;
    },
    async getVersion(): Promise<VersionResponse> {
      const response = await client.get<VersionResponse>("/api/v1/version");
      return response.data;
    },
  };
}
