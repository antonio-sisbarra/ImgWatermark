#include <iostream>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
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
    cil::CImg<unsigned char> imginp, imgout;

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
    void* svc(std::string *imgFileName) {

        try{
            
            /* TASK JOB */
            // read phase
            imginpname_actual = *mImgInpName;
            file_inpimg = (imginpname_actual.append(*imgFileName)).c_str();
            imginp = cil::CImg<unsigned char>(file_inpimg);

            //Verify if we have to save imgs in a folder or not
            if(mDirOutputName->length() < 4){
                //Removing the format .jpg from the string
                std::string imginpstring = *imgFileName;
                imginpstring.erase(imginpstring.find("."),4);
                fileoutputname = imginpstring.append("_marked.jpg");
                file_outimg = fileoutputname.c_str();
            }
            else{
                dirOutputName_actual = *mDirOutputName;
                file_outimg = (dirOutputName_actual.append(*imgFileName)).c_str();
            }

            //Preparing outimg
            imgout = cil::CImg<unsigned char>(imginp);

            // mark phase
            //If there is a problem in marking img
            if(computeWatermarkedImg(*mMarkImg, imginp, imgout) == -1){
                std::cerr << "Problem in marking an img\n";
            }

            // write phase
            if(file_outimg) imgout.save(file_outimg);
            
            //Increment counter of imgs marked
            totphotomarked++;

            //Free memory
            delete imgFileName;

        }
        catch(cil::CImgException& e){
            std::cerr << e.what() << "\n";
            std::cerr << "Error in working on a img...\n";
        }

        //Worker goes on, when it receives EOS, svc_end is called
        return GO_ON; 
        
    }

    
    //Work ended for this worker
    void svc_end() {
        
         std::cout << "Thread ended...\n";
        
    }

};


int main(int argc, char *argv[]) {
    std::string markImgFilename, dirOutput, *dirOutputName;
    int nw;

    if (argc<4 || argc>5) {
        std::cerr << "use: " << argv[0]  << " pardegree markimgfile dirinput [diroutput] \n\n";
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
            dirOutputName = new std::string(std::string(argv[4]).append("/"));
        else 
            dirOutputName = new std::string(argv[4]);
    }
    else{
        dirOutput = GetCurrentWorkingDir();
        dirOutputName = new std::string(" ");
    }
    
    markImgFilename = argv[2];
    dirInput = GetCurrentWorkingDir().append("/").append(argv[3]);

    std::cout << "dirInput is: " << dirInput << "\n";
    std::cout << "dirOutput is: " << dirOutput << "\n";

    if(checkParams(markImgFilename.c_str(), dirInput.c_str(), dirOutput.c_str()) == false){
        std::cerr << "problems in the params \n";
        return -1;
    }

    //Read markimgfile and initialize input img
    std::cout << "Reading markimg file...\n";
    const char *file_markimg = markImgFilename.c_str();
    cil::CImg<unsigned char> *markimg = new cil::CImg<unsigned char>(file_markimg);
    std::cout << "Reading markimg ok, now starting reading images...\n";

    //Useful for reading imgs
    std::string* imginpname = new std::string(argv[3]);

    //Check if arg ends with /
    if(imginpname->back() != '/')
        imginpname->append("/");

    //Create stages and workers for farm
    pathnameGenStage  first;
    std::vector<std::unique_ptr<ff_node> > W;
    for(int i=0;i<nw;++i) W.push_back(make_unique<workingStage>(imginpname, dirOutputName, markimg));

    //Create farm and pipe
    ff_Farm<std::string> farm(std::move(W), first); 
    farm.set_scheduling_ondemand();  // set auto-scheduling to the farm
    farm.remove_collector(); // remove unused collector for the workers

    ffTime(START_TIME);

    std::cout << "Working on imgs...\n";

    if (farm.run_and_wait_end()<0) {
        error("running farm");
        return -1;
    }

    ffTime(STOP_TIME);
    std::cout << "Marked correctly " << totphotomarked << " imgs...\n";
    std::cout << "Time: " << ffTime(GET_TIME) << "msec\n";

    //Free memory
    if(imginpname != nullptr) delete imginpname;
    if(dirOutputName != nullptr) delete dirOutputName;
    if(markimg != nullptr) delete markimg;

    return 0;
}