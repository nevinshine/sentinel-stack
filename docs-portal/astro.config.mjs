// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

import react from '@astrojs/react';
import tailwindcss from '@tailwindcss/vite';

// https://astro.build/config
export default defineConfig({
  integrations: [
    starlight({
      title: 'Sentinel Stack',
      description: 'Terminal-native architectural knowledge graph for the Sentinel Stack security framework.',
      customCss: ['./src/styles/globals.css'],
      social: [
        { icon: 'github', label: 'GitHub', href: 'https://github.com/nevinshine/sentinel-stack' }
      ],
      sidebar: [
        {
          label: 'Architecture',
          items: [{ autogenerate: { directory: 'architecture' } }],
        },
        {
          label: 'Engineering',
          items: [{ autogenerate: { directory: 'engineering' } }],
        },
        {
          label: 'Evidence',
          items: [{ autogenerate: { directory: 'evidence' } }],
        },
        {
          label: 'Generated API',
          items: [{ autogenerate: { directory: 'generated' } }],
        },
      ],
    }),
    react(),
  ],

  vite: {
    plugins: [tailwindcss()],
  },
});