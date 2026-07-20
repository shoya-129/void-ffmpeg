# @voidwasm/ffmpeg

A high-performance WebAssembly FFmpeg plugin built with the **Void** framework and executed via `@voidwasm/runtime` inside Node.js (functioning seamlessly as a Void plugin, just like Vite and Vite plugins). It enables native, cross-platform media processing—such as video resizing, cropping, trimming, video concatenation/merging, thumbnail extraction, and audio isolation—directly in JavaScript through a synchronous host filesystem bridge.

---

## 🚀 Installation & Setup

1. **Add the plugin to your Void application project**:
   ```bash
   npx void add @voidwasm/void-ffmpeg
   ```
2. **Import and use the plugin** in your Javascript/Typescript files:
   ```javascript
   import ffmpeg from "@voidwasm/ffmpeg";
   ```

---

## 📖 API Usage Guide

The plugin exposes a clean, promise-based API with the following methods:

### 1. `open(args: { path: string })`
Opens a video file and retrieves its metadata (dimensions, duration, and a unique session handle).
```javascript
const info = await ffmpeg.open({ path: "input.mp4" });
console.log(info);
// Output: { handle: "video_1", width: 1920, height: 1080, duration: 5.758 }
```

### 2. `thumbnail(args: { handle: string; time: number; outputPath: string })`
Extracts a frame at a specific timestamp and saves it as a JPEG image.
```javascript
await ffmpeg.thumbnail({
  handle: info.handle,
  time: 2.5, // Extract frame at t = 2.5 seconds
  outputPath: "thumbnail.jpg"
});
```

### 3. `extractAudio(args: { handle: string; outputPath: string })`
Extracts the audio track from the video and saves it to the output path (supports copy encoding).
```javascript
await ffmpeg.extractAudio({
  handle: info.handle,
  outputPath: "audio.mp3"
});
```

### 4. `merge(args: { handle: string; otherPath: string })`
Queues a second video file to be merged/concatenated sequentially. Call `save()` afterwards to perform the action.
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
Queues a resolution change. Call `save()` afterwards to transcode.
```javascript
await ffmpeg.resize({ handle: info.handle, width: 640, height: 480 });
```

### 6. `crop(args: { handle: string; x: number; y: number; width: number; height: number })`
Queues a cropping transformation. Call `save()` afterwards to transcode.
```javascript
await ffmpeg.crop({
  handle: info.handle,
  x: 100,
  y: 100,
  width: 400,
  height: 300
});
```

### 7. `trim(args: { handle: string; start: number; end: number })`
Queues a duration trim. Call `save()` afterwards to transcode.
```javascript
await ffmpeg.trim({
  handle: info.handle,
  start: 1.5, // Start at 1.5s
  end: 4.5    // End at 4.5s
});
```

### 8. `convert(args: { handle: string; format: string })`
Queues a container format conversion. Call `save()` afterwards to transcode.
```javascript
await ffmpeg.convert({ handle: info.handle, format: "mp4" });
```

### 9. `save(args: { handle: string; outputPath: string })`
Executes all queued transformations (resize, crop, trim, convert, merge) via a transcoder pipeline and saves the result.
```javascript
// Example: Resize, crop, trim, and transcode to output.mp4
await ffmpeg.resize({ handle: info.handle, width: 320, height: 240 });
await ffmpeg.crop({ handle: info.handle, x: 0, y: 0, width: 200, height: 200 });
await ffmpeg.trim({ handle: info.handle, start: 1.0, end: 3.0 });

await ffmpeg.save({
  handle: info.handle,
  outputPath: "output.mp4"
});
```

---

## 🛠 Development & Compiling

To make changes to the C++ core or rebuild the plugin:

1. **Install SDK dependencies**:
   ```bash
   npm install
   npm run cpp-init
   ```
2. **Build/Compile to WASM**:
   ```bash
   npx void build
   ```
3. **Link/Install local build for testing**:
   ```bash
   cd test
   void add ../
   node app.js
   ```
