declare module 'ffmpeg' {
  export interface VideoInfo {
    handle: string;
    duration: number;
    width: number;
    height: number;
    fps: number;
    bitrate: number;
    format: string;
    hasVideo: boolean;
    hasAudio: boolean;
    videoCodec: string;
    audioCodec: string;
    pixelFormat: string;
    sampleRate: number;
    channels: number;
  }

  export interface SuccessResult {
    success: boolean;
    message?: string;
  }

  export interface Ffmpeg {
    open(args: { path: string }): Promise<VideoInfo>;
    resize(args: { handle: string; width: number; height: number }): Promise<SuccessResult>;
    trim(args: { handle: string; start: number; end: number }): Promise<SuccessResult>;
    crop(args: { handle: string; x: number; y: number; width: number; height: number }): Promise<SuccessResult>;
    extractAudio(args: { handle: string; outputPath: string }): Promise<SuccessResult>;
    merge(args: { handle: string; otherPath: string }): Promise<SuccessResult>;
    convert(args: { handle: string; format: string }): Promise<SuccessResult>;
    thumbnail(args: { handle: string; time: number; outputPath: string }): Promise<SuccessResult>;
    save(args: { handle: string; outputPath: string }): Promise<SuccessResult>;

    // Extended APIs
    videoCodec(args: { handle: string; codec: string }): Promise<SuccessResult>;
    audioCodec(args: { handle: string; codec: string }): Promise<SuccessResult>;
    pixelFormat(args: { handle: string; format: string }): Promise<SuccessResult>;
    preset(args: { handle: string; preset: string }): Promise<SuccessResult>;
    filter(args: { handle: string; graph: string }): Promise<SuccessResult>;
    videoBitrate(args: { handle: string; bitrate: string | number }): Promise<SuccessResult>;
    audioBitrate(args: { handle: string; bitrate: string | number }): Promise<SuccessResult>;
    sampleRate(args: { handle: string; rate: number }): Promise<SuccessResult>;
    frameRate(args: { handle: string; fps: number }): Promise<SuccessResult>;
    streamCopy(args: { handle: string }): Promise<SuccessResult>;
    metadata(args: { handle: string; [key: string]: any }): Promise<SuccessResult>;
    outputFormat(args: { handle: string; format: string }): Promise<SuccessResult>;
    threads(args: { handle: string; count: number }): Promise<SuccessResult>;
  }

  const ffmpeg: Ffmpeg;
  export default ffmpeg;
}
