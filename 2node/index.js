const platform = process.platform;
let recorder = undefined

if(platform === 'win32')
  recorder = require('./platform/win32/recorder.node')


class EasyRecorder {
    /**
     * @returns {{id:string,name:string,isDefault:boolean}[]}
     */
    GetSpeakers(){
        return recorder.GetSpeakers();
    }

    /**
     * @returns {{id:string,name:string,isDefault:boolean}[]}
     */
    GetMics(){
        return recorder.GetMics();
    }

    /**
     * @returns {{id:string,name:string,isDefault:boolean}[]}
     */
    GetCameras(){
        return recorder.GetCameras();
    }

    /**
     * 
     * @param {(duration:number)=>void} cb 
     */
    SetDurationCallBack(cb){
        return recorder.SetDurationCallBack(cb);
    }

    /**
     * type: 0 video | 1 speaker | 2 mic
     * @param {(type:0|1|2)=>void} cb 
     */
    SetDeviceChangeCallBack(cb){
        return recorder.SetDeviceChangeCallBack(cb);
    }

    /**
     * 
     * @param {(error:number)=>void} cb 
     */
    SetErrorCallBack(cb){
        return recorder.SetErrorCallBack(cb);
    }

    /**
     * 
     * @param {(image:{size:number,width:number,height:number,type:number,data:Buffer})=>void} cb 
     */
    SetPreviewYuvCallBack(cb){
        return recorder.SetPreviewYuvCallBack(cb);
    }

    /**
     * 
     * @param {number} qb 0-100
     * @param {number} fps 10-30
     * @param {string} output output file path
     * @param {string} speakerName 
     * @param {string} speakerId 
     * @param {string} micName 
     * @param {string} micId 
     * @returns {number} 0 succed, otherwise return error code
     */
    Init(qb,fps,output,speakerName,speakerId,micName,micId) {
        return recorder.Init(qb,fps,output,speakerName,speakerId,micName,micId);
    }

    /**
     * release recorder resources
     */
    Release(){
        return recorder.Release();
    }

    /**
     * start to record
     */
    Start(){
        return recorder.Start();
    }
    

    /**
     * stop to record
     */
    Stop(){
        return recorder.Stop();
    }

    /**
     * pause recording
     */
    Pause(){
        return recorder.Pause();
    }

    /**
     * resume recording
     */
    Resume(){
        return recorder.Resume();
    }

    /**
     * 
     * @param {number} timestamp 
     */
    Wait(timestamp){
        return recorder.Wait(timestamp);
    }
}

module.exports = EasyRecorder;