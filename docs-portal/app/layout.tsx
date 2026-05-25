import type { Metadata } from "next";
import "./globals.css";
import LayoutWrapper from "@/components/LayoutWrapper";

export const metadata: Metadata = {
  title: "Sentinel Stack — Knowledge Portal",
  description:
    "Terminal-native architectural knowledge graph for the Sentinel Stack security framework. Deterministic, kernel-native defense from Ring -1 to Layer 7.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className="dark">
      <body className="bg-terminal-base text-terminal-fg antialiased">
        <LayoutWrapper>{children}</LayoutWrapper>
      </body>
    </html>
  );
}
