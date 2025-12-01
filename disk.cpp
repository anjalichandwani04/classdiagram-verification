#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <map>

const int MAXTRACKS = 1000;

// States that a request/disk go through
enum State {
    STATE_NULL = 0,
    STATE_SEEK = 1,
    STATE_ROTATE = 2,
    STATE_XFER = 3,
    STATE_DONE = 4
};

struct BlockInfo {
    int track;
    double angle;
    int name;
};

struct Request {
    int block;
    int index;
};

class Disk {
private:
    std::vector<int> requests;
    std::vector<int> lateRequests;
    std::string policy;
    double seekSpeed;
    double rotateSpeed;
    int skew;
    int window;
    bool compute;
    std::vector<int> zoning;
    
    // Block layout
    std::vector<BlockInfo> blockInfoList;
    std::map<int, int> blockToTrackMap;
    std::map<int, double> blockToAngleMap;
    std::map<int, std::pair<int, int>> tracksBeginEnd;
    std::vector<int> blockAngleOffset;
    int maxBlock;
    
    // Tracks
    std::map<int, double> tracks;
    double trackWidth;
    
    // Disk arm
    int armTrack;
    double armSpeedBase;
    double armSpeed;
    double armX1;
    double armTargetX1;
    int armTarget;
    
    // Queue
    int requestCount;
    std::vector<Request> requestQueue;
    std::vector<State> requestState;
    int currWindow;
    
    // Current state
    int currentIndex;
    int currentBlock;
    State state;
    double angle;
    
    // Timing
    double timer;
    double seekBegin;
    double rotBegin;
    double xferBegin;
    double totalEst;
    
    // Stats
    double seekTotal;
    double rotTotal;
    double xferTotal;
    
    // Fairness
    int fairWindow;
    
    // Late requests
    int lateCount;
    
    bool isDone;
    
    // Helper functions
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(s);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
    
    void initBlockLayout() {
        blockInfoList.clear();
        blockToTrackMap.clear();
        blockToAngleMap.clear();
        tracksBeginEnd.clear();
        blockAngleOffset.clear();
        
        for (int i = 0; i < 3; i++) {
            blockAngleOffset.push_back(zoning[i] / 2);
        }
        
        // Track 0 (outer)
        int track = 0;
        int angleOffset = 2 * blockAngleOffset[track];
        int block = 0;
        for (int angle = 0; angle < 360; angle += angleOffset) {
            block = angle / angleOffset;
            blockToTrackMap[block] = track;
            blockToAngleMap[block] = angle;
            blockInfoList.push_back({track, (double)angle, block});
        }
        tracksBeginEnd[track] = {0, block};
        int pblock = block + 1;
        
        // Track 1 (middle)
        track = 1;
        int skewVal = skew;
        angleOffset = 2 * blockAngleOffset[track];
        for (int angle = 0; angle < 360; angle += angleOffset) {
            block = (angle / angleOffset) + pblock;
            blockToTrackMap[block] = track;
            blockToAngleMap[block] = angle + (angleOffset * skewVal);
            blockInfoList.push_back({track, (double)(angle + (angleOffset * skewVal)), block});
        }
        tracksBeginEnd[track] = {pblock, block};
        pblock = block + 1;
        
        // Track 2 (inner)
        track = 2;
        skewVal = 2 * skew;
        angleOffset = 2 * blockAngleOffset[track];
        for (int angle = 0; angle < 360; angle += angleOffset) {
            block = (angle / angleOffset) + pblock;
            blockToTrackMap[block] = track;
            blockToAngleMap[block] = angle + (angleOffset * skewVal);
            blockInfoList.push_back({track, (double)(angle + (angleOffset * skewVal)), block});
        }
        tracksBeginEnd[track] = {pblock, block};
        maxBlock = pblock;
        
        // Adjust angles
        for (auto& pair : blockToAngleMap) {
            pair.second = fmod(pair.second + 180, 360);
        }
    }
    
    std::vector<int> makeRequests(const std::string& addr, const std::string& addrDesc, std::mt19937& gen) {
        if (addr != "-1") {
            std::vector<std::string> tokens = split(addr, ',');
            std::vector<int> result;
            for (const auto& token : tokens) {
                result.push_back(std::stoi(token));
            }
            return result;
        }
        
        std::vector<std::string> desc = split(addrDesc, ',');
        int numRequests = std::stoi(desc[0]);
        int maxRequest = std::stoi(desc[1]);
        int minRequest = std::stoi(desc[2]);
        
        if (maxRequest == -1) {
            maxRequest = maxBlock;
        }
        
        std::uniform_int_distribution<> dis(minRequest, maxRequest);
        std::vector<int> result;
        for (int i = 0; i < numRequests; i++) {
            result.push_back(dis(gen));
        }
        return result;
    }
    
    void addQueueEntry(int block, int index) {
        requestQueue.push_back({block, index});
        requestState.push_back(STATE_NULL);
    }
    
    void switchState(State newState) {
        state = newState;
        requestState[currentIndex] = newState;
    }
    
    bool radiallyCloseTo(double a1, double a2) {
        double v = std::abs(a1 - a2);
        return v < rotateSpeed;
    }
    
    bool doneWithTransfer() {
        int angleOffset = blockAngleOffset[armTrack];
        if (radiallyCloseTo(angle, fmod(blockToAngleMap[currentBlock] + angleOffset, 360))) {
            switchState(STATE_DONE);
            requestCount++;
            return true;
        }
        return false;
    }
    
    bool doneWithRotation() {
        int angleOffset = blockAngleOffset[armTrack];
        if (radiallyCloseTo(angle, fmod(blockToAngleMap[currentBlock] - angleOffset, 360))) {
            switchState(STATE_XFER);
            return true;
        }
        return false;
    }
    
//     void planSeek(int track) {
//         seekBegin = timer;
//         switchState(STATE_SEEK);
//         if (track == armTrack) {
//             rotBegin = timer;
//             switchState(STATE_ROTATE);
//             return;
//         }
//         armTarget = track;
//         armTargetX1 = 140 - tracks[track] - (trackWidth / 2.0);
//         // if (track >= armTrack) {
//         //     armSpeed = armSpeedBase;
//         // } else {
//         //     armSpeed = -armSpeedBase;
//         // }
//         if (track >= armTrack) {
//     armSpeed = -armSpeedBase;   // move inward (decreasing X1)
// } else {
//     armSpeed = armSpeedBase;    // move outward (increasing X1)
// }

//     }
    

void planSeek(int track) {
    seekBegin = timer;
    switchState(STATE_SEEK);
    if (track == armTrack) {
        rotBegin = timer;
        switchState(STATE_ROTATE);
        return;
    }
    armTarget = track;
    armTargetX1 = 140 - tracks[track] - (trackWidth / 2.0);

    // Ensure sign moves armX1 toward armTargetX1
    if (armTargetX1 < armX1) {
        armSpeed = -std::abs(armSpeedBase);
    } else {
        armSpeed = std::abs(armSpeedBase);
    }

    std::cerr << "[planSeek] timer=" << timer
              << " currTrack=" << armTrack
              << " targetTrack=" << track
              << " armX1=" << armX1
              << " armTargetX1=" << armTargetX1
              << " armSpeed=" << armSpeed << std::endl;
}

    bool doneWithSeek() {
        armX1 += armSpeed;
        if ((armSpeed > 0.0 && armX1 >= armTargetX1) || (armSpeed < 0.0 && armX1 <= armTargetX1)) {
            armTrack = armTarget;
            return true;
        }
        return false;
    }
    
    std::pair<int, int> doSATF(const std::vector<Request>& rList) {
        int minBlock = -1;
        int minIndex = -1;
        double minEst = -1;
        
        for (const auto& req : rList) {
            int block = req.block;
            int index = req.index;
            
            if (requestState[index] == STATE_DONE) {
                continue;
            }
            
            int track = blockToTrackMap[block];
            double angle_val = blockToAngleMap[block];
            
            // Estimate seek time
            int dist = std::abs(armTrack - track);
            double seekEst = (trackWidth / armSpeedBase) * dist;
            
            // Estimate rotate time
            int angleOffset = blockAngleOffset[track];
            double angleAtArrival = angle + (seekEst * rotateSpeed);
            while (angleAtArrival > 360.0) angleAtArrival -= 360.0;
            
            double rotDist = (angle_val - angleOffset) - angleAtArrival;
            while (rotDist > 360.0) rotDist -= 360.0;
            while (rotDist < 0.0) rotDist += 360.0;
            double rotEst = rotDist / rotateSpeed;
            
            // Transfer time
            double xferEst = (angleOffset * 2.0) / rotateSpeed;
            
            double totalEst = seekEst + rotEst + xferEst;
            
            if (minEst == -1 || totalEst < minEst) {
                minEst = totalEst;
                minBlock = block;
                minIndex = index;
            }
        }
        
        totalEst = minEst;
        return {minBlock, minIndex};
    }
    
    std::vector<Request> doSSTF(const std::vector<Request>& rList) {
        int minDist = MAXTRACKS;
        std::vector<Request> trackList;
        
        for (const auto& req : rList) {
            if (requestState[req.index] == STATE_DONE) {
                continue;
            }
            int track = blockToTrackMap[req.block];
            int dist = std::abs(armTrack - track);
            
            if (dist < minDist) {
                trackList.clear();
                trackList.push_back(req);
                minDist = dist;
            } else if (dist == minDist) {
                trackList.push_back(req);
            }
        }
        return trackList;
    }
    
    void updateWindow() {
        if (fairWindow == -1 && currWindow > 0 && currWindow < (int)requestQueue.size()) {
            currWindow++;
        }
    }
    
    int getWindow() {
        if (currWindow <= -1) {
            return requestQueue.size();
        } else {
            if (fairWindow != -1) {
                if (requestCount > 0 && (requestCount % fairWindow == 0)) {
                    currWindow = currWindow + fairWindow;
                }
                return currWindow;
            } else {
                return currWindow;
            }
        }
    }
    
    void getNextIO() {
        if (requestCount == (int)requestQueue.size()) {
            if (currentBlock < 0 || blockToTrackMap.find(currentBlock) == blockToTrackMap.end()) {
                std::cerr << "[ERROR getNextIO] picked invalid currentBlock=" << currentBlock << " currentIndex=" << currentIndex << std::endl;
            }

            printStats();
            isDone = true;
            return;
        }
        
        if (policy == "FIFO") {
            currentBlock = requestQueue[requestCount].block;
            currentIndex = requestQueue[requestCount].index;
            std::vector<Request> singleReq = {requestQueue[requestCount]};
            doSATF(singleReq);
        } else if (policy == "SATF" || policy == "BSATF") {
            int endIndex = getWindow();
            if (endIndex > (int)requestQueue.size()) {
                endIndex = requestQueue.size();
            }
            std::vector<Request> subList(requestQueue.begin(), requestQueue.begin() + endIndex);
            auto result = doSATF(subList);
            currentBlock = result.first;
            currentIndex = result.second;
        } else if (policy == "SSTF") {
            std::vector<Request> subList(requestQueue.begin(), requestQueue.begin() + getWindow());
            std::vector<Request> trackList = doSSTF(subList);
            auto result = doSATF(trackList);
            currentBlock = result.first;
            currentIndex = result.second;
        }
        
        planSeek(blockToTrackMap[currentBlock]);
        
        if (lateCount < (int)lateRequests.size()) {
            addQueueEntry(lateRequests[lateCount], requestQueue.size());
            lateCount++;
        }
    }
    
    void doRequestStats() {
        double seekTime = rotBegin - seekBegin;
        double rotTime = xferBegin - rotBegin;
        double xferTime = timer - xferBegin;
        double totalTime = timer - seekBegin;
        
        if (compute) {
            std::cout << "Block: " << std::setw(3) << currentBlock 
                      << "  Seek:" << std::setw(3) << (int)seekTime
                      << "  Rotate:" << std::setw(3) << (int)rotTime
                      << "  Transfer:" << std::setw(3) << (int)xferTime
                      << "  Total:" << std::setw(4) << (int)totalTime << std::endl;
        }
        
        seekTotal += seekTime;
        rotTotal += rotTime;
        xferTotal += xferTime;
    }
    
    void printStats() {
        if (compute) {
            std::cout << "\nTOTALS      Seek:" << std::setw(3) << (int)seekTotal
                      << "  Rotate:" << std::setw(3) << (int)rotTotal
                      << "  Transfer:" << std::setw(3) << (int)xferTotal
                      << "  Total:" << std::setw(4) << (int)timer << "\n" << std::endl;
        }
    }
    
    // void animate() {
    //     timer += 1;
        
    //     angle += rotateSpeed;
    //     if (angle >= 360.0) {
    //         angle = 0.0;
    //     }
        
    //     if (state == STATE_SEEK) {
    //         if (doneWithSeek()) {
    //             rotBegin = timer;
    //             switchState(STATE_ROTATE);
    //         }
    //     }
    //     if (state == STATE_ROTATE) {
    //         if (doneWithRotation()) {
    //             xferBegin = timer;
    //             switchState(STATE_XFER);
    //         }
    //     }
    //     if (state == STATE_XFER) {
    //         if (doneWithTransfer()) {
    //             doRequestStats();
    //             switchState(STATE_DONE);
    //             updateWindow();
    //             int prevBlock = currentBlock;
    //             getNextIO();
    //             int nextBlock = currentBlock;
                
    //             if (!isDone && blockToTrackMap[prevBlock] == blockToTrackMap[nextBlock]) {
    //                 auto tbe = tracksBeginEnd[armTrack];
    //                 if ((prevBlock == tbe.second && nextBlock == tbe.first) || (prevBlock + 1 == nextBlock)) {
    //                     rotBegin = timer;
    //                     seekBegin = timer;
    //                     xferBegin = timer;
    //                     switchState(STATE_XFER);
    //                 }
    //             }
    //         }
    //     }
    // }

    void animate() {
    timer += 1;

    // Advance rotational position
    angle += rotateSpeed;
    if (angle >= 360.0) angle -= 360.0;

    // Debug snapshot every tick
    std::cerr << "[tick] t=" << timer
              << " state=" << state
              << " currBlock=" << currentBlock
              << " armTrack=" << armTrack
              << " armTarget=" << armTarget
              << " armX1=" << armX1
              << " armTargetX1=" << armTargetX1
              << " angle=" << angle
              << std::endl;

    if (state == STATE_SEEK) {
        if (doneWithSeek()) {
            rotBegin = timer;
            switchState(STATE_ROTATE);
            std::cerr << "[transition] SEEK -> ROTATE at t=" << timer << " armTrack=" << armTrack << std::endl;
        }
    }
    if (state == STATE_ROTATE) {
        if (doneWithRotation()) {
            xferBegin = timer;
            switchState(STATE_XFER);
            std::cerr << "[transition] ROTATE -> XFER at t=" << timer << " angle=" << angle << std::endl;
        }
    }
    if (state == STATE_XFER) {
        if (doneWithTransfer()) {
            doRequestStats();
            switchState(STATE_DONE);
            std::cerr << "[transition] XFER -> DONE block=" << currentBlock << " t=" << timer << std::endl;
            updateWindow();
            int prevBlock = currentBlock;
            getNextIO();
            int nextBlock = currentBlock;
            std::cerr << "[getNextIO] nextBlock=" << nextBlock << " isDone=" << isDone << std::endl;

            if (!isDone && blockToTrackMap[prevBlock] == blockToTrackMap[nextBlock]) {
                auto tbe = tracksBeginEnd[armTrack];
                if ((prevBlock == tbe.second && nextBlock == tbe.first) || (prevBlock + 1 == nextBlock)) {
                    rotBegin = timer;
                    seekBegin = timer;
                    xferBegin = timer;
                    switchState(STATE_XFER);
                    std::cerr << "[optimization] staying in XFER for adjacent blocks\n";
                }
            }
        }
    }
}

public:
    Disk(const std::string& addr, const std::string& addrDesc,
         const std::string& lateAddr, const std::string& lateAddrDesc,
         const std::string& policy, double seekSpeed, double rotateSpeed,
         int skew, int window, bool compute, const std::string& zoning, int seed)
        : policy(policy), seekSpeed(seekSpeed), rotateSpeed(rotateSpeed),
          skew(skew), window(window), compute(compute) {
        
        // Parse zoning
        std::vector<std::string> zoningTokens = split(zoning, ',');
        for (const auto& token : zoningTokens) {
            this->zoning.push_back(std::stoi(token));
        }
        
        // Initialize block layout
        initBlockLayout();
        
        // Create random generator
        std::mt19937 gen(seed);
        
        // Make requests
        requests = makeRequests(addr, addrDesc, gen);
        lateRequests = makeRequests(lateAddr, lateAddrDesc, gen);
        
        // Initialize tracks
        trackWidth = 40;
        tracks[0] = 140;
        tracks[1] = tracks[0] - trackWidth;
        tracks[2] = tracks[1] - trackWidth;
        
        // Initialize disk arm
        armTrack = 0;
        armSpeedBase = seekSpeed;
        armSpeed = seekSpeed;
        armX1 = 250 - tracks[armTrack];
        
        // Initialize queue
        requestCount = 0;
        for (size_t i = 0; i < requests.size(); i++) {
            addQueueEntry(requests[i], i);
        }
        
        // Scheduling window
        currWindow = window;
        
        // Fairness
        if (policy == "BSATF" && window != -1) {
            fairWindow = window;
        } else {
            fairWindow = -1;
        }
        
        // Initial state
        currentIndex = -1;
        currentBlock = -1;
        state = STATE_NULL;
        angle = 0.0;
        
        // Timer and stats
        timer = 0;
        seekTotal = 0.0;
        rotTotal = 0.0;
        xferTotal = 0.0;
        
        lateCount = 0;
        isDone = false;
        
        // Print requests
        std::cout << "REQUESTS [";
        for (size_t i = 0; i < requests.size(); i++) {
            std::cout << requests[i];
            if (i < requests.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n" << std::endl;
        
        if (!lateRequests.empty()) {
            std::cout << "LATE REQUESTS [";
            for (size_t i = 0; i < lateRequests.size(); i++) {
                std::cout << lateRequests[i];
                if (i < lateRequests.size() - 1) std::cout << ", ";
            }
            std::cout << "]\n" << std::endl;
        }
        
        if (!compute) {
            std::cout << "\nFor the requests above, compute the seek, rotate, and transfer times." << std::endl;
            std::cout << "Use -c to see the answers.\n" << std::endl;
        }
    }
    
    void go() {
        getNextIO();
        while (!isDone) {
            animate();
        }
        if (!isDone) std::cerr << "Simulation stopped: max iterations reached\n";
    }
};

int main(int argc, char* argv[]) {
    // Default options
    int seed = 0;
    std::string addr = "-1";
    std::string addrDesc = "5,-1,0";
    std::string seekSpeed = "1";
    std::string rotateSpeed = "1";
    std::string policy = "FIFO";
    int window = -1;
    int skew = 0;
    std::string zoning = "30,30,30";
    std::string lateAddr = "-1";
    std::string lateAddrDesc = "0,-1,0";
    bool compute = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-s" || arg == "--seed") && i + 1 < argc) {
            seed = std::stoi(argv[++i]);
        } else if ((arg == "-a" || arg == "--addr") && i + 1 < argc) {
            addr = argv[++i];
        } else if ((arg == "-A" || arg == "--addrDesc") && i + 1 < argc) {
            addrDesc = argv[++i];
        } else if ((arg == "-S" || arg == "--seekSpeed") && i + 1 < argc) {
            seekSpeed = argv[++i];
        } else if ((arg == "-R" || arg == "--rotSpeed") && i + 1 < argc) {
            rotateSpeed = argv[++i];
        } else if ((arg == "-p" || arg == "--policy") && i + 1 < argc) {
            policy = argv[++i];
        } else if ((arg == "-w" || arg == "--schedWindow") && i + 1 < argc) {
            window = std::stoi(argv[++i]);
        } else if ((arg == "-o" || arg == "--skewOffset") && i + 1 < argc) {
            skew = std::stoi(argv[++i]);
        } else if ((arg == "-z" || arg == "--zoning") && i + 1 < argc) {
            zoning = argv[++i];
        } else if ((arg == "-l" || arg == "--lateAddr") && i + 1 < argc) {
            lateAddr = argv[++i];
        } else if ((arg == "-L" || arg == "--lateAddrDesc") && i + 1 < argc) {
            lateAddrDesc = argv[++i];
        } else if (arg == "-c" || arg == "--compute") {
            compute = true;
        }
    }
    
    std::cout << "OPTIONS seed " << seed << std::endl;
    std::cout << "OPTIONS addr " << addr << std::endl;
    std::cout << "OPTIONS addrDesc " << addrDesc << std::endl;
    std::cout << "OPTIONS seekSpeed " << seekSpeed << std::endl;
    std::cout << "OPTIONS rotateSpeed " << rotateSpeed << std::endl;
    std::cout << "OPTIONS skew " << skew << std::endl;
    std::cout << "OPTIONS window " << window << std::endl;
    std::cout << "OPTIONS policy " << policy << std::endl;
    std::cout << "OPTIONS compute " << (compute ? "True" : "False") << std::endl;
    std::cout << "OPTIONS zoning " << zoning << std::endl;
    std::cout << "OPTIONS lateAddr " << lateAddr << std::endl;
    std::cout << "OPTIONS lateAddrDesc " << lateAddrDesc << std::endl;
    std::cout << std::endl;
    
    Disk disk(addr, addrDesc, lateAddr, lateAddrDesc, policy,
              std::stod(seekSpeed), std::stod(rotateSpeed), skew, window,
              compute, zoning, seed);
    
    disk.go();
    
    return 0;
}