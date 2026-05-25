"use client";

import React, { useState, useEffect, useCallback, useRef } from "react";
import { useRouter } from "next/navigation";

interface SearchResult {
  id: string;
  title: string;
  description: string;
  path: string;
  view: string;
  component?: string;
}

// Static search index — loaded client-side
const searchData: SearchResult[] = [
  { id: "arch-overview", title: "Architecture Overview", description: "Sentinel Stack unified defense architecture from Ring -1 to Layer 7", path: "/architecture/overview", view: "architect" },
  { id: "arch-threat", title: "Threat Model", description: "Kernel Compromise, AI Agent Exfiltration, Wire-Speed Network Threats", path: "/architecture/threat-model", view: "architect" },
  { id: "arch-defense", title: "Unified Defense Graph", description: "Cross-layer enforcement topology and signal flow", path: "/architecture/unified-defense-graph", view: "architect" },
  { id: "eng-hyperion", title: "Hyperion XDP Overview", description: "Wire-speed XDP_DROP network defense at the NIC driver", path: "/engineering/hyperion-xdp/overview", view: "engineer", component: "hyperion-xdp" },
  { id: "eng-hyperion-maps", title: "eBPF Map Topologies", description: "policy_map, flow_map, telemetry_ringbuf — BPF map architecture", path: "/engineering/hyperion-xdp/ebpf-maps", view: "engineer", component: "hyperion-xdp" },
  { id: "eng-hyperion-struct", title: "C/Go Struct Alignment", description: "hyp_event 40-byte binary parity between kernel C and Go control plane", path: "/engineering/hyperion-xdp/struct-alignment", view: "engineer", component: "hyperion-xdp" },
  { id: "eng-vmi", title: "Sentinel VMI Overview", description: "Ring -1 hypervisor introspection via AMD-V NPT Guard and ARMv8 EL2", path: "/engineering/sentinel-vmi/overview", view: "engineer", component: "sentinel-vmi" },
  { id: "eng-vmi-reg", title: "Register Matrices", description: "VTTBR_EL2, VTCR_EL2, S2AP — AArch64 hardware register layouts", path: "/engineering/sentinel-vmi/register-matrices", view: "engineer", component: "sentinel-vmi" },
  { id: "eng-vmi-npt", title: "NPT Guard", description: "Nested Page Table sys_call_table write-protection and #NPF trap handling", path: "/engineering/sentinel-vmi/npt-guard", view: "engineer", component: "sentinel-vmi" },
  { id: "eng-telos-rt", title: "Telos Runtime Overview", description: "Intent-based AI security with eBPF-LSM Dual-Gate enforcement", path: "/engineering/telos-runtime/overview", view: "engineer", component: "telos-runtime" },
  { id: "eng-telos-gate", title: "Dual-Gate Architecture", description: "Execution Gate + Network Gate with IFC taint tracking and Network Slam", path: "/engineering/telos-runtime/dual-gate", view: "engineer", component: "telos-runtime" },
  { id: "eng-telos-lang", title: "Telos Lang Compiler", description: "Rust/LLVM/Z3 policy-as-code compiler with dual-target BPF pipeline", path: "/engineering/telos-lang/overview", view: "engineer", component: "telos-lang" },
  { id: "eng-kv", title: "Sentinel KV Analyzer", description: "LLVM IR memory safety analysis with Ed25519 ring-aware attestation", path: "/engineering/sentinel-kv/overview", view: "engineer", component: "sentinel-kv" },
  { id: "ev-bench", title: "Performance Benchmarks", description: "10M-operation benchmarks, sub-microsecond overhead, O(1) map lookups", path: "/evidence/benchmarks", view: "debugger" },
  { id: "ev-telemetry", title: "Telemetry Specification", description: "M5 hyp_event struct, ringbuf architecture, Prometheus metrics", path: "/evidence/telemetry-spec", view: "debugger" },
];

const viewColors: Record<string, string> = {
  architect: "text-terminal-purple",
  engineer: "text-terminal-blue",
  debugger: "text-terminal-yellow",
};

export default function SearchOmnibox() {
  const [isOpen, setIsOpen] = useState(false);
  const [query, setQuery] = useState("");
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef<HTMLInputElement>(null);
  const router = useRouter();

  const [meiliResults, setMeiliResults] = useState<SearchResult[]>([]);
  const useMeili = !!process.env.NEXT_PUBLIC_MEILISEARCH_HOST;

  useEffect(() => {
    if (!useMeili || query.length === 0) {
      setMeiliResults([]);
      return;
    }
    const fetchMeili = async () => {
      try {
        const res = await fetch(`${process.env.NEXT_PUBLIC_MEILISEARCH_HOST}/indexes/sentinel_docs/search`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            ...(process.env.NEXT_PUBLIC_MEILISEARCH_KEY ? { 'Authorization': `Bearer ${process.env.NEXT_PUBLIC_MEILISEARCH_KEY}` } : {})
          },
          body: JSON.stringify({ q: query, limit: 10 })
        });
        const data = await res.json();
        setMeiliResults(data.hits.map((hit: any) => ({
          id: hit.id || hit.title,
          title: hit.title,
          description: hit.description || "",
          path: hit.path,
          view: hit.view || "engineer"
        })));
      } catch (e) {
        console.error("Meilisearch error:", e);
      }
    };
    const debounce = setTimeout(fetchMeili, 150);
    return () => clearTimeout(debounce);
  }, [query, useMeili]);

  const results = useMeili && query.length > 0
    ? meiliResults
    : query.length > 0
    ? searchData.filter(
        (item) =>
          item.title.toLowerCase().includes(query.toLowerCase()) ||
          item.description.toLowerCase().includes(query.toLowerCase())
      )
    : searchData;

  const open = useCallback(() => {
    setIsOpen(true);
    setQuery("");
    setSelectedIndex(0);
    setTimeout(() => inputRef.current?.focus(), 50);
  }, []);

  const close = useCallback(() => {
    setIsOpen(false);
    setQuery("");
    setSelectedIndex(0);
  }, []);

  const navigate = useCallback(
    (path: string) => {
      close();
      router.push(path);
    },
    [close, router]
  );

  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === "k") {
        e.preventDefault();
        if (isOpen) close();
        else open();
      }
      if (e.key === "Escape" && isOpen) close();
    };
    document.addEventListener("keydown", handler);
    return () => document.removeEventListener("keydown", handler);
  }, [isOpen, open, close]);

  useEffect(() => {
    setSelectedIndex(0);
  }, [query]);

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "ArrowDown") {
      e.preventDefault();
      setSelectedIndex((i) => Math.min(i + 1, results.length - 1));
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      setSelectedIndex((i) => Math.max(i - 1, 0));
    } else if (e.key === "Enter" && results[selectedIndex]) {
      navigate(results[selectedIndex].path);
    }
  };

  return (
    <>
      {/* Trigger Button */}
      <button
        onClick={open}
        className="flex items-center gap-3 w-full max-w-xl px-4 py-2 bg-terminal-surface border border-terminal-border rounded-lg text-sm font-mono text-terminal-muted hover:border-terminal-accent/40 hover:text-terminal-fg transition-all group"
        aria-label="Search documentation"
      >
        <svg className="w-4 h-4 text-terminal-muted group-hover:text-terminal-accent transition-colors" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
        </svg>
        <span className="flex-1 text-left">Search docs, structs, registers...</span>
        <kbd className="hidden sm:inline-flex items-center gap-1 px-2 py-0.5 text-[10px] bg-terminal-base border border-terminal-border rounded text-terminal-muted">
          <span>⌘</span>K
        </kbd>
      </button>

      {/* Modal Overlay */}
      {isOpen && (
        <div className="fixed inset-0 z-50 flex items-start justify-center pt-[15vh]" onClick={close}>
          <div className="fixed inset-0 bg-black/60 backdrop-blur-sm" />
          <div
            className="relative w-full max-w-2xl bg-terminal-surface border border-terminal-border rounded-xl shadow-2xl shadow-black/50 animate-fade-in overflow-hidden"
            onClick={(e) => e.stopPropagation()}
          >
            {/* Search Input */}
            <div className="flex items-center gap-3 px-4 py-3 border-b border-terminal-border">
              <svg className="w-5 h-5 text-terminal-accent" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
              </svg>
              <input
                ref={inputRef}
                type="text"
                value={query}
                onChange={(e) => setQuery(e.target.value)}
                onKeyDown={handleKeyDown}
                placeholder="Search architecture, structs, registers, threat models..."
                className="flex-1 bg-transparent text-terminal-fg text-sm font-mono outline-none placeholder:text-terminal-muted"
                aria-label="Search query"
              />
              <kbd className="text-[10px] px-2 py-0.5 bg-terminal-base border border-terminal-border rounded text-terminal-muted font-mono">
                ESC
              </kbd>
            </div>

            {/* Results */}
            <div className="max-h-80 overflow-y-auto py-2">
              {results.length === 0 ? (
                <div className="px-4 py-8 text-center text-terminal-muted text-sm font-mono">
                  No results found for &quot;{query}&quot;
                </div>
              ) : (
                results.map((result, idx) => (
                  <button
                    key={result.id}
                    onClick={() => navigate(result.path)}
                    onMouseEnter={() => setSelectedIndex(idx)}
                    className={`w-full text-left px-4 py-2.5 flex items-start gap-3 transition-colors ${
                      idx === selectedIndex
                        ? "bg-terminal-accent/10"
                        : "hover:bg-terminal-surface-hover"
                    }`}
                  >
                    <div className="flex-1 min-w-0">
                      <div className="flex items-center gap-2">
                        <span className="text-sm font-mono text-terminal-fg truncate">
                          {result.title}
                        </span>
                        <span
                          className={`text-[10px] font-mono px-1.5 py-0.5 rounded ${viewColors[result.view]} bg-current/10`}
                          style={{ background: "currentColor", WebkitBackgroundClip: "text", backgroundClip: "text" }}
                        >
                          <span className={`${viewColors[result.view]} opacity-100`} style={{ WebkitTextFillColor: "initial" }}>
                            {result.view.toUpperCase()}
                          </span>
                        </span>
                      </div>
                      <p className="text-xs text-terminal-muted mt-0.5 truncate">
                        {result.description}
                      </p>
                    </div>
                    <span className="text-terminal-muted text-xs font-mono mt-1 shrink-0">↵</span>
                  </button>
                ))
              )}
            </div>

            {/* Footer */}
            <div className="px-4 py-2 border-t border-terminal-border flex items-center gap-4 text-[10px] font-mono text-terminal-muted">
              <span className="flex items-center gap-1">
                <kbd className="px-1 py-0.5 bg-terminal-base border border-terminal-border rounded">↑↓</kbd>
                Navigate
              </span>
              <span className="flex items-center gap-1">
                <kbd className="px-1 py-0.5 bg-terminal-base border border-terminal-border rounded">↵</kbd>
                Open
              </span>
              <span className="flex items-center gap-1">
                <kbd className="px-1 py-0.5 bg-terminal-base border border-terminal-border rounded">ESC</kbd>
                Close
              </span>
            </div>
          </div>
        </div>
      )}
    </>
  );
}
