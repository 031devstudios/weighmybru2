# ⚡ Quick Start - Deploy to Your Astro Website

## 🎯 What You Need to Do

Your ESP32 Web Tools components are ready to deploy to your Astro website!

### 📍 Your Directories
- **Source** (components): `D:\Weigh My Bru\Concept\WeighMyBru²\astro-components\`
- **Destination** (your website): `D:\Weigh My Bru\Concept\weighmybru-site\`

## 🚀 Run the Deployment

### Option 1: Simple Batch Script (Recommended)

```batch
# Open PowerShell/Command Prompt
cd "D:\Weigh My Bru\Concept\WeighMyBru²\astro-components"

# Run the deployment script
deploy-to-astro.bat
```

**This will automatically:**
- ✅ Copy all components to the right places
- ✅ Create the `/flash` page  
- ✅ Set up directory structure
- ✅ Add GitHub workflow
- ✅ Show you next steps

### Option 2: PowerShell Script (Advanced)

```powershell
# Open PowerShell as Administrator
cd "D:\Weigh My Bru\Concept\WeighMyBru²\astro-components"

# Run with dry-run to see what will happen
.\deploy-to-astro.ps1 -DryRun

# Run the actual deployment
.\deploy-to-astro.ps1
```

## 🔧 After Deployment

1. **Install dependencies**:
   ```bash
   cd "D:\Weigh My Bru\Concept\weighmybru-site"
   npm install @astrojs/tailwind tailwindcss
   ```

2. **Test locally**:
   ```bash
   npm run dev
   # Visit: http://localhost:4321/flash
   ```

3. **Deploy to production**:
   ```bash
   npm run build
   # Upload to Cloudflare Pages
   ```

## 📁 What Gets Created

```
weighmybru-site/
├── src/
│   ├── layouts/BaseLayout.astro          # Main layout
│   ├── components/
│   │   ├── Navigation.astro              # Your themed navigation
│   │   ├── FlashFirmware.astro           # ESP32 Web Tools page
│   │   ├── HeroSection.astro             # Homepage hero
│   │   └── FeaturesGrid.astro            # Features section
│   └── pages/
│       └── flash.astro                   # Flash page (new!)
├── public/releases/                      # Firmware files directory
└── .github/workflows/                    # Auto-sync workflow
```

## ✨ What Your Users Get

- **🌐 Easy flashing**: Visit `yoursite.com/flash` and click install
- **📱 Mobile friendly**: Works on all devices (flashing needs desktop)
- **🔄 Auto-updates**: Always serves latest firmware from GitHub
- **🎨 Perfect theme**: Matches your existing WeighMyBru² design
- **🛡️ Professional UX**: Error handling, loading states, help text

## 🎉 Ready to Deploy!

Just run `deploy-to-astro.bat` and you're minutes away from having browser-based ESP32 flashing on your website!

**Questions?** Check `SETUP_GUIDE.md` for detailed instructions.

---
**Happy flashing! 🚀☕**