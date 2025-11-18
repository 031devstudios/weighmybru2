# 🚀 WeighMyBru² Astro Website Setup Guide

This guide will help you integrate the ESP32 Web Tools components into your existing Astro website.

## 📁 Your Directory Structure

```
D:\Weigh My Bru\Concept\
├── WeighMyBru²\              # Firmware project (current workspace)
│   └── astro-components\     # ESP32 Web Tools components (ready to deploy)
└── weighmybru-site\          # Your Astro website (destination)
    ├── src\
    ├── public\
    ├── package.json
    └── astro.config.mjs
```

## 🎯 Quick Deployment

### Option 1: Automated Deployment (Recommended)

Run the deployment script from the firmware project:

```powershell
# Navigate to the astro-components directory
cd "D:\Weigh My Bru\Concept\WeighMyBru²\astro-components"

# Run the deployment script
.\deploy-to-astro.bat
```

This will automatically:
- ✅ Copy all components to the correct Astro directories
- ✅ Create the `/flash` page
- ✅ Set up the releases directory
- ✅ Add GitHub workflow for release syncing

### Option 2: Manual Deployment

If you prefer manual control:

```powershell
# Navigate to source
cd "D:\Weigh My Bru\Concept\WeighMyBru²\astro-components"

# Copy components
xcopy "BaseLayout.astro" "D:\Weigh My Bru\Concept\weighmybru-site\src\layouts\" /Y
xcopy "Navigation.astro" "D:\Weigh My Bru\Concept\weighmybru-site\src\components\" /Y
xcopy "HeroSection.astro" "D:\Weigh My Bru\Concept\weighmybru-site\src\components\" /Y
xcopy "FeaturesGrid.astro" "D:\Weigh My Bru\Concept\weighmybru-site\src\components\" /Y
xcopy "FlashFirmware.astro" "D:\Weigh My Bru\Concept\weighmybru-site\src\components\" /Y
```

## 🔧 Astro Website Configuration

### 1. Install Required Dependencies

```bash
cd "D:\Weigh My Bru\Concept\weighmybru-site"

# Install Tailwind CSS integration
npm install @astrojs/tailwind tailwindcss

# Install additional utilities (optional)
npm install @fontsource/inter
```

### 2. Update astro.config.mjs

Copy the sample configuration or merge with your existing config:

```javascript
import { defineConfig } from 'astro/config';
import tailwind from '@astrojs/tailwind';

export default defineConfig({
  site: 'https://weighmybru.com', // Your domain
  integrations: [tailwind()],
  output: 'static'
});
```

### 3. Configure Tailwind CSS

Create or update `tailwind.config.mjs` with the WeighMyBru² theme:

```javascript
export default {
  content: ['./src/**/*.{astro,html,js,jsx,md,mdx,ts,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        'black-discord': '#121214',
        'black-hover': '#1D1D1E',
        'black-select': '#363638',
        'black-background': '#1A1A1E',
        'button-green': '#197C28',
      }
    }
  }
};
```

## 📄 File Structure After Deployment

```
weighmybru-site/
├── src/
│   ├── layouts/
│   │   └── BaseLayout.astro          # Main layout with ESP32 Web Tools
│   ├── components/
│   │   ├── Navigation.astro          # Responsive navigation
│   │   ├── HeroSection.astro         # Homepage hero section
│   │   ├── FeaturesGrid.astro        # Features showcase
│   │   └── FlashFirmware.astro       # Complete flash page
│   └── pages/
│       ├── flash.astro               # ESP32 Web Tools page
│       └── example-homepage.astro    # Homepage example
├── public/
│   └── releases/                     # Firmware files (populated by workflow)
├── .github/
│   └── workflows/
│       └── sync-weighmybru-release.yml # Auto-sync releases
├── astro.config.mjs
├── tailwind.config.mjs
└── ASTRO_INTEGRATION.md
```

## 🌐 Pages You'll Get

### 1. Flash Firmware Page (`/flash`)

A complete ESP32 Web Tools integration page featuring:
- ✅ Board selection (Supermini & XIAO)
- ✅ One-click browser-based flashing  
- ✅ Automatic latest version detection
- ✅ Comprehensive troubleshooting guide
- ✅ Your exact WeighMyBru² theme

### 2. Example Homepage (`/example-homepage`)

A complete homepage template with:
- ✅ Hero section with CTAs
- ✅ Features grid
- ✅ Getting started guide
- ✅ Community links
- ✅ Footer with version info

## 🔄 Automatic Release Syncing

The GitHub workflow will automatically:
1. Monitor your firmware repository for new releases
2. Download firmware files and manifests
3. Deploy to your website's `/releases/` directory
4. Rebuild and deploy your Astro site

## 🧪 Testing Your Setup

### 1. Start Development Server

```bash
cd "D:\Weigh My Bru\Concept\weighmybru-site"
npm run dev
```

### 2. Test Pages

- **Homepage**: http://localhost:4321/
- **Flash Page**: http://localhost:4321/flash
- **Example**: http://localhost:4321/example-homepage

### 3. Test ESP32 Web Tools

1. Connect an ESP32 board
2. Visit the flash page
3. Click "Install Firmware" 
4. Follow browser prompts

## 🚀 Deployment to Production

### Cloudflare Pages (Recommended)

1. **Connect Repository**:
   - Go to Cloudflare Dashboard → Pages
   - Connect your `weighmybru-site` repository

2. **Build Settings**:
   - Build command: `npm run build`
   - Output directory: `dist`

3. **Custom Domain**:
   - Add your domain (e.g., weighmybru.com)
   - Configure DNS

### Manual Deployment

```bash
# Build the site
npm run build

# Upload the 'dist' folder to your hosting provider
```

## 🔧 Customization Options

### Update Navigation

Edit `src/components/Navigation.astro` to add/remove menu items:

```javascript
const navItems = [
  { href: '/', label: 'Home', icon: 'fa-home' },
  { href: '/flash', label: 'Flash Firmware', icon: 'fa-microchip' },
  // Add your own pages here
];
```

### Modify Theme Colors

Update `tailwind.config.mjs` to change colors:

```javascript
colors: {
  'button-green': '#your-color', // Change accent color
  'black-background': '#your-bg-color', // Change background
}
```

### Add Custom Content

Create new pages in `src/pages/` using your components:

```astro
---
import BaseLayout from '../layouts/BaseLayout.astro';
import Navigation from '../components/Navigation.astro';
---

<BaseLayout title="Custom Page">
  <Navigation currentPage="/custom" />
  <div class="p-8">
    <!-- Your content -->
  </div>
</BaseLayout>
```

## ✅ Final Checklist

- [ ] Components deployed to Astro website
- [ ] Dependencies installed (`@astrojs/tailwind`)
- [ ] Configuration files updated
- [ ] Flash page working (`/flash`)
- [ ] Development server running
- [ ] ESP32 Web Tools tested with real hardware
- [ ] Production deployment configured
- [ ] GitHub workflow enabled for auto-sync
- [ ] Custom domain configured (if applicable)

## 📞 Support

- **Discord**: [WeighMyBru² Community](https://discord.gg/HYp4TSEjSf)
- **GitHub**: [Firmware Repository](https://github.com/031devstudios/weighmybru2)
- **Documentation**: `ASTRO_INTEGRATION.md`

## 🎉 You're Ready!

Your website now has professional ESP32 Web Tools integration! Users can flash WeighMyBru² firmware directly from their browser without installing any software.

**Happy brewing! ☕🚀**