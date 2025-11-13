// API Response Types based on swagger.json

// Health Check
export interface HealthResponse {
  status: string;
  service: string;
  timestamp: number;
  checks: {
    [key: string]: string;
  };
}

export interface ReadyResponse {
  ready: boolean;
  service: string;
}

export interface LiveResponse {
  alive: boolean;
}

export interface VersionResponse {
  version: string;
  api_version: string;
  build_date: string;
  build_time: string;
  compiler: string;
}

// Stats types
export interface SystemStats {
  buckets: number;
  objects: number;
  users: number;
  storage_used: number;
  storage_total: number;
  storage_available: number;
  status: string;
  timestamp: number;
}

export interface CapacityStats {
  total: number;
  used: number;
  available: number;
  usage_percent: number;
}

export interface BucketActivity {
  name: string;
  objects: number;
  size: number;
  timestamp: number;
}

export interface ActivityStats {
  recent_buckets: BucketActivity[];
  total_activity: number;
}

// Authentication
export interface LoginRequest {
  accessKey: string;
  secretKey: string;
}

export interface LoginResponse {
  token: string;
  expires_at: string;
}

export interface SessionResponse {
  authenticated: boolean;
  username: string;
  expires_at: string;
}

// Buckets
export interface Bucket {
  name: string;
  creation_date: string;
  size?: number;
  objects_count?: number;
  region?: string;
  versioning?: boolean;
}

export interface ListBucketsResponse {
  buckets: Bucket[];
  total?: number;
}

export interface CreateBucketRequest {
  name: string;
  region?: string;
  versioning?: boolean;
  object_locking?: boolean;
}

export interface BucketInfo {
  name: string;
  creation_date: string;
  size?: number;
  objects_count?: number;
  versioning_enabled?: boolean;
  versioning?: boolean;
  object_locking_enabled?: boolean;
  region?: string;
}

export interface BucketPolicy {
  policy: string;
}

export interface PolicyStatement {
  Sid?: string;
  Effect: "Allow" | "Deny";
  Principal: string | { [key: string]: string[] };
  Action: string | string[];
  Resource: string | string[];
  Condition?: { [key: string]: any };
}

// Objects
export interface S3Object {
  key: string;
  size: number;
  last_modified: string;
  etag: string;
  content_type: string;
  storage_class: string;
  owner?: {
    id: string;
    display_name: string;
  };
}

export interface ListObjectsResponse {
  objects: S3Object[];
  prefixes: string[];
  total: number;
  is_truncated: boolean;
  next_marker?: string;
}

export interface UploadObjectRequest {
  bucket: string;
  key: string;
  file: File;
}

export interface ObjectInfo {
  key: string;
  size: number;
  last_modified: string;
  etag: string;
  content_type: string;
  metadata: { [key: string]: string };
  tags: { [key: string]: string };
}

export interface PresignedUrlResponse {
  url: string;
  expires_at: string;
}

export interface CopyObjectRequest {
  source_bucket: string;
  source_key: string;
  destination_bucket: string;
  destination_key: string;
}

export interface ObjectTags {
  tags: { [key: string]: string };
}

// Users
export interface User {
  access_key: string;
  status: "enabled" | "disabled";
  policies: string[];
  groups: string[];
  created_at: string;
}

export interface ListUsersResponse {
  users: User[];
  total: number;
}

export interface CreateUserRequest {
  access_key: string;
  secret_key: string;
  policies?: string[];
  groups?: string[];
}

export interface UpdateUserRequest {
  status?: "enabled" | "disabled";
  policies?: string[];
  groups?: string[];
}

export interface UserPoliciesResponse {
  policies: string[];
}

// Error Response
export interface ApiError {
  code: number;
  message: string;
  detail?: string;
}

// Pagination params
export interface PaginationParams {
  limit?: number;
  offset?: number;
}

// List objects params
export interface ListObjectsParams extends PaginationParams {
  prefix?: string;
  delimiter?: string;
  marker?: string;
  max_keys?: number;
}
