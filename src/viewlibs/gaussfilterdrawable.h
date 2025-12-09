/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-05-16 14:52:37
 * @LastEditTime: 2025-12-09 15:15:27
 * @FilePath: /cy_frame/src/viewlibs/gaussfilterdrawable.h
 * @Description: 高斯模糊
 * @BugList:1、不使用fromview的方法，待测试
 *          2、不适用fromview的方法，scale的值为3时，概率会出问题，待处理
 * 
 * Copyright (c) 2025 by cy, All Rights Reserved. 
 * 
*/

#ifndef __GAUSS_FILTER_DRAWABLE_H__
#define __GAUSS_FILTER_DRAWABLE_H__
// #include <drawables/drawable.h>
#include <view/view.h>

class GaussFilterDrawable:public Drawable{

private:
    unsigned char *mBitmapData;
    unsigned char *mGaussData;
    
    View        *mFromView;
    int         mGaussRadius;
    double      mScale;     // 先将图层进行缩放，再进行模糊计算

    Rect        mGaussRegion;
    int         mGaussWidth;
    int         mGaussHeight;
    bool        mFirstDraw;
    Cairo::RefPtr<Cairo::ImageSurface>mBitmap;
    Cairo::RefPtr<Cairo::ImageSurface>mDrawBitmap;
    Cairo::RefPtr<Cairo::ImageSurface>mGaissBitmap;

protected:

    int mRadii[4];

    int mPaddingLeft;
    int mPaddingRight;
    int mPaddingTop;
    int mPaddingBottom;

    int mMaskColor;

public:
    // fromview：模糊图像的源控件
    // rect：模糊图像，在fromview相对的位置以及大小
    // ksize：模糊半径（越大越模糊）
    // scale：先将图像缩小的倍率（加快模糊时间，但失真更严重，建议0.3 - 0.5，越小越模糊）
    // maskColor：蒙版的颜色
    GaussFilterDrawable(View *fromView,Rect rect,int ksize,double scale /* = 2*/,int maskColor/* = 0x66000000 */ , bool isGauss = true);
    
    // rect：模糊图像，在fromview相对的位置以及大小
    // ksize：模糊半径（越大越模糊）
    // scale：先将图像缩小的倍率（加快模糊时间，但失真更严重，建议2-3，越大越模糊）
    // maskColor：蒙版的颜色
    // 该版本图像源从canvas、获取
    GaussFilterDrawable(Rect rect,int ksize,double scale /* = 2*/,int maskColor/* = 0x66000000 */);
    
    ~GaussFilterDrawable();
    void setGaussBitmip(Cairo::RefPtr<Cairo::ImageSurface> &bmp,Rect rect);
    void setGaussBitmip(View *fromView,Rect rect);
    void setGaussRadius(int radius);    // 设置 高斯半径（越高越模糊）
    void setGaussRegion(Rect rect);     // 设置 高斯模糊 的区域
    void computeBitmapGasuss();         // 计算 高斯模糊
    void computeBitmapSize();           // 计算 size

    void setCornerRadii(int radius);    // 设置 四边 的圆角（适合用作弹窗时使用）
    void setCornerRadii(int topLeftRadius,int topRightRadius,int bottomRightRadius,int bottomLeftRadius); // 分别设置 四边 的圆角
    void setPadding(int LeftPadding,int topPadding,int rightPadding,int bottomPadding); // 设置四边padding（若以上的圆角设置负值，则需配合padding来实现）

    void draw(Canvas&canvas)override;
};

#endif

