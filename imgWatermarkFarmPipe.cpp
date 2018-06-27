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
#include <atomic>
#include <thread>

//For marking imgs"
#include "watermarkUtil.cpp"

//For util functions
#include "utils.cpp"

//For moving objects to farm workers
#include "myqueue.cpp"

//Definition of EOS
#define EOS "null"

using namespace cimg_library;

//Compute the marked imgs with a farm. Each worker reads marks and writes
int main(int argc, char *argv[]) { 
    std::string markImgFilename, dirInput, dirOutput, dirOutputName;
    int parDegree;

    if (argc<4 || argc>5) {
        std::cerr << "use: " << argv[0]  << " pardegree markimgfile dirinput [diroutput] \n\n";
        return -1;
    }

    parDegree   = atoi(argv[1]); //par degree
    if(parDegree < 1){
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

    //Each worker must contain a pipe, so we create parDegree / 2 queues (for workers of the farm)
    std::vector<myqueue<std::string*>*> vecQueues;
    int nWorkersFarm;
    if(parDegree % 2 == 0)
        nWorkersFarm = (int)(parDegree/2);
    else
        nWorkersFarm = ((int)(parDegree/2)) + 1;
    if(nWorkersFarm == 0) nWorkersFarm++;

    //Create a queue of input for each worker of the farm
    for(int i=0; i<nWorkersFarm; i++){
        myqueue<std::string*>* q = new myqueue<std::string*>();
        vecQueues.push_back(q);
    }

    /* THREAD BODY FUNCTION -> READ MARK (WRITE ON A NEW THREAD) -> PIPELINE */
    auto body = [&](int ti, myqueue<std::string*> *inpQueue, bool isPipe) {
        int tn = 0;
        bool keepon = true;

        /* WRITING THREAD BODY FUNCTION */
        auto writebody = [&](int ti, myqueue<std::pair<std::string*,CImg<unsigned char>*>*> *inpQueue){
            int tn = 0;
            bool keepon = true;

            while(keepon){
                try{
                    //Read input of queue
                    std::pair<std::string*,CImg<unsigned char>*>* imgToWrite = inpQueue->pop();
                    std::string* file_outimg = imgToWrite->first;
                    CImg<unsigned char>* imgout = imgToWrite->second;

                    // if we got something
                    if(*file_outimg != EOS) {
                        
                        /* TASK JOB */
                        // write phase
                        if(file_outimg) (*imgout).save_jpeg((*file_outimg).c_str());
                        
                        //Increment counter of imgs marked
                        tn++;

                        //Free memory
                        if(file_outimg != nullptr)
                            delete file_outimg;
                        if(imgout != nullptr)
                            delete imgout;
                        if(imgToWrite != nullptr)
                            delete imgToWrite;

                    } 
                    else {

                        // otherwise terminate
                        keepon = false;
                        if(tn != 0){
                            totphotomarked += tn;
                        }

                        //Free memory
                        if(file_outimg != nullptr)
                            delete file_outimg;
                        if(imgToWrite != nullptr)
                            delete imgToWrite;
                    }

                }
                catch(CImgException& e){
                    std::cerr << e.what() << "\n";
                    std::cerr << "Error in working on a img...\n";
                }
            }
        };

        std::string imginpname_actual, dirOutputName_actual, fileoutputname;
        const char *file_inpimg, *file_outimg;
        CImg<unsigned char> imginp, *imgout;

        std::thread writingThread; myqueue<std::pair<std::string*,CImg<unsigned char>*>*>* inpQueueWriteThread;

        //If is a pipe create the inputQueue for the writing thread and start the thread
        if(isPipe){
            inpQueueWriteThread = new myqueue<std::pair<std::string*,CImg<unsigned char>*>*>();
            writingThread = std::thread(writebody, ti+parDegree-1, inpQueueWriteThread);
        }


        while(keepon) {
            try{

                std::string* imgFileName = inpQueue->pop();
                // if we got something
                if(*imgFileName != EOS) {
                    
                    /* TASK JOB */
                    // read phase
                    imginpname_actual = imginpname;
                    file_inpimg = cimg_option("-impimg",(imginpname_actual.append(*imgFileName)).c_str(),"Input Image");
                    imginp = CImg<unsigned char>(file_inpimg);

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
                    imgout = new CImg<unsigned char>(imginp);

                    // mark phase
                    //If there is a problem in marking img
                    if(computeWatermarkedImg(markimg, imginp, *imgout) == -1){
                        std::cerr << "Problem in marking an img\n";
                        continue;
                    }

                    //If we are in pipeline we must send img to writingthread
                    if(isPipe){
                        std::pair<std::string*,CImg<unsigned char>*>* inpPair;
                        inpPair = new std::pair<std::string*,CImg<unsigned char>*>(new std::string(file_outimg), imgout);
                        inpQueueWriteThread->push(inpPair);
                    }
                    else
                        if(file_outimg){
                            imgout->save_jpeg(file_outimg);
                            //free memory
                            delete imgout;
                        }
                    
                    //Increment counter of imgs read (or marked if i'm not in pipe)
                    tn++;

                } 
                else {

                    // otherwise terminate
                    if(isPipe == false){
                        if(tn != 0){
                            totphotomarked += tn;
                        }
                    }
                    else{
                        //I'm in pipe, i must send eos to writingthread
                        std::pair<std::string*,CImg<unsigned char>*>* inpPair;
                        inpPair = new std::pair<std::string*,CImg<unsigned char>*>(new std::string(EOS), (CImg<unsigned char>*)EOS);
                        inpQueueWriteThread->push(inpPair);

                        //Waiting the writingthread
                        writingThread.join();
                    }
                    keepon = false;
                }

                //Free memory
                if(imgFileName != nullptr)
                    delete imgFileName;

            }
            catch(CImgException& e){
                std::cerr << "Error in working on a img...\n";
            }
        }

        //Free memory before exiting the thread and if I'm in pipe
        if(isPipe)
            delete inpQueueWriteThread;
    };


    //For elapsed time of the program
    auto start = std::chrono::high_resolution_clock::now();

    std::string* photoFileName; 
    int circularInd = 0; //for round robin policy

    // create executor threads (workers of the farm, pipeline or not)
    std::vector<std::thread> tid; int nWorkersToCreate = parDegree; int i = 0;
    while(nWorkersToCreate > 0){
        if(nWorkersToCreate >= 2){
            tid.push_back(std::thread(body, i, vecQueues.at(i), true));
            nWorkersToCreate -= 2; 
        }
        else{
            tid.push_back(std::thread(body, i, vecQueues.at(i), false));
            nWorkersToCreate--; 
        }
        i++;
    }

    // fill the input queues with the imgs
    for (auto & p : std::experimental::filesystem::directory_iterator(dirInput)){

        photoFileName = new std::string(p.path().filename().string());
        (*(vecQueues.at(circularInd))).push(photoFileName);
        circularInd = (circularInd + 1) % nWorkersFarm;

    }

    //Send EOS to all queues
    for(int i=0; i<nWorkersFarm; i++){
        vecQueues.at(i)->push(new std::string(EOS));
    }

    std::cout << "Working on imgs...\n";

    // await termination
    for(int i=0; i<nWorkersFarm; i++)
        tid[i].join();

    auto totelapsed = std::chrono::high_resolution_clock::now() - start;
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(totelapsed).count();

    std::cout << "Computed " << totphotomarked << " imgs marking using " <<
        parDegree << " threads in " << msec << " msecs" << "\n"; 

    //free memory
    for(int i=0; i<nWorkersFarm; i++){
        if(vecQueues.at(i) != nullptr)
            delete vecQueues.at(i);
    }

    return 0;
}