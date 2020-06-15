"use strict";
const YUVBuffer = require('yuv-buffer');
const YUVCanvas = require('yuv-canvas');

class Render {
    constructor() {
        this.element = undefined;
        this.container = undefined;
        this.canvas = undefined;
        this.yuv = undefined;
    }

    /**
     * bind render element
     * @param {Element} element 
     */
    Bind(element) {
        if (element === undefined) {
            console.error("invalid bind parameter of element", element);
            return;
        }
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
        this.canvas.width = this.element.clientWidth;
        this.canvas.height = this.element.clientHeight;
        this.container.appendChild(this.canvas);
        this.yuv = YUVCanvas.attach(this.canvas, {
            webGL: true
        });
    }

    /**
     * un bind render element
     */
    UnBind() {
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
    Draw(size, width, height, type, data) {
        if (this.yuv === undefined) return;

        /*
        if (this.element.clientWidth / this.element.clientHeight > width / height) {
            this.canvas.style.zoom = this.element.clientWidth / width;
        } else {
            this.canvas.style.zoom = this.element.clientHeight / height;
        }
        */

        // auto zoom by width
        this.canvas.style.zoom = this.element.clientWidth / width;

        const format = YUVBuffer.format({
            width,
            height,
            chromaWidth: width / 2,
            chromaHeight: height / 2
        });

        const y = YUVBuffer.lumaPlane(format, data);
        const u = YUVBuffer.chromaPlane(format, data, undefined, width * height);
        const v = YUVBuffer.chromaPlane(format, data, undefined, (width * height * 5) / 4);
        const frame = YUVBuffer.frame(format, y, u, v);

        this.yuv.drawFrame(frame);
    }

    Clear() {
        if (this.yuv === undefined) return;

        this.yuv.clear();
    }

}

module.exports = Render;