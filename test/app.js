import ffmpeg from "@voidwasm/ffmpeg";

async function test() {
  console.log("1. Probing video file metadata via open()...");
  const info = await ffmpeg.open({ path: "vid.mp4" });
  console.log("   Full probed metadata:", JSON.stringify(info, null, 2));

  console.log("\n2. Benchmark 1: Single-Threaded Transcoding (count: 1)...");
  const infoSingle = await ffmpeg.open({ path: "vid.mp4" });
  await ffmpeg.threads({ handle: infoSingle.handle, count: 1 });
  await ffmpeg.resize({ handle: infoSingle.handle, width: 640, height: 360 });
  
  const startSingle = performance.now();
  await ffmpeg.save({ handle: infoSingle.handle, outputPath: "single_thread.mp4" });
  const endSingle = performance.now();
  const timeSingle = (endSingle - startSingle).toFixed(2);
  console.log(`   Single-Threaded Save Time: ${timeSingle} ms`);

  console.log("\n3. Benchmark 2: Multi-Threaded Transcoding (count: 0 / auto)...");
  const infoMulti = await ffmpeg.open({ path: "vid.mp4" });
  await ffmpeg.threads({ handle: infoMulti.handle, count: 0 }); // 16 CPU cores/threads
  await ffmpeg.resize({ handle: infoMulti.handle, width: 640, height: 360 });
  
  const startMulti = performance.now();
  await ffmpeg.save({ handle: infoMulti.handle, outputPath: "multi_thread.mp4" });
  const endMulti = performance.now();
  const timeMulti = (endMulti - startMulti).toFixed(2);
  console.log(`   Multi-Threaded Save Time: ${timeMulti} ms`);

  console.log("\n4. Benchmark Comparison:");
  console.log(`   - Single-Threaded: ${timeSingle} ms`);
  console.log(`   - Multi-Threaded:  ${timeMulti} ms`);

  console.log("\n5. Testing fast streamCopy() remuxing...");
  const copyInfo = await ffmpeg.open({ path: "vid.mp4" });
  await ffmpeg.streamCopy({ handle: copyInfo.handle });
  await ffmpeg.save({ handle: copyInfo.handle, outputPath: "copy.mp4" });
  console.log("   streamCopy completed successfully.");

  console.log("\nAll benchmarks and API tests completed successfully!");
}

test().catch(err => {
  console.error("Test failed with error:", err);
});