#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>
#include <string>
#include <experimental/filesystem>

#define cimg_use_jpeg  //to use native library to convert imgs
#include "CImg.h"

#include <unistd.h>

//For marking imgs"
#include "watermarkUtil.cpp"

//For util functions
#include "utils.cpp"

using namespace cimg_library;



int main(int argc, char *argv[]) {   
    std::string markImgFilename, dirInput, dirOutput, dirOutputName;

    //Useful for computing tGen, tRead, tMark and tWrite in avg of a single image
    float tGen = 0, tRead = 0, tMark = 0, tWrite = 0;

    if (argc<3 || argc>4) {
        std::cerr << "use: " << argv[0]  << " markimgfile dirinput [diroutput] \n";
        return -1;
    }

    //dirOutputName is useful for saving the images in output. It's not the path, but just a name
    if (argc == 4){ 
        dirOutput = GetCurrentWorkingDir().append("/").append(argv[3]);

        //Check if arg ends with /
        if(dirOutput.back() != '/')
            dirOutputName = std::string(argv[3]).append("/");
        else
            dirOutputName = std::string(argv[3]);
    }
    else{
        dirOutput = GetCurrentWorkingDir();
        dirOutputName = std::string(" ");
    }
    
    markImgFilename = argv[1];
    dirInput = GetCurrentWorkingDir().append("/").append(argv[2]);

    std::cout << "dirInput is: " << dirInput << "\n";
    std::cout << "dirOutput is: " << dirOutput << "\n";

    if(checkParams(markImgFilename.c_str(), dirInput.c_str(), dirOutput.c_str()) == false){
        std::cerr << "problems in the params \n";
        return -1;
    }

    //Useful to understand weight of r/w ops
    int totreadwrite = 0, totread = 0, totwrite = 0;

    //To count the photos marked
    int totphotomarked = 0;


    auto start = std::chrono::high_resolution_clock::now();

    //Read markimgfile and initialize input img
    std::cout << "Reading markimg file...\n";
    const char *file_markimg = cimg_option("-markimg", (markImgFilename).c_str(), "Watermark Image");
    CImg<unsigned char> markimg(file_markimg);
    std::cout << "Reading markimg ok, now starting reading images...\n";
    const char *file_inpimg, *file_outimg;
    CImg<unsigned char> imginp, imgout;

    //Useful for reading imgs
    std::string imginpname(argv[2]), imginpname_actual, dirOutputName_actual, fileoutputname;

    //Check if arg ends with /
    if(imginpname.back() != '/')
        imginpname.append("/");

    std::cout << "Working on imgs...\n";

    //Loop on all images
    auto startGenerating = std::chrono::high_resolution_clock::now();
    for (auto & p : std::experimental::filesystem::directory_iterator(dirInput)){
        auto elapsedGenerating = std::chrono::high_resolution_clock::now() - startGenerating;
        auto usecGenerating = std::chrono::duration_cast<std::chrono::microseconds>(elapsedGenerating).count();
        tGen += usecGenerating;

        try{

            auto startloading = std::chrono::high_resolution_clock::now();
            imginpname_actual = imginpname;
            file_inpimg = cimg_option("-impimg",(imginpname_actual.append(p.path().filename())).c_str(),"Input Image");

            
            //Init input image and output image
            imginp = CImg<unsigned char>(file_inpimg);
            auto elapsedloading = std::chrono::high_resolution_clock::now() - startloading;
            auto msecloading = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedloading).count();
            totreadwrite += msecloading;
            totread += msecloading;
            tRead += msecloading;

            //Verify if we have to save imgs in a folder or not
            if(dirOutputName.length() < 4){
                //Removing the format .jpg from the string
                std::string imginpstring = p.path().filename().string();
                imginpstring.erase(imginpstring.find("."),4);
                fileoutputname = imginpstring.append("_marked.jpg");
                file_outimg = cimg_option("-outimg",fileoutputname.c_str(),"Output Image");
            }
            else{
                dirOutputName_actual = dirOutputName;
                file_outimg = cimg_option("-outimg",(dirOutputName_actual.append(p.path().filename().string())).c_str(),"Output Image");
            }

            imgout = CImg<unsigned char>(imginp); //initialize with input pixels

            auto startmarking = std::chrono::high_resolution_clock::now();
            //If there is a problem in marking img
            if(computeWatermarkedImg(markimg, imginp, imgout) == -1){
                std::cerr << "Problem in marking an img\n";
                continue;
            }
            auto elapsedmarking = std::chrono::high_resolution_clock::now() - startmarking;
            auto msecmarking = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedmarking).count();
            tMark += msecmarking;

            auto startsaving = std::chrono::high_resolution_clock::now();
            //Save outputimg
            if(file_outimg) imgout.save_jpeg(file_outimg);
            auto elapsedsaving = std::chrono::high_resolution_clock::now() - startsaving;
            auto msecsaving = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedsaving).count();
            totreadwrite += msecsaving;
            totwrite += msecsaving;
            tWrite += msecsaving;

            totphotomarked++;

        }
        catch(CImgException& e){
            std::cerr << "Error on working on a img...\n";
        }

        //For counting elapsed time for generation pathFilenameImg
        startGenerating = std::chrono::high_resolution_clock::now();
    }

    auto totelapsed = std::chrono::high_resolution_clock::now() - start;
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(totelapsed).count();

    std::cout << "----------------------------\n";

    std::cout << "- msec elapsed: " << msec << "\n";
    std::cout << "- sec elapsed: " << (msec/1000) << "\n";

    std::cout << "- msec elapsed reading and writing: " << totreadwrite << "\n";
    std::cout << "- sec elapsed reading and writing: " << (totreadwrite/1000) << "\n";

    std::cout << "- msec elapsed reading: " << totread << "\n";
    std::cout << "- sec elapsed reading: " << (totread/1000) << "\n";

    std::cout << "- msec elapsed writing: " << totwrite << "\n";
    std::cout << "- sec elapsed writing: " << (totwrite/1000) << "\n";

    std::cout << "- total number of photos marked: " << totphotomarked << "\n";

    std::cout << "----------------------------\n";

    std::cout << "- Avg tGen of one pathNameImg: " << tGen/totphotomarked << " usec\n";
    std::cout << "- Avg tRead of one img: " << tRead/totphotomarked << " msec\n";
    std::cout << "- Avg tMark of one img: " << tMark/totphotomarked << " msec\n";
    std::cout << "- Avg tWrite of one img: " << tWrite/totphotomarked << " msec\n";

    std::cout << "----------------------------\n";    

    return 0;
}