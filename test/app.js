import ffmpeg from "@voidwasm/ffmpeg";

async function test() {
  console.log("1. Opening video file...");
  const info = await ffmpeg.open({ path: "vid.mp4" });
  console.log("   Video opened successfully:", info);

  console.log("\n2. Generating thumbnail at t = 2.5s...");
  
  const thumbResult = await ffmpeg.thumbnail({
    handle: info.handle,
    time: 2.5,
    outputPath: "thumb.jpg",
  });

  console.log("   Thumbnail result:", thumbResult);

  console.log("\n3. Testing resize, crop, and trim transformations...");
  const resizeRes = await ffmpeg.resize({ handle: info.handle, width: 320, height: 240 });
  const cropRes = await ffmpeg.crop({ handle: info.handle, x: 10, y: 10, width: 200, height: 150 });
  const trimRes = await ffmpeg.trim({ handle: info.handle, start: 1.0, end: 3.0 });
  console.log("   Transformations queued successfully.");

  console.log("\n4. Saving processed video...");
  const saveRes = await ffmpeg.save({
    handle: info.handle,
    outputPath: "edited.mp4",
  });
  console.log("   Save result:", saveRes);

  console.log("\n5. Testing video merge...");
  // Re-open since the previous handle was consumed/closed in save
  const infoForMerge = await ffmpeg.open({ path: "vid.mp4" });
  const mergeQueueRes = await ffmpeg.merge({
    handle: infoForMerge.handle,
    otherPath: "vid.mp4",
  });
  console.log("   Merge queued. Saving merged file...");
  const mergeSaveRes = await ffmpeg.save({
    handle: infoForMerge.handle,
    outputPath: "merged.mp4",
  });
  console.log("   Merge result:", mergeSaveRes);

  console.log("\n6. Testing audio extraction...");
  const infoForAudio = await ffmpeg.open({ path: "vid.mp4" });
  const audioRes = await ffmpeg.extractAudio({
    handle: infoForAudio.handle,
    outputPath: "output.mp3",
  });
  console.log("   Audio extraction result:", audioRes);

  console.log("\nAll functions tested successfully!");
}

test().catch(err => {
  console.error("Test failed with error:", err);
});