/** API base URL from Vite env (empty = same origin). */
export function getApiUrlFromEnv(): string | undefined {
  const value = import.meta.env.VITE_API_URL;
  if (typeof value === "string" && value.trim().length > 0) {
    return value.trim();
  }
  return undefined;
}
