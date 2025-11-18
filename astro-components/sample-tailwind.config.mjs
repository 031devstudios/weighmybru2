// tailwind.config.mjs - WeighMyBru² Theme Configuration
/** @type {import('tailwindcss').Config} */
export default {
  content: ['./src/**/*.{astro,html,js,jsx,md,mdx,svelte,ts,tsx,vue}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        // WeighMyBru² Brand Colors
        'black-discord': '#121214',
        'black-hover': '#1D1D1E',
        'black-select': '#363638', 
        'black-background': '#1A1A1E',
        'button-green': '#197C28',
        
        // Additional semantic colors
        primary: {
          50: '#f0fdf4',
          100: '#dcfce7',
          200: '#bbf7d0',
          300: '#86efac',
          400: '#4ade80',
          500: '#197C28', // Your main green
          600: '#16a34a',
          700: '#15803d',
          800: '#166534',
          900: '#14532d',
        },
        
        // Gray scale for dark theme
        gray: {
          50: '#f9fafb',
          100: '#f3f4f6',
          200: '#e5e7eb',
          300: '#d1d5db',
          400: '#9ca3af',
          500: '#6b7280',
          600: '#4b5563',
          700: '#374151',
          800: '#1f2937',
          900: '#111827',
        }
      },
      
      fontFamily: {
        sans: ['Inter', 'ui-sans-serif', 'system-ui', 'sans-serif'],
        mono: ['Fira Code', 'ui-monospace', 'monospace'],
      },
      
      animation: {
        'fade-in': 'fadeIn 0.6s ease-out',
        'slide-up': 'slideUp 0.4s ease-out',
        'pulse-glow': 'pulseGlow 2s infinite',
      },
      
      keyframes: {
        fadeIn: {
          '0%': { opacity: '0', transform: 'translateY(20px)' },
          '100%': { opacity: '1', transform: 'translateY(0)' },
        },
        slideUp: {
          '0%': { transform: 'translateY(100px)' },
          '100%': { transform: 'translateY(0)' },
        },
        pulseGlow: {
          '0%, 100%': { boxShadow: '0 0 20px rgba(25, 124, 40, 0.3)' },
          '50%': { boxShadow: '0 0 30px rgba(25, 124, 40, 0.6)' },
        },
      },
      
      boxShadow: {
        'glow': '0 0 20px rgba(25, 124, 40, 0.3)',
        'glow-lg': '0 0 30px rgba(25, 124, 40, 0.4)',
      },
      
      backdropBlur: {
        xs: '2px',
      },
    },
  },
  plugins: [
    // Add any additional plugins you need
    function({ addUtilities }) {
      addUtilities({
        '.text-glow': {
          textShadow: '0 0 10px rgba(25, 124, 40, 0.8)',
        },
        '.bg-glass': {
          backgroundColor: 'rgba(255, 255, 255, 0.05)',
          backdropFilter: 'blur(10px)',
          border: '1px solid rgba(255, 255, 255, 0.1)',
        },
      })
    }
  ],
}