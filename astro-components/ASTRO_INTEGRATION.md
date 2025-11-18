# 🎨 WeighMyBru² Astro Integration Guide

This guide shows you how to integrate the ESP32 Web Tools flash functionality into your existing Astro website while maintaining your current theme and design.

## 📋 What's Included

The Astro components are designed to match your existing WeighMyBru² theme with:
- **Dark color scheme**: `black-background`, `black-discord`, `black-select`
- **Green accents**: `#197C28` (your signature green)
- **Responsive design**: Mobile-first with your exact navigation pattern
- **Font Awesome icons**: Matching your current icon usage
- **Alpine.js integration**: For interactive elements

## 🗂️ Component Structure

```
src/
├── components/
│   ├── BaseLayout.astro          # Main layout with theme setup
│   ├── Navigation.astro          # Responsive nav matching your design
│   ├── HeroSection.astro         # Homepage hero with CTAs
│   ├── FeaturesGrid.astro        # Features showcase
│   └── FlashFirmware.astro       # Complete ESP32 Web Tools page
└── pages/
    ├── index.astro               # Homepage example
    └── flash.astro               # Flash firmware page
```

## 🚀 Installation Steps

### 1. Copy Components to Your Astro Project

```bash
# Copy the components to your Astro project
cp astro-components/*.astro your-astro-website/src/components/
```

### 2. Create the Flash Page

Create `src/pages/flash.astro`:

```astro
---
import FlashFirmware from '../components/FlashFirmware.astro';
---

<FlashFirmware />
```

### 3. Add Navigation Link

Update your existing navigation to include the flash page:

```astro
<!-- In your existing navigation component -->
<a href="/flash" class="nav-link-class">
  <i class="fa fa-microchip mr-2"></i>
  Flash Firmware
</a>
```

### 4. Set Up Release File Hosting

You'll need to sync GitHub releases to your website. Choose one option:

#### Option A: Manual Sync
1. Use the provided sync scripts from the `website/` directory
2. Upload the `releases/` folder to your website's public directory
3. Run sync script after each firmware release

#### Option B: Automated Sync (Recommended)
1. Set up GitHub Actions to sync releases automatically
2. Use Cloudflare Workers for real-time updates
3. Configure webhooks for instant deployment

### 5. Update Your Homepage

You can either:
- Replace your current homepage with the provided example
- Add individual components to your existing homepage
- Use the components as reference for custom implementations

## 🎨 Theme Customization

### Color Variables
The components use your exact color scheme:

```css
:root {
  --black-background: #1A1A1E;
  --black-discord: #121214;
  --black-hover: #1D1D1E;
  --black-select: #363638;
  --button-green: #197C28;
}
```

### Custom Tailwind Config
Add to your `tailwind.config.mjs`:

```js
export default {
  theme: {
    extend: {
      colors: {
        'black-discord': '#121214',
        'black-hover': '#1D1D1E', 
        'black-select': '#363638',
        'black-background': '#1A1A1E',
        'button-green': '#197C28'
      }
    }
  }
}
```

### Font Awesome
Ensure Font Awesome is loaded (already included in BaseLayout):

```html
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
```

## 🔧 Component Usage Examples

### Basic Flash Page
```astro
---
// src/pages/flash.astro
import FlashFirmware from '../components/FlashFirmware.astro';
---

<FlashFirmware />
```

### Custom Homepage with Components
```astro
---
import BaseLayout from '../components/BaseLayout.astro';
import Navigation from '../components/Navigation.astro';
import HeroSection from '../components/HeroSection.astro';
---

<BaseLayout title="WeighMyBru² - Your Custom Title">
  <Navigation currentPage="/" />
  <HeroSection 
    title="Your Custom Title"
    subtitle="Your custom subtitle"
    showCTAs={true}
  />
  
  <!-- Your existing content -->
  
</BaseLayout>
```

### Standalone Navigation
```astro
---
import Navigation from '../components/Navigation.astro';
---

<!-- Use in any page -->
<Navigation currentPage="/current-path" />
```

## 📱 Responsive Behavior

The components follow your exact responsive patterns:

- **Mobile**: Collapsible navigation, full-width content
- **Tablet**: Optimized spacing, readable text sizes
- **Desktop**: Fixed sidebar navigation, centered content

## 🔄 Release Management

### Automatic Updates
The components automatically fetch the latest release info from GitHub:

```javascript
// Fetches latest version for display
const response = await fetch('https://api.github.com/repos/031devstudios/weighmybru2/releases/latest');
```

### Manual Override
You can override version info in components:

```astro
<div id="version-info">v2.1.0</div> <!-- Fallback if API fails -->
```

## 🧪 Testing Your Integration

1. **Build your Astro site**: `npm run build`
2. **Test ESP32 Web Tools**: Visit `/flash` page
3. **Verify responsive design**: Test on different screen sizes
4. **Check GitHub integration**: Ensure version fetching works

## 🎯 Deployment

### Cloudflare Pages (Recommended)
1. Connect your Astro website repo to Cloudflare Pages
2. Set up the release sync workflow
3. Configure custom domains and SSL

### Static Hosting
1. Build your Astro site: `npm run build`
2. Upload `dist/` folder to your hosting provider
3. Ensure the `releases/` directory is accessible

## 🔧 Troubleshooting

### Common Issues

**ESP32 Web Tools not working:**
- Ensure HTTPS is enabled
- Check that manifest files are accessible
- Verify browser compatibility (Chrome/Edge/Opera)

**Version info not loading:**
- Check GitHub API rate limits
- Verify repository name in fetch URLs
- Test API endpoints directly

**Styling conflicts:**
- Check Tailwind CSS configuration
- Verify custom color definitions
- Test CSS specificity issues

### Debug Mode
Add to any component for debugging:

```astro
<div class="debug-info" style="display: none;">
  <!-- Debug information -->
</div>
```

## 📞 Support

- **Discord**: [Join our community](https://discord.gg/HYp4TSEjSf)
- **GitHub**: [Open an issue](https://github.com/031devstudios/weighmybru2/issues)
- **Documentation**: Check the main project README

## 🎉 Go Live!

Once integrated:

1. **Test thoroughly** on different devices and browsers
2. **Update your navigation** to prominently feature the flash tool
3. **Announce to your community** - make firmware flashing accessible!
4. **Monitor usage** and gather feedback for improvements

Your users can now flash WeighMyBru² firmware directly from your website without any software installation! 🚀