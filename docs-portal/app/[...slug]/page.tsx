import { getAllDocSlugs, getDocBySlug } from "@/lib/mdx";
import { notFound } from "next/navigation";
import DocPageClient from "./DocPageClient";

export async function generateStaticParams() {
  const slugs = getAllDocSlugs();
  return slugs.map((slug) => ({ slug }));
}

export async function generateMetadata({ params }: { params: Promise<{ slug: string[] }> }) {
  const { slug } = await params;
  const doc = getDocBySlug(slug);
  if (!doc) return { title: "Not Found" };
  return {
    title: `${doc.meta.title} — Sentinel Stack`,
    description: doc.meta.description,
  };
}

export default async function DocPage({ params }: { params: Promise<{ slug: string[] }> }) {
  const { slug } = await params;
  const doc = getDocBySlug(slug);

  if (!doc) {
    notFound();
  }

  return <DocPageClient doc={doc} />;
}
