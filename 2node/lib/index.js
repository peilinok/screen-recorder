"use strict";
const platform = process.platform;
let recorder = undefined

if(platform === 'win32')
  recorder = require('../platform/win32/recorder.node')

if(global.navigator === undefined)
    global.navigator = {
        userAgent:'nodejs'
    }   

const GLRender = require('./glrender');
const SoftRender = require('./render');

class EasyRecorder {

    constructor(){
        this.previewEnable = false;

        this.glRender = new GLRender();
        this.softRender = new SoftRender();

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
          this.softRender.Render(size,width,height,type,data);
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
        this.softRender.Bind(element);
    }

    /**
     * un set preview element
     */
    UnSetPreviewElement(){
        this.softRender.UnBind();
    }

    /**
     * 
     * @param {boolean} enable 
     */
    EnablePreview(enable = false){
        this.previewEnable = enable
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
        let ret = recorder.Init(qb,fps,output,speakerName,speakerId,micName,micId);
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