"use client";

import React, { type JSX } from "react";
import type { DocPage } from "@/lib/mdx";
import Link from "next/link";
import TableOfContents from "@/components/TableOfContents";
import RegisterMatrix from "@/components/RegisterMatrix";
import StructAlignmentView from "@/components/StructAlignmentView";
import EbpfMapTopology from "@/components/EbpfMapTopology";
import LabTelemetryDashboard from "@/components/LabTelemetryDashboard";

// Render markdown content as React elements
function MarkdownRenderer({ content }: { content: string }) {
  const lines = content.split("\n");
  const elements: React.ReactNode[] = [];
  let i = 0;

  while (i < lines.length) {
    const line = lines[i];

    // Code blocks
    if (line.startsWith("```")) {
      const lang = line.slice(3).trim();
      const codeLines: string[] = [];
      i++;
      while (i < lines.length && !lines[i].startsWith("```")) {
        codeLines.push(lines[i]);
        i++;
      }
      i++; // skip closing ```
      elements.push(
        <pre
          key={elements.length}
          className="bg-terminal-surface border border-terminal-border rounded-lg p-4 my-4 overflow-x-auto text-sm leading-relaxed"
        >
          {lang && (
            <div className="text-[10px] text-terminal-muted font-mono mb-2 -mt-1 uppercase tracking-wider">
              {lang}
            </div>
          )}
          <code className="text-terminal-fg font-mono">{codeLines.join("\n")}</code>
        </pre>
      );
      continue;
    }

    // Headings
    const headingMatch = line.match(/^(#{1,4})\s+(.+)$/);
    if (headingMatch) {
      const level = headingMatch[1].length;
      const text = headingMatch[2];
      const id = text
        .toLowerCase()
        .replace(/[^\w\s-]/g, "")
        .replace(/\s+/g, "-");
      const Tag = `h${level}` as keyof JSX.IntrinsicElements;
      elements.push(
        <Tag key={elements.length} id={id}>
          {text}
        </Tag>
      );
      i++;
      continue;
    }

    // Horizontal rule
    if (line.match(/^---+$/)) {
      elements.push(<hr key={elements.length} />);
      i++;
      continue;
    }

    // Blockquote
    if (line.startsWith("> ")) {
      const quoteLines: string[] = [];
      while (i < lines.length && lines[i].startsWith("> ")) {
        quoteLines.push(lines[i].slice(2));
        i++;
      }
      elements.push(
        <blockquote key={elements.length}>
          {quoteLines.map((ql, idx) => (
            <p key={idx}>{renderInline(ql)}</p>
          ))}
        </blockquote>
      );
      continue;
    }

    // Table
    if (line.includes("|") && i + 1 < lines.length && lines[i + 1]?.match(/^\|[\s:-]+\|/)) {
      const tableLines: string[] = [];
      while (i < lines.length && lines[i].includes("|")) {
        tableLines.push(lines[i]);
        i++;
      }
      if (tableLines.length >= 2) {
        const headers = tableLines[0]
          .split("|")
          .filter(Boolean)
          .map((h) => h.trim());
        const rows = tableLines.slice(2).map((row) =>
          row
            .split("|")
            .filter(Boolean)
            .map((c) => c.trim())
        );
        elements.push(
          <table key={elements.length}>
            <thead>
              <tr>
                {headers.map((h, idx) => (
                  <th key={idx}>{renderInline(h)}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {rows.map((row, rIdx) => (
                <tr key={rIdx}>
                  {row.map((cell, cIdx) => (
                    <td key={cIdx}>{renderInline(cell)}</td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        );
      }
      continue;
    }

    // Unordered list
    if (line.match(/^[-*]\s/)) {
      const listItems: string[] = [];
      while (i < lines.length && lines[i].match(/^[-*]\s/)) {
        listItems.push(lines[i].replace(/^[-*]\s/, ""));
        i++;
      }
      elements.push(
        <ul key={elements.length}>
          {listItems.map((item, idx) => (
            <li key={idx}>{renderInline(item)}</li>
          ))}
        </ul>
      );
      continue;
    }

    // Empty lines
    if (line.trim() === "") {
      i++;
      continue;
    }

    // Paragraph
    const paraLines: string[] = [];
    while (i < lines.length && lines[i].trim() !== "" && !lines[i].startsWith("#") && !lines[i].startsWith("```") && !lines[i].startsWith("---") && !lines[i].startsWith(">") && !lines[i].startsWith("- ") && !lines[i].startsWith("* ") && !lines[i].includes("|")) {
      paraLines.push(lines[i]);
      i++;
    }
    if (paraLines.length > 0) {
      elements.push(
        <p key={elements.length}>{renderInline(paraLines.join(" "))}</p>
      );
    }
  }

  return <>{elements}</>;
}

function renderInline(text: string): React.ReactNode {
  const parts: React.ReactNode[] = [];
  let remaining = text;
  let key = 0;

  while (remaining.length > 0) {
    const codeMatch = remaining.match(/^(.*?)`([^`]+)`/);
    const boldMatch = remaining.match(/^(.*?)\*\*([^*]+)\*\*/);

    let bestMatch = null;
    let type = "";

    if (codeMatch) {
      bestMatch = codeMatch;
      type = "code";
    }

    if (boldMatch) {
      if (!bestMatch || boldMatch[1].length < bestMatch[1].length) {
        bestMatch = boldMatch;
        type = "bold";
      }
    }

    if (!bestMatch) {
      parts.push(renderPlain(remaining, key++));
      break;
    }

    if (bestMatch[1]) {
      parts.push(renderPlain(bestMatch[1], key++));
    }

    if (type === "code") {
      parts.push(
        <code key={key++} className="bg-terminal-surface text-terminal-accent px-1.5 py-0.5 rounded text-[0.88em] border border-terminal-border">
          {bestMatch[2]}
        </code>
      );
    } else if (type === "bold") {
      parts.push(<strong key={key++} className="font-bold text-terminal-fg">{bestMatch[2]}</strong>);
    }

    remaining = remaining.slice(bestMatch[0].length);
  }

  return parts;
}

function renderPlain(text: string, key: number): React.ReactNode {
  return <span key={key}>{text}</span>;
}

// Component injection based on page slug
function getInteractiveComponents(slug: string): React.ReactNode[] {
  const components: React.ReactNode[] = [];

  if (slug === "engineering/hyperion-xdp/ebpf-maps") {
    components.push(<EbpfMapTopology key="ebpf-maps" />);
  }

  if (slug === "engineering/hyperion-xdp/struct-alignment") {
    components.push(
      <StructAlignmentView
        key="struct-align"
        title="struct hyp_event"
        description="M5 Telemetry Event — Cross-space binary parity"
        totalSize={40}
        cSourceFile="hyperion_core.c"
        goSourceFile="main.go"
        cFields={[
          { name: "event_type", type: "__u8", size: 1, offset: 0, comment: "0=ACCEPT, 1=DROP, 2=SIG_MATCH" },
          { name: "_pad1[3]", type: "__u8[3]", size: 3, offset: 1, isPadding: true, comment: "Alignment padding" },
          { name: "src_ip", type: "__u32", size: 4, offset: 4, comment: "Source IP (network byte order)" },
          { name: "dst_ip", type: "__u32", size: 4, offset: 8, comment: "Destination IP" },
          { name: "src_port", type: "__u16", size: 2, offset: 12, comment: "Source port" },
          { name: "dst_port", type: "__u16", size: 2, offset: 14, comment: "Destination port" },
          { name: "protocol", type: "__u8", size: 1, offset: 16, comment: "IP protocol (6=TCP)" },
          { name: "_pad2[7]", type: "__u8[7]", size: 7, offset: 17, isPadding: true, comment: "8-byte alignment" },
          { name: "timestamp", type: "__u64", size: 8, offset: 24, comment: "bpf_ktime_get_ns()" },
          { name: "signature[8]", type: "char[8]", size: 8, offset: 32, comment: "Matched signature" },
        ]}
        goFields={[
          { name: "EventType", type: "uint8", size: 1, offset: 0, comment: "0=ACCEPT, 1=DROP, 2=SIG_MATCH" },
          { name: "_", type: "[3]uint8", size: 3, offset: 1, isPadding: true, comment: "Alignment padding" },
          { name: "SrcIP", type: "uint32", size: 4, offset: 4, comment: "Source IP" },
          { name: "DstIP", type: "uint32", size: 4, offset: 8, comment: "Destination IP" },
          { name: "SrcPort", type: "uint16", size: 2, offset: 12, comment: "Source port" },
          { name: "DstPort", type: "uint16", size: 2, offset: 14, comment: "Destination port" },
          { name: "Protocol", type: "uint8", size: 1, offset: 16, comment: "IP protocol" },
          { name: "_", type: "[7]uint8", size: 7, offset: 17, isPadding: true, comment: "8-byte alignment" },
          { name: "Timestamp", type: "uint64", size: 8, offset: 24, comment: "bpf_ktime_get_ns()" },
          { name: "Signature", type: "[8]byte", size: 8, offset: 32, comment: "Matched signature" },
        ]}
      />
    );
  }

  if (slug === "engineering/sentinel-vmi/register-matrices") {
    components.push(
      <RegisterMatrix
        key="vttbr"
        name="VTTBR_EL2"
        description="Virtualization Translation Table Base Register"
        width={64}
        sourceRef="ARM Architecture Reference Manual (IHI 0062C)"
        fields={[
          { name: "VMID", bits: [63, 48], description: "Virtual Machine Identifier. Width depends on VTCR_EL2.VS: 8-bit (bits [55:48]) when VS=0, or 16-bit (bits [63:48]) when VS=1.", color: "purple" },
          { name: "RES0", bits: [47, 44], description: "Reserved, RES0. Reads as zero, writes ignored.", color: "muted" },
          { name: "BADDR", bits: [43, 1], description: "Translation Table Base Address. Physical address of the first level translation table. Aligned to the table size.", color: "accent" },
          { name: "CnP", bits: [0, 0], description: "Common not Private. When set, indicates this translation table entry may be shared across Processing Elements.", color: "cyan" },
        ]}
      />
    );
  }

  if (slug === "evidence/live-telemetry") {
    components.push(<LabTelemetryDashboard key="lab-telemetry" />);
  }

  return components;
}

export default function DocPageClient({ doc }: { doc: DocPage }) {
  const slugStr = doc.slug.join("/");
  const viewColors: Record<string, string> = {
    architect: "text-terminal-purple",
    engineer: "text-terminal-blue",
    debugger: "text-terminal-yellow",
  };
  const viewBgs: Record<string, string> = {
    architect: "bg-terminal-purple/10",
    engineer: "bg-terminal-blue/10",
    debugger: "bg-terminal-yellow/10",
  };

  const interactiveComponents = getInteractiveComponents(slugStr);

  return (
    <div className="flex animate-fade-in">
      {/* Main Content */}
      <article className="flex-1 min-w-0 px-8 py-8">
        {/* Breadcrumb */}
        <nav className="flex items-center gap-2 font-mono text-xs text-terminal-muted mb-6" aria-label="Breadcrumb">
          <Link href="/" className="hover:text-terminal-fg transition-colors no-underline text-terminal-muted">
            ~
          </Link>
          {doc.slug.map((part, idx) => (
            <React.Fragment key={idx}>
              <span className="text-terminal-border">/</span>
              <span className={idx === doc.slug.length - 1 ? "text-terminal-fg" : ""}>
                {part}
              </span>
            </React.Fragment>
          ))}
          <span className={`ml-3 px-2 py-0.5 rounded text-[10px] ${viewColors[doc.meta.view]} ${viewBgs[doc.meta.view]}`}>
            {doc.meta.view.toUpperCase()}
          </span>
        </nav>

        {/* Prose Content */}
        <div className="prose max-w-none">
          <MarkdownRenderer content={doc.content} />

          {/* Inject interactive components at appropriate positions */}
          {interactiveComponents.length > 0 && (
            <div className="mt-8">
              {interactiveComponents}
            </div>
          )}
        </div>
      </article>

      {/* Table of Contents */}
      <TableOfContents headings={doc.headings} />
    </div>
  );
}
