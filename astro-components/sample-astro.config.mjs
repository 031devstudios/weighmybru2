// astro.config.mjs - Sample configuration for WeighMyBru² website
import { defineConfig } from 'astro/config';
import tailwind from '@astrojs/tailwind';

export default defineConfig({
  site: 'https://weighmybru.com', // Replace with your domain
  integrations: [
    tailwind({
      // Apply Tailwind's base styles
      applyBaseStyles: true,
      // Use your custom Tailwind config
      configFile: './tailwind.config.mjs'
    })
  ],
  
  // Build configuration
  build: {
    // Generate static files for Cloudflare Pages
    format: 'directory',
    // Ensure releases directory is included
    assets: '_astro'
  },
  
  // Server configuration for development
  server: {
    port: 4321,
    host: true
  },
  
  // SEO and meta configuration
  experimental: {
    contentCollectionCache: true
  },
  
  // Cloudflare Pages specific settings
  output: 'static',
  
  // Handle ESP32 Web Tools assets
  vite: {
    assetsInclude: ['**/*.bin', '**/*.json'],
    build: {
      rollupOptions: {
        external: ['esp-web-tools']
      }
    }
  }
});