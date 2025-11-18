# 🚀 WeighMyBru² Website Deployment Guide

This guide will help you deploy the ESP32 Web Tools integration to your Cloudflare-hosted website.

## 📋 Prerequisites

- Cloudflare account with your domain configured
- Access to your website's file system (via FTP, git, or Cloudflare dashboard)
- Windows/Linux/macOS computer for running sync scripts

## 🗂️ File Structure

After deployment, your website should have this structure:

```
your-website/
├── flash.html                 # Main ESP32 Web Tools installation page
├── styles.css                 # Custom CSS (optional, uses Tailwind CDN)
├── releases/                  # Release files directory
│   ├── index.json            # Release index file
│   ├── latest/               # Latest release files (symlink or copy)
│   │   ├── manifest-supermini.json
│   │   ├── manifest-xiao.json
│   │   ├── firmware-supermini.bin
│   │   ├── firmware-xiao.bin
│   │   ├── bootloader.bin
│   │   ├── partitions.bin
│   │   └── spiffs.bin
│   └── v2.1.0/              # Version-specific directories
│       └── (same files as latest/)
└── assets/                   # Website assets
    ├── logo.png
    ├── supermini-board.jpg
    └── xiao-board.jpg
```

## 🎯 Deployment Steps

### Step 1: Prepare Your Local Environment

1. **Download the website files** from your WeighMyBru² project:
   ```bash
   # Copy the website directory from your project
   cp -r /path/to/weighmybru2/website/* /path/to/your-website/
   ```

2. **Make sync scripts executable** (Linux/macOS):
   ```bash
   chmod +x sync-releases.sh
   ```

### Step 2: Initial Release Sync

Run the sync script to download the latest GitHub release:

**Linux/macOS:**
```bash
./sync-releases.sh
```

**Windows:**
```batch
sync-releases.bat
```

This will create:
- `releases/` directory with the latest firmware files
- `releases/latest/` pointing to the most recent version
- `releases/index.json` with release metadata

### Step 3: Upload Files to Your Website

#### Option A: Manual Upload
1. Upload all files in the `website/` directory to your website root
2. Ensure the `releases/` directory and all its contents are uploaded
3. Verify file permissions allow web server access

#### Option B: Using Git (Recommended)
1. Initialize git in your website directory:
   ```bash
   cd /path/to/your-website
   git init
   git add .
   git commit -m "Initial ESP32 Web Tools deployment"
   ```

2. Set up automated deployment via Cloudflare Pages (optional)

#### Option C: Cloudflare Dashboard
1. Use Cloudflare's file manager to upload files directly
2. Create the folder structure manually if needed

### Step 4: Configure Your Website

1. **Update the main page** to link to the flash tool:
   ```html
   <a href="/flash.html" class="btn btn-primary">
     📥 Flash WeighMyBru² Firmware
   </a>
   ```

2. **Test the installation page**:
   - Visit `https://yourwebsite.com/flash.html`
   - Verify both board options are visible
   - Check that manifest files load correctly

3. **Verify HTTPS**: ESP32 Web Tools requires HTTPS to work properly

### Step 5: Set Up Automatic Updates (Optional)

#### Option A: GitHub Actions Webhook
Add this to your GitHub repository's workflow:

```yaml
# .github/workflows/deploy-website.yml
name: Deploy to Website

on:
  release:
    types: [published]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Sync to Website
        run: |
          # Your deployment script here
          # Could use rsync, FTP, or Cloudflare API
```

#### Option B: Cloudflare Workers (Advanced)
1. Deploy the provided `cloudflare-worker.js` to handle automatic syncing
2. Set up GitHub webhook to trigger updates
3. Configure KV storage for release files

#### Option C: Cron Job (Simple)
Set up a cron job to run the sync script daily:

```bash
# Run daily at 2 AM
0 2 * * * cd /path/to/your-website && ./sync-releases.sh
```

## 🧪 Testing Your Deployment

### 1. Browser Compatibility Test
- **Chrome/Edge/Opera**: Should work perfectly
- **Firefox/Safari**: Should show "unsupported" message

### 2. ESP32 Web Tools Test
1. Connect an ESP32 board to your computer
2. Visit your flash page
3. Click "Install Firmware" for your board type
4. Follow the browser prompts
5. Verify successful installation

### 3. Version Display Test
- Check that the latest version number displays correctly
- Verify release date shows properly
- Test that GitHub API integration works

## 🔧 Troubleshooting

### Common Issues

#### "Manifest file not found"
- **Cause**: Release files not properly synced
- **Solution**: Re-run the sync script and check file permissions

#### "Browser not supported" for Chrome users
- **Cause**: Site not served over HTTPS
- **Solution**: Ensure Cloudflare SSL is enabled

#### Board not detected during flashing
- **Cause**: Driver issues or USB connection problems
- **Solution**: Update troubleshooting section on your page

#### Version info shows "Loading..." forever
- **Cause**: GitHub API rate limiting or CORS issues
- **Solution**: Implement server-side version fetching

### Debug Steps

1. **Check console logs** in browser developer tools
2. **Verify file paths** are correct and accessible
3. **Test manifest files** directly by visiting their URLs
4. **Check Cloudflare cache** settings for dynamic content

## 📱 Mobile Support

The flash page is responsive and will work on tablets, but ESP32 Web Tools requires:
- Desktop/laptop computer
- USB cable connection
- Supported browser

Consider adding a mobile-friendly message:

```html
<!-- Add to your flash.html -->
<div class="mobile-warning md:hidden bg-yellow-50 p-4 rounded-lg mb-4">
    <p>📱 <strong>Mobile Device Detected</strong></p>
    <p>ESP32 flashing requires a desktop computer with USB connection. Please visit this page on a desktop or laptop.</p>
</div>
```

## 🔄 Maintenance

### Regular Tasks

1. **Monitor releases**: Check that new GitHub releases sync properly
2. **Update documentation**: Keep troubleshooting section current
3. **Test compatibility**: Verify with different browsers periodically
4. **Check logs**: Monitor for any sync script errors

### Version Updates

When you release new firmware versions:
1. Tag the release in GitHub
2. Wait for build to complete
3. Run sync script (or let automation handle it)
4. Test the new version on your website
5. Announce the update to users

## 🎉 Go Live!

Once everything is tested and working:

1. **Update your main website** to prominently feature the flash tool
2. **Share the link** with your community
3. **Monitor usage** and gather feedback
4. **Celebrate** - you've made ESP32 flashing beginner-friendly! 🎉

## 📞 Support

If you encounter issues during deployment:

- Check the [GitHub repository](https://github.com/031devstudios/weighmybru2) for updates
- Join the [Discord community](https://discord.gg/HYp4TSEjSf) for help
- Review ESP32 Web Tools [documentation](https://esphome.github.io/esp-web-tools/)

---

**Happy Flashing! 🚀**