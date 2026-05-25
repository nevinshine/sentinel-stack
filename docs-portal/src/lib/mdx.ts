// MDX content loading and processing utilities
import fs from "fs";
import path from "path";
import matter from "gray-matter";

const contentDirectory = path.join(process.cwd(), "content");

export interface DocMeta {
  title: string;
  description: string;
  view: "architect" | "engineer" | "debugger";
  component?: string;
  order?: number;
  lastUpdated?: string;
}

export interface DocPage {
  slug: string[];
  meta: DocMeta;
  content: string;
  headings: Heading[];
}

export interface Heading {
  level: number;
  text: string;
  id: string;
}

/**
 * Extract headings from markdown content for Table of Contents generation.
 */
export function extractHeadings(content: string): Heading[] {
  const headingRegex = /^(#{1,4})\s+(.+)$/gm;
  const headings: Heading[] = [];
  let match;

  while ((match = headingRegex.exec(content)) !== null) {
    const level = match[1].length;
    const text = match[2].replace(/[`*_~]/g, "").trim();
    const id = text
      .toLowerCase()
      .replace(/[^\w\s-]/g, "")
      .replace(/\s+/g, "-")
      .replace(/-+/g, "-");
    headings.push({ level, text, id });
  }

  return headings;
}

/**
 * Get all MDX file paths recursively from the content directory.
 */
function getMdxFiles(dir: string, basePath: string[] = []): string[][] {
  if (!fs.existsSync(dir)) return [];
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  const paths: string[][] = [];

  for (const entry of entries) {
    if (entry.isDirectory()) {
      paths.push(...getMdxFiles(path.join(dir, entry.name), [...basePath, entry.name]));
    } else if (entry.name.endsWith(".mdx")) {
      const name = entry.name.replace(/\.mdx$/, "");
      paths.push([...basePath, name]);
    }
  }

  return paths;
}

/**
 * Get all document slugs for static generation.
 */
export function getAllDocSlugs(): string[][] {
  return getMdxFiles(contentDirectory);
}

/**
 * Load a single document by its slug path.
 */
export function getDocBySlug(slug: string[]): DocPage | null {
  const filePath = path.join(contentDirectory, ...slug) + ".mdx";

  if (!fs.existsSync(filePath)) {
    return null;
  }

  const fileContent = fs.readFileSync(filePath, "utf-8");
  const { data, content } = matter(fileContent);
  const headings = extractHeadings(content);

  return {
    slug,
    meta: {
      title: data.title || slug[slug.length - 1],
      description: data.description || "",
      view: data.view || "engineer",
      component: data.component,
      order: data.order,
      lastUpdated: data.lastUpdated,
    },
    content,
    headings,
  };
}

/**
 * Generate search index data from all documents.
 */
export function generateSearchIndex(): Array<{
  id: string;
  title: string;
  description: string;
  content: string;
  path: string;
  view: string;
  component?: string;
}> {
  const slugs = getAllDocSlugs();
  const index = [];

  for (const slug of slugs) {
    const doc = getDocBySlug(slug);
    if (!doc) continue;

    // Strip markdown syntax for plain text search
    const plainContent = doc.content
      .replace(/```[\s\S]*?```/g, "")
      .replace(/`[^`]+`/g, "")
      .replace(/[#*_~\[\]()!>|]/g, "")
      .replace(/\n+/g, " ")
      .trim()
      .slice(0, 2000); // Limit content length for index size

    index.push({
      id: slug.join("/"),
      title: doc.meta.title,
      description: doc.meta.description,
      content: plainContent,
      path: "/" + slug.join("/"),
      view: doc.meta.view,
      component: doc.meta.component,
    });
  }

  return index;
}
