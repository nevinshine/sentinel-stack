"use client";
import React, { useState } from "react";
import Sidebar from "./Sidebar";
import SearchOmnibox from "./SearchOmnibox";

export default function LayoutWrapper({ children }: { children: React.ReactNode }) {
  const [isSidebarOpen, setIsSidebarOpen] = useState(false);
  const [isDesktopSidebarOpen, setIsDesktopSidebarOpen] = useState(true);

  return (
    <>
      {/* Mobile Sidebar Overlay */}
      {isSidebarOpen && (
        <div 
          className="fixed inset-0 bg-black/60 z-40 md:hidden backdrop-blur-sm"
          onClick={() => setIsSidebarOpen(false)}
        />
      )}

      {/* Sidebar - responsive */}
      <div 
        className={`fixed inset-y-0 left-0 z-50 transform transition-transform duration-300 
          ${isSidebarOpen ? 'translate-x-0' : '-translate-x-full'} 
          ${isDesktopSidebarOpen ? 'md:translate-x-0' : 'md:-translate-x-full'}
        `}
      >
        <Sidebar onClose={() => setIsSidebarOpen(false)} />
      </div>

      <div 
        className={`min-h-screen flex flex-col transition-all duration-300 ${
          isDesktopSidebarOpen ? 'md:ml-64' : 'md:ml-0'
        }`}
      >
        {/* Top Bar */}
        <header className="sticky top-0 z-20 bg-terminal-base/90 backdrop-blur-md border-b border-terminal-border">
          <div className="flex items-center gap-3 px-4 md:px-6 py-3">
            <button 
              className="p-1.5 text-terminal-muted hover:text-terminal-fg rounded bg-terminal-surface border border-terminal-border transition-colors"
              onClick={() => {
                if (window.innerWidth < 768) {
                  setIsSidebarOpen(true);
                } else {
                  setIsDesktopSidebarOpen(!isDesktopSidebarOpen);
                }
              }}
              aria-label="Toggle Menu"
            >
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <line x1="3" y1="12" x2="21" y2="12"></line>
                <line x1="3" y1="6" x2="21" y2="6"></line>
                <line x1="3" y1="18" x2="21" y2="18"></line>
              </svg>
            </button>
            
            <SearchOmnibox />
            
            <div className="hidden md:flex items-center gap-3 ml-auto">
              <a href="https://github.com/nevinshine/sentinel-stack" target="_blank" rel="noopener noreferrer" className="font-mono text-xs text-terminal-muted hover:text-terminal-fg transition-colors no-underline">
                GitHub
              </a>
              <span className="text-terminal-border">|</span>
              <span className="font-mono text-[10px] text-terminal-muted">v1.0-rc1</span>
            </div>
          </div>
        </header>

        {/* Main Content */}
        <main className="flex-1 w-full max-w-[100vw] overflow-x-hidden">
          {children}
        </main>

        {/* Footer */}
        <footer className="border-t border-terminal-border px-4 md:px-6 py-4">
          <div className="flex flex-col md:flex-row items-center justify-between font-mono text-[11px] text-terminal-muted gap-2">
            <span>Sentinel Stack Knowledge Portal</span>
            <span className="text-center md:text-right">Ring -1 to Layer 7 — Every layer is a perimeter.</span>
          </div>
        </footer>
      </div>
    </>
  );
}
