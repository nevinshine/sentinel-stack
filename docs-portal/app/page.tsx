import Link from "next/link";
import InteroperabilityMatrix from "@/components/InteroperabilityMatrix";

const pillars = [
  {
    name: "sentinel-vmi",
    ring: "Ring -1",
    title: "Hypervisor Introspection",
    description: "Operates below the OS at AMD-V / ARMv8 EL2. NPT Guard write-protects sys_call_table at the hardware level. Even a fully compromised kernel cannot detect or disable it.",
    tech: ["AMD-V", "NPT Guard", "kvmi-v7", "ARMv8 EL2"],
    href: "/engineering/sentinel-vmi/overview",
    color: "#E06C75",
    icon: "[*]",
  },
  {
    name: "telos-runtime",
    ring: "Ring 0",
    title: "Intent-Based AI Security",
    description: "eBPF-LSM Dual-Gate enforcement with Information Flow Control. If a process reads /etc/shadow, all network access is permanently revoked via Network Slam.",
    tech: ["eBPF-LSM", "IFC", "Taint Tracking", "gRPC"],
    href: "/engineering/telos-runtime/overview",
    color: "#C678DD",
    icon: "[*]",
  },
  {
    name: "hyperion-xdp",
    ring: "Wire",
    title: "Wire-Speed Network Defense",
    description: "XDP programs drop malicious packets at the NIC driver before sk_buff allocation. Sub-microsecond enforcement with O(1) deterministic lookups.",
    tech: ["XDP", "eBPF", "BPF_MAP_TYPE_LRU_HASH", "Ringbuf"],
    href: "/engineering/hyperion-xdp/overview",
    color: "#56B6C2",
    icon: "[*]",
  },
  {
    name: "telos-lang",
    ring: "Compile",
    title: "Formally Verified Compiler",
    description: "Rust/LLVM/Z3 policy-as-code compiler. Mathematically proves eBPF safety via Hoare Logic and BitVector analysis before kernel deployment.",
    tech: ["Rust", "LLVM", "Z3 SMT", "Dual-Target IR"],
    href: "/engineering/telos-lang/overview",
    color: "#E5C07B",
    icon: "[*]",
  },
  {
    name: "sentinel-kv",
    ring: "Verify",
    title: "LLVM IR Static Analyzer",
    description: "SMT-backed memory safety analysis with Ed25519 ring-aware attestation. HITL verification gate ensures AI hallucinations never compromise kernel safety.",
    tech: ["LLVM IR", "Z3", "Ed25519", "HITL Gate"],
    href: "/engineering/sentinel-kv/overview",
    color: "#98C379",
    icon: "[*]",
  },
];

export default function Home() {
  return (
    <div className="animate-fade-in">
      {/* Hero Section */}
      <section className="px-8 pt-16 pb-12 border-b border-terminal-border">
        <div className="max-w-4xl">
          <div className="flex items-center gap-3 mb-4">
            <div className="w-10 h-10 rounded-lg bg-terminal-accent/10 border border-terminal-accent/30 flex items-center justify-center">
              <span className="text-terminal-accent font-mono text-lg font-bold">S</span>
            </div>
            <div>
              <span className="font-mono text-[10px] text-terminal-muted tracking-widest uppercase block">
                Knowledge Portal
              </span>
            </div>
          </div>

          <h1 className="font-mono text-4xl font-bold text-terminal-fg mb-2 tracking-tight leading-tight border-none">
            Sentinel Stack
          </h1>
          <p className="font-mono text-lg text-terminal-accent mb-6">
            Deterministic, Kernel-Native Defense from Ring -1 to Layer 7
          </p>

          <p className="text-terminal-fg/80 max-w-2xl leading-relaxed mb-8">
            The architectural knowledge graph for a unified systems-security framework
            that bridges the semantic gap between compile-time intent and runtime enforcement.
            Spanning hypervisor introspection, eBPF-LSM kernel hooks, wire-speed XDP filtering,
            and formally verified policy compilation.
          </p>

          {/* Quick Stats */}
          <div className="flex flex-wrap gap-6 font-mono text-sm">
            <div className="flex items-center gap-2">
              <span className="w-2 h-2 rounded-full bg-terminal-accent animate-pulse-glow" />
              <span className="text-terminal-muted">5 Enforcement Layers</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="w-2 h-2 rounded-full bg-terminal-red" />
              <span className="text-terminal-muted">Ring -1 to Layer 7</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="w-2 h-2 rounded-full bg-terminal-blue" />
              <span className="text-terminal-muted">C / Go / Rust / Python</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="w-2 h-2 rounded-full bg-terminal-yellow" />
              <span className="text-terminal-muted">Z3 Formal Verification</span>
            </div>
          </div>
        </div>
      </section>

      {/* Navigation Views */}
      <section className="px-8 py-10 border-b border-terminal-border">
        <h2 className="font-mono text-lg font-bold text-terminal-fg mb-6 border-none">
          <span className="text-terminal-accent">&gt;</span> Multi-View Navigation
        </h2>
        <div className="grid grid-cols-3 gap-4 max-w-4xl">
          <Link
            href="/architecture/overview"
            className="group p-5 rounded-lg border border-terminal-border bg-terminal-surface hover:border-terminal-purple/50 hover:bg-terminal-surface-hover transition-all duration-200 no-underline"
          >
            <div className="flex items-center gap-2 mb-2">
              <span className="font-mono text-xs px-2 py-0.5 rounded bg-terminal-purple/15 text-terminal-purple">
                ARCHITECT
              </span>
            </div>
            <h3 className="font-mono text-sm font-semibold text-terminal-fg group-hover:text-terminal-purple transition-colors mb-1 border-none">
              Semantic Intent
            </h3>
            <p className="text-xs text-terminal-muted leading-relaxed">
              High-level threat models, defense philosophy, and the semantic-to-execution pipeline.
            </p>
          </Link>

          <Link
            href="/engineering/hyperion-xdp/overview"
            className="group p-5 rounded-lg border border-terminal-border bg-terminal-surface hover:border-terminal-blue/50 hover:bg-terminal-surface-hover transition-all duration-200 no-underline"
          >
            <div className="flex items-center gap-2 mb-2">
              <span className="font-mono text-xs px-2 py-0.5 rounded bg-terminal-blue/15 text-terminal-blue">
                ENGINEER
              </span>
            </div>
            <h3 className="font-mono text-sm font-semibold text-terminal-fg group-hover:text-terminal-blue transition-colors mb-1 border-none">
              Implementation
            </h3>
            <p className="text-xs text-terminal-muted leading-relaxed">
              Register matrices, eBPF map topologies, struct alignment, API boundaries.
            </p>
          </Link>

          <Link
            href="/evidence/benchmarks"
            className="group p-5 rounded-lg border border-terminal-border bg-terminal-surface hover:border-terminal-yellow/50 hover:bg-terminal-surface-hover transition-all duration-200 no-underline"
          >
            <div className="flex items-center gap-2 mb-2">
              <span className="font-mono text-xs px-2 py-0.5 rounded bg-terminal-yellow/15 text-terminal-yellow">
                DEBUGGER
              </span>
            </div>
            <h3 className="font-mono text-sm font-semibold text-terminal-fg group-hover:text-terminal-yellow transition-colors mb-1 border-none">
              Telemetry & Traces
            </h3>
            <p className="text-xs text-terminal-muted leading-relaxed">
              Performance benchmarks, execution traces, and kernel telemetry data.
            </p>
          </Link>
        </div>
      </section>

      {/* Pillar Cards */}
      <section className="px-8 py-10 border-b border-terminal-border">
        <h2 className="font-mono text-lg font-bold text-terminal-fg mb-6 border-none">
          <span className="text-terminal-accent">&gt;</span> Defense Quadrant
        </h2>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4 max-w-5xl">
          {pillars.map((pillar) => (
            <Link
              key={pillar.name}
              href={pillar.href}
              className="group p-5 rounded-lg border border-terminal-border bg-terminal-surface hover:bg-terminal-surface-hover transition-all duration-200 no-underline"
              style={{ borderColor: undefined }}
            >
              <div className="flex items-center justify-between mb-3">
                <span className="text-2xl" style={{ color: pillar.color }}>
                  {pillar.icon}
                </span>
                <span
                  className="font-mono text-[10px] px-2 py-0.5 rounded border"
                  style={{
                    color: pillar.color,
                    borderColor: `${pillar.color}33`,
                    backgroundColor: `${pillar.color}10`,
                  }}
                >
                  {pillar.ring}
                </span>
              </div>
              <h3
                className="font-mono text-sm font-bold mb-1 transition-colors border-none"
                style={{ color: pillar.color }}
              >
                {pillar.name}
              </h3>
              <p className="font-mono text-xs text-terminal-muted mb-1 border-none">
                {pillar.title}
              </p>
              <p className="text-xs text-terminal-muted/70 leading-relaxed mb-3">
                {pillar.description}
              </p>
              <div className="flex flex-wrap gap-1.5">
                {pillar.tech.map((t) => (
                  <span
                    key={t}
                    className="font-mono text-[9px] px-1.5 py-0.5 rounded bg-terminal-base text-terminal-muted border border-terminal-border"
                  >
                    {t}
                  </span>
                ))}
              </div>
            </Link>
          ))}
        </div>
      </section>

      {/* Interoperability Matrix */}
      <section className="px-8 py-10">
        <InteroperabilityMatrix />
      </section>
    </div>
  );
}
