import type { AxiosInstance } from "axios";
import type {
  InfrastructureDrivesResponse,
  InfrastructureHealStatus,
  InfrastructurePoolsResponse,
  InfrastructureServersResponse,
  InfrastructureSummary,
} from "../../api/types";

export function createInfrastructureApi(client: AxiosInstance) {
  return {
    async getInfrastructureSummary(): Promise<InfrastructureSummary> {
      const response = await client.get<InfrastructureSummary>(
        "/api/v1/infrastructure/summary",
      );
      return response.data;
    },
    async listInfrastructureServers(): Promise<InfrastructureServersResponse> {
      const response = await client.get<InfrastructureServersResponse>(
        "/api/v1/infrastructure/servers",
      );
      return response.data;
    },
    async listInfrastructureDrives(): Promise<InfrastructureDrivesResponse> {
      const response = await client.get<InfrastructureDrivesResponse>(
        "/api/v1/infrastructure/drives",
      );
      return response.data;
    },
    async listInfrastructurePools(): Promise<InfrastructurePoolsResponse> {
      const response = await client.get<InfrastructurePoolsResponse>(
        "/api/v1/infrastructure/pools",
      );
      return response.data;
    },
    async getInfrastructureHealStatus(): Promise<InfrastructureHealStatus> {
      const response = await client.get<InfrastructureHealStatus>(
        "/api/v1/infrastructure/heal",
      );
      return response.data;
    },
  };
}
