#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iomanip>
#include <getopt.h>

#define MAXTRACKS 1000

enum State {
    STATE_NULL = 0,
    STATE_SEEK = 1,
    STATE_ROTATE = 2,
    STATE_XFER = 3,
    STATE_DONE = 4
};

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

class Disk {
public:
    std::string addr, addrDesc, lateAddr, lateAddrDesc, policy, zoning_str;
    double seekSpeed, rotateSpeed;
    int skew, window;
    bool compute;
    std::vector<int> requests, lateRequests;
    int lateCount;
    int fairWindow;
    double width = 500.0;
    double cx = width / 2.0;
    double cy = width / 2.0;
    double spindleX;
    double spindleY;
    std::vector<double> tracks;
    double trackWidth = 40.0;
    double armSpeedBase;
    double armSpeed;
    int armTrack;
    double armWidth = 20.0;
    double headWidth = 10.0;
    double armX, armX1, armX2, armY1, armY2;
    double headX1, headX2, headY1, headY2;
    double targetSize = 10.0;
    int requestCount;
    std::vector<std::pair<int, int>> requestQueue;
    std::vector<int> requestState;
    int currWindow;
    int currentIndex, currentBlock;
    int state;
    std::vector<int> blockAngleOffset;
    std::vector<int> blockToTrack;
    std::vector<double> blockToAngle;
    std::vector<std::pair<int, int>> tracksBeginEnd;
    int maxBlock;
    double angle;
    int timer;
    double seekTotal, rotTotal, xferTotal;
    bool isDone;
    int seekBegin, rotBegin, xferBegin;
    double totalEst;
    int armTarget;
    double armTargetX1;

    Disk(std::string addr_, std::string addrDesc_, std::string lateAddr_, std::string lateAddrDesc_,
         std::string policy_, double seekSpeed_, double rotateSpeed_, int skew_, int window_,
         bool compute_, std::string zoning_) :
        addr(addr_), addrDesc(addrDesc_), lateAddr(lateAddr_), lateAddrDesc(lateAddrDesc_),
        policy(policy_), seekSpeed(seekSpeed_), rotateSpeed(rotateSpeed_), skew(skew_), window(window_),
        compute(compute_), zoning_str(zoning_) {

        InitBlockLayout();

        requests = MakeRequests(addr, addrDesc);
        lateRequests = MakeRequests(lateAddr, lateAddrDesc);

        if (policy == "BSATF" && window != -1) {
            fairWindow = window;
        } else {
            fairWindow = -1;
        }

        std::cout << "REQUESTS";
        for (int r : requests) {
            std::cout << " " << r;
        }
        std::cout << std::endl << std::endl;

        lateCount = 0;
        if (!lateRequests.empty()) {
            std::cout << "LATE REQUESTS";
            for (int r : lateRequests) {
                std::cout << " " << r;
            }
            std::cout << std::endl << std::endl;
        }

        if (!compute) {
            std::cout << std::endl;
            std::cout << "For the requests above, compute the seek, rotate, and transfer times." << std::endl;
            std::cout << "Use -c or the graphical mode (-G) to see the answers." << std::endl;
            std::cout << std::endl;
        }

        tracks.resize(3);
        tracks[0] = 140.0;
        tracks[1] = tracks[0] - trackWidth;
        tracks[2] = tracks[1] - trackWidth;

        if (seekSpeed > 1 && std::fmod(trackWidth, seekSpeed) != 0) {
            std::cerr << "Seek speed (" << seekSpeed << ") must divide evenly into track width (" << trackWidth << ")" << std::endl;
            std::exit(1);
        }
        if (seekSpeed < 1) {
            double x = trackWidth / seekSpeed;
            int y = static_cast<int>(x);
            if (x != static_cast<double>(y)) {
                std::cerr << "Seek speed (" << seekSpeed << ") must divide evenly into track width (" << trackWidth << ")" << std::endl;
                std::exit(1);
            }
        }

        spindleX = cx;
        spindleY = cy;

        armTrack = 0;
        armSpeedBase = seekSpeed;

        double distFromSpindle = tracks[armTrack];
        armX = spindleX - (distFromSpindle * std::cos(0.0 * 4 / 180.0));
        armX1 = armX - armWidth;
        armX2 = armX + armWidth;
        armY1 = 50.0;
        armY2 = width / 2.0;
        headX1 = armX - headWidth;
        headX2 = armX + headWidth;
        headY1 = armY2 - headWidth;
        headY2 = armY2 + headWidth;

        requestCount = 0;
        for (size_t index = 0; index < requests.size(); ++index) {
            AddQueueEntry(requests[index], static_cast<int>(index));
        }

        currWindow = window;

        currentIndex = -1;
        currentBlock = -1;

        state = STATE_NULL;

        angle = 0.0;

        timer = 0;

        seekTotal = 0.0;
        rotTotal = 0.0;
        xferTotal = 0.0;

        isDone = false;
    }

    void InitBlockLayout() {
        auto zones = split(zoning_str, ',');
        if (zones.size() != 3) {
            std::cerr << "Zoning must have exactly 3 values" << std::endl;
            std::exit(1);
        }
        for (const auto& z : zones) {
            blockAngleOffset.push_back(std::stoi(z) / 2);
        }

        int pblock = 0;
        tracksBeginEnd.resize(3);

        // Track 0 (outer)
        int track = 0;
        int angleOffset = 2 * blockAngleOffset[track];
        int skew_local = 0;
        for (int angle = 0; angle < 360; angle += angleOffset) {
            int block = (angle / angleOffset) + pblock;
            blockToTrack.push_back(track);
            blockToAngle.push_back(static_cast<double>(angle + (angleOffset * skew_local)));
        }
        int last_block = (360 - angleOffset) / angleOffset + pblock;
        tracksBeginEnd[track] = {pblock, last_block};
        pblock = last_block + 1;

        // Track 1 (middle)
        track = 1;
        angleOffset = 2 * blockAngleOffset[track];
        skew_local = skew;
        for (int angle = 0; angle < 360; angle += angleOffset) {
            int block = (angle / angleOffset) + pblock;
            blockToTrack.push_back(track);
            blockToAngle.push_back(static_cast<double>(angle + (angleOffset * skew_local)));
        }
        last_block = (360 - angleOffset) / angleOffset + pblock;
        tracksBeginEnd[track] = {pblock, last_block};
        pblock = last_block + 1;

        // Track 2 (inner)
        track = 2;
        angleOffset = 2 * blockAngleOffset[track];
        skew_local = 2 * skew;
        for (int angle = 0; angle < 360; angle += angleOffset) {
            int block = (angle / angleOffset) + pblock;
            blockToTrack.push_back(track);
            blockToAngle.push_back(static_cast<double>(angle + (angleOffset * skew_local)));
        }
        last_block = (360 - angleOffset) / angleOffset + pblock;
        tracksBeginEnd[track] = {pblock, last_block};
        pblock = last_block + 1;

        maxBlock = pblock;

        for (int i = 0; i < maxBlock; ++i) {
            blockToAngle[i] = std::fmod(blockToAngle[i] + 180.0, 360.0);
            if (blockToAngle[i] < 0) blockToAngle[i] += 360.0;
        }
    }

    std::vector<int> MakeRequests(const std::string& a, const std::string& adesc) {
        if (a == "-1") {
            auto desc = split(adesc, ',');
            if (desc.size() != 3) {
                std::cerr << "Bad address description (" << adesc << ")" << std::endl;
                std::cerr << "The address description must be a comma-separated list of length three, without spaces." << std::endl;
                std::cerr << "For example, \"10,100,0\" would indicate that 10 addresses should be generated, with" << std::endl;
                std::cerr << "100 as the maximum value, and 0 as the minumum. A max of -1 means just use the highest" << std::endl;
                std::cerr << "possible value as the max address to generate." << std::endl;
                std::exit(1);
            }
            int numRequests = std::stoi(desc[0]);
            int maxRequest = std::stoi(desc[1]);
            int minRequest = std::stoi(desc[2]);
            if (maxRequest == -1) maxRequest = maxBlock;
            std::vector<int> tmpList;
            for (int i = 0; i < numRequests; ++i) {
                int val = static_cast<int>((static_cast<double>(std::rand()) / RAND_MAX) * (maxRequest - minRequest)) + minRequest;
                tmpList.push_back(val);
            }
            return tmpList;
        } else {
            auto vals = split(a, ',');
            std::vector<int> tmpList;
            for (const auto& v : vals) {
                tmpList.push_back(std::stoi(v));
            }
            return tmpList;
        }
    }

    void Go() {
        GetNextIO();
        while (!isDone) {
            Animate();
        }
    }

    void AddQueueEntry(int block, int index) {
        requestQueue.push_back({block, index});
        requestState.push_back(STATE_NULL);
    }

    void AddRequest(int block) {
        AddQueueEntry(block, static_cast<int>(requestQueue.size()));
    }

    void SwitchState(int newState) {
        state = newState;
        requestState[currentIndex] = newState;
    }

    bool RadiallyCloseTo(double a1, double a2) {
        double v = std::fabs(a1 - a2);
        return v < rotateSpeed;
    }

    bool DoneWithTransfer() {
        int angleOffset = blockAngleOffset[armTrack];
        double target = std::fmod(blockToAngle[currentBlock] + angleOffset, 360.0);
        if (target < 0) target += 360.0;
        if (RadiallyCloseTo(angle, target)) {
            SwitchState(STATE_DONE);
            requestCount++;
            return true;
        }
        return false;
    }

    bool DoneWithRotation() {
        int angleOffset = blockAngleOffset[armTrack];
        double target = std::fmod(blockToAngle[currentBlock] - angleOffset, 360.0);
        if (target < 0) target += 360.0;
        if (RadiallyCloseTo(angle, target)) {
            SwitchState(STATE_XFER);
            return true;
        }
        return false;
    }

    void PlanSeek(int track) {
        seekBegin = timer;
        SwitchState(STATE_SEEK);
        if (track == armTrack) {
            rotBegin = timer;
            SwitchState(STATE_ROTATE);
            return;
        }
        armTarget = track;
        armTargetX1 = spindleX - tracks[track] - (trackWidth / 2.0);
        if (track >= armTrack) {
            armSpeed = armSpeedBase;
        } else {
            armSpeed = -armSpeedBase;
        }
    }

    bool DoneWithSeek() {
        armX1 += armSpeed;
        armX2 += armSpeed;
        headX1 += armSpeed;
        headX2 += armSpeed;
        if ((armSpeed > 0.0 && armX1 >= armTargetX1) || (armSpeed < 0.0 && armX1 <= armTargetX1)) {
            armTrack = armTarget;
            return true;
        }
        return false;
    }

    std::pair<int, int> DoSATF(const std::vector<std::pair<int, int>>& rList) {
        double minEst = -1.0;
        int minBlock = -1;
        int minIndex = -1;
        for (const auto& p : rList) {
            int block = p.first;
            int index = p.second;
            if (requestState[index] == STATE_DONE) continue;
            int track = blockToTrack[block];
            double bangle = blockToAngle[block];
            double dist = std::fabs(static_cast<double>(armTrack - track));
            double seekEst = (trackWidth / armSpeedBase) * dist;
            int angleOffset = blockAngleOffset[track];
            double angleAtArrival = angle + (seekEst * rotateSpeed);
            while (angleAtArrival > 360.0) angleAtArrival -= 360.0;
            while (angleAtArrival < 0.0) angleAtArrival += 360.0;
            double rotDist = (bangle - static_cast<double>(angleOffset)) - angleAtArrival;
            while (rotDist > 360.0) rotDist -= 360.0;
            while (rotDist < 0.0) rotDist += 360.0;
            double rotEst = rotDist / rotateSpeed;
            double xferEst = (static_cast<double>(angleOffset) * 2.0) / rotateSpeed;
            double totalEst = seekEst + rotEst + xferEst;
            if (minEst == -1.0 || totalEst < minEst) {
                minEst = totalEst;
                minBlock = block;
                minIndex = index;
            }
        }
        totalEst = minEst;
        return {minBlock, minIndex};
    }

    std::vector<std::pair<int, int>> DoSSTF(const std::vector<std::pair<int, int>>& rList) {
        int minDist = MAXTRACKS;
        std::vector<std::pair<int, int>> trackList;
        for (const auto& p : rList) {
            int block = p.first;
            int index = p.second;
            if (requestState[index] == STATE_DONE) continue;
            int track = blockToTrack[block];
            int dist = std::abs(armTrack - track);
            if (dist < minDist) {
                trackList.clear();
                trackList.push_back(p);
                minDist = dist;
            } else if (dist == minDist) {
                trackList.push_back(p);
            }
        }
        return trackList;
    }

    void UpdateWindow() {
        if (fairWindow == -1 && currWindow > 0 && currWindow < static_cast<int>(requestQueue.size())) {
            currWindow++;
        }
    }

    int GetWindow() {
        if (currWindow <= -1) {
            return static_cast<int>(requestQueue.size());
        } else {
            if (fairWindow != -1) {
                if (requestCount > 0 && (requestCount % fairWindow == 0)) {
                    currWindow += fairWindow;
                }
                return currWindow;
            } else {
                return currWindow;
            }
        }
    }

    void GetNextIO() {
        if (requestCount == static_cast<int>(requestQueue.size())) {
            PrintStats();
            isDone = true;
            return;
        }
        if (policy == "FIFO") {
            auto p = requestQueue[requestCount];
            currentBlock = p.first;
            currentIndex = p.second;
            DoSATF({p});
        } else if (policy == "SATF" || policy == "BSATF") {
            int endIndex = GetWindow();
            if (endIndex > static_cast<int>(requestQueue.size())) endIndex = static_cast<int>(requestQueue.size());
            std::vector<std::pair<int, int>> rList(requestQueue.begin(), requestQueue.begin() + endIndex);
            auto res = DoSATF(rList);
            currentBlock = res.first;
            currentIndex = res.second;
        } else if (policy == "SSTF") {
            int endIndex = GetWindow();
            if (endIndex > static_cast<int>(requestQueue.size())) endIndex = static_cast<int>(requestQueue.size());
            std::vector<std::pair<int, int>> rList(requestQueue.begin(), requestQueue.begin() + endIndex);
            auto trackList = DoSSTF(rList);
            auto res = DoSATF(trackList);
            currentBlock = res.first;
            currentIndex = res.second;
        } else {
            std::cerr << "policy (" << policy << ") not implemented" << std::endl;
            std::exit(1);
        }
        PlanSeek(blockToTrack[currentBlock]);
        if (!lateRequests.empty() && lateCount < static_cast<int>(lateRequests.size())) {
            AddRequest(lateRequests[lateCount]);
            lateCount++;
        }
    }

    void Animate() {
        timer++;
        angle += rotateSpeed;
        if (angle >= 360.0) angle -= 360.0;
        if (angle < 0.0) angle += 360.0;
        if (state == STATE_SEEK) {
            if (DoneWithSeek()) {
                rotBegin = timer;
                SwitchState(STATE_ROTATE);
            }
        }
        if (state == STATE_ROTATE) {
            if (DoneWithRotation()) {
                xferBegin = timer;
                SwitchState(STATE_XFER);
            }
        }
        if (state == STATE_XFER) {
            if (DoneWithTransfer()) {
                DoRequestStats();
                SwitchState(STATE_DONE);
                UpdateWindow();
                int currentBlock_save = currentBlock;
                GetNextIO();
                int nextBlock = currentBlock;
                if (blockToTrack[currentBlock_save] == blockToTrack[nextBlock]) {
                    if ((currentBlock_save == tracksBeginEnd[armTrack].second && nextBlock == tracksBeginEnd[armTrack].first) ||
                        (currentBlock_save + 1 == nextBlock)) {
                        rotBegin = seekBegin = xferBegin = timer;
                        SwitchState(STATE_XFER);
                    }
                }
            }
        }
    }

    void DoRequestStats() {
        int seekTime = rotBegin - seekBegin;
        int rotTime = xferBegin - rotBegin;
        int xferTime = timer - xferBegin;
        int totalTime = timer - seekBegin;
        if (compute) {
            std::cout << "Block: " << std::setw(3) << currentBlock
                      << "  Seek:" << std::setw(3) << seekTime
                      << "  Rotate:" << std::setw(3) << rotTime
                      << "  Transfer:" << std::setw(3) << xferTime
                      << "  Total:" << std::setw(4) << totalTime << std::endl;
        }
        seekTotal += seekTime;
        rotTotal += rotTime;
        xferTotal += xferTime;
    }

    void PrintStats() {
        if (compute) {
            std::cout << "\nTOTALS      Seek:" << std::setw(3) << seekTotal
                      << "  Rotate:" << std::setw(3) << rotTotal
                      << "  Transfer:" << std::setw(3) << xferTotal
                      << "  Total:" << std::setw(4) << timer << std::endl << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    std::string seed_str = "0";
    std::string addr = "-1";
    std::string addrDesc = "5,-1,0";
    std::string seekSpeed_str = "1";
    std::string rotateSpeed_str = "1";
    std::string policy = "FIFO";
    int window = -1;
    int skew = 0;
    std::string zoning = "30,30,30";
    std::string lateAddr = "-1";
    std::string lateAddrDesc = "0,-1,0";
    bool compute = false;

    static struct option long_options[] = {
        {"seed", required_argument, NULL, 's'},
        {"addr", required_argument, NULL, 'a'},
        {"addrDesc", required_argument, NULL, 'A'},
        {"seekSpeed", required_argument, NULL, 'S'},
        {"rotSpeed", required_argument, NULL, 'R'},
        {"policy", required_argument, NULL, 'p'},
        {"schedWindow", required_argument, NULL, 'w'},
        {"skewOffset", required_argument, NULL, 'o'},
        {"zoning", required_argument, NULL, 'z'},
        {"lateAddr", required_argument, NULL, 'l'},
        {"lateAddrDesc", required_argument, NULL, 'L'},
        {"compute", no_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "s:a:A:S:R:p:w:o:z:l:L:c", long_options, NULL)) != -1) {
        switch (opt) {
            case 's': seed_str = optarg; break;
            case 'a': addr = optarg; break;
            case 'A': addrDesc = optarg; break;
            case 'S': seekSpeed_str = optarg; break;
            case 'R': rotateSpeed_str = optarg; break;
            case 'p': policy = optarg; break;
            case 'w': window = std::stoi(optarg); break;
            case 'o': skew = std::stoi(optarg); break;
            case 'z': zoning = optarg; break;
            case 'l': lateAddr = optarg; break;
            case 'L': lateAddrDesc = optarg; break;
            case 'c': compute = true; break;
            default: std::exit(1);
        }
    }

    int seed = std::stoi(seed_str);
    std::srand(seed);

    double seekSpeed = std::stod(seekSpeed_str);
    double rotateSpeed = std::stod(rotateSpeed_str);

    if (window == 0) {
        std::cerr << "Scheduling window (" << window << ") must be positive or -1 (which means a full window)" << std::endl;
        std::exit(1);
    }

    std::cout << "OPTIONS seed " << seed << std::endl;
    std::cout << "OPTIONS addr " << addr << std::endl;
    std::cout << "OPTIONS addrDesc " << addrDesc << std::endl;
    std::cout << "OPTIONS seekSpeed " << seekSpeed << std::endl;
    std::cout << "OPTIONS rotateSpeed " << rotateSpeed << std::endl;
    std::cout << "OPTIONS skew " << skew << std::endl;
    std::cout << "OPTIONS window " << window << std::endl;
    std::cout << "OPTIONS policy " << policy << std::endl;
    std::cout << "OPTIONS compute " << compute << std::endl;
    std::cout << "OPTIONS zoning " << zoning << std::endl;
    std::cout << "OPTIONS lateAddr " << lateAddr << std::endl;
    std::cout << "OPTIONS lateAddrDesc " << lateAddrDesc << std::endl;
    std::cout << std::endl;

    Disk d(addr, addrDesc, lateAddr, lateAddrDesc, policy, seekSpeed, rotateSpeed, skew, window, compute, zoning);
    d.Go();

    return 0;
}