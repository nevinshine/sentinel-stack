// Navigation tree data for the documentation sidebar.
// Maps the knowledge graph structure to the file-browser navigation.

export interface NavItem {
  title: string;
  href?: string;
  icon?: string;
  children?: NavItem[];
  tag?: "architect" | "engineer" | "debugger";
  badge?: string;
}

export const navigationTree: NavItem[] = [
  {
    title: "architecture",
    icon: "[A]",
    tag: "architect",
    children: [
      {
        title: "overview.mdx",
        href: "/architecture/overview",
        icon: "◆",
      },
      {
        title: "threat-model.mdx",
        href: "/architecture/threat-model",
        icon: "◆",
      },
      {
        title: "unified-defense-graph.mdx",
        href: "/architecture/unified-defense-graph",
        icon: "◆",
      },
    ],
  },
  {
    title: "engineering",
    icon: "[E]",
    tag: "engineer",
    children: [
      {
        title: "hyperion-xdp",
        icon: "▸",
        children: [
          {
            title: "overview.mdx",
            href: "/engineering/hyperion-xdp/overview",
            icon: "◆",
          },
          {
            title: "ebpf-maps.mdx",
            href: "/engineering/hyperion-xdp/ebpf-maps",
            icon: "◆",
          },
          {
            title: "struct-alignment.mdx",
            href: "/engineering/hyperion-xdp/struct-alignment",
            icon: "◆",
          },
        ],
      },
      {
        title: "sentinel-vmi",
        icon: "▸",
        children: [
          {
            title: "overview.mdx",
            href: "/engineering/sentinel-vmi/overview",
            icon: "◆",
          },
          {
            title: "register-matrices.mdx",
            href: "/engineering/sentinel-vmi/register-matrices",
            icon: "◆",
          },
          {
            title: "npt-guard.mdx",
            href: "/engineering/sentinel-vmi/npt-guard",
            icon: "◆",
          },
        ],
      },
      {
        title: "telos-runtime",
        icon: "▸",
        children: [
          {
            title: "overview.mdx",
            href: "/engineering/telos-runtime/overview",
            icon: "◆",
          },
          {
            title: "dual-gate.mdx",
            href: "/engineering/telos-runtime/dual-gate",
            icon: "◆",
          },
        ],
      },
      {
        title: "telos-lang",
        icon: "▸",
        children: [
          {
            title: "overview.mdx",
            href: "/engineering/telos-lang/overview",
            icon: "◆",
          },
        ],
      },
      {
        title: "sentinel-kv",
        icon: "▸",
        children: [
          {
            title: "overview.mdx",
            href: "/engineering/sentinel-kv/overview",
            icon: "◆",
          },
        ],
      },
    ],
  },
  {
    title: "evidence",
    icon: "[D]",
    tag: "debugger",
    children: [
      {
        title: "benchmarks.mdx",
        href: "/evidence/benchmarks",
        icon: "◆",
      },
      {
        title: "telemetry-spec.mdx",
        href: "/evidence/telemetry-spec",
        icon: "◆",
      },
      {
        title: "live-telemetry.mdx",
        href: "/evidence/live-telemetry",
        icon: "◆",
      },
    ],
  },
];
