package com.softwinner;
/* @hide */
interface IDisplayService {
  int displayCtrl(int disp, int cmd0, int cmd1, int data);
  int getVersion();
  int getHdmiMode();
  boolean setHdmiMode(int mode);
  boolean getSmartBacklight();
  void setSmartBacklight(boolean on);
  boolean getSmartBacklightDemo();
  void setSmartBacklightDemo(boolean on);
  boolean getColorEnhance();
  void setColorEnhance(boolean on);
  boolean getColorEnhanceDemo();
  void setColorEnhanceDemo(boolean on);
  boolean getReadingMode();
  void setReadingMode(boolean on);
  int getColorTemperature();
  void setColorTemperature(int value);
  int getHSLBright();
  void setHSLBright(int value);
  int getHSLContrast();
  void setHSLContrast(int value);
  int getHSLSaturation();
  void setHSLSaturation(int value);
  int get3DMode();
  boolean set3DMode(int mode);
  int getMarginWidth();
  void setMarginWidth(int scale);
  int getMarginHeight();
  void setMarginHeight(int scale);
  boolean getHdmiFullscreen();
  void setHdmiFullscreen(boolean full);
}
