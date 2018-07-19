/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/


#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>
#include <sys/stat.h>

#include<opencv2/core/core.hpp>

#include<System.h>

using namespace std;

void LoadImages(const string &strFile, vector<string> &vstrImageFilenames,
                vector<double> &vTimestamps);

int main(int argc, char **argv)
{
    if(argc < 4 || argc>8)
    {
        cerr << endl << "Usage: ./mono_record path_to_vocabulary path_to_settings path_to_sequence n_reset=1 reset_rate=1 record_map=1 save_dir=path_to_sequence/trajectories" << endl;
        return 1;
    }

    // Retrieve paths to images
    vector<string> vstrImageFilenames;
    vector<double> vTimestamps;
    string strFile = string(argv[3])+"/rgb.txt";
    LoadImages(strFile, vstrImageFilenames, vTimestamps);

    int nImages = vstrImageFilenames.size();

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);

    // Vector for tracking time statistics
    vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);

    cout << endl << "-------" << endl;
    cout << "Start processing sequence ..." << endl;
    cout << "Images in the sequence: " << nImages << endl << endl;

    // Main loop
    cv::Mat im;
    int n_reset=argc>4?stoi(argv[4]):1;
    int reset_rate=argc>5?stoi(argv[5]):1;
    bool save_map=bool(argc>6?stoi(argv[6]):1);
    string save_dir=argc>7?string(argv[7]):string(argv[3])+"/trajectories";
    mkdir(save_dir.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for(int mmi=0; mmi<n_reset*reset_rate; mmi++){
        cout << "Trajectory " <<mmi<< endl;
        //string fname=string(argv[3])+"/trajectories/traj_"+to_string(mmi)+".txt";
        //cout << fname << endl;
        ofstream traj_file(save_dir+"/traj_"+to_string(mmi)+".txt");
	ofstream map_file;
        if(save_map)map_file.open(save_dir+"/map_"+to_string(mmi)+".txt");
        if(mmi%reset_rate==0){
            cout << "Reset" << endl;
            SLAM.Reset();
        }
        for(int ni=0; ni<nImages; ni++)
        {
            // Read image from file
            im = cv::imread(string(argv[3])+"/"+vstrImageFilenames[ni],CV_LOAD_IMAGE_UNCHANGED);
            double tframe = vTimestamps[ni];

            if(im.empty())
            {
                cerr << endl << "Failed to load image at: "
                     << string(argv[3]) << "/" << vstrImageFilenames[ni] << endl;
                return 1;
            }

    #ifdef COMPILEDWITHC11
            std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    #else
            std::chrono::monotonic_clock::time_point t1 = std::chrono::monotonic_clock::now();
    #endif

            // Pass the image to the SLAM system
            cv::Mat x=SLAM.TrackMonocular(im,tframe);
            //map_file << ni << endl;
            for(int pi=0; pi<SLAM.GetTrackedMapPoints().size(); pi++)
            {
                //cout<<SLAM.GetTrackedMapPoints()[pi]<<endl;
                if(save_map&&SLAM.GetTrackedMapPoints()[pi]!=0){
                    auto x=SLAM.GetTrackedMapPoints()[pi]->GetWorldPos();
                    map_file << ni <<", "<<format(x.reshape (x.rows * x.cols), cv::Formatter::FMT_CSV)<<endl;
                }
            }
            //map_file<<endl;
            //for(int pi=0; pi<SLAM.mTrackedKeyPointsUn.size(); pi++)
            //    cout<<&(SLAM.mTrackedKeyPointsUn[pi])<<" ";
            //cout<<SLAM.GetTrackedMapPoints().size()<<endl;
            //cout<<SLAM.GetTrackedKeyPointsUn().size()<<endl;
            //cout<<endl;
            //cout << x << endl;
            //cout<<format(x.reshape (x.rows * x.cols), cv::Formatter::FMT_CSV)<< endl;
            traj_file << ni<<", "<<format(x.reshape (x.rows * x.cols), cv::Formatter::FMT_CSV) << endl;
            /*for(int i = 0; i < x.rows; i++)
            {
                const double* Mi = x.ptr<double>(i);
                for(int j = 0; j < x.cols; j++)
                    file<< ", "<<Mi[j];
            }
            file << endl;*/
            //cout << SLAM.GetTrackingState() << endl;
            //cout << im << endl;


    #ifdef COMPILEDWITHC11
            std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
    #else
            std::chrono::monotonic_clock::time_point t2 = std::chrono::monotonic_clock::now();
    #endif

            double ttrack= std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();

            vTimesTrack[ni]=ttrack;

            // Wait to load the next frame
            double T=0;
            if(ni<nImages-1)
                T = vTimestamps[ni+1]-tframe;
            else if(ni>0)
                T = tframe-vTimestamps[ni-1];

            //if(ttrack<T)
            //    usleep((T-ttrack)*1e6);
        }
        traj_file.flush();
        traj_file.close();

    }
    cout << "-------" << endl << endl;
    cout << "Done, cleaning up" << endl << endl;

    // Stop all threads
    SLAM.Shutdown();
    cout << "-------" << endl << endl;

    // Tracking time statistics
    sort(vTimesTrack.begin(),vTimesTrack.end());
    float totaltime = 0;
    for(int ni=0; ni<nImages; ni++)
    {
        totaltime+=vTimesTrack[ni];
    }
    cout << "-------" << endl << endl;
    cout << "median tracking time: " << vTimesTrack[nImages/2] << endl;
    cout << "mean tracking time: " << totaltime/nImages << endl;

    // Save camera trajectory
    //SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");

    return 0;
}

void LoadImages(const string &strFile, vector<string> &vstrImageFilenames, vector<double> &vTimestamps)
{
    ifstream f;
    f.open(strFile.c_str());

    // skip first three lines
    string s0;
    getline(f,s0);
    getline(f,s0);
    getline(f,s0);

    while(!f.eof())
    {
        string s;
        getline(f,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            double t;
            string sRGB;
            ss >> t;
            vTimestamps.push_back(t);
            ss >> sRGB;
            vstrImageFilenames.push_back(sRGB);
        }
    }
}
