"use client";

import React, { useEffect, useState } from "react";

interface Heading {
  level: number;
  text: string;
  id: string;
}

export default function TableOfContents({ headings }: { headings: Heading[] }) {
  const [activeId, setActiveId] = useState("");

  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) {
          if (entry.isIntersecting) {
            setActiveId(entry.target.id);
          }
        }
      },
      { rootMargin: "-80px 0px -80% 0px", threshold: 0 }
    );

    for (const heading of headings) {
      const el = document.getElementById(heading.id);
      if (el) observer.observe(el);
    }

    return () => observer.disconnect();
  }, [headings]);

  if (headings.length === 0) return null;

  return (
    <nav className="hidden xl:block w-56 shrink-0" aria-label="Table of contents">
      <div className="fixed w-56 max-h-[calc(100vh-8rem)] overflow-y-auto py-8 pr-4">
        <h5 className="font-mono text-[10px] text-terminal-muted tracking-widest uppercase mb-3">
          On This Page
        </h5>
        <ul className="space-y-1">
          {headings
            .filter((h) => h.level <= 3)
            .map((heading) => (
              <li key={heading.id}>
                <a
                  href={`#${heading.id}`}
                  className={`block font-mono text-xs py-0.5 transition-colors duration-150 no-underline border-l-2 ${
                    activeId === heading.id
                      ? "text-terminal-accent border-terminal-accent"
                      : "text-terminal-muted hover:text-terminal-fg border-transparent hover:border-terminal-border"
                  }`}
                  style={{ paddingLeft: `${(heading.level - 1) * 12 + 8}px` }}
                >
                  {heading.text}
                </a>
              </li>
            ))}
        </ul>
      </div>
    </nav>
  );
}
