// Converted from disk.py (non-GUI version).
// Keeps original variable and function names as in the Python source.

#include <bits/stdc++.h>
#include <getopt.h>
using namespace std;

#define MAXTRACKS 1000

// states that a request/disk go through
const int STATE_NULL   = 0;
const int STATE_SEEK   = 1;
const int STATE_ROTATE = 2;
const int STATE_XFER   = 3;
const int STATE_DONE   = 4;

struct BlockInfo {
    int track;
    double angle;
    int name;
    BlockInfo(int t=0,double a=0,int n=0):track(t),angle(a),name(n){}
};

struct OptionValues {
    int seed = 0;
    string addr = "-1";
    string addrDesc = "5,-1,0";
    string seekSpeed = "1";
    string rotateSpeed = "1";
    string policy = "FIFO";
    int window = -1;
    int skew = 0;
    string zoning = "30,30,30";
    bool graphics = false;
    string lateAddr = "-1";
    string lateAddrDesc = "0,-1,0";
    bool compute = false;
} options_global;

static void random_seed_fun(int seed) {
    srand(seed);
}

class Disk {
public:
    // options passed in
    string addr, addrDesc, lateAddr, lateAddrDesc, policy, zoning;
    double seekSpeed, rotateSpeed;
    int skew;
    int window;
    bool compute;
    bool graphics;

    // zoning/blocks
    vector<BlockInfo> blockInfoList;
    unordered_map<int,int> blockToTrackMap;
    unordered_map<int,double> blockToAngleMap;
    unordered_map<int,pair<int,int>> tracksBeginEnd; // begin,end for each track
    vector<int> blockAngleOffset;
    int maxBlock;

    // requests
    vector<int> requests;        // list of request block numbers
    vector<int> lateRequests;
    int lateCount;

    // scheduling/fairness
    int fairWindow;
    int currWindow;

    // queue / state
    vector<pair<int,int>> requestQueue; // (block, index)
    vector<int> requestState; // STATE_*
    int requestCount;

    // disk geometry simulated
    int trackWidth;

    // arm state
    int armTrack;
    double armSpeedBase; // base speed
    double armSpeed;     // sign indicates direction in original gui; we'll use durations
    // we simulate timings rather than coords
    double targetSize;

    // timing
    double angle; // rotation angle [0..360)
    double rotateSpeedPerTick; // rotateSpeed variable
    double timer; // integer ticks in python; keep as double

    // stats
    double seekTotal, rotTotal, xferTotal;

    // per-request timing bookkeeping
    double seekBegin, rotBegin, xferBegin;
    double currSeekDuration, currRotDuration, currXferDuration;
    int currentIndex;
    int currentBlock;
    int state; // STATE_*

    // constructor
    Disk(string addr_, string addrDesc_, string lateAddr_, string lateAddrDesc_,
         string policy_, double seekSpeed_, double rotateSpeed_, int skew_, int window_,
         bool compute_, bool graphics_, string zoning_)
    : addr(addr_), addrDesc(addrDesc_), lateAddr(lateAddr_), lateAddrDesc(lateAddrDesc_),
      policy(policy_), seekSpeed(seekSpeed_), rotateSpeed(rotateSpeed_), skew(skew_), window(window_),
      compute(compute_), graphics(graphics_), zoning(zoning_)
    {
        // initialize
        blockInfoList.clear();
        blockToTrackMap.clear();
        blockToAngleMap.clear();
        tracksBeginEnd.clear();
        blockAngleOffset.clear();
        requests.clear();
        lateRequests.clear();
        fairWindow = -1;

        // figure out zones / layout
        InitBlockLayout();

        // requests
        random_seed_fun(options_global.seed);
        requests = MakeRequests(this->addr, this->addrDesc);
        lateRequests = MakeRequests(this->lateAddr, this->lateAddrDesc);

        if (policy == "BSATF" && window != -1) {
            fairWindow = window;
        } else {
            fairWindow = -1;
        }

        // fairness stuff
        if (lateRequests.size() > 0) lateCount = 0;
        else lateCount = 0;

        // track info
        trackWidth = 40;

        if ((seekSpeed > 1.0) && ( (trackWidth % (int)seekSpeed) != 0 )) {
            cerr << "Seek speed (" << seekSpeed << ") must divide evenly into track width (" << trackWidth << ")\n";
            exit(1);
        }

        armTrack = 0;
        armSpeedBase = seekSpeed;
        armSpeed = armSpeedBase;

        targetSize = 10.0;

        // queue and initial states
        requestCount = 0;
        for (size_t i=0;i<requests.size();++i) {
            AddQueueEntry(requests[i], (int)i);
        }

        currWindow = window;
        if (currWindow == -1) currWindow = (int)requestQueue.size();

        angle = 0.0;
        rotateSpeedPerTick = rotateSpeed;
        timer = 0.0;

        // stats
        seekTotal = 0.0;
        rotTotal  = 0.0;
        xferTotal = 0.0;

        // state
        doAnimate = false;
        isDone = false;
        currentIndex = -1;
        currentBlock = -1;
        state = STATE_NULL;

        // print initial lists similar to Python
        cout << "REQUESTS ";
        for (size_t i=0;i<requests.size();++i) {
            if (i) cout << ",";
            cout << requests[i];
        }
        cout << "\n\n";
        if (lateRequests.size() > 0) {
            cout << "LATE REQUESTS ";
            for (size_t i=0;i<lateRequests.size();++i) {
                if (i) cout << ",";
                cout << lateRequests[i];
            }
            cout << "\n\n";
        }

        if (compute == false) {
            cout << "\nFor the requests above, compute the seek, rotate, and transfer times.\nUse -c to see the answers.\n\n";
        }
    }

    // call this to start simulation
    void Go() {
        // no GUI mainloop: just run simulation until done
        GetNextIO();
        while (!isDone) {
            Animate();
        }
    }

    // helper to print message and exit for addrDesc errors
    void PrintAddrDescMessage(const string &value) {
        cerr << "Bad address description (" << value << ")\n";
        cerr << "The address description must be a comma-separated list of length three, without spaces.\n";
        cerr << "For example, \"10,100,0\" would indicate that 10 addresses should be generated, with\n";
        cerr << "100 as the maximum value, and 0 as the minumum. A max of -1 means just use the highest\n";
        cerr << "possible value as the max address to generate.\n";
        exit(1);
    }

    // ZONES AND BLOCK LAYOUT
    void InitBlockLayout() {
        blockInfoList.clear();
        blockToTrackMap.clear();
        blockToAngleMap.clear();
        tracksBeginEnd.clear();
        blockAngleOffset.clear();

        // parse zoning -> 3 ints
        vector<int> zones;
        {
            string s = zoning;
            stringstream ss(s);
            string item;
            while (getline(ss,item,',')) {
                zones.push_back(stoi(item));
            }
            if (zones.size() != 3) {
                cerr << "zoning must be 3 comma-separated ints\n";
                exit(1);
            }
        }

        for (size_t i=0;i<zones.size();++i) {
            blockAngleOffset.push_back(zones[i] / 2);
        }

        int track = 0;
        int angleOffset = 2 * blockAngleOffset[track];
        int block = 0;
        for (int angle=0; angle<360; angle+=angleOffset) {
            block = angle / angleOffset;
            blockToTrackMap[block] = track;
            blockToAngleMap[block] = angle;
            blockInfoList.emplace_back(track, angle, block);
        }
        tracksBeginEnd[track] = make_pair(0, block);
        int pblock = block + 1;

        track = 1;
        int skew_local = skew;
        angleOffset = 2 * blockAngleOffset[track];
        for (int angle=0; angle<360; angle+=angleOffset) {
            block = (angle / angleOffset) + pblock;
            blockToTrackMap[block] = track;
            blockToAngleMap[block] = angle + (angleOffset * skew_local);
            blockInfoList.emplace_back(track, angle + (angleOffset * skew_local), block);
        }
        tracksBeginEnd[track] = make_pair(pblock, block);
        pblock = block + 1;

        track = 2;
        skew_local = 2 * skew;
        angleOffset = 2 * blockAngleOffset[track];
        for (int angle=0; angle<360; angle+=angleOffset) {
            block = (angle / angleOffset) + pblock;
            blockToTrackMap[block] = track;
            blockToAngleMap[block] = angle + (angleOffset * skew_local);
            blockInfoList.emplace_back(track, angle + (angleOffset * skew_local), block);
        }
        tracksBeginEnd[track] = make_pair(pblock, block);
        maxBlock = pblock;

        // adjust angle to starting position relative
        for (auto &kv : blockToAngleMap) {
            kv.second = fmod(kv.second + 180.0, 360.0);
        }
    }

    // Make requests from addr or addrDesc
    vector<int> MakeRequests(const string &addr_, const string &addrDesc_) {
        vector<int> tmpList;
        if (addr_ == "-1") {
            // parse desc
            vector<string> parts;
            stringstream ss(addrDesc_);
            string token;
            while (getline(ss, token, ',')) parts.push_back(token);
            if (parts.size() != 3) {
                PrintAddrDescMessage(addrDesc_);
            }
            int numRequests = stoi(parts[0]);
            int maxRequest = stoi(parts[1]);
            int minRequest = stoi(parts[2]);
            if (maxRequest == -1) maxRequest = maxBlock;
            for (int i=0;i<numRequests;++i) {
                double r = (double)rand() / (double)RAND_MAX;
                int val = int(r * maxRequest) + minRequest;
                tmpList.push_back(val);
            }
            return tmpList;
        } else {
            // parse comma-separated ints
            string s = addr_;
            stringstream ss(s);
            string token;
            while (getline(ss, token, ',')) {
                if (token.size() == 0) continue;
                tmpList.push_back(stoi(token));
            }
            return tmpList;
        }
    }

    // BUTTONS/QUEUE operations (non-GUI)
    void AddRequest(int block) {
        AddQueueEntry(block, (int)requestQueue.size());
    }

    pair<int,int> QueueMap(int index) {
        int numPerRow = 400 / (this->queueBoxSize);
        return make_pair(index % numPerRow, index / numPerRow);
    }

    // DrawWindow is GUI-only; no-op here
    void DrawWindow() { }

    void AddQueueEntry(int block, int index) {
        requestQueue.emplace_back(block, index);
        requestState.push_back(STATE_NULL);
    }

    void SwitchColors(const string &c) {
        // GUI-only placeholder; don't do anything
        (void)c;
    }

    void SwitchState(int newState) {
        state = newState;
        if (currentIndex >= 0 && currentIndex < (int)requestState.size())
            requestState[currentIndex] = newState;
    }

    bool RadiallyCloseTo(double a1, double a2) {
        double v = fabs(a1 - a2);
        if (v < rotateSpeedPerTick) return true;
        return false;
    }

    // When transfer finishes: checks angular alignment
    bool DoneWithTransfer() {
        int angleOffset = blockAngleOffset[armTrack];
        // same radial check but we are using durations; keep both checks to be safe
        double targetAngle = fmod(blockToAngleMap[currentBlock] + angleOffset, 360.0);
        if (RadiallyCloseTo(angle, targetAngle)) {
            SwitchState(STATE_DONE);
            requestCount += 1;
            return true;
        }
        // fallback: if currXferDuration used, check completion by timer
        if ( (timer - xferBegin) >= currXferDuration - 1e-9 ) {
            SwitchState(STATE_DONE);
            requestCount += 1;
            return true;
        }
        return false;
    }

    bool DoneWithRotation() {
        int angleOffset = blockAngleOffset[armTrack];
        double targetAngle = fmod(blockToAngleMap[currentBlock] - angleOffset + 360.0, 360.0);
        if (RadiallyCloseTo(angle, targetAngle)) {
            SwitchState(STATE_XFER);
            return true;
        }
        // Also check by duration
        if ( (timer - rotBegin) >= currRotDuration - 1e-9 ) {
            SwitchState(STATE_XFER);
            return true;
        }
        return false;
    }

    // Plan a seek to `track`: set seek begin time and compute seek duration
    void PlanSeek(int track) {
        seekBegin = timer;
        SwitchColors("orange");
        SwitchState(STATE_SEEK);

        if (track == armTrack) {
            // no seek necessary
            rotBegin = timer;
            SwitchColors("lightblue");
            SwitchState(STATE_ROTATE);

            // compute rotation duration now based on current angle and target
            int angleOffset = blockAngleOffset[armTrack];
            double targetAngle = fmod(blockToAngleMap[currentBlock] - angleOffset + 360.0, 360.0);
            double angleAtArrival = angle; // since no seek delay
            double rotDist = targetAngle - angleAtArrival;
            while (rotDist < 0.0) rotDist += 360.0;
            while (rotDist >= 360.0) rotDist -= 360.0;
            currRotDuration = rotDist / rotateSpeedPerTick;
            currXferDuration = (angleOffset * 2.0) / rotateSpeedPerTick;
            rotBegin = timer;
            xferBegin = rotBegin + currRotDuration;
            return;
        }

        // compute seek duration estimate similar to DoSATF's seekEst
        int dist = abs(armTrack - track);
        double seekEst = (trackWidth / armSpeedBase) * (double)dist;
        currSeekDuration = seekEst;
        // set arm target for when seek completes
        armTarget = track;
    }

    // this returns (minBlock, minIndex) for SATF (shortest access time first)
    pair<int,int> DoSATF(const vector<pair<int,int>> &rList) {
        int minBlock = -1;
        int minIndex = -1;
        double minEst = -1;

        for (const auto &p : rList) {
            int block = p.first;
            int index = p.second;
            if (requestState[index] == STATE_DONE) continue;
            int track = blockToTrackMap[block];
            double angleAt = blockToAngleMap[block];

            int dist = (int)fabs(armTrack - track);
            double seekEst = (trackWidth / armSpeedBase) * dist;

            int angleOffset = blockAngleOffset[track];
            double angleArrival = angle + (seekEst * rotateSpeedPerTick);
            while (angleArrival > 360.0) angleArrival -= 360.0;

            double rotDist = ((angleAt - angleOffset) - angleArrival);
            while (rotDist > 360.0) rotDist -= 360.0;
            while (rotDist < 0.0) rotDist += 360.0;
            double rotEst = rotDist / rotateSpeedPerTick;

            double xferEst = (angleOffset * 2.0) / rotateSpeedPerTick;

            double totalEst = seekEst + rotEst + xferEst;

            if (minEst < 0 || totalEst < minEst) {
                minEst = totalEst;
                minBlock = block;
                minIndex = index;
            }
        }
        // store for diagnostics if needed
        totalEst = minEst;
        if (minBlock == -1 || minIndex == -1) {
            cerr << "DoSATF failed to find a block\n";
            exit(1);
        }
        return make_pair(minBlock, minIndex);
    }

    // DoSSTF: select all blocks on nearest track (closest track distance)
    vector<pair<int,int>> DoSSTF(const vector<pair<int,int>> &rList) {
        int minDist = MAXTRACKS;
        vector<pair<int,int>> trackList;
        for (const auto &p : rList) {
            int block = p.first;
            int index = p.second;
            if (requestState[index] == STATE_DONE) continue;
            int track = blockToTrackMap[block];
            int dist = (int)fabs(armTrack - track);
            if (dist < minDist) {
                trackList.clear();
                trackList.emplace_back(block, index);
                minDist = dist;
            } else if (dist == minDist) {
                trackList.emplace_back(block, index);
            }
        }
        if (trackList.empty()) {
            cerr << "DoSSTF found nothing (shouldn't happen)\n";
            exit(1);
        }
        return trackList;
    }

    void UpdateWindow() {
        if (fairWindow == -1 && currWindow > 0 && currWindow < (int)requestQueue.size()) {
            currWindow += 1;
        }
    }

    int GetWindow() {
        if (currWindow <= -1) return (int)requestQueue.size();
        else {
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

    void GetNextIO() {
        // check if done
        if (requestCount == (int)requestQueue.size()) {
            UpdateTime();
            PrintStats();
            doAnimate = false;
            isDone = true;
            return;
        }

        if (policy == "FIFO") {
            currentBlock = requestQueue[requestCount].first;
            currentIndex = requestQueue[requestCount].second;
            // PlanSeek would be called
            vector<pair<int,int>> tmp;
            tmp.push_back(requestQueue[requestCount]);
            DoSATF(tmp); // compute totalEst but don't use it
        } else if (policy == "SATF" || policy == "BSATF") {
            int endIndex = GetWindow();
            if (endIndex > (int)requestQueue.size()) endIndex = (int)requestQueue.size();
            vector<pair<int,int>> sub(requestQueue.begin(), requestQueue.begin()+endIndex);
            auto pr = DoSATF(sub);
            currentBlock = pr.first;
            currentIndex = pr.second;
        } else if (policy == "SSTF") {
            int w = GetWindow();
            vector<pair<int,int>> sub(requestQueue.begin(), requestQueue.begin()+min(w,(int)requestQueue.size()));
            auto trackList = DoSSTF(sub);
            auto pr = DoSATF(trackList);
            currentBlock = pr.first;
            currentIndex = pr.second;
        } else {
            cerr << "policy (" << policy << ") not implemented\n";
            exit(1);
        }

        // plan the seek now to selected block
        PlanSeek(blockToTrackMap[currentBlock]);

        // add another late block if present
        if (lateRequests.size() > 0 && lateCount < (int)lateRequests.size()) {
            AddRequest(lateRequests[lateCount]);
            lateCount += 1;
        }
    }

    // Animate one tick (non-graphical)
    void Animate() {
        // tick
        timer += 1.0;
        UpdateTime();

        // angle update (rotation)
        angle += rotateSpeedPerTick;
        if (angle >= 360.0) angle -= 360.0;

        // handle state transitions based on durations and timing
        if (state == STATE_SEEK) {
            // check if done with seek (use currSeekDuration computed in PlanSeek)
            if ( (timer - seekBegin) >= currSeekDuration - 1e-9 ) {
                // seek complete: update armTrack
                armTrack = armTarget;
                rotBegin = timer;
                SwitchState(STATE_ROTATE);
                SwitchColors("lightblue");

                // compute rotation duration based on computed values
                int angleOffset = blockAngleOffset[armTrack];
                double targetAng = fmod(blockToAngleMap[currentBlock] - angleOffset + 360.0, 360.0);
                double angleAtArrival = angle; // after we've applied one tick; approximate
                double rotDist = targetAng - angleAtArrival;
                while (rotDist < 0.0) rotDist += 360.0;
                while (rotDist >= 360.0) rotDist -= 360.0;
                currRotDuration = rotDist / rotateSpeedPerTick;
                currXferDuration = (angleOffset * 2.0) / rotateSpeedPerTick;
                rotBegin = timer;
                xferBegin = rotBegin + currRotDuration;
            }
        }

        if (state == STATE_ROTATE) {
            if (DoneWithRotation()) {
                xferBegin = timer;
                SwitchState(STATE_XFER);
                SwitchColors("green");
            }
        }

        if (state == STATE_XFER) {
            if (DoneWithTransfer()) {
                DoRequestStats();
                SwitchState(STATE_DONE);
                SwitchColors("red");
                UpdateWindow();
                int prevBlock = currentBlock;
                GetNextIO();
                int nextBlock = currentBlock;
                // special case: if staying on same track and maybe sequential blocks,
                // original code special-cased to remain in XFER. We'll emulate that:
                if (blockToTrackMap[prevBlock] == blockToTrackMap[nextBlock]) {
                    auto bounds = tracksBeginEnd[armTrack];
                    if ( (prevBlock == bounds.second && nextBlock == bounds.first) || (prevBlock + 1 == nextBlock) ) {
                        // stay in transfer mode
                        rotBegin = timer;
                        seekBegin = timer;
                        xferBegin = timer;
                        SwitchState(STATE_XFER);
                        SwitchColors("green");
                    }
                }
            }
        }
        // continue until done
        if (!isDone) {
            // loop will call Animate again
        }
    }

    void DoRequestStats() {
        double seekTime  = rotBegin  - seekBegin;
        double rotTime   = xferBegin - rotBegin;
        double xferTime  = timer     - xferBegin;
        double totalTime = timer     - seekBegin;

        if (compute == true) {
            cout << "Block: " << setw(3) << currentBlock
                 << "  Seek:" << setw(3) << (int)seekTime
                 << "  Rotate:" << setw(3) << (int)rotTime
                 << "  Transfer:" << setw(3) << (int)xferTime
                 << "  Total:" << setw(4) << (int)totalTime << "\n";
        }
        seekTotal += seekTime;
        rotTotal  += rotTime;
        xferTotal += xferTime;
    }

    void PrintStats() {
        if (compute == true) {
            cout << "\nTOTALS      Seek:" << (int)seekTotal
                 << "  Rotate:" << (int)rotTotal
                 << "  Transfer:" << (int)xferTotal
                 << "  Total:" << (int)timer << "\n\n";
        }
    }

    void UpdateTime() {
        if (compute) {
            // In GUI code this updated text elements; here we may optionally print timer occasionally.
            // Keep silent to avoid huge output; only print when compute is true at each second tick? we'll skip.
            (void)timer;
        }
    }

private:
    // internal fields not in original python but needed
    bool doAnimate;
    bool isDone;
    int armTarget;
    double totalEst;
    int queueBoxSize = 20;
};

///// Option parsing helper (mimic OptionParser defaults in python script)
void print_usage_and_exit(const char *prog) {
    cerr << "Usage: " << prog << " [options]\n";
    cerr << "Options (same as Python script):\n";
    cerr << "  -s, --seed N\n  -a, --addr LIST (comma-separated or -1)\n  -A, --addrDesc NUM,MAX,MIN\n  -S, --seekSpeed FLOAT\n  -R, --rotSpeed FLOAT\n  -p, --policy STR (FIFO,SSTF,SATF,BSATF)\n  -w, --schedWindow INT\n  -o, --skewOffset INT\n  -z, --zoning STR\n  -G, --graphics (ignored)\n  -l, --lateAddr LIST\n  -L, --lateAddrDesc NUM,MAX,MIN\n  -c, --compute\n";
    exit(1);
}

int main(int argc, char **argv) {
    // set defaults same as Python OptionParser defaults
    OptionValues opts;

    static struct option long_options[] = {
        {"seed", required_argument, 0, 's'},
        {"addr", required_argument, 0, 'a'},
        {"addrDesc", required_argument, 0, 'A'},
        {"seekSpeed", required_argument, 0, 'S'},
        {"rotSpeed", required_argument, 0, 'R'},
        {"policy", required_argument, 0, 'p'},
        {"schedWindow", required_argument, 0, 'w'},
        {"skewOffset", required_argument, 0, 'o'},
        {"zoning", required_argument, 0, 'z'},
        {"graphics", no_argument, 0, 'G'},
        {"lateAddr", required_argument, 0, 'l'},
        {"lateAddrDesc", required_argument, 0, 'L'},
        {"compute", no_argument, 0, 'c'},
        {0,0,0,0}
    };

    int c;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "s:a:A:S:R:p:w:o:z:Gl:L:c", long_options, &option_index)) != -1) {
        switch (c) {
            case 's': opts.seed = atoi(optarg); break;
            case 'a': opts.addr = string(optarg); break;
            case 'A': opts.addrDesc = string(optarg); break;
            case 'S': opts.seekSpeed = string(optarg); break;
            case 'R': opts.rotateSpeed = string(optarg); break;
            case 'p': opts.policy = string(optarg); break;
            case 'w': opts.window = atoi(optarg); break;
            case 'o': opts.skew = atoi(optarg); break;
            case 'z': opts.zoning = string(optarg); break;
            case 'G': opts.graphics = true; break;
            case 'l': opts.lateAddr = string(optarg); break;
            case 'L': opts.lateAddrDesc = string(optarg); break;
            case 'c': opts.compute = true; break;
            case '?':
            default:
                print_usage_and_exit(argv[0]);
        }
    }

    // mirror printouts from python script
    cout << "OPTIONS seed " << opts.seed << "\n";
    cout << "OPTIONS addr " << opts.addr << "\n";
    cout << "OPTIONS addrDesc " << opts.addrDesc << "\n";
    cout << "OPTIONS seekSpeed " << opts.seekSpeed << "\n";
    cout << "OPTIONS rotateSpeed " << opts.rotateSpeed << "\n";
    cout << "OPTIONS skew " << opts.skew << "\n";
    cout << "OPTIONS window " << opts.window << "\n";
    cout << "OPTIONS policy " << opts.policy << "\n";
    cout << "OPTIONS compute " << (opts.compute ? "True" : "False") << "\n";
    cout << "OPTIONS graphics " << (opts.graphics ? "True" : "False") << "\n";
    cout << "OPTIONS zoning " << opts.zoning << "\n";
    cout << "OPTIONS lateAddr " << opts.lateAddr << "\n";
    cout << "OPTIONS lateAddrDesc " << opts.lateAddrDesc << "\n\n";

    if (opts.window == 0) {
        cerr << "Scheduling window (" << opts.window << ") must be positive or -1 (which means a full window)\n";
        return 1;
    }

    if (opts.graphics && opts.compute == false) {
        cout << "\nWARNING: Setting compute flag to True, as graphics are on\n\n";
        opts.compute = true;
    }

    // prepare global options for random seed function
    options_global.seed = opts.seed;

    // convert numeric speeds
    double seekSpeed_d = stod(opts.seekSpeed);
    double rotateSpeed_d = stod(opts.rotateSpeed);

    Disk d(opts.addr, opts.addrDesc, opts.lateAddr, opts.lateAddrDesc,
           opts.policy, seekSpeed_d, rotateSpeed_d, opts.skew, opts.window,
           opts.compute, opts.graphics, opts.zoning);

    // run simulation
    d.Go();

    return 0;
}