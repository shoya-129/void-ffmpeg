import ffmpeg from "@voidwasm/ffmpeg";

async function test() {
  console.log("1. Probing video file metadata via open()...");
  const info = await ffmpeg.open({ path: "vid.mp4" });
  console.log("   Full probed metadata:", JSON.stringify(info, null, 2));

  console.log("\n2. Generating thumbnail at t = 2.5s...");
  const thumbResult = await ffmpeg.thumbnail({
    handle: info.handle,
    time: 2.5,
    outputPath: "thumb.jpg",
  });
  console.log("   Thumbnail result:", thumbResult);

  console.log("\n3. Testing transformation APIs (resize, crop, trim)...");
  await ffmpeg.resize({ handle: info.handle, width: 320, height: 240 });
  await ffmpeg.crop({ handle: info.handle, x: 10, y: 10, width: 200, height: 150 });
  await ffmpeg.trim({ handle: info.handle, start: 1.0, end: 3.0 });
  console.log("   Transformations queued.");

  console.log("\n4. Testing advanced encoding APIs (videoCodec, audioCodec, bitrates, filter, preset, metadata)...");
  await ffmpeg.videoCodec({ handle: info.handle, codec: "mpeg4" });
  await ffmpeg.audioCodec({ handle: info.handle, codec: "aac" });
  await ffmpeg.pixelFormat({ handle: info.handle, format: "yuv420p" });
  await ffmpeg.preset({ handle: info.handle, preset: "medium" });
  await ffmpeg.filter({ handle: info.handle, graph: "scale=320:240" });
  await ffmpeg.videoBitrate({ handle: info.handle, bitrate: "2M" });
  await ffmpeg.audioBitrate({ handle: info.handle, bitrate: "192k" });
  await ffmpeg.sampleRate({ handle: info.handle, rate: 48000 });
  await ffmpeg.frameRate({ handle: info.handle, fps: 30 });
  await ffmpeg.outputFormat({ handle: info.handle, format: "mp4" });
  await ffmpeg.metadata({
    handle: info.handle,
    title: "Test Video",
    author: "Void FFmpeg Plugin"
  });
  console.log("   Advanced options configured successfully.");

  console.log("\n5. Saving processed video...");
  const saveRes = await ffmpeg.save({
    handle: info.handle,
    outputPath: "edited.mp4",
  });
  console.log("   Save result:", saveRes);

  console.log("\n6. Testing fast streamCopy() remuxing...");
  const copyInfo = await ffmpeg.open({ path: "vid.mp4" });
  await ffmpeg.streamCopy({ handle: copyInfo.handle });
  await ffmpeg.metadata({ handle: copyInfo.handle, title: "Remuxed Copy" });
  const copyRes = await ffmpeg.save({
    handle: copyInfo.handle,
    outputPath: "copy.mp4",
  });
  console.log("   streamCopy result:", copyRes);

  console.log("\n7. Testing video merge...");
  const infoForMerge = await ffmpeg.open({ path: "vid.mp4" });
  await ffmpeg.merge({
    handle: infoForMerge.handle,
    otherPath: "vid.mp4",
  });
  const mergeSaveRes = await ffmpeg.save({
    handle: infoForMerge.handle,
    outputPath: "merged.mp4",
  });
  console.log("   Merge result:", mergeSaveRes);

  console.log("\n8. Testing audio extraction...");
  const infoForAudio = await ffmpeg.open({ path: "vid.mp4" });
  const audioRes = await ffmpeg.extractAudio({
    handle: infoForAudio.handle,
    outputPath: "output.mp3",
  });
  console.log("   Audio extraction result:", audioRes);

  console.log("\nAll 24 FFmpeg APIs tested successfully!");
}

test().catch(err => {
  console.error("Test failed with error:", err);
});