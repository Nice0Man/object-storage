export type Capability =
  | "dashboard.view"
  | "buckets.view"
  | "buckets.manage"
  | "objects.view"
  | "objects.manage"
  | "users.view"
  | "users.manage"
  | "system.view"
  | "profile.view";

export type CapabilityMap = Record<Capability, boolean>;

const ALL_CAPABILITIES: Capability[] = [
  "dashboard.view",
  "buckets.view",
  "buckets.manage",
  "objects.view",
  "objects.manage",
  "users.view",
  "users.manage",
  "system.view",
  "profile.view",
];

function emptyCapabilities(): CapabilityMap {
  return ALL_CAPABILITIES.reduce(
    (acc, capability) => {
      acc[capability] = false;
      return acc;
    },
    {} as CapabilityMap,
  );
}

function applyRoleBaseline(map: CapabilityMap, role?: string, isAdmin?: boolean): void {
  if (isAdmin || role === "admin") {
    ALL_CAPABILITIES.forEach((capability) => {
      map[capability] = true;
    });
    return;
  }

  map["dashboard.view"] = true;
  map["buckets.view"] = true;
  map["objects.view"] = true;
  map["profile.view"] = true;

  if (role === "editor") {
    map["buckets.manage"] = true;
    map["objects.manage"] = true;
    map["system.view"] = true;
  }
}

function applyPolicyOverrides(map: CapabilityMap, policies: string[] = []): void {
  for (const policy of policies) {
    // Lightweight conventional overrides:
    // - allow:<capability>
    // - deny:<capability>
    if (policy.startsWith("allow:")) {
      const capability = policy.slice("allow:".length) as Capability;
      if (ALL_CAPABILITIES.includes(capability)) {
        map[capability] = true;
      }
    }
    if (policy.startsWith("deny:")) {
      const capability = policy.slice("deny:".length) as Capability;
      if (ALL_CAPABILITIES.includes(capability)) {
        map[capability] = false;
      }
    }
    if (policy === "readonly") {
      map["dashboard.view"] = true;
      map["buckets.view"] = true;
      map["objects.view"] = true;
    }
    if (policy === "readwrite") {
      map["dashboard.view"] = true;
      map["buckets.view"] = true;
      map["buckets.manage"] = true;
      map["objects.view"] = true;
      map["objects.manage"] = true;
    }
  }
}

export function buildCapabilities(params: {
  role?: string;
  isAdmin?: boolean;
  policies?: string[];
}): CapabilityMap {
  const map = emptyCapabilities();
  applyRoleBaseline(map, params.role, params.isAdmin);
  applyPolicyOverrides(map, params.policies);
  return map;
}

export function can(capabilities: CapabilityMap, capability: Capability): boolean {
  return Boolean(capabilities[capability]);
}
