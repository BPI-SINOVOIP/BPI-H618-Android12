#ifndef __HDR_H___
#define __HDR_H___

#define  MAX(a,b)              (((a) > (b)) ? (a) : (b))
#define  MIN(a,b)              (((a) < (b)) ? (a) : (b))

#define Radius(u,v)            ((Au=Abs((u))) > (Av=Abs((v))) ? \
                                (Aw=Av/Au, Au*sqrt_new(1.0+Aw*Aw)) : \
                                (Av ? (Aw=Au/Av, Av*sqrt_new(1.+Aw*Aw)) : 0.0f) )

#define Sign(u,v)               ( (v)>=0.0 ? Abs(u) : -Abs(u) )

#define Max(u,v)                ( (u)>(v)?  (u) : (v) )

#define Abs(x)                  ( (x)>0.0f?  (x) : (-(x)) )

#define Sqrt(a, b)              (MAX(a,b) + MIN(a,b)/2)

#define  CLIP(a,i,s)            (((a) > (s)) ? (s) : MAX(a,i))

struct HarrisPoint
{
    int row;
    int col;
    float row_sub;
    float col_sub;
};

struct NormPoint
{
    double x;
    double y;
    double w;
};

struct PixelPoint
{
    int x;
    int y;
};

struct LineSegment
{
    PixelPoint corner[2];
};

struct Quadrilateral
{
    PixelPoint corner[4];
};

struct Octagon
{
    int numgon;
    PixelPoint corner[8];
};

struct Rectangle
{
    int xmin;
    int xmax;
    int ymin;
    int ymax;
};


extern "C" void ImgRGB2NV21_neon(unsigned char *pu8RgbBuffer, unsigned char *pu8SrcYUV, int *l32Width_stride, int l32Height);

extern "C" void ImgNV212RGB_neon(unsigned char *pu8RgbBuffer, int pu8SrcYUV, int l32Width, int l32Height);

extern void homology_display(double *matH);

extern void GetExposureGain(const int width, const int height, unsigned int *AeStat, unsigned int *HistStat,
                        double *gain_dark_ptr, double *gain_bright_ptr);

extern void captureHDR(void * DarkYuv, void * LightYuv, void * TransYuv, void * HDRYuv, int * HDRMode_ptr,
                const int width, const int height, const double gain_bright, const double gain_dark);

extern void captureDenoise(void * Image0Yuv, void * Image1Yuv, void * Image2Yuv, void * Image3Yuv, void * Image4Yuv, int * DenosieMode_ptr, const int width, const int height);

#endif     // __HDR_H___