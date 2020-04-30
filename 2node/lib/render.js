"use strict";
const YUVBuffer = require('yuv-buffer');
const YUVCanvas = require('yuv-canvas');


class SoftRender{
    constructor(){
        this.element = undefined;
        this.container = undefined;
        this.canvas = undefined;
        this.yuv = undefined;
    }


    /**
     * bind render element
     * @param {Element} element 
     */
    Bind(element){
        this.element = element;
        let container = document.createElement('div');
        Object.assign(container.style, {
            width: '100%',
            height: '100%',
            display: 'flex',
            justifyContent: 'center',
            alignItems: 'center'
        });
        this.container = container;
        element.appendChild(this.container);
        // create canvas
        this.canvas = document.createElement('canvas');
        this.container.appendChild(this.canvas);
        this.yuv = YUVCanvas.attach(this.canvas, { webGL: false });
    }

    /**
     * un bind render element
     */
    UnBind(){
        this.container && this.container.removeChild(this.canvas);
        this.element && this.element.removeChild(this.container);
        this.element = undefined;
        this.container = undefined;
        this.canvas = undefined;
        this.yuv = undefined;
    }

    /**
     * render one frame
     * @param {number} size 
     * @param {number} width 
     * @param {number} height 
     * @param {number} type 
     * @param {Buffer} data 
     */
    Render(size,width,height,type,data){
        if(this.yuv === undefined) return;

        const format = YUVBuffer.format({
            width,
            height,
            chromaWidth: width / 2,
            chromaHeight: height / 2
          });
      
          const y = YUVBuffer.lumaPlane(format, data);
          const u = YUVBuffer.chromaPlane(format, data, undefined, width * height);
          const v = YUVBuffer.chromaPlane(
            format,
            data,
            undefined,
            (width * height * 5) / 4
          );
          const frame = YUVBuffer.frame(format, y, u, v);
      
          this.yuv.drawFrame(frame);
    }

}

module.exports = SoftRender;