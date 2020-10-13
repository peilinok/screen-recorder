declare type TDevice = {
    /**
     * device id
     */
    id: string;
  
    /**
     * device name
     */
    name: string;
  
    /**
     * is system default device
     */
    isDefault: 0 | 1;
  };
  
  declare type TEncoder = {
    /**
     * encoder id
     */
    id: number;
  
    /**
     * encoder name
     */
    name: string;
  };
  
  /**
   * device type
   * 0 : video
   * 1 : speaker
   * 2 : microphone
   */
  declare type TDeviceType = 0 | 1 | 2;
  
  /**
   * remuxing state
   * 0 : stopped
   * 1 : remuxing
   */
  declare type TRemuxState = 0 | 1;
  
  export default class EasyRecorder {
    constructor();
  
    /**
     * Get all available speakers
     * @returns TDevice[] or error words
     */
    GetSpeakers(): TDevice[] | string;
  
    /**
     * Get all available microphones
     * @returns TDevice[] or error words
     */
    GetMics(): TDevice[] | string;
  
    /**
     * Get all available cameras
     * @returns TDevice[] or error words
     */
    GetCameras(): TDevice[] | string;
  
    /**
     * Get all available video encoders
     * @returns TEncoder[]
     */
    GetVideoEncoders(): TEncoder[];

    /**
     * Set recorder log file path
     * @param path log file path
     */
    SetLogPath(path:string):boolean;
  
    /**
     * Set duration callback function
     * @param cb callback function
     */
    SetDurationCallBack(cb: (duration: number) => void): boolean;
  
    /**
     * Set device changed callback function
     * @param cb callback function
     */
    SetDeviceChangeCallBack(cb: (type: TDeviceType) => void): boolean;
  
    /**
     * Set error callback function
     * @param cb callback function
     */
    SetErrorCallBack(cb: (error: number) => void): boolean;
  
    /**
     * Set picture preview root element
     * @param element preview root element
     */
    SetPreviewElement(element: HTMLElement): void;
  
    /**
     * Delete picture preview element
     */
    UnSetPreviewElement(): void;
  
    /**
     * Enable or disable preview
     * @param enable if enable preview
     */
    EnablePreview(enable: boolean): void;
  
    /**
     * Initialize recorder
     * @param qb quality must be 0-100
     * @param fps frame rate 10-30
     * @param output output file path
     * @param speakerName name of speaker device
     * @param speakerId id of speaker device
     * @param micName name of microphone device
     * @param micId id of microphone device
     * @param vencoderId id of video encoder id
     * @returns 0 for success, others will be error code
     */
    Init(
      qb: number,
      fps: number,
      output: string,
      speakerName: string,
      speakerId: string,
      micName: string,
      micId: string,
      vencoderId: number
    ): number;
  
    /**
     * Release recorder
     */
    Release(): boolean;
  
    /**
     * Start recording
     * Must call the initialize before
     */
    Start(): number;
  
    /**
     * Stop recording
     */
    Stop(): boolean;
  
    /**
     * Pause recording
     */
    Pause(): boolean;
  
    /**
     * Resume recording
     */
    Resume(): boolean;
  
    /**
     * Wait for timestamp
     * @param timestamp timestamp in millisecond
     */
    Wait(timestamp: number): boolean;
  
    /**
     * Get error string by specified error code
     * @param code error code
     */
    GetErrorStr(code: number): string;
  
    /**
     * Remux a file from one format to another
     * @param src source file path
     * @param dst destination file path
     * @param cbProgress remux progress callback function
     * @param cbState remux state callback function
     * @returns 0 for success, others will be error code
     */
    RemuxFile(
      src: string,
      dst: string,
      cbProgress: (src: string, progress: number, total: number) => void,
      cbState: (src: string, state: TRemuxState, error: number) => void
    ): number;
  }
  