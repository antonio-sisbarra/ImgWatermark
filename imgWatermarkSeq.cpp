#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>
#include <string>
#include <experimental/filesystem>
#include "CImg-2.2.4_pre060418/CImg.h"
#include <unistd.h>
#include "watermarkUtil.cpp"

#define GetCurrentDir getcwd


using namespace cimg_library;

//Checks the correctness of filename params
bool checkParams(const char *markFileName, const char *inpPath, const char *outPath){
    struct stat info;

    if(stat( markFileName, &info ) != 0){
        std::cerr << "Cannot access to file\n";
        return false; //cannot access to file
    }

    if(stat( inpPath, &info ) != 0){
        std::cerr << "Cannot access to directory input\n";
        return false; //cannot access to directory
    }
        
    if( !(info.st_mode & S_IFDIR) ){
        std::cerr << "It is not a directory\n";
        return false; //is not a directory
    }
    return true;
}

//Extract filename without extension
std::string getOnlyFilename(const std::string &filewithformat){
    size_t position = filewithformat.find(".");
    std::string extractName = (std::string::npos == position)? filewithformat : filewithformat.substr(0, position);
    return extractName;
}

//Returns the current directory of the program
std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  if(!GetCurrentDir( buff, FILENAME_MAX ))
    return "";
  std::string current_working_dir(buff);
  return current_working_dir;
}

int main(int argc, char *argv[]) {   
    std::string markImgFilename, dirInput, dirOutput, dirOutputName;

    if (argc<3 && argc>4) {
        std::cerr << "use: " << argv[0]  << " markimgfile dirinput [diroutput] \n";
        return -1;
    }

    //dirOutputName is useful for saving the images in output. It's not the path, but just a name
    if (argc == 4){ 
        dirOutput = GetCurrentWorkingDir().append("/").append(argv[3]);
        dirOutputName = std::string(argv[3]).append("/");
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
    int totreadwrite = 0;


    auto start = std::chrono::high_resolution_clock::now();

    //Read markimgfile and initialize input img
    std::cout << "Reading markimg file...\n";
    const char *file_markimg = cimg_option("-markimg", (markImgFilename).c_str(), "Watermark Image");
    cil::CImg<unsigned char> markimg(file_markimg);
    std::cout << "Reading markimg ok, now starting reading images...\n";
    const char *file_inpimg, *file_outimg;
    cil::CImg<unsigned char> imginp, imgout;

    //Useful for reading imgs
    std::string imginpname(argv[2]), imginpname_actual, dirOutputName_actual, fileoutputname;
    imginpname.append("/");

    //Loop on all images
    for (auto & p : std::experimental::filesystem::directory_iterator(dirInput)){
        auto startloading = std::chrono::high_resolution_clock::now();
        imginpname_actual = imginpname;
        file_inpimg = cimg_option("-impimg",(imginpname_actual.append(p.path().filename())).c_str(),"Input Image");

        
        //Init input image and output image
        imginp = cil::CImg<unsigned char>(file_inpimg);
        auto elapsedloading = std::chrono::high_resolution_clock::now() - startloading;
        auto msecloading = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedloading).count();
        totreadwrite += msecloading;

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

        imgout = cil::CImg<unsigned char>(imginp); //initialize with input pixels

        int r, g, b, rmark, gmark, bmark, grayinpvalue, grayavgvalue;
        int graymarkvalue;

        computeWatermarkedImg(markimg, imginp, imgout);

        auto startsaving = std::chrono::high_resolution_clock::now();
        //Save outputimg
        if(file_outimg) imgout.save(file_outimg);
        auto elapsedsaving = std::chrono::high_resolution_clock::now() - startsaving;
        auto msecsaving = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedsaving).count();
        totreadwrite += msecsaving;

        std::cout << "img marked and saved successfully...\n";
    }

    auto totelapsed = std::chrono::high_resolution_clock::now() - start;
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(totelapsed).count();

    std::cout << "msec elapsed: " << msec << "\n";
    std::cout << "sec elapsed: " << (msec/1000) << "\n";

    std::cout << "msec elapsed reading and writing: " << totreadwrite << "\n";
    std::cout << "sec elapsed reading and writing: " << (totreadwrite/1000) << "\n";

    return 0;
}