import type { Metadata } from "next";
import "./globals.css";
import Sidebar from "@/components/Sidebar";
import SearchOmnibox from "@/components/SearchOmnibox";

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
        <Sidebar />
        <div className="ml-64 min-h-screen flex flex-col">
          {/* Top Bar */}
          <header className="sticky top-0 z-20 bg-terminal-base/80 backdrop-blur-md border-b border-terminal-border">
            <div className="flex items-center gap-4 px-6 py-3">
              <SearchOmnibox />
              <div className="hidden md:flex items-center gap-3 ml-auto">
                <a
                  href="https://github.com/nevinshine/sentinel-stack"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="font-mono text-xs text-terminal-muted hover:text-terminal-fg transition-colors no-underline"
                >
                  GitHub
                </a>
                <span className="text-terminal-border">|</span>
                <span className="font-mono text-[10px] text-terminal-muted">
                  v1.0-rc1
                </span>
              </div>
            </div>
          </header>

          {/* Main Content */}
          <main className="flex-1">{children}</main>

          {/* Footer */}
          <footer className="border-t border-terminal-border px-6 py-4">
            <div className="flex items-center justify-between font-mono text-[11px] text-terminal-muted">
              <span>
                Sentinel Stack Knowledge Portal
              </span>
              <span>
                Ring -1 to Layer 7 — Every layer is a perimeter.
              </span>
            </div>
          </footer>
        </div>
      </body>
    </html>
  );
}
