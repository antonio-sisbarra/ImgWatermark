#include "CImg-2.2.4_pre060418/CImg.h"

using namespace cimg_library;

void computeWatermarkedImg(cil::CImg<unsigned char>& mark, cil::CImg<unsigned char>& imginp, cil::CImg<unsigned char>& imgout){

    //Compute the image with the watermark
    cimg_forXY(mark, x, y){
        //Gray value of the markimg
        int rmark = (int)mark(x, y, 0);
        int gmark = (int)mark(x, y, 1);
        int bmark = (int)mark(x, y, 2);
        int graymarkvalue = (int)(0.33*rmark + 0.33*gmark + 0.33*bmark);

        //get RGB values of input image
        int r = (int)imginp(x, y, 0);
        int g = (int)imginp(x, y, 1);
        int b = (int)imginp(x, y, 2);
        int grayinpvalue = (int)(0.33*r + 0.33*g + 0.33*b);

        //modify pixel iff there is a black pixel in watermark
        if(graymarkvalue <= 60){
            int grayavgvalue = (int)((grayinpvalue + 255) / 2);
            imgout(x, y, 0) = grayavgvalue;
            imgout(x, y, 1) = grayavgvalue;
            imgout(x, y, 2) = grayavgvalue;
        }
        else{
            imgout(x, y, 0) = imginp(x, y, 0);
            imgout(x, y, 1) = imginp(x, y, 1);
            imgout(x, y, 2) = imginp(x, y, 2);
        }
    }


}