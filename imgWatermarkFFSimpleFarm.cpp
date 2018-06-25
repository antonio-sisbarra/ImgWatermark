#include <iostream>
#include <ff/farm.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>
#include <string>
#include <experimental/filesystem>
#include "CImg.h"
#include <unistd.h>
#include <atomic>

//For marking imgs"
#include "watermarkUtil.cpp"

//For util functions
#include "utils.cpp"

using namespace ff;
using namespace cimg_library;

//To count the photos marked
std::atomic<int> totphotomarked(0);

//Input photo directory
std::string dirInput;

/* GENERATION PATHNAMES STAGE */
struct pathnameGenStage: ff_node_t<std::string> {
    std::string* svc(std::string *) {
        for (auto & p : std::experimental::filesystem::directory_iterator(dirInput)){
            ff_send_out(new std::string(p.path().filename().string()));
        }

        return EOS;
    }
};

/* WORKING (READ, MARK, WRITE) STAGE */
struct workingStage: ff_node_t<std::string, void> {
    //Useful to manage imgs
    std::string imginpname_actual, dirOutputName_actual, fileoutputname;
    const char *file_inpimg, *file_outimg;
    CImg<unsigned char> imginp, imgout;

    //Refs to general variable
    std::string* mImgInpName;
    std::string* mDirOutputName;
    cil::CImg<unsigned char>* mMarkImg;

    //Constructor
    workingStage(std::string* imgInpName, std::string* dirOutputName, cil::CImg<unsigned char>* markImg){
        mImgInpName = imgInpName;
        mDirOutputName = dirOutputName;
        mMarkImg = markImg;
    }

    //The work: read img, mark and write 
    void* svc(std::string *imgFilename) {

        try{
            // compute task time
            auto startTask   = std::chrono::high_resolution_clock::now();
            
            /* TASK JOB */
            // read phase
            imginpname_actual = *mImgInpName;
            file_inpimg = cimg_option("-impimg",(imginpname_actual.append(*imgFileName)).c_str(),"Input Image");
            imginp = CImg<unsigned char>(file_inpimg);

            //Verify if we have to save imgs in a folder or not
            if(mDirOutputName->length() < 4){
                //Removing the format .jpg from the string
                std::string imginpstring = *imgFileName;
                imginpstring.erase(imginpstring.find("."),4);
                fileoutputname = imginpstring.append("_marked.jpg");
                file_outimg = cimg_option("-outimg",fileoutputname.c_str(),"Output Image");
            }
            else{
                dirOutputName_actual = dirOutputName;
                file_outimg = cimg_option("-outimg",(dirOutputName_actual.append(*imgFileName)).c_str(),"Output Image");
            }

            //Preparing outimg
            imgout = CImg<unsigned char>(imginp);

            // mark phase
            //If there is a problem in marking img
            if(computeWatermarkedImg(markimg, imginp, imgout) == -1){
                std::cerr << "Problem in marking an img\n";
                continue;
            }

            // write phase
            if(file_outimg) imgout.save(file_outimg);

            auto elapsedTask = std::chrono::high_resolution_clock::now() - startTask;
            auto usec    = std::chrono::duration_cast<std::chrono::microseconds>(elapsedTask).count();
            if(usec < usecmin)
            usecmin = usec;
            if(usec > usecmax)
            usecmax = usec;
            usectot += usec;
            
            //Increment counter of imgs marked
            tn++;

            // otherwise terminate
            keepon = false;
            if(tn != 0){
                std::cout << "Thread " << ti << " computed " << tn << " tasks "
                    << " (min max avg = " << usecmin << " " << usecmax
                    << " " << usectot/tn << ") "
                    << "\n";
                totphotomarked += tn;
            }

            //Free memory
            delete imgFileName;

        }
        catch(CImgException& e){
            std::cerr << "Error in working on a img...\n";
        }

        /*
        //std::cout<< "thirdStage received " << t << "\n";
        sum += t; 
        delete task;
        */
        return GO_ON; 
        
    }

    //Work ended for this worker
    void svc_end() {
        /*
         std::cout << "sum = " << sum << "\n";
        */
    }
};

int main(int argc, char* argv[]){
    std::string markImgFilename, dirInput, dirOutput, dirOutputName;
    int nw;

    if (argc<4 || argc>5) {
        std::cerr << "use: " << argv[0]  << " pardegree markimgfile dirinput [diroutput] \n dirinput and diroutput without / \n";
        return -1;
    }

    nw   = atoi(argv[1]); //par degree
    if(nw < 1){
        std::cerr << "use nw >= 1\n";
        return -1;
    }

    //dirOutputName is useful for saving the images in output. It's not the path, but just a name
    if (argc == 5){ 
        dirOutput = GetCurrentWorkingDir().append("/").append(argv[4]);

        //Check if arg ends with /
        if(dirOutput.back() != '/')
            dirOutputName = std::string(argv[4]).append("/");
        else 
            dirOutputName = std::string(argv[4]);
    }
    else{
        dirOutput = GetCurrentWorkingDir();
        dirOutputName = std::string(" ");
    }
    
    markImgFilename = argv[2];
    dirInput = GetCurrentWorkingDir().append("/").append(argv[3]);

    std::cout << "dirInput is: " << dirInput << "\n";
    std::cout << "dirOutput is: " << dirOutput << "\n";

    if(checkParams(markImgFilename.c_str(), dirInput.c_str(), dirOutput.c_str()) == false){
        std::cerr << "problems in the params \n";
        return -1;
    }

    //To count the photos marked
    std::atomic<int> totphotomarked(0);

    //Read markimgfile and initialize input img
    std::cout << "Reading markimg file...\n";
    const char *file_markimg = cimg_option("-markimg", (markImgFilename).c_str(), "Watermark Image");
    CImg<unsigned char> markimg(file_markimg);
    std::cout << "Reading markimg ok, now starting reading images...\n";

    //Useful for reading imgs
    std::string imginpname(argv[3]);

    //Check if arg ends with /
    if(imginpname.back() != '/')
        imginpname.append("/");




    ffTime(START_TIME);
}