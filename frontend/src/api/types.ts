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

// Server and Drive Stats
export interface ServerStatus {
  id: string;
  name: string;
  status: "online" | "offline";
  endpoint: string;
  uptime?: number;
}

export interface DriveStatus {
  id: string;
  path: string;
  status: "online" | "offline";
  capacity: number;
  used: number;
  available: number;
}

export interface ServerStats {
  servers: ServerStatus[];
  online_count: number;
  offline_count: number;
  total_count: number;
}

export interface DriveStats {
  drives: DriveStatus[];
  online_count: number;
  offline_count: number;
  total_count: number;
}

// Pool Stats
export interface PoolInfo {
  id: string;
  name: string;
  capacity: number;
  available: number;
  used: number;
  drives_count: number;
  online_drives: number;
  offline_drives: number;
}

// API Error Stats (24 hours)
export interface ApiErrorData {
  timestamp: number;
  time: string;
  requests: number;      // Total requests in this period
  count?: number;        // Deprecated, use requests
  error_4xx: number;
  error_5xx: number;
}

export interface ApiErrorStats {
  data: ApiErrorData[];
  total_errors: number;
  error_rate: number;
}

// Data Throughput Stats (24 hours)
export interface DataThroughputData {
  timestamp: number;
  time: string;
  read_bytes: number;
  write_bytes: number;
  total_bytes: number;
  read_mbps?: number;
  write_mbps?: number;
}

export interface DataThroughputStats {
  data: DataThroughputData[];
  total_read: number;
  total_write: number;
  average_throughput: number;
  peak_throughput: number;
}

// Encryption Stats
export interface EncryptionStats {
  total_objects: number;
  encrypted_objects: number;
  unencrypted_objects: number;
  sse_s3_count: number;
  sse_c_count: number;
  encrypted_size: number;
  unencrypted_size: number;
  encryption_percentage: number;
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
  access_key?: string;
  is_admin?: boolean;
  role?: "viewer" | "editor" | "admin" | string;
  groups?: string[];
  policies?: string[];
  expires_at: string;
}

/** GET /api/v1/auth/me */
export interface CurrentUserResponse {
  username: string;
  access_key: string;
  is_admin: boolean;
  role?: string;
  groups?: string[];
  policies?: string[];
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
  // Encryption fields
  encrypted?: boolean;
  encryption_algorithm?: string;
  sse_type?: string;
  sse_customer_key_md5?: string;
  original_size?: number;
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
  original_size?: number;
  last_modified: string;
  etag: string;
  content_type: string;
  metadata: { [key: string]: string };
  tags: { [key: string]: string };
  // Server-Side Encryption fields
  encrypted?: boolean;
  encryption_algorithm?: string;
  sse_type?: "SSE-S3" | "SSE-C";
  sse_customer_key_md5?: string;
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

// ============================================================================
// Multipart Upload Types
// ============================================================================

export interface MultipartUploadInfo {
  upload_id: string;
  bucket: string;
  key: string;
  content_type: string;
  initiated: number;
  parts: UploadPart[];
}

export interface UploadPart {
  part_number: number;
  etag: string;
  size: number;
}

export interface CompletedPart {
  part_number: number;
  etag: string;
}

export interface InitiateMultipartUploadRequest {
  key: string;
  content_type?: string;
  metadata?: { [key: string]: string };
}

export interface CompleteMultipartUploadRequest {
  key: string;
  parts: CompletedPart[];
}

export interface ListMultipartUploadsResponse {
  uploads: MultipartUploadInfo[];
  bucket: string;
  prefix: string;
}

// ============================================================================
// Object Versioning Types
// ============================================================================

export interface ObjectVersion {
  version_id: string;
  key: string;
  bucket: string;
  size: number;
  last_modified: number;
  etag: string;
  is_latest: boolean;
  is_delete_marker: boolean;
}

export interface ListObjectVersionsResponse {
  versions: ObjectVersion[];
  bucket: string;
  key: string;
}

// ============================================================================
// Object Lock and Retention Types
// ============================================================================

export interface ObjectRetention {
  mode: 'GOVERNANCE' | 'COMPLIANCE';
  retain_until_date: number;
}

export interface ObjectLegalHold {
  status: boolean;
}

export interface BucketObjectLockConfig {
  object_lock_enabled: boolean;
  default_retention?: {
    mode: string;
    days?: number;
    years?: number;
  };
}

// ============================================================================
// Bucket Configuration Types
// ============================================================================

export interface BucketTagsResponse {
  tags: { [key: string]: string };
}

export interface BucketEncryptionConfig {
  enabled: boolean;
  algorithm: 'AES256' | 'aws:kms';
  kms_master_key_id?: string;
}

export interface LifecycleRule {
  id: string;
  status: 'Enabled' | 'Disabled';
  filter: {
    prefix?: string;
    tags?: { [key: string]: string };
    object_size_greater_than?: number;
    object_size_less_than?: number;
  };
  actions: LifecycleAction[];
}

export interface LifecycleAction {
  type: 'Expiration' | 'AbortIncompleteMultipartUpload' | 'NoncurrentVersionExpiration' | 'Transition';
  days?: number;
  noncurrent_days?: number;
  storage_class?: string;
}

export interface LifecycleConfiguration {
  rules: LifecycleRule[];
}

// ============================================================================
// Dashboard Types
// ============================================================================

export type WidgetType =
  | 'capacity'
  | 'servers'
  | 'drives'
  | 'buckets'
  | 'api_errors'
  | 'throughput'
  | 'encryption'
  | 'pools'
  | 'quick_actions';

export interface DashboardBoard {
  id: string;
  name: string;
  order_index: number;
  created_at?: number;
  updated_at?: number;
}

export interface WidgetPosition {
  x: number;
  y: number;
}

export interface WidgetSize {
  w: number;
  h: number;
}

export interface WidgetSettings {
  timeRange?: '1h' | '6h' | '24h' | '7d';
  chartMode?: 'stacked' | 'line' | 'bar';
  showLegend?: boolean;
  [key: string]: unknown;
}

export interface DashboardWidget {
  id: string;
  board_id: string;
  widget_type: WidgetType;
  position: WidgetPosition;
  size: WidgetSize;
  visible: boolean;
  refresh_interval: number; // seconds, 0 = disabled
  settings: WidgetSettings;
  order_index: number;
}

export interface CreateBoardRequest {
  name: string;
  order_index?: number;
}

export interface UpdateBoardRequest {
  name?: string;
  order_index?: number;
}

export interface SaveWidgetsRequest {
  widgets: DashboardWidget[];
}

export interface UpdateWidgetRequest {
  position?: WidgetPosition;
  size?: WidgetSize;
  visible?: boolean;
  refresh_interval?: number;
  settings?: WidgetSettings;
  order_index?: number;
}
