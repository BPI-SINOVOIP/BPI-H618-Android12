#extension GL_OES_EGL_image_external : require
 precision highp float;
 varying highp vec2 vTextureCoord;

 uniform samplerExternalOES texture;

  uniform highp float scaleRatio;// 缩放系数，0无缩放，大于0则放大

  uniform highp float radius;// 缩放算法的作用域半径

  uniform highp vec2 leftEyeCenterPosition; // 左眼控制点，越远变形越小

  uniform highp vec2 rightEyeCenterPosition; // 右眼控制点

  uniform float aspectRatio; // 所处理图像的宽高比
  uniform sampler2D textureY;
  uniform sampler2D textureUV;
    uniform vec2 leftContourPoints_vec[3];
    uniform vec2 rightContourPoints_vec[3];
   // uniform bool boolFaceScale;

 uniform float faceWarpRadius;     // 形变半径   ,origin 0.17  ,0.1 bearly ,0.15 middle
 uniform float deltaArray[3];       // strength, 形变量，要大于radius，good at 3 times radius
 const int arraySize = 3;



    //  Scale Function for eyes
 highp vec2 warpScaleToUse(vec2 centerPostion, vec2 currentPosition, float radius, float scaleRatio, float aspectRatio){
  vec2 positionToUse = currentPosition;
  vec2 currentPositionToUse = vec2(currentPosition.x, currentPosition.y * aspectRatio + 0.5 - 0.5 * aspectRatio);
  vec2 centerPostionToUse = vec2(centerPostion.x, centerPostion.y * aspectRatio + 0.5 - 0.5 * aspectRatio);
  float r = distance(currentPositionToUse, centerPostionToUse);
  if(r < radius){
   float alpha = 1.0 - scaleRatio * pow(r / radius - 1.0, 2.0);
   positionToUse = centerPostion + alpha * (currentPosition - centerPostion);
  }
  return positionToUse;
 }

    // Reposition Function
    // warp points in certain radius
    // this method perform great in effect, but a little bitte expensive
 highp vec2 warpPositionToUse(vec2 currentPoint, vec2 contourPointA,  vec2 contourPointB, float radius, float delta, float aspectRatio){
  vec2 positionToUse = currentPoint;
  vec2 currentPointToUse = vec2(currentPoint.x, currentPoint.y * aspectRatio + 0.5 - 0.5 * aspectRatio);
  vec2 contourPointAToUse = vec2(contourPointA.x, contourPointA.y * aspectRatio + 0.5 - 0.5 * aspectRatio);
  // The calculation below refers to thesis "Interactive Image Warping - Andreas Gustafson" on page 38
  float r = distance(currentPointToUse, contourPointAToUse);
  if(r < radius){
   vec2 dir = normalize(contourPointB - contourPointA);
   float dist = radius * radius - r * r;
   float alpha = dist / (dist + (r-delta) * (r-delta));
   alpha = alpha * alpha;
   positionToUse = positionToUse - alpha * delta * dir;
  }
  return positionToUse;
 }

    // Reposition Function
    // Mesh warp
 highp vec2 meshWarp(vec2 textureCoord, vec2 originPosition, vec2 targetPosition, float radius, float curve)
    {
     vec2 direction = targetPosition - originPosition;
     float lengthA = length(direction);
     if(lengthA<0.0001)   return direction;
     float lengthB = min(lengthA, radius);
     direction *= lengthB / lengthA;
     float infect = distance(textureCoord, originPosition)/radius;
     infect = clamp(1.0-infect,0.0,1.0);
     infect = pow(infect, curve);
     return direction * infect;
    }

    // Test Code for Reposition
 highp vec2 reposition(vec2 currentPosition,vec2 rightEyeCenter,vec2 leftEyeCenter){
     float warpStrength = 5.2;
     float radius = 0.1;

  vec2 curCoord = currentPosition;
        vec2 srcPoint = vec2(0.0);
        vec2 dstPoint = vec2(0.0);
        srcPoint = rightEyeCenter;
        dstPoint = srcPoint+(leftEyeCenter-srcPoint)*warpStrength;

        vec2 offset = meshWarp(curCoord,srcPoint,dstPoint,radius,1.0);
        curCoord = curCoord-offset;
  return curCoord;
 }

 void main(){
  vec2 positionToUse = vTextureCoord;
  float r, g, b, y, u, v;
  float y2, u2, v2;



          y = texture2D(textureY, positionToUse).r;
          u = texture2D(textureUV, positionToUse).a - 0.5;
          v = texture2D(textureUV, positionToUse).r - 0.5;
          r = y + 1.402*v;
          g = y - 0.34414*u - 0.71414*v;
          b = y + 1.772*u;
          r = clamp(r, 0.0, 1.0);
          g = clamp(g, 0.0, 1.0);
          b = clamp(b, 0.0, 1.0);


  if((abs(rightEyeCenterPosition.x -vTextureCoord.x)<0.03) &&(abs(rightEyeCenterPosition.y -vTextureCoord.y) <=0.03) ){
  r=0.0;
  g =1.0;
  b = 0.0;
  } else  if((abs(leftEyeCenterPosition.x -vTextureCoord.x)<0.03) &&(abs(leftEyeCenterPosition.y -vTextureCoord.y) <=0.03) ){
              r=0.0;
              g =0.0;
              b = 1.0;
              }


          y2 = 0.299*r + 0.587*g + 0.114*b;
          u2 = - 0.1687*r - 0.3313*g + 0.5*b+0.5;
          v2 = 0.5*r - 0.4187*g - 0.0813*b+0.5;
          gl_FragColor =vec4(y2,u2,v2,1.0);
  //gl_FragColor = texture2D(texture, positionToUse);
 }