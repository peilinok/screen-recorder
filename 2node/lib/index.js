"use strict";
const platform = process.platform;
let recorder = undefined

if(platform === 'win32')
  recorder = require('../platform/win32/screen-recorder.node')

if(global.navigator === undefined)
    global.navigator = {
        userAgent:'nodejs'
    }   

const Render = require('./render');

class EasyRecorder {

    constructor(){
        this.previewEnable = false;

        this.render = new Render();

        this._onYuvData = this._onYuvData.bind(this);
    }

    /**
     * 
     * @param {number} size 
     * @param {number} width 
     * @param {number} height 
     * @param {number} type 
     * @param {Buffer} data 
     */
    _onYuvData(size,width,height,type,data){
        if(this.previewEnable === true)
          this.render.Draw(size,width,height,type,data);
    }

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
    _SetPreviewYuvCallBack(cb){
        return recorder.SetPreviewYuvCallBack(cb);
    }

    /**
     * set preview element
     * @param {Element} element 
     */
    SetPreviewElement(element){
        this.render.Bind(element);
    }

    /**
     * un set preview element
     */
    UnSetPreviewElement(){
        this.render.UnBind();
    }

    /**
     * Enable preview
     * @param {boolean} enable 
     */
    EnablePreview(enable = false){
        this.previewEnable = enable
        this.render.Clear();

        recorder.SetPreviewYuvEnable(enable);

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
     * @param {number} vencoderId video encoder id
     * @returns {number} 0 succed, otherwise return error code
     */
    Init(qb,fps,output,speakerName,speakerId,micName,micId,vencoderId) {
        let ret = recorder.Init(qb,fps,output,speakerName,speakerId,micName,micId,vencoderId);
        if(ret === 0){
            recorder.SetPreviewYuvCallBack(this._onYuvData);
        }

        return ret
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
        this.render.Clear();
        return recorder.Start();
    }
    

    /**
     * stop to record
     */
    Stop(){
        this.render.Clear();
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
     * wait for timestamp
     * @param {number} timestamp 
     */
    Wait(timestamp){
        return recorder.Wait(timestamp);
    }

    /**
     * trans error code to error string
     * @param {number} code 
     */
    GetErrorStr(code){
        return recorder.GetErrorStr(code);
    }

    /**
     * @returns {{id:number,name:string}[]}
     */
    GetVideoEncoders(){
        return recorder.GetVideoEncoders();
    }

    /**
     * remux file to specified path and format by dst
     * @param {string} src 
     * @param {string} dst 
     * @param {(src:string,progress:number,total:number)=>void} cbProgress 
     * @param {(src:string,state:number,error:number)=>void} cbState 
     * 
     * @returns {number} error code
     */
    RemuxFile(src,dst,cbProgress,cbState){
        return recorder.RemuxFile(src,dst,cbProgress,cbState);
    }
}

module.exports = EasyRecorder;