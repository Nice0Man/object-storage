import type { AxiosInstance } from "axios";
import type {
  BucketInfo,
  BucketObjectLockConfig,
  BucketPolicy,
  BucketTagsResponse,
  BucketVisibilityResponse,
  CreateBucketRequest,
  LifecycleConfiguration,
  ListBucketsResponse,
  BucketEncryptionConfig,
} from "../../api/types";

/** Bucket domain API — visibility, versioning, tags, encryption, lifecycle, object-lock. */
export function createBucketsApi(client: AxiosInstance) {
  return {
    async listBuckets(): Promise<ListBucketsResponse> {
      const response = await client.get<ListBucketsResponse>("/api/v1/buckets");
      return response.data;
    },
    async createBucket(request: CreateBucketRequest): Promise<void> {
      await client.post("/api/v1/buckets", request);
    },
    async deleteBucket(bucketName: string): Promise<void> {
      await client.delete(`/api/v1/buckets/${bucketName}`);
    },
    async getBucketInfo(bucketName: string): Promise<BucketInfo> {
      const response = await client.get<BucketInfo>(
        `/api/v1/buckets/${bucketName}`,
      );
      return response.data;
    },
    async getBucketPolicy(bucketName: string): Promise<BucketPolicy> {
      const response = await client.get<BucketPolicy>(
        `/api/v1/buckets/${bucketName}/policy`,
      );
      return response.data;
    },
    async setBucketPolicy(
      bucketName: string,
      policy: BucketPolicy,
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/policy`, policy);
    },
    async getBucketVisibility(
      bucketName: string,
    ): Promise<BucketVisibilityResponse> {
      const response = await client.get<BucketVisibilityResponse>(
        `/api/v1/buckets/${bucketName}/visibility`,
      );
      return response.data;
    },
    async setBucketVisibility(
      bucketName: string,
      groups: string[],
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/visibility`, { groups });
    },
    async getBucketObjectLockConfig(
      bucketName: string,
    ): Promise<BucketObjectLockConfig> {
      const response = await client.get<BucketObjectLockConfig>(
        `/api/v1/buckets/${bucketName}/object-lock`,
      );
      return response.data;
    },
    async setBucketObjectLockConfig(
      bucketName: string,
      config: BucketObjectLockConfig,
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/object-lock`, config);
    },
    async setBucketVersioning(
      bucketName: string,
      enabled: boolean,
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/versioning`, { enabled });
    },
    async getBucketVersioning(
      bucketName: string,
    ): Promise<{ enabled: boolean }> {
      const response = await client.get<{ enabled: boolean }>(
        `/api/v1/buckets/${bucketName}/versioning`,
      );
      return response.data;
    },
    async getBucketTags(bucketName: string): Promise<BucketTagsResponse> {
      const response = await client.get<BucketTagsResponse>(
        `/api/v1/buckets/${bucketName}/tags`,
      );
      return response.data;
    },
    async setBucketTags(
      bucketName: string,
      tags: { [key: string]: string },
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/tags`, { tags });
    },
    async deleteBucketTags(bucketName: string): Promise<void> {
      await client.delete(`/api/v1/buckets/${bucketName}/tags`);
    },
    async getBucketEncryption(
      bucketName: string,
    ): Promise<BucketEncryptionConfig> {
      const response = await client.get<BucketEncryptionConfig>(
        `/api/v1/buckets/${bucketName}/encryption`,
      );
      return response.data;
    },
    async setBucketEncryption(
      bucketName: string,
      config: BucketEncryptionConfig,
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/encryption`, config);
    },
    async deleteBucketEncryption(bucketName: string): Promise<void> {
      await client.delete(`/api/v1/buckets/${bucketName}/encryption`);
    },
    async getBucketLifecycle(
      bucketName: string,
    ): Promise<LifecycleConfiguration> {
      const response = await client.get<LifecycleConfiguration>(
        `/api/v1/buckets/${bucketName}/lifecycle`,
      );
      return response.data;
    },
    async setBucketLifecycle(
      bucketName: string,
      config: LifecycleConfiguration,
    ): Promise<void> {
      await client.put(`/api/v1/buckets/${bucketName}/lifecycle`, config);
    },
    async deleteBucketLifecycle(bucketName: string): Promise<void> {
      await client.delete(`/api/v1/buckets/${bucketName}/lifecycle`);
    },
  };
}
