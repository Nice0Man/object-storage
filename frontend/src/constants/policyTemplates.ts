export interface PolicyTemplate {
  id: string;
  name: string;
  descriptionKey: string;
  document: Record<string, unknown>;
}

const baseStatement = (effect: "Allow" | "Deny", actions: string[], resources: string[]) => ({
  Effect: effect,
  Action: actions,
  Resource: resources,
});

export const POLICY_TEMPLATES: PolicyTemplate[] = [
  {
    id: "readonly",
    name: "ReadOnlyAccess",
    descriptionKey: "users.policies.templates.readonly",
    document: {
      Version: "2012-10-17",
      Statement: [
        baseStatement("Allow", ["s3:GetObject", "s3:ListBucket"], [
          "arn:aws:s3:::*",
          "arn:aws:s3:::*/*",
        ]),
      ],
    },
  },
  {
    id: "readwrite",
    name: "ReadWriteAccess",
    descriptionKey: "users.policies.templates.readwrite",
    document: {
      Version: "2012-10-17",
      Statement: [
        baseStatement(
          "Allow",
          ["s3:GetObject", "s3:PutObject", "s3:DeleteObject", "s3:ListBucket"],
          ["arn:aws:s3:::*", "arn:aws:s3:::*/*"],
        ),
      ],
    },
  },
  {
    id: "list-only",
    name: "ListBucketsOnly",
    descriptionKey: "users.policies.templates.listOnly",
    document: {
      Version: "2012-10-17",
      Statement: [baseStatement("Allow", ["s3:ListBucket"], ["arn:aws:s3:::*"])],
    },
  },
  {
    id: "admin-bucket",
    name: "BucketAdmin",
    descriptionKey: "users.policies.templates.bucketAdmin",
    document: {
      Version: "2012-10-17",
      Statement: [
        baseStatement(
          "Allow",
          [
            "s3:CreateBucket",
            "s3:DeleteBucket",
            "s3:ListBucket",
            "s3:GetBucketPolicy",
            "s3:PutBucketPolicy",
          ],
          ["arn:aws:s3:::*"],
        ),
      ],
    },
  },
  {
    id: "deny-delete",
    name: "DenyDelete",
    descriptionKey: "users.policies.templates.denyDelete",
    document: {
      Version: "2012-10-17",
      Statement: [
        baseStatement("Allow", ["s3:GetObject", "s3:ListBucket"], [
          "arn:aws:s3:::*",
          "arn:aws:s3:::*/*",
        ]),
        baseStatement("Deny", ["s3:DeleteObject"], ["arn:aws:s3:::*/*"]),
      ],
    },
  },
];
