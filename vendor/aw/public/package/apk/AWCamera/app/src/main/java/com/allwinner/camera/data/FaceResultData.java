package com.allwinner.camera.data;

/**
 * 某个部位的动态贴纸数据
 */
public class FaceResultData {
    public int faceLength;                   ///< 人脸个数
    public float score;                ///< 置信度
    public float yaw;                  ///< 水平转角,真实度量的左负右正
    public float pitch;                ///< 俯仰角,真实度量的上负下正
    public float roll;                 ///< 旋转角,真实度量的左负右正

    public int getFaceLength() {
        return faceLength;
    }

    public void setFaceLength(int faceLength) {
        this.faceLength = faceLength;
    }

    public float getScore() {
        return score;
    }

    public void setScore(float score) {
        this.score = score;
    }

    public float getYaw() {
        return yaw;
    }

    public void setYaw(float yaw) {
        this.yaw = yaw;
    }

    public float getPitch() {
        return pitch;
    }

    public void setPitch(float pitch) {
        this.pitch = pitch;
    }

    public float getRoll() {
        return roll;
    }

    public void setRoll(float roll) {
        this.roll = roll;
    }
}
