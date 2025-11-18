// WeighMyBru² Cloudflare Worker for GitHub Release Webhook
// This worker automatically syncs new releases to your website

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    
    // Handle GitHub webhook
    if (url.pathname === '/webhook/github' && request.method === 'POST') {
      return handleGitHubWebhook(request, env);
    }
    
    // Handle release API endpoints
    if (url.pathname.startsWith('/api/releases')) {
      return handleReleaseAPI(url, env);
    }
    
    // Default response
    return new Response('WeighMyBru² Release Sync Service', {
      headers: { 'Content-Type': 'text/plain' }
    });
  }
};

async function handleGitHubWebhook(request, env) {
  try {
    // Verify GitHub webhook signature (if configured)
    const signature = request.headers.get('X-Hub-Signature-256');
    const body = await request.text();
    
    // Parse webhook payload
    const payload = JSON.parse(body);
    
    // Only process release events
    if (payload.action !== 'published' || !payload.release) {
      return new Response('Event not processed', { status: 200 });
    }
    
    const release = payload.release;
    const tagName = release.tag_name;
    
    console.log(`Processing release: ${tagName}`);
    
    // Download and process release assets
    const syncResult = await syncReleaseToKV(release, env);
    
    if (syncResult.success) {
      return new Response(JSON.stringify({
        success: true,
        message: `Release ${tagName} synced successfully`,
        assets: syncResult.assets
      }), {
        headers: { 'Content-Type': 'application/json' }
      });
    } else {
      return new Response(JSON.stringify({
        success: false,
        error: syncResult.error
      }), {
        status: 500,
        headers: { 'Content-Type': 'application/json' }
      });
    }
    
  } catch (error) {
    console.error('Webhook error:', error);
    return new Response(JSON.stringify({
      success: false,
      error: error.message
    }), {
      status: 500,
      headers: { 'Content-Type': 'application/json' }
    });
  }
}

async function handleReleaseAPI(url, env) {
  const pathParts = url.pathname.split('/');
  
  try {
    // GET /api/releases - List all releases
    if (pathParts.length === 3) {
      const releases = await env.RELEASES.list();
      const releaseList = releases.keys
        .filter(key => key.name.startsWith('release:'))
        .map(key => key.name.replace('release:', ''))
        .sort((a, b) => b.localeCompare(a)); // Sort versions descending
      
      return new Response(JSON.stringify({
        releases: releaseList,
        latest: releaseList[0] || null,
        count: releaseList.length
      }), {
        headers: { 'Content-Type': 'application/json' }
      });
    }
    
    // GET /api/releases/latest - Get latest release
    if (pathParts[3] === 'latest') {
      const latest = await getLatestRelease(env);
      if (latest) {
        return new Response(JSON.stringify(latest), {
          headers: { 'Content-Type': 'application/json' }
        });
      } else {
        return new Response(JSON.stringify({
          error: 'No releases found'
        }), {
          status: 404,
          headers: { 'Content-Type': 'application/json' }
        });
      }
    }
    
    // GET /api/releases/{version} - Get specific release
    const version = pathParts[3];
    if (version) {
      const release = await env.RELEASES.get(`release:${version}`, 'json');
      if (release) {
        return new Response(JSON.stringify(release), {
          headers: { 'Content-Type': 'application/json' }
        });
      } else {
        return new Response(JSON.stringify({
          error: `Release ${version} not found`
        }), {
          status: 404,
          headers: { 'Content-Type': 'application/json' }
        });
      }
    }
    
  } catch (error) {
    return new Response(JSON.stringify({
      error: error.message
    }), {
      status: 500,
      headers: { 'Content-Type': 'application/json' }
    });
  }
  
  return new Response('Invalid API endpoint', { status: 404 });
}

async function syncReleaseToKV(release, env) {
  try {
    const tagName = release.tag_name;
    const assets = [];
    
    // Process each asset
    for (const asset of release.assets) {
      const assetName = asset.name;
      const downloadUrl = asset.browser_download_url;
      
      console.log(`Downloading asset: ${assetName}`);
      
      // Download asset
      const response = await fetch(downloadUrl);
      if (!response.ok) {
        throw new Error(`Failed to download ${assetName}: ${response.status}`);
      }
      
      const content = await response.arrayBuffer();
      
      // Store in KV with asset name as key
      const kvKey = `asset:${tagName}:${assetName}`;
      await env.RELEASES.put(kvKey, content);
      
      assets.push({
        name: assetName,
        size: content.byteLength,
        kvKey: kvKey
      });
    }
    
    // Store release metadata
    const releaseData = {
      tag_name: tagName,
      name: release.name,
      body: release.body,
      published_at: release.published_at,
      assets: assets,
      html_url: release.html_url
    };
    
    await env.RELEASES.put(`release:${tagName}`, JSON.stringify(releaseData));
    
    // Update latest pointer
    await env.RELEASES.put('latest', tagName);
    
    console.log(`Successfully synced release ${tagName} with ${assets.length} assets`);
    
    return {
      success: true,
      assets: assets.length,
      version: tagName
    };
    
  } catch (error) {
    console.error('Sync error:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

async function getLatestRelease(env) {
  try {
    const latestVersion = await env.RELEASES.get('latest');
    if (!latestVersion) return null;
    
    const release = await env.RELEASES.get(`release:${latestVersion}`, 'json');
    return release;
  } catch (error) {
    console.error('Error getting latest release:', error);
    return null;
  }
}

// Helper function to verify GitHub webhook signature
async function verifyGitHubSignature(body, signature, secret) {
  if (!signature || !secret) return false;
  
  const encoder = new TextEncoder();
  const key = await crypto.subtle.importKey(
    'raw',
    encoder.encode(secret),
    { name: 'HMAC', hash: 'SHA-256' },
    false,
    ['sign']
  );
  
  const computedSignature = await crypto.subtle.sign('HMAC', key, encoder.encode(body));
  const computedHex = 'sha256=' + Array.from(new Uint8Array(computedSignature))
    .map(b => b.toString(16).padStart(2, '0'))
    .join('');
  
  return computedHex === signature;
}