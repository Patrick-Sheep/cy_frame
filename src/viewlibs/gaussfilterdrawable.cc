#include <gaussfilterdrawable.h>
#include <cdlog.h>
#include <random>
// gauss
#include "gaussFilter.h"
#include "gaussianblur.h"

#define USER_NENO 1
#include <pixman.h>

#include "wind_mgr.h"
#include <cairomm/types.h>

using namespace Cairo;

GaussFilterDrawable::GaussFilterDrawable(View *fromView,Rect rect,int ksize,double scale,int maskColor, bool isGauss):mGaussRadius(ksize){
    mGaussRegion.setEmpty();
    mBitmapData = nullptr;
    mFirstDraw = false;

    mRadii[0] = mRadii[1] = 0;
    mRadii[2] = mRadii[3] = 0;

    mPaddingLeft    = mPaddingTop    = 0;
    mPaddingRight   = mPaddingBottom = 0;

    mMaskColor = maskColor;

    mScale = scale;
    mFromView = fromView;
    setGaussBitmip(mFromView,rect);

    if(isGauss) computeBitmapGasuss();
}

GaussFilterDrawable::GaussFilterDrawable(Rect rect,int ksize,double scale /* = 2*/,int maskColor/* = 0x66000000 */):mGaussRadius(ksize){
    mGaussRegion.setEmpty();
    mBitmapData = nullptr;
    mFirstDraw = true;

    mRadii[0] = mRadii[1] = 0;
    mRadii[2] = mRadii[3] = 0;

    mPaddingLeft    = mPaddingTop    = 0;
    mPaddingRight   = mPaddingBottom = 0;

    mMaskColor = maskColor;

    mScale = scale;
    
    mGaussRegion = rect;
}

GaussFilterDrawable::~GaussFilterDrawable(){
    
}

void GaussFilterDrawable::setGaussBitmip(Cairo::RefPtr<Cairo::ImageSurface> &bmp,Rect rect){
    if(rect.width == -1)    rect.width  = bmp->get_width();
    if(rect.height == -1)   rect.height = bmp->get_height();
    
    mBitmap = bmp;
    mBitmapData  = mBitmap->get_data();
    setGaussRegion(rect);
}

void GaussFilterDrawable::setGaussBitmip(View *fromView,Rect rect){
    if(rect.width == -1)    rect.width  = fromView->getWidth();
    if(rect.height == -1)   rect.height = fromView->getHeight();
    
    mBitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,fromView->getWidth(), fromView->getHeight());
    Canvas *mcanvas = new Canvas(mBitmap);
    fromView->draw(*mcanvas);
    delete mcanvas;

    mBitmapData  = mBitmap->get_data();
    setGaussRegion(rect);
}

void GaussFilterDrawable::setGaussRadius(int radius){
    if(mGaussRadius != radius){
        mGaussRadius = radius;
    }
}

void GaussFilterDrawable::setGaussRegion(Rect rect){
    if(mGaussRegion != rect){
        mGaussRegion = rect;
        computeBitmapSize();
        // 高斯模糊后图像
        mGaissBitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,mFromView->getWidth(), mFromView->getHeight());
        mGaussData = mGaissBitmap->get_data();
        mDrawBitmap = Cairo::ImageSurface::create(mGaussData+(mGaussRegion.top*mBitmap->get_stride()+mGaussRegion.left*4),Cairo::Surface::Format::ARGB32,mGaussRegion.width, mGaussRegion.height,mBitmap->get_stride());
    }
}

void GaussFilterDrawable::setCornerRadii(int radius){
    mRadii[0] = mRadii[1] = radius;
    mRadii[2] = mRadii[3] = radius;
}

void GaussFilterDrawable::setCornerRadii(int topLeftRadius,int topRightRadius,int bottomRightRadius,int bottomLeftRadius){
    mRadii[0] = topLeftRadius;
    mRadii[1] = topRightRadius;
    mRadii[2] = bottomRightRadius;
    mRadii[3] = bottomLeftRadius;
    
}

void GaussFilterDrawable::setPadding(int LeftPadding,int topPadding,int rightPadding,int bottomPadding){
    mPaddingLeft   = LeftPadding;
    mPaddingTop    = topPadding;
    mPaddingRight  = rightPadding;
    mPaddingBottom = bottomPadding;
}

void GaussFilterDrawable::computeBitmapSize(){
    if(mGaussRegion.right() > mBitmap->get_width()){
        mGaussRegion.width = mBitmap->get_width()-mGaussRegion.left;
    } 
    if(mGaussRegion.bottom() > mBitmap->get_height()){
        mGaussRegion.height = mBitmap->get_height()-mGaussRegion.top;
    } 
    mGaussWidth = std::round((float)mGaussRegion.width*mScale);
    mGaussHeight = std::round((float)mGaussRegion.height*mScale);

    if( mGaussWidth % 4 != 0 ){
        mGaussWidth = (mGaussWidth + 3) & ~3; // 4字节对齐
        mScale = (float)mGaussWidth/mGaussRegion.width;
        mGaussHeight = std::round((float)mGaussRegion.height*mScale);
    }
    LOGE("(%d, %d)mGaussWidth = %d mGaussHeight = %d mScale = %f",mGaussRegion.left,mGaussRegion.top,mGaussWidth,mGaussHeight, mScale);
}

// 手动实现像素旋转，不依赖pixman变换
void rotatePixelsDirect(uint32_t* srcData, uint32_t* dstData, int srcWidth, int srcHeight,  int rotation) {
    int64_t startTime = SystemClock::uptimeMillis();

     // 创建目标 surface
    const int swapeWH = (rotation == Display::ROTATION_90)||(rotation == Display::ROTATION_270);

    // 逻辑尺寸（旋转前的原始尺寸）
    const int logicalWidth = swapeWH ? srcHeight : srcWidth;
    const int logicalHeight = swapeWH ? srcWidth : srcHeight;

    // 创建 Pixman 图像
    pixman_image_t* srcImage = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        srcWidth, srcHeight,
        (uint32_t*)srcData, srcWidth*4);
    
    pixman_image_t* dstImage = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        logicalWidth, logicalHeight,
        (uint32_t*)dstData, logicalWidth*4);

    // 旋转, 位移
    pixman_transform_t rotate, translate;
    switch (rotation) {
        case Display::ROTATION_90: {
            pixman_transform_init_rotate(&rotate,
                                pixman_double_to_fixed(cos(M_PI*3.0/2)),
                                pixman_double_to_fixed(sin(M_PI*3.0/2)));
            // 移动回中心（新的中心）
            pixman_transform_init_translate(&translate, 
                                pixman_int_to_fixed(0),
                                pixman_int_to_fixed(logicalWidth));
            break;
        }
        
        case Display::ROTATION_180: {
            // 旋转180度
            pixman_transform_init_rotate(&rotate,
                                pixman_double_to_fixed(cos(M_PI)),
                                pixman_double_to_fixed(sin(M_PI)));
            // 移动回中心（新的中心）
            pixman_transform_init_translate(&translate, 
                                pixman_int_to_fixed(logicalWidth),
                                pixman_int_to_fixed(logicalHeight));
            break;
        }
        
        case Display::ROTATION_270: {
            // 逆时针旋转90度
            pixman_transform_init_rotate(&rotate,
                                pixman_double_to_fixed(cos(M_PI/2)),
                                pixman_double_to_fixed(sin(M_PI/2)));
            // 移动回中心（新的中心）
            pixman_transform_init_translate(&translate, 
                                pixman_int_to_fixed(logicalHeight),
                                pixman_int_to_fixed(0));
            break;
        }
    }

    // 创建变换矩阵
    pixman_transform_t transform;
    pixman_transform_init_identity(&transform);
    pixman_transform_multiply(&transform, &translate, &rotate);   
    
    // 应用变换矩阵
    pixman_image_set_transform(srcImage, &transform);
    
    // 设置过滤器质量（可选）
    pixman_image_set_filter(srcImage, PIXMAN_FILTER_BILINEAR, nullptr, 0);
    
    // 执行图像合成（旋转操作）
    pixman_image_composite(PIXMAN_OP_SRC,
                        srcImage, nullptr, dstImage,
                        0, 0, 0, 0, 0, 0,
                        logicalWidth, logicalHeight);
    
    // 清理资源
    pixman_image_unref(srcImage);
    pixman_image_unref(dstImage);
    LOGI("rotateTime = %lld", SystemClock::uptimeMillis() - startTime);
}

void GaussFilterDrawable::computeBitmapGasuss(){

    int64_t startTime = SystemClock::uptimeMillis();
    int bitmapPos,gaussPos;
    unsigned char * bitmapRGBData = (unsigned char *)malloc(mGaussWidth*mGaussHeight*4);
#if defined(USER_NENO) && (USER_NENO)
    unsigned char * bitmapRGBGaussData = (unsigned char *)malloc(mGaussWidth*mGaussHeight*4);
#endif

    // 创建一个 Pixman 图像表面(源数据的image)
    pixman_image_t* srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mBitmap->get_width(), mBitmap->get_height(), (uint32_t *)mBitmapData, mBitmap->get_width() * 4);

    // 创建目标图像
    pixman_image_t *dstImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mGaussWidth, mGaussHeight, (uint32_t *)bitmapRGBData, mGaussWidth * 4);

    // 设置缩放和转换参数
    pixman_transform_t transform;
    pixman_transform_init_scale(&transform, pixman_double_to_fixed(1.f/mScale),pixman_double_to_fixed(1.f/mScale));

    // 执行缩放和格式转换
    pixman_image_set_filter(srcImage, PIXMAN_FILTER_NEAREST, NULL, 0);
    pixman_image_set_transform(srcImage, &transform);
    pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage, std::round((float)mGaussRegion.left*mScale),std::round((float)mGaussRegion.top*mScale), 0, 0, 0, 0, mGaussWidth, mGaussHeight);

    // 创建一个 Pixman 蒙版
    pixman_color_t color_t = {
        ((mMaskColor >> 16) & 0xFF) << 8,   // 16位红色分量
        ((mMaskColor >> 8) & 0xFF) << 8,    // 16位绿色分量
        ( mMaskColor & 0xFF) << 8,          // 16位蓝色分量
        ((mMaskColor >> 24) & 0xFF) << 8}; // 16位透明度分量

    pixman_image_t* maskColorImage = pixman_image_create_solid_fill(&color_t);
    // // 将 maskColorImage 设置为 dstImage 的蒙版
    pixman_image_composite(PIXMAN_OP_OVER, maskColorImage, maskColorImage, dstImage, 0, 0, 0, 0, 0, 0, mGaussWidth, mGaussHeight);

    // // 释放资源
    pixman_image_unref(srcImage);
    pixman_image_unref(dstImage);
    pixman_image_unref(maskColorImage);

    LOGI("scale copy Time = %lld",SystemClock::uptimeMillis()-startTime);
    
    int64_t startTime2 = SystemClock::uptimeMillis();
    //////////////////////////////////////////////////////////////////////////
#if defined(USER_NENO) && (USER_NENO)
    // 使用Neon的指令集进行模糊的计算 
    gaussianFilter_u8_Neon(bitmapRGBData, bitmapRGBGaussData, mGaussHeight , mGaussWidth, 4, mGaussRadius);
#else 
    // 使用纯算法进行模糊的计算
    GaussianBlurFilter(bitmapRGBData, mGaussWidth ,mGaussHeight, mGaussRadius);
#endif
    /////////////////////////////////////////////////////////////////////////
    LOGI("gaussianFilter_u8_Neon time = %ld USER_NENO = %d",SystemClock::uptimeMillis()-startTime2,USER_NENO);
    int64_t startTime3 = SystemClock::uptimeMillis();

    // 执行缩放的image
#if defined(USER_NENO) && (USER_NENO)
    srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mGaussWidth, mGaussHeight, (uint32_t *)bitmapRGBGaussData, mGaussWidth * 4);
#else
    srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mGaussWidth, mGaussHeight, (uint32_t *)bitmapRGBData, mGaussWidth * 4);
#endif
    dstImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mBitmap->get_width(), mBitmap->get_height(),(uint32_t *)mGaussData, mBitmap->get_width() * 4);
    
    // 设置缩放和转换参数
    pixman_transform_init_scale(&transform, pixman_double_to_fixed(mScale),pixman_double_to_fixed(mScale));

    // 执行缩放和格式转换
    pixman_image_set_filter(srcImage, PIXMAN_FILTER_NEAREST, NULL, 0);
    pixman_image_set_transform(srcImage, &transform);
    pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage, 0, 0, 0, 0, mGaussRegion.left,mGaussRegion.top, mGaussRegion.width, mGaussRegion.height);

    LOGI("scale copy 2 Time = %ld",SystemClock::uptimeMillis()-startTime3);

    // // 释放资源
    pixman_image_unref(srcImage);
    pixman_image_unref(dstImage);

    free(bitmapRGBData);
#if defined(USER_NENO) && (USER_NENO)
    free(bitmapRGBGaussData);
#endif
    LOGI("gauss all  Time = %ld",SystemClock::uptimeMillis()-startTime);
}   

void GaussFilterDrawable::draw(Canvas&canvas){
    int64_t startTime = SystemClock::uptimeMillis();

    // 可以直接对canvas的部分进行模糊
    if(mFirstDraw){
        Cairo::RefPtr<Cairo::ImageSurface> canvasimg = std::dynamic_pointer_cast<Cairo::ImageSurface>(canvas.get_target());
        const int rotation  = WindowManager::getInstance().getDefaultDisplay().getRotation();
        const int swapeWH = (rotation == Display::ROTATION_90)||(rotation == Display::ROTATION_270);
        const int canvasWidth = canvasimg->get_width();      // 旋转后的宽度（物理尺寸）
        const int canvasHeight= canvasimg->get_height();    // 旋转后的高度（物理尺寸）

        // 逻辑尺寸（旋转前的原始尺寸）
        const int logicalWidth = swapeWH ? canvasHeight : canvasWidth;
        const int logicalHeight = swapeWH ? canvasWidth : canvasHeight;

        Cairo::RefPtr<Cairo::ImageSurface> rotateCanvasImg = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,logicalWidth, logicalHeight);
        
        rotatePixelsDirect((uint32_t*)canvasimg->get_data(), (uint32_t*)rotateCanvasImg->get_data(), canvasWidth, canvasHeight, rotation);

        if(mGaussRegion.width == -1)    mGaussRegion.width  = logicalWidth;
        if(mGaussRegion.height == -1)   mGaussRegion.height = logicalHeight;
        if(mBitmap == nullptr){
            // 保存原始图像数据
            mBitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,logicalWidth, logicalHeight);
            mBitmapData = mBitmap->get_data();

            // 高斯模糊后图像
            mGaissBitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,logicalWidth, logicalHeight);
            mGaussData = mGaissBitmap->get_data();

            mDrawBitmap = Cairo::ImageSurface::create(mGaussData+(mGaussRegion.top*mBitmap->get_stride()+mGaussRegion.left*4),Cairo::Surface::Format::ARGB32,mGaussRegion.width, mGaussRegion.height,mBitmap->get_stride());
            computeBitmapSize();
        }

        bool isNeedGauss = false; // 判断是否需要高斯模糊
        // 创建一个 Pixman 图像表面(源数据的image)
        pixman_image_t* srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, logicalWidth, logicalHeight, (uint32_t *)rotateCanvasImg->get_data(), rotateCanvasImg->get_stride());
#if 1
        // 获取当前绘制的裁剪区域
        try {
            std::vector<Cairo::Rectangle>clip_rects;
            Cairo::Matrix matrix2;
            
            canvas.save();
            Cairo::Matrix matrix = Cairo::identity_matrix();
            
            switch(rotation){
            case Display::ROTATION_0:break;
            case Display::ROTATION_90:
                matrix.rotate(M_PI/2);
                matrix.translate(0,-canvasHeight);
                canvas.transform(matrix);
                break;
            case Display::ROTATION_180:
                matrix.translate(canvasWidth,canvasHeight);
                matrix.scale(-1,-1);
                canvas.transform(matrix);
                break;
            case Display::ROTATION_270:
                matrix.rotate(-M_PI/2);
                matrix.translate(-canvasWidth,0);
                canvas.transform(matrix);
                break;
            }
            canvas.get_matrix(matrix2);  // 获取canvas移动的位置
            canvas.copy_clip_rectangle_list(clip_rects);
            
            for(int i=0;i<clip_rects.size();i++){
                Cairo::Rectangle r=clip_rects.at(i);
                Rect rect;
                switch(rotation){
                case Display::ROTATION_0:break;
                case Display::ROTATION_90:
                    rect = Rect::Make((int)std::round(logicalWidth - (r.y+matrix2.y0 + r.height)), (int)std::round(r.x+matrix2.x0), (int)r.height, (int)r.width);
                    break;
                case Display::ROTATION_180:
                    matrix.translate(canvasWidth,canvasHeight);
                    matrix.scale(-1,-1);
                    canvas.transform(matrix);
                    break;
                case Display::ROTATION_270:
                    rect = Rect::Make((int)std::round(r.y+matrix2.y0), (int)std::round(logicalHeight - (r.x+matrix2.x0+r.width)), (int)r.height,  (int)r.width);
                    break;
                }
                // LOGI("%d:rect=(%f,%f,%f,%f) --- (%d,%d,%d,%d)",i,r.x,r.y,matrix2.x0,matrix2.y0, rect.left, rect.top, rect.width, rect.height);
                LOGV("%d:rect=(%d,%d,%d,%d)",i, rect.left, rect.top, rect.width, rect.height);
                // 判断是否重叠
                if(mGaussRegion.contains(rect)){
                    // 获取重叠区域
                    rect.intersect(mGaussRegion);
                    // 创建目标图像
                    pixman_image_t *dstImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mBitmap->get_width(), mBitmap->get_height(), (uint32_t *)mBitmapData, mBitmap->get_width() * 4);

                    pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage, rect.left, rect.top, 0, 0, rect.left, rect.top, rect.width, rect.height);

                    pixman_image_unref(dstImage);
                    isNeedGauss = true;
                }
            }
            canvas.restore();
        } catch (const std::exception& e) {
            std::cerr << "无法获取裁剪矩形列表: " << e.what() << std::endl;
        }
#else
        // 以下做法是直接获取window层的脏区，理论上没问题，但获取window层的脏区比较麻烦，所以采用上面的做法
        Cairo::RefPtr<Cairo::Region> pendingRgn = g_window->getPendingRgn();
        int num = pendingRgn->get_num_rectangles();
        LOGV("pendingRgn --- %d:rect=(%d,%d,%d,%d)",i,r.x,r.y,r.width,r.height);
        for(int i = 0;i < num; i ++){
            RectangleInt r = pendingRgn->get_rectangle(i);
            Rect rect = Rect::Make(r.x, r.y, r.width, r.height);
            // 判断是否重叠
            if(mGaussRegion.contains(rect)){
                // 获取重叠区域
                rect.intersect(mGaussRegion);
                // 创建目标图像
                pixman_image_t *dstImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mBitmap->get_width(), mBitmap->get_height(), (uint32_t *)mBitmapData, mBitmap->get_width() * 4);

                pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage, rect.left, rect.top, 0, 0, rect.left, rect.top, r.width, r.height);

                pixman_image_unref(dstImage);
                isNeedGauss = true;
            }
        }
        
#endif
        pixman_image_unref(srcImage);
        if(isNeedGauss){
            computeBitmapGasuss();
        }
        LOGI("Copy dirty and gauss all time = %lld  isNeedGauss = %d",SystemClock::uptimeMillis() - startTime,isNeedGauss);
        mFirstDraw = true;
    }
    startTime = SystemClock::uptimeMillis();
    // 实现四边圆角（负角度仅个人项目需求：提示框的右下角是反圆角差不多的效果，因此做了个反圆角的判断）
    if(mRadii[0]||mRadii[1]||mRadii[2]||mRadii[3]){
        const double degrees = M_PI / 180.f;
        const int width = mBounds.width;
        const int height= mBounds.height;
	    canvas.begin_new_sub_path();
        // 左上角
        if(mRadii[0] >= 0 ) canvas.arc( mRadii[0]+mPaddingLeft, mRadii[0]+mPaddingTop, mRadii[0], 180 * degrees, 270 * degrees);
        else                canvas.arc_negative( mRadii[0]+mPaddingLeft, -mRadii[0]+mPaddingTop, -mRadii[0], 0 * degrees, -90 * degrees);
        // 右上角
        if(mRadii[1] > 0)   canvas.arc( width - mRadii[1]-mPaddingRight, mRadii[1]+mPaddingTop, mRadii[1], -90 * degrees, 0 * degrees);   
        else                canvas.arc_negative( width - mRadii[1]-mPaddingRight, -mRadii[1]+mPaddingTop, -mRadii[1], -90 * degrees, -180 * degrees); 
        // 右下角
        if(mRadii[2] > 0)   canvas.arc( width - mRadii[2]-mPaddingRight, height - mRadii[2]-mPaddingBottom, mRadii[2], 0 * degrees, 90 * degrees);
        else                canvas.arc_negative( width - mRadii[2]-mPaddingRight, height - (-mRadii[2])-mPaddingBottom, -mRadii[2], 180 * degrees, 90 * degrees);
        // 左下角
        if(mRadii[3] > 0)   canvas.arc( mRadii[3]+mPaddingLeft, height - mRadii[3]-mPaddingBottom, mRadii[3], 90 * degrees, 180 * degrees);
        else                canvas.arc_negative( mRadii[3]+mPaddingLeft, height - (-mRadii[3])-mPaddingBottom, -mRadii[3], 90 * degrees, 0 * degrees);
        canvas.close_path();
        canvas.clip();
    }
    canvas.save();
    canvas.rectangle(mBounds.left,mBounds.top,mBounds.width,mBounds.height); // 限制绘画区域，加快刷新速率
    canvas.clip();
    canvas.set_operator(Cairo::Context::Operator::SOURCE);  // bitmapDrawable 的做法是判断元素是否完全不透，完全不透则设置 SOURCE 模式，加快刷新速率
    canvas.set_source(mDrawBitmap,0,0);
    canvas.get_source_for_surface()->set_filter(Cairo::SurfacePattern::Filter::NEAREST);    // 设置刷新模式为 临近插值
    canvas.paint();
    canvas.restore();
    LOGE("(%d,%d,%d,%d)  get_width = %d drawTime = %lld",mBounds.left,mBounds.top,mBounds.width,mBounds.height,mDrawBitmap->get_width(), SystemClock::uptimeMillis() - startTime);
}
