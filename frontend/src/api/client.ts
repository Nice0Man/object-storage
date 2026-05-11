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
  MultipartUploadInfo,
  CompletedPart,
  ListMultipartUploadsResponse,
  ListObjectVersionsResponse,
  ObjectRetention,
  ObjectLegalHold,
  BucketObjectLockConfig,
  BucketTagsResponse,
  BucketEncryptionConfig,
  LifecycleConfiguration,
  EncryptionStats,
  DashboardBoard,
  DashboardWidget,
  CreateBoardRequest,
  UpdateBoardRequest,
  UpdateWidgetRequest,
} from "./types";

function resolveApiBaseURL(explicit?: string): string {
  if (explicit !== undefined && explicit.length > 0) {
    return explicit;
  }
  const env = process.env.REACT_APP_API_URL;
  if (typeof env === "string" && env.trim().length > 0) {
    return env.trim();
  }
  // Same origin: dev server (CRA) proxies /api → backend; production SPA is served from the API host.
  return "";
}

class ApiClient {
  private client: AxiosInstance;
  private token: string | null = null;

  constructor(baseURL?: string) {
    const resolved = resolveApiBaseURL(baseURL);
    this.client = axios.create({
      baseURL: resolved,
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
    fileOrData: File | Uint8Array,
    contentType?: string,
    sseCustomerKey?: string,
    onProgress?: (progress: number) => void,
  ): Promise<void> {
    // Handle both File and Uint8Array
    let data: ArrayBuffer;
    let mimeType: string;

    if (fileOrData instanceof File) {
      data = await fileOrData.arrayBuffer();
      mimeType = contentType || fileOrData.type || "application/octet-stream";
    } else {
      data = fileOrData.buffer as ArrayBuffer;
      mimeType = contentType || "application/octet-stream";
    }

    const headers: Record<string, string> = {
      "Content-Type": mimeType,
    };

    // Add SSE-C header if customer key provided
    if (sseCustomerKey) {
      headers["x-amz-server-side-encryption-customer-key"] = sseCustomerKey;
    }

    const config: AxiosRequestConfig = {
      headers,
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
      data,
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

  async downloadObject(bucketName: string, key: string, sseCustomerKey?: string): Promise<Blob> {
    const headers: Record<string, string> = {};

    // Add SSE-C header if customer key provided
    if (sseCustomerKey) {
      headers["x-amz-server-side-encryption-customer-key"] = sseCustomerKey;
    }

    const response = await this.client.get(
      `/api/v1/buckets/${bucketName}/objects/${key}/download`,
      {
        responseType: "blob",
        headers,
      },
    );
    return response.data;
  }

  async copyObject(request: CopyObjectRequest): Promise<void> {
    await this.client.post(
      `/api/v1/buckets/${request.source_bucket}/objects/${encodeURIComponent(request.source_key)}/copy`,
      {
        destination_bucket: request.destination_bucket,
        destination_key: request.destination_key,
      },
    );
  }

  async getPresignedUrl(
    bucketName: string,
    key: string,
    expiresIn: number = 3600,
  ): Promise<PresignedUrlResponse> {
    const response = await this.client.get<PresignedUrlResponse>(
      `/api/v1/buckets/${bucketName}/objects/${encodeURIComponent(key)}/presigned-url`,
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

  // ==================== Multipart Upload ====================
  async initiateMultipartUpload(
    bucketName: string,
    key: string,
    contentType: string = "application/octet-stream",
    metadata: { [key: string]: string } = {},
  ): Promise<MultipartUploadInfo> {
    const response = await this.client.post<MultipartUploadInfo>(
      `/api/v1/buckets/${bucketName}/uploads`,
      { key, content_type: contentType, metadata },
    );
    return response.data;
  }

  async uploadPart(
    bucketName: string,
    uploadId: string,
    partNumber: number,
    key: string,
    data: ArrayBuffer,
    onProgress?: (progress: number) => void,
  ): Promise<string> {
    const config: AxiosRequestConfig = {
      headers: { "Content-Type": "application/octet-stream" },
      params: { key },
    };
    if (onProgress) {
      config.onUploadProgress = (e) => {
        if (e.total) onProgress(Math.round((e.loaded * 100) / e.total));
      };
    }
    const response = await this.client.put(
      `/api/v1/buckets/${bucketName}/uploads/${uploadId}/parts/${partNumber}`,
      data,
      config,
    );
    return response.data.etag;
  }

  async completeMultipartUpload(
    bucketName: string,
    uploadId: string,
    key: string,
    parts: CompletedPart[],
  ): Promise<ObjectInfo> {
    const response = await this.client.post<ObjectInfo>(
      `/api/v1/buckets/${bucketName}/uploads/${uploadId}/complete`,
      { key, parts },
    );
    return response.data;
  }

  async abortMultipartUpload(
    bucketName: string,
    uploadId: string,
    key: string,
  ): Promise<void> {
    await this.client.delete(
      `/api/v1/buckets/${bucketName}/uploads/${uploadId}`,
      { params: { key } },
    );
  }

  async listMultipartUploads(
    bucketName: string,
    prefix: string = "",
  ): Promise<ListMultipartUploadsResponse> {
    const response = await this.client.get<ListMultipartUploadsResponse>(
      `/api/v1/buckets/${bucketName}/uploads`,
      { params: { prefix } },
    );
    return response.data;
  }

  async listParts(
    bucketName: string,
    uploadId: string,
    key: string,
  ): Promise<MultipartUploadInfo> {
    const response = await this.client.get<MultipartUploadInfo>(
      `/api/v1/buckets/${bucketName}/uploads/${uploadId}/parts`,
      { params: { key } },
    );
    return response.data;
  }

  // Advanced multipart upload with automatic chunking
  async uploadLargeFile(
    bucketName: string,
    key: string,
    file: File,
    onProgress?: (progress: number) => void,
    chunkSize: number = 10 * 1024 * 1024, // 10MB chunks
  ): Promise<void> {
    const totalSize = file.size;
    const numParts = Math.ceil(totalSize / chunkSize);

    // Initiate multipart upload
    const uploadInfo = await this.initiateMultipartUpload(
      bucketName,
      key,
      file.type || "application/octet-stream",
    );

    const completedParts: CompletedPart[] = [];
    let uploadedBytes = 0;

    try {
      for (let i = 0; i < numParts; i++) {
        const start = i * chunkSize;
        const end = Math.min(start + chunkSize, totalSize);
        const chunk = file.slice(start, end);
        const chunkBuffer = await chunk.arrayBuffer();

        const etag = await this.uploadPart(
          bucketName,
          uploadInfo.upload_id,
          i + 1,
          key,
          chunkBuffer,
        );

        completedParts.push({ part_number: i + 1, etag });
        uploadedBytes += (end - start);

        if (onProgress) {
          onProgress(Math.round((uploadedBytes * 100) / totalSize));
        }
      }

      // Complete the upload
      await this.completeMultipartUpload(
        bucketName,
        uploadInfo.upload_id,
        key,
        completedParts,
      );
    } catch (error) {
      // Abort on failure
      await this.abortMultipartUpload(bucketName, uploadInfo.upload_id, key);
      throw error;
    }
  }

  // ==================== Object Versioning ====================
  async listObjectVersions(
    bucketName: string,
    key: string,
  ): Promise<ListObjectVersionsResponse> {
    const response = await this.client.get<ListObjectVersionsResponse>(
      `/api/v1/buckets/${bucketName}/objects/${key}/versions`,
    );
    return response.data;
  }

  async getObjectVersion(
    bucketName: string,
    key: string,
    versionId: string,
  ): Promise<Blob> {
    const response = await this.client.get(
      `/api/v1/buckets/${bucketName}/objects/${key}/versions/${versionId}`,
      { responseType: "blob" },
    );
    return response.data;
  }

  async deleteObjectVersion(
    bucketName: string,
    key: string,
    versionId: string,
  ): Promise<void> {
    await this.client.delete(
      `/api/v1/buckets/${bucketName}/objects/${key}/versions/${versionId}`,
    );
  }

  async restoreObjectVersion(
    bucketName: string,
    key: string,
    versionId: string,
  ): Promise<ObjectInfo> {
    const response = await this.client.post<ObjectInfo>(
      `/api/v1/buckets/${bucketName}/objects/${key}/versions/${versionId}/restore`,
    );
    return response.data;
  }

  // ==================== Bucket Versioning ====================
  async setBucketVersioning(
    bucketName: string,
    enabled: boolean,
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/versioning`, {
      enabled,
    });
  }

  async getBucketVersioning(bucketName: string): Promise<{ enabled: boolean }> {
    const response = await this.client.get(
      `/api/v1/buckets/${bucketName}/versioning`,
    );
    return response.data;
  }

  // ==================== Bucket Tags ====================
  async getBucketTags(bucketName: string): Promise<BucketTagsResponse> {
    const response = await this.client.get<BucketTagsResponse>(
      `/api/v1/buckets/${bucketName}/tags`,
    );
    return response.data;
  }

  async setBucketTags(
    bucketName: string,
    tags: { [key: string]: string },
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/tags`, { tags });
  }

  async deleteBucketTags(bucketName: string): Promise<void> {
    await this.client.delete(`/api/v1/buckets/${bucketName}/tags`);
  }

  // ==================== Bucket Encryption ====================
  async getBucketEncryption(
    bucketName: string,
  ): Promise<BucketEncryptionConfig> {
    const response = await this.client.get<BucketEncryptionConfig>(
      `/api/v1/buckets/${bucketName}/encryption`,
    );
    return response.data;
  }

  async setBucketEncryption(
    bucketName: string,
    config: BucketEncryptionConfig,
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/encryption`, config);
  }

  async deleteBucketEncryption(bucketName: string): Promise<void> {
    await this.client.delete(`/api/v1/buckets/${bucketName}/encryption`);
  }

  // ==================== Bucket Lifecycle ====================
  async getBucketLifecycle(
    bucketName: string,
  ): Promise<LifecycleConfiguration> {
    const response = await this.client.get<LifecycleConfiguration>(
      `/api/v1/buckets/${bucketName}/lifecycle`,
    );
    return response.data;
  }

  async setBucketLifecycle(
    bucketName: string,
    config: LifecycleConfiguration,
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/lifecycle`, config);
  }

  async deleteBucketLifecycle(bucketName: string): Promise<void> {
    await this.client.delete(`/api/v1/buckets/${bucketName}/lifecycle`);
  }

  // ==================== Object Lock ====================
  async getBucketObjectLockConfig(
    bucketName: string,
  ): Promise<BucketObjectLockConfig> {
    const response = await this.client.get<BucketObjectLockConfig>(
      `/api/v1/buckets/${bucketName}/object-lock`,
    );
    return response.data;
  }

  async setBucketObjectLockConfig(
    bucketName: string,
    config: BucketObjectLockConfig,
  ): Promise<void> {
    await this.client.put(`/api/v1/buckets/${bucketName}/object-lock`, config);
  }

  async getObjectRetention(
    bucketName: string,
    key: string,
  ): Promise<ObjectRetention> {
    const response = await this.client.get<ObjectRetention>(
      `/api/v1/buckets/${bucketName}/objects/${key}/retention`,
    );
    return response.data;
  }

  async setObjectRetention(
    bucketName: string,
    key: string,
    retention: ObjectRetention,
  ): Promise<void> {
    await this.client.put(
      `/api/v1/buckets/${bucketName}/objects/${key}/retention`,
      retention,
    );
  }

  async getObjectLegalHold(
    bucketName: string,
    key: string,
  ): Promise<ObjectLegalHold> {
    const response = await this.client.get<ObjectLegalHold>(
      `/api/v1/buckets/${bucketName}/objects/${key}/legal-hold`,
    );
    return response.data;
  }

  async setObjectLegalHold(
    bucketName: string,
    key: string,
    enabled: boolean,
  ): Promise<void> {
    await this.client.put(
      `/api/v1/buckets/${bucketName}/objects/${key}/legal-hold`,
      { status: enabled },
    );
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

  async getServerStats(): Promise<any> {
    const response = await this.client.get("/api/v1/stats/servers");
    return response.data;
  }

  async getDriveStats(): Promise<any> {
    const response = await this.client.get("/api/v1/stats/drives");
    return response.data;
  }

  async getPoolStats(): Promise<any> {
    const response = await this.client.get("/api/v1/stats/pools");
    return response.data;
  }

  async getApiErrorStats(): Promise<any> {
    const response = await this.client.get("/api/v1/stats/api-errors");
    return response.data;
  }

  async getDataThroughputStats(): Promise<any> {
    const response = await this.client.get("/api/v1/stats/data-throughput");
    return response.data;
  }

  async getEncryptionStats(): Promise<EncryptionStats> {
    const response = await this.client.get("/api/v1/stats/encryption");
    return response.data;
  }

  // ==================== Dashboard ====================
  async listDashboardBoards(): Promise<DashboardBoard[]> {
    const response = await this.client.get<DashboardBoard[]>("/api/v1/dashboard/boards");
    return response.data;
  }

  async createDashboardBoard(request: CreateBoardRequest): Promise<DashboardBoard> {
    const response = await this.client.post<DashboardBoard>("/api/v1/dashboard/boards", request);
    return response.data;
  }

  async getDashboardBoard(id: string): Promise<DashboardBoard> {
    const response = await this.client.get<DashboardBoard>(`/api/v1/dashboard/boards/${id}`);
    return response.data;
  }

  async updateDashboardBoard(id: string, request: UpdateBoardRequest): Promise<DashboardBoard> {
    const response = await this.client.put<DashboardBoard>(`/api/v1/dashboard/boards/${id}`, request);
    return response.data;
  }

  async deleteDashboardBoard(id: string): Promise<void> {
    await this.client.delete(`/api/v1/dashboard/boards/${id}`);
  }

  async listDashboardWidgets(boardId: string): Promise<DashboardWidget[]> {
    const response = await this.client.get<DashboardWidget[]>(`/api/v1/dashboard/boards/${boardId}/widgets`);
    return response.data;
  }

  async saveDashboardWidgets(boardId: string, widgets: DashboardWidget[]): Promise<void> {
    await this.client.put(`/api/v1/dashboard/boards/${boardId}/widgets`, widgets);
  }

  async updateDashboardWidget(id: string, request: UpdateWidgetRequest): Promise<DashboardWidget> {
    const response = await this.client.put<DashboardWidget>(`/api/v1/dashboard/widgets/${id}`, request);
    return response.data;
  }
}

// Export singleton instance
export const apiClient = new ApiClient();
export default apiClient;
