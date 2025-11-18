# Website Assets

This directory contains images and other assets for the WeighMyBru² website.

## Required Files

Please add these image files to this directory:

### Board Images
- `supermini-board.jpg` - Photo of ESP32-S3 Supermini board (recommended: 400x300px)
- `xiao-board.jpg` - Photo of XIAO ESP32S3 board (recommended: 400x300px)

### Branding
- `logo.png` - WeighMyBru² logo (recommended: 128x128px or larger)
- `favicon.ico` - Website favicon (16x16px, 32x32px)

## Image Guidelines

### Board Photos
- Use clear, well-lit photos showing the board clearly
- White or light background preferred
- Show the USB connector and any relevant markings
- Crop to focus on the board itself

### Logo
- Transparent background PNG preferred
- High resolution for scalability
- Should work on both light and dark backgrounds

## Optimization

Before uploading to your website, consider optimizing images:

```bash
# Using imagemagick to optimize
convert supermini-board.jpg -quality 85 -resize 400x300 supermini-board.jpg
convert xiao-board.jpg -quality 85 -resize 400x300 xiao-board.jpg

# Using webp for better compression (optional)
cwebp -q 85 supermini-board.jpg -o supermini-board.webp
cwebp -q 85 xiao-board.jpg -o xiao-board.webp
```

## Fallbacks

If you don't have actual board photos yet, you can:

1. Use placeholder images with the board names
2. Find Creative Commons images online
3. Use manufacturer product photos (check licensing)
4. Generate simple graphics with the board names

The website will still function without images - they'll show as empty placeholders.