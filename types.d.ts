declare module 'ffmpeg' {
  export interface VideoInfo {
    handle: string;
    width: number;
    height: number;
    duration: number;
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
  }

  const ffmpeg: Ffmpeg;
  export default ffmpeg;
}
