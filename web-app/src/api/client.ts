import axios, { AxiosInstance, AxiosError, AxiosRequestConfig } from "axios";
import type {
  HealthResponse,
  ReadyResponse,
  LiveResponse,
  VersionResponse,
  SystemStats,
  CapacityStats,
  ActivityStats,
  LoginRequest,
  LoginResponse,
  SessionResponse,
  ListBucketsResponse,
  CreateBucketRequest,
  BucketInfo,
  BucketPolicy,
  ListObjectsResponse,
  ListObjectsParams,
  ObjectInfo,
  PresignedUrlResponse,
  CopyObjectRequest,
  ObjectTags,
  ListUsersResponse,
  CreateUserRequest,
  UpdateUserRequest,
  UserPoliciesResponse,
  ApiError,
} from "./types";

class ApiClient {
  private client: AxiosInstance;
  private token: string | null = null;

  constructor(baseURL: string = "http://localhost:9090") {
    this.client = axios.create({
      baseURL,
      headers: {
        "Content-Type": "application/json",
      },
      // withCredentials: false by default - we use JWT in localStorage, not cookies
    });

    // Load token from localStorage
    this.token = localStorage.getItem("auth_token");
    if (this.token) {
      this.setAuthToken(this.token);
    }

    // Response interceptor for error handling
    this.client.interceptors.response.use(
      (response) => response,
      (error: AxiosError<ApiError>) => {
        if (error.response?.status === 401) {
          // Clear token and redirect to login
          this.clearAuthToken();
          window.location.href = "/login";
        }
        return Promise.reject(error);
      },
    );
  }

  private setAuthToken(token: string) {
    this.token = token;
    this.client.defaults.headers.common["Authorization"] = `Bearer ${token}`;
    localStorage.setItem("auth_token", token);
  }

  private clearAuthToken() {
    this.token = null;
    delete this.client.defaults.headers.common["Authorization"];
    localStorage.removeItem("auth_token");
  }

  // Health Check APIs
  async getHealth(): Promise<HealthResponse> {
    const response = await this.client.get<HealthResponse>("/api/v1/health");
    return response.data;
  }

  async getReady(): Promise<ReadyResponse> {
    const response = await this.client.get<ReadyResponse>("/api/v1/ready");
    return response.data;
  }

  async getLive(): Promise<LiveResponse> {
    const response = await this.client.get<LiveResponse>("/api/v1/live");
    return response.data;
  }

  async getVersion(): Promise<VersionResponse> {
    const response = await this.client.get<VersionResponse>("/api/v1/version");
    return response.data;
  }

  // Authentication APIs
  async login(credentials: LoginRequest): Promise<LoginResponse> {
    const response = await this.client.post<LoginResponse>(
      "/api/v1/auth/login",
      credentials,
    );
    if (response.data.token) {
      this.setAuthToken(response.data.token);
    }
    return response.data;
  }

  async logout(): Promise<void> {
    await this.client.post("/api/v1/auth/logout");
    this.clearAuthToken();
  }

  async refreshToken(): Promise<LoginResponse> {
    const response = await this.client.post<LoginResponse>(
      "/api/v1/auth/refresh",
    );
    if (response.data.token) {
      this.setAuthToken(response.data.token);
    }
    return response.data;
  }

  async getSession(): Promise<SessionResponse> {
    const response = await this.client.get<SessionResponse>(
      "/api/v1/auth/session",
    );
    return response.data;
  }

  async getCurrentUser(): Promise<any> {
    const response = await this.client.get("/api/v1/auth/me");
    return response.data;
  }

  async changePassword(request: {
    old_password: string;
    new_password: string;
  }): Promise<void> {
    await this.client.put("/api/v1/auth/password", request);
  }

  // Buckets APIs
  async listBuckets(): Promise<ListBucketsResponse> {
    const response =
      await this.client.get<ListBucketsResponse>("/api/v1/buckets");
    return response.data;
  }

  async createBucket(request: CreateBucketRequest): Promise<void> {
    await this.client.post("/api/v1/buckets", request);
  }

  async deleteBucket(bucketName: string): Promise<void> {
    await this.client.delete(`/api/v1/buckets/${bucketName}`);
  }

  async getBucketInfo(bucketName: string): Promise<BucketInfo> {
    const response = await this.client.get<BucketInfo>(
      `/api/v1/buckets/${bucketName}`,
    );
    return response.data;
  }

  async getBucketPolicy(bucketName: string): Promise<BucketPolicy> {
    const response = await this.client.get<BucketPolicy>(
      `/api/v1/buckets/${bucketName}/policy`,
    );
    return response.data;
  }

  async setBucketPolicy(
    bucketName: string,
    policy: BucketPolicy,
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/policy`, policy);
  }

  // Objects APIs
  async listObjects(
    bucketName: string,
    params?: ListObjectsParams,
  ): Promise<ListObjectsResponse> {
    const response = await this.client.get<ListObjectsResponse>(
      `/api/v1/buckets/${bucketName}/objects`,
      { params },
    );
    return response.data;
  }

  async uploadObject(
    bucketName: string,
    key: string,
    file: File,
    onProgress?: (progress: number) => void,
  ): Promise<void> {
    // Read file as ArrayBuffer
    const fileBuffer = await file.arrayBuffer();

    const config: AxiosRequestConfig = {
      headers: {
        "Content-Type": file.type || "application/octet-stream",
      },
      params: {
        key: key, // Send key as query parameter
      },
    };

    if (onProgress) {
      config.onUploadProgress = (progressEvent) => {
        if (progressEvent.total) {
          const progress = Math.round(
            (progressEvent.loaded * 100) / progressEvent.total,
          );
          onProgress(progress);
        }
      };
    }

    await this.client.post(
      `/api/v1/buckets/${bucketName}/objects`,
      fileBuffer,
      config,
    );
  }

  async deleteObject(bucketName: string, key: string): Promise<void> {
    await this.client.delete(`/api/v1/buckets/${bucketName}/objects/${key}`);
  }

  async batchDeleteObjects(bucketName: string, keys: string[]): Promise<void> {
    await this.client.post(
      `/api/v1/buckets/${bucketName}/objects/batch-delete`,
      { keys },
    );
  }

  async getObjectInfo(bucketName: string, key: string): Promise<ObjectInfo> {
    const response = await this.client.get<ObjectInfo>(
      `/api/v1/buckets/${bucketName}/objects/${key}/info`,
    );
    return response.data;
  }

  async downloadObject(bucketName: string, key: string): Promise<Blob> {
    const response = await this.client.get(
      `/api/v1/buckets/${bucketName}/objects/${key}/download`,
      {
        responseType: "blob",
      },
    );
    return response.data;
  }

  async copyObject(request: CopyObjectRequest): Promise<void> {
    await this.client.post(
      `/api/v1/buckets/${request.destination_bucket}/objects/${request.destination_key}/copy`,
      {
        source_bucket: request.source_bucket,
        source_key: request.source_key,
      },
    );
  }

  async getPresignedUrl(
    bucketName: string,
    key: string,
    expiresIn: number = 3600,
  ): Promise<PresignedUrlResponse> {
    const response = await this.client.get<PresignedUrlResponse>(
      `/api/v1/buckets/${bucketName}/objects/${key}/presigned-url`,
      { params: { expires_in: expiresIn } },
    );
    return response.data;
  }

  async getObjectTags(bucketName: string, key: string): Promise<ObjectTags> {
    const response = await this.client.get<ObjectTags>(
      `/api/v1/buckets/${bucketName}/objects/${key}/tags`,
    );
    return response.data;
  }

  async setObjectTags(
    bucketName: string,
    key: string,
    tags: { [key: string]: string },
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/objects/${key}/tags`, {
      tags,
    });
  }

  // Users APIs
  async listUsers(): Promise<ListUsersResponse> {
    const response = await this.client.get<ListUsersResponse>("/api/v1/users");
    return response.data;
  }

  async createUser(request: CreateUserRequest): Promise<void> {
    await this.client.post("/api/v1/users", request);
  }

  async deleteUser(accessKey: string): Promise<void> {
    await this.client.delete(`/api/v1/users/${accessKey}`);
  }

  async updateUser(
    accessKey: string,
    request: UpdateUserRequest,
  ): Promise<void> {
    await this.client.put(`/api/v1/users/${accessKey}`, request);
  }

  async getUserPolicies(accessKey: string): Promise<UserPoliciesResponse> {
    const response = await this.client.get<UserPoliciesResponse>(
      `/api/v1/users/${accessKey}/policies`,
    );
    return response.data;
  }

  async attachUserPolicy(accessKey: string, policyName: string): Promise<void> {
    await this.client.put(`/api/v1/users/${accessKey}/policies/${policyName}`);
  }

  async assignUserPolicy(
    accessKey: string,
    policyName: string,
    policy: { policy: string },
  ): Promise<void> {
    await this.client.put(
      `/api/v1/users/${accessKey}/policies/${policyName}`,
      policy,
    );
  }

  async detachUserPolicy(accessKey: string, policyName: string): Promise<void> {
    await this.client.delete(
      `/api/v1/users/${accessKey}/policies/${policyName}`,
    );
  }

  async removeUserPolicy(accessKey: string, policyName: string): Promise<void> {
    await this.client.delete(
      `/api/v1/users/${accessKey}/policies/${policyName}`,
    );
  }

  async addUserToGroup(accessKey: string, groupName: string): Promise<void> {
    await this.client.put(`/api/v1/users/${accessKey}/groups/${groupName}`);
  }

  async removeUserFromGroup(
    accessKey: string,
    groupName: string,
  ): Promise<void> {
    await this.client.delete(`/api/v1/users/${accessKey}/groups/${groupName}`);
  }

  // ==================== Stats ====================
  async getSystemStats(): Promise<SystemStats> {
    const response = await this.client.get<SystemStats>("/api/v1/stats/system");
    return response.data;
  }

  async getCapacityStats(): Promise<CapacityStats> {
    const response = await this.client.get<CapacityStats>("/api/v1/stats/capacity");
    return response.data;
  }

  async getActivityStats(): Promise<ActivityStats> {
    const response = await this.client.get<ActivityStats>("/api/v1/stats/activity");
    return response.data;
  }
}

// Export singleton instance
export const apiClient = new ApiClient();
export default apiClient;
