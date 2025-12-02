/*
 * @Author: cy
 * @Email: patrickcchan@163.com
 * @Date: 2024-05-23 00:04:17
 * @LastEditTime: 2025-12-02 10:10:26
 * @FilePath: /cy_frame/src/windows/page_home.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Cy, All Rights Reserved.
 *
 */


#include "gaussfilterdrawable.h"

#include "page_home.h"
#include "wind_mgr.h"

HomePage::HomePage() :PageBase("@layout/page_home") {
    initUI();
}

HomePage::~HomePage() {
}

void HomePage::onTick() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick - g_window->mLastAction >= 120000) {
        if (tick - g_window->mLastAction <= 123000)
            g_window->removePop();
        g_window->showBlack();
    }
}

int8_t HomePage::getType() const {
    return PAGE_HOME;
}

void HomePage::setView() {
    mRootView->setOnClickListener([](View&){LOGE("HELLO WORLD!!!");});
    tv = (TextView *)this->findViewById(cy_frame::R::id::test1_tv);
    tv2 = (TextView *)this->findViewById(cy_frame::R::id::test2_tv);
    gaussView = this->findViewById(cy_frame::R::id::gauss);

    tv->setOnClickListener([this](View &v){
        static int index = 0;
        ((TextView &)v).setText("Test" + std::to_string(index++));
        gaussView->setBackground(new GaussFilterDrawable({0,0,-1,-1},10,0.33,0x66000000));
        gaussView->setVisibility(View::VISIBLE);
    });

    gaussView->setOnClickListener([this](View &){
        gaussView->setVisibility(View::GONE);
    });
    

    LOGE("gaussView = %p tv = %p",gaussView,tv);

    static int index = 0;
    runnable = [this]() {
        LOGE("gaussView Visibility= %d tv = %p",gaussView->getVisibility(),tv);
        if(gaussView->getVisibility() == View::VISIBLE){
            gaussView->performClick();
            mRootView->postDelayed(runnable,5000);
        }else{
            // tv->performClick();
            tv->setText("Test" + std::to_string(index++));
            tv2->setText("Cdroid" + std::to_string(index++));
            gaussView->setBackground(new GaussFilterDrawable({0,0,-1,-1},10,0.33,0x66000000));
            gaussView->setVisibility(View::VISIBLE);
            mRootView->postDelayed(runnable2,1000);
        }
        
    };

    runnable2 = [this]() {
        if(gaussView->getVisibility() == View::VISIBLE){
            tv->setText("Test" + std::to_string(index++));
            tv2->setText("Cdroid" + std::to_string(index++));
            mRootView->postDelayed(runnable2,1000);
        }
    };
    mRootView->postDelayed(runnable,2000);

}