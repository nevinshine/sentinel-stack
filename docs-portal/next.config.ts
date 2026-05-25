import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  output: "export",
  images: {
    unoptimized: true,
  },
  // Trailing slashes for static hosting compatibility
  trailingSlash: true,
  basePath: process.env.GITHUB_ACTIONS ? "/sentinel-stack" : "",
};

export default nextConfig;
