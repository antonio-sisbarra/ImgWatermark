#include <iostream>
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
#include <thread>

//For marking imgs"
#include "watermarkUtil.cpp"

//For uutil functions
#include "utils.cpp"

//For moving objects to farm workers
#include "myqueue.cpp"

//Definition of EOS
#define EOS "null"

using namespace cimg_library;

//Compute the marked imgs with a farm. Each worker reads marks and writes
int main(int argc, char *argv[]) { 
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
    cil::CImg<unsigned char> markimg(file_markimg);
    std::cout << "Reading markimg ok, now starting reading images...\n";

    //Useful for reading imgs
    std::string imginpname(argv[3]);

    //Check if arg ends with /
    if(imginpname.back() != '/')
        imginpname.append("/");

    //Create a queue of input for each worker of the farm
    std::vector<myqueue<std::string*>*> vecQueues;
    for(int i=0; i<nw; i++){
        myqueue<std::string*>* q = new myqueue<std::string*>();
        vecQueues.push_back(q);
    }


    /* THREAD BODY FUNCTION -> READ MARK WRITE */
    auto body = [&](int ti, myqueue<std::string*> *inpQueue) {
        int tn = 0;
        int usecmin = INT_MAX;
        int usecmax = 0;
        long usectot = 0; 
        bool keepon = true;

        std::string imginpname_actual, dirOutputName_actual, fileoutputname;
        const char *file_inpimg, *file_outimg;
        cil::CImg<unsigned char> imginp, imgout;

        while(keepon) {
            try{

                std::string* imgFileName = inpQueue->pop();
                // if we got something
                if(*imgFileName != EOS) {

                    // compute task time
                    auto startTask   = std::chrono::high_resolution_clock::now();
                    
                    /* TASK JOB */
                    // read phase
                    imginpname_actual = imginpname;
                    file_inpimg = cimg_option("-impimg",(imginpname_actual.append(*imgFileName)).c_str(),"Input Image");
                    imginp = cil::CImg<unsigned char>(file_inpimg);

                    //Verify if we have to save imgs in a folder or not
                    if(dirOutputName.length() < 4){
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
                    imgout = cil::CImg<unsigned char>(imginp);

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

                } 
                else {

                    // otherwise terminate
                    keepon = false;
                    if(tn != 0){
                        std::cout << "Thread " << ti << " computed " << tn << " tasks "
                            << " (min max avg = " << usecmin << " " << usecmax
                            << " " << usectot/tn << ") "
                            << "\n";
                        totphotomarked += tn;
                    }
                }

                //Free memory
                delete imgFileName;

            }
            catch(CImgException& e){
                std::cerr << "Error in working on a img...\n";
            }
        }
    };


    //For elapsed time of the program
    auto start = std::chrono::high_resolution_clock::now();

    std::string* photoFileName; 
    int circularInd = 0; //for round robin policy

    // create executor threads
    std::vector<std::thread> tid;
    for(int i=0; i<nw; i++) {
        tid.push_back(std::thread(body, i, vecQueues.at(i))); 
    }

    // fill the input queues with the imgs
    for (auto & p : std::experimental::filesystem::directory_iterator(dirInput)){

        photoFileName = new std::string(p.path().filename().string());
        (*(vecQueues.at(circularInd))).push(photoFileName);
        circularInd = (circularInd + 1) % nw;

    }

    //Send EOS to all queues
    for(int i=0; i<nw; i++){
        vecQueues.at(i)->push(new std::string(EOS));
    }

    std::cout << "Working on imgs...\n";

    // await termination
    for(int i=0; i<nw; i++)
        tid[i].join();

    auto totelapsed = std::chrono::high_resolution_clock::now() - start;
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(totelapsed).count();

    std::cout << "Computed " << totphotomarked << " imgs marking using " <<
        nw << " threads in " << msec << " msecs" << "\n"; 

    // free memory
    for(int i=0; i<nw; i++){
        delete vecQueues.at(i);
    }

    return 0;
}