# @voidwasm/ffmpeg

A high-performance WebAssembly FFmpeg plugin built with the **Void** framework and executed via `@voidwasm/runtime` inside Node.js (functioning seamlessly as a Void plugin, just like Vite and Vite plugins). It enables native, cross-platform media processing—such as video resizing, cropping, trimming, video concatenation/merging, thumbnail extraction, and audio isolation—directly in JavaScript through a synchronous host filesystem bridge.

---

## 🚀 Installation & Setup

1. **Add the plugin to your Void application project**:
   ```bash
   npx void add @voidwasm/ffmpeg
   ```
2. **Import and use the plugin** in your JavaScript/TypeScript files:
   ```javascript
   import ffmpeg from "@voidwasm/ffmpeg";
   ```

---

## 📖 API Reference

### 1. `open(args: { path: string })`
Opens a video file and probes comprehensive stream metadata.
```javascript
const info = await ffmpeg.open({ path: "input.mp4" });
console.log(info);
/*
Output:
{
  "handle": "video_1",
  "duration": 5.758,
  "width": 1920,
  "height": 1080,
  "fps": 29.97,
  "bitrate": 3956897,
  "format": "mov,mp4,m4a,3gp,3g2,mj2",
  "hasVideo": true,
  "hasAudio": true,
  "videoCodec": "h264",
  "audioCodec": "aac",
  "pixelFormat": "yuv420p",
  "sampleRate": 44100,
  "channels": 2
}
*/
```

### 2. `thumbnail(args: { handle: string; time: number; outputPath: string })`
Extracts a video frame at a given time timestamp (in seconds) and outputs a JPEG image.
```javascript
await ffmpeg.thumbnail({
  handle: info.handle,
  time: 2.5,
  outputPath: "thumbnail.jpg"
});
```

### 3. `extractAudio(args: { handle: string; outputPath: string })`
Isolates and extracts the audio stream into a standalone file.
```javascript
await ffmpeg.extractAudio({
  handle: info.handle,
  outputPath: "audio.mp3"
});
```

### 4. `merge(args: { handle: string; otherPath: string })`
Queues a second video file to be merged sequentially. Call `save()` afterwards to perform the operation.
```javascript
await ffmpeg.merge({
  handle: info.handle,
  otherPath: "second_video.mp4"
});
await ffmpeg.save({
  handle: info.handle,
  outputPath: "merged_output.mp4"
});
```

### 5. `resize(args: { handle: string; width: number; height: number })`
Queues a target resolution change.
```javascript
await ffmpeg.resize({ handle: info.handle, width: 640, height: 480 });
```

### 6. `crop(args: { handle: string; x: number; y: number; width: number; height: number })`
Queues a rectangular crop area.
```javascript
await ffmpeg.crop({ handle: info.handle, x: 100, y: 100, width: 400, height: 300 });
```

### 7. `trim(args: { handle: string; start: number; end: number })`
Queues a duration trim range (in seconds).
```javascript
await ffmpeg.trim({ handle: info.handle, start: 1.5, end: 4.5 });
```

### 8. `convert(args: { handle: string; format: string })`
Queues a target container format.
```javascript
await ffmpeg.convert({ handle: info.handle, format: "mp4" });
```

### 9. `outputFormat(args: { handle: string; format: string })`
Sets the explicit output container / format (alias/enhancement for `convert`).
```javascript
await ffmpeg.outputFormat({ handle: info.handle, format: "mp4" });
```

### 10. `videoCodec(args: { handle: string; codec: string })`
Specifies the target video encoder codec (e.g. `"libx264"`, `"mpeg4"`, `"mjpeg"`).
```javascript
await ffmpeg.videoCodec({ handle: info.handle, codec: "mpeg4" });
```

### 11. `audioCodec(args: { handle: string; codec: string })`
Specifies the target audio encoder codec (e.g. `"aac"`, `"mp3"`).
```javascript
await ffmpeg.audioCodec({ handle: info.handle, codec: "aac" });
```

### 12. `pixelFormat(args: { handle: string; format: string })`
Specifies the pixel format (e.g. `"yuv420p"`, `"yuvj420p"`).
```javascript
await ffmpeg.pixelFormat({ handle: info.handle, format: "yuv420p" });
```

### 13. `preset(args: { handle: string; preset: string })`
Configures encoding preset speed/quality tradeoff (e.g. `"slow"`, `"fast"`, `"ultrafast"`).
```javascript
await ffmpeg.preset({ handle: info.handle, preset: "slow" });
```

### 14. `filter(args: { handle: string; graph: string })`
Applies custom FFmpeg video filter graph expressions.
```javascript
await ffmpeg.filter({ handle: info.handle, graph: "scale=1280:-2,crop=500:500" });
```

### 15. `videoBitrate(args: { handle: string; bitrate: string | number })`
Configures target video bitrate (accepts `"5M"`, `"2000k"`, or raw number `2000000`).
```javascript
await ffmpeg.videoBitrate({ handle: info.handle, bitrate: "5M" });
```

### 16. `audioBitrate(args: { handle: string; bitrate: string | number })`
Configures target audio bitrate (accepts `"192k"` or raw number `192000`).
```javascript
await ffmpeg.audioBitrate({ handle: info.handle, bitrate: "192k" });
```

### 17. `sampleRate(args: { handle: string; rate: number })`
Sets audio sample rate in Hz (e.g. `48000`).
```javascript
await ffmpeg.sampleRate({ handle: info.handle, rate: 48000 });
```

### 18. `frameRate(args: { handle: string; fps: number })`
Sets target video framerate (e.g. `60`).
```javascript
await ffmpeg.frameRate({ handle: info.handle, fps: 60 });
```

### 19. `streamCopy(args: { handle: string })`
Enables fast stream copy / remuxing without re-encoding video/audio streams.
```javascript
await ffmpeg.streamCopy({ handle: info.handle });
```

### 20. `metadata(args: { handle: string; [key: string]: any })`
Sets container metadata tags (title, author, artist, comment, etc.).
```javascript
await ffmpeg.metadata({
  handle: info.handle,
  title: "Movie Title",
  author: "Soham"
});
```

### 21. `save(args: { handle: string; outputPath: string })`
Executes all queued transformations and saves the output file.
```javascript
await ffmpeg.save({
  handle: info.handle,
  outputPath: "output.mp4"
});
```

---

## 🛠 Development & Compiling

1. **Install SDK dependencies**:
   ```bash
   npm install
   npm run cpp-init
   ```
2. **Build WebAssembly Plugin**:
   ```bash
   npx void build
   ```
3. **Link and Test**:
   ```bash
   cd test
   void add ../
   node app.js
   ```
