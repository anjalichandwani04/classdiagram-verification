#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>


const int SECTORS_PER_TRACK = 12;
const int DEGREES_PER_SECTOR = 360 / SECTORS_PER_TRACK;
const double ROTATION_SPEED = 1.0; 
const double SEEK_SPEED = 1.0;     
const int TRACK_DISTANCE = 40;     
const int TRANSFER_TIME = 30;      


// Data structures
struct DiskRequest {
    int sector;
    int track;
    int angle;
    double seekTime;
    double rotateTime;
    double transferTime;
    double totalTime;
};


// Helper functions


int getTrack(int sector) {
    // sectors 0–11 outer, 12–23 are  middle, 24–35 are inner 
    return sector / SECTORS_PER_TRACK;
}

int getAngle(int sector) {
    // each is 30 degrees
    int sectorInTrack = sector % SECTORS_PER_TRACK;
    return sectorInTrack * DEGREES_PER_SECTOR;
}

// Counter-clockwise rotation
int rotationDistanceCCW(int from, int to) {
    if (to >= from) return to - from;
    return 360 - (from - to);
}

int main(int argc, char *argv[]) {
    std::vector<int> requests;
    bool computeOnly = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-a" && i + 1 < argc) {
            std::stringstream ss(argv[++i]);
            std::string token;
            while (std::getline(ss, token, ',')) {
                requests.push_back(std::stoi(token));
            }
        } else if (arg == "-c") {
            computeOnly = true;
        } else {
            std::cerr << "Usage: ./disk -a <sector list> [-c]\n";
            return 1;
        }
    }

    if (requests.empty()) {
        std::cerr << "No requests specified.\n";
        return 1;
    }

    // Initial state
    int headTrack = 0;   
    int headAngle = 180; // sector 6 = 180 degree, halfway = 180 degree

    double totalSeek = 0, totalRotate = 0, totalTransfer = 0, totalTime = 0;

    // Process each request
    for (size_t i = 0; i < requests.size(); i++) {
        int sector = requests[i];
        int targetTrack = getTrack(sector);
        int targetAngle = getAngle(sector);

        DiskRequest r;
        r.sector = sector;
        r.track = targetTrack;
        r.angle = targetAngle;

        double seekDist = std::abs(targetTrack - headTrack) * TRACK_DISTANCE;
        r.seekTime = seekDist / SEEK_SPEED;

        int beginAngle = (targetAngle - DEGREES_PER_SECTOR / 2 + 360) % 360;
        int endAngle = (targetAngle + DEGREES_PER_SECTOR / 2) % 360;

        double rotationDegrees = rotationDistanceCCW(headAngle, beginAngle);
        r.rotateTime = rotationDegrees / ROTATION_SPEED;

        if (r.seekTime > 0 && r.rotateTime > r.seekTime) {
            r.rotateTime -= r.seekTime;
        } else if (r.seekTime > r.rotateTime) {
            r.rotateTime = 0; 
        }

        r.transferTime = TRANSFER_TIME;

        // TOTAL
        r.totalTime = r.seekTime + r.rotateTime + r.transferTime;

        std::cout << "Sector:" << std::setw(4) << r.sector
                  << "  Seek:" << std::setw(4) << (int)r.seekTime
                  << "  Rotate:" << std::setw(4) << (int)r.rotateTime
                  << "  Transfer:" << std::setw(4) << (int)r.transferTime
                  << "  Total:" << std::setw(4) << (int)r.totalTime << "\n";

        totalSeek += r.seekTime;
        totalRotate += r.rotateTime;
        totalTransfer += r.transferTime;
        totalTime += r.totalTime;

        headTrack = targetTrack;
        headAngle = endAngle;
    }


    std::cout << "TOTALS       Seek:" << std::setw(4) << (int)totalSeek
              << "  Rotate:" << std::setw(4) << (int)totalRotate
              << "  Transfer:" << std::setw(4) << (int)totalTransfer
              << "  Total:" << std::setw(4) << (int)totalTime << "\n";

    return 0;
}