"use client";

import React, { useState, useCallback } from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { NavItem, navigationTree } from "@/lib/navigation";

function NavNode({ item, depth = 0, onClose }: { item: NavItem; depth?: number; onClose?: () => void }) {
  const pathname = usePathname();
  const [isOpen, setIsOpen] = useState(() => {
    // Auto-expand if current path matches any child
    if (!item.children) return false;
    const checkMatch = (items: NavItem[]): boolean =>
      items.some(
        (i) =>
          i.href === pathname ||
          (i.children && checkMatch(i.children))
      );
    return checkMatch(item.children);
  });

  const hasChildren = item.children && item.children.length > 0;
  const isActive = item.href === pathname;
  const paddingLeft = 12 + depth * 16;

  const toggle = useCallback(() => {
    if (hasChildren) setIsOpen((o) => !o);
  }, [hasChildren]);

  if (hasChildren) {
    return (
      <div>
        <button
          onClick={toggle}
          className="flex items-center w-full text-left group transition-colors duration-150"
          style={{ paddingLeft }}
          aria-expanded={isOpen}
        >
          <span
            className="mr-1.5 text-xs text-terminal-muted transition-transform duration-200 inline-block"
            style={{ transform: isOpen ? "rotate(90deg)" : "rotate(0deg)" }}
          >
            ▶
          </span>
          <span className="text-sm font-mono text-terminal-muted group-hover:text-terminal-fg transition-colors py-1">
            {item.icon && <span className="mr-1.5 text-xs">{item.icon}</span>}
            {item.title}
          </span>
          {item.tag && (
            <span
              className={`ml-auto mr-3 text-[10px] font-mono px-1.5 py-0.5 rounded ${
                item.tag === "architect"
                  ? "bg-terminal-purple/15 text-terminal-purple"
                  : item.tag === "engineer"
                  ? "bg-terminal-blue/15 text-terminal-blue"
                  : "bg-terminal-yellow/15 text-terminal-yellow"
              }`}
            >
              {item.tag.toUpperCase()}
            </span>
          )}
        </button>
        <div
          className="overflow-hidden transition-all duration-200"
          style={{
            maxHeight: isOpen ? `${item.children!.length * 200}px` : "0px",
            opacity: isOpen ? 1 : 0,
          }}
        >
          {item.children!.map((child, idx) => (
            <NavNode key={`${child.title}-${idx}`} item={child} depth={depth + 1} onClose={onClose} />
          ))}
        </div>
      </div>
    );
  }

  // Leaf node (link)
  return (
    <Link
      href={item.href || "#"}
      className={`flex items-center py-1 text-sm font-mono transition-all duration-150 group ${
        isActive
          ? "text-terminal-accent bg-terminal-accent/5 border-r-2 border-terminal-accent"
          : "text-terminal-muted hover:text-terminal-fg hover:bg-terminal-surface-hover"
      }`}
      style={{ paddingLeft }}
      onClick={onClose}
    >
      <span className={`mr-1.5 text-[8px] ${isActive ? "text-terminal-accent" : "text-terminal-border"}`}>
        {item.icon || "◆"}
      </span>
      <span className="truncate">{item.title}</span>
    </Link>
  );
}

export default function Sidebar({ onClose }: { onClose?: () => void }) {
  return (
    <aside
      className="w-64 h-screen fixed left-0 top-0 bg-terminal-surface border-r border-terminal-border flex flex-col overflow-hidden z-30"
      role="navigation"
      aria-label="Documentation navigation"
    >
      {/* Header */}
      <div className="px-4 pt-5 pb-4 border-b border-terminal-border">
        <Link href="/" className="flex items-center gap-2.5 group no-underline" onClick={onClose}>
          <div className="w-7 h-7 rounded bg-terminal-accent/10 border border-terminal-accent/30 flex items-center justify-center group-hover:bg-terminal-accent/20 transition-colors">
            <span className="text-terminal-accent font-mono text-xs font-bold">S</span>
          </div>
          <div>
            <div className="font-mono text-sm font-semibold text-terminal-fg group-hover:text-terminal-accent transition-colors">
              Sentinel Stack
            </div>
            <div className="font-mono text-[10px] text-terminal-muted tracking-wider">
              KNOWLEDGE PORTAL
            </div>
          </div>
        </Link>
      </div>

      {/* File Tree */}
      <nav className="flex-1 overflow-y-auto py-3">
        <div className="px-4 mb-2">
          <span className="text-[10px] font-mono text-terminal-muted tracking-widest uppercase">
            Explorer
          </span>
        </div>
        {navigationTree.map((item, idx) => (
          <NavNode key={`${item.title}-${idx}`} item={item} depth={0} onClose={onClose} />
        ))}
      </nav>

      {/* Footer */}
      <div className="px-4 py-3 border-t border-terminal-border">
        <div className="flex items-center gap-2 text-[10px] font-mono text-terminal-muted">
          <span className="w-1.5 h-1.5 rounded-full bg-terminal-accent animate-pulse-glow"></span>
          <span>v1.0-rc1</span>
          <span className="ml-auto opacity-50">Ring -1 to Layer 7</span>
        </div>
      </div>
    </aside>
  );
}
