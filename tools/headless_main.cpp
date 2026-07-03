// Headless GameBoy harness for automated playtesting.
//
// Runs GBLib with no window: scripted joypad input, BMP screenshots, serial
// output capture (blargg test ROMs print results over the link port).
//
// Usage:
//   GBHeadless <romFolder> <romFile> [options]
//     --frames N            stop after N emulated frames (default 600)
//     --script FILE         input/screenshot script (see below)
//     --out DIR             directory for screenshots (default ".")
//     --emutype MODE        cart|dmg|cgb (default cart)
//     --save                write battery save on exit
//
// Diagnostics (see Debugger.h):
//     --debug               dump CPU/IO/VRAM state at end of run
//     --profile             print hot-PC histogram over the last 120 frames
//     --watch LO HI         log writes in [LO, HI] (hex)
//     --watchmin V          only log watched writes with value >= V (hex)
//     --pcwatch PC LO HI    dump registers at PC when BC outside [LO, HI] (hex)
//     --pcwatcha PC A       dump registers at PC when register A == A (hex)
//     --pcbank B            restrict pc watches to ROM bank B (hex)
//     --ring                record instruction ring trace; auto-dumps to
//                           ringtrace.bin on watch hit or crash-to-0000
//     --tracestart/--traceend N   write bank:PC flow trace for frames [N..M]
//                                 to <out>/pctrace.bin
//
// Script format (one command per line, '#' comments):
//   hold <A|B|START|SELECT|UP|DOWN|LEFT|RIGHT> <startFrame> <endFrame>
//   shot <frame> <name>
//   end <frame>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Debugger.h"
#include "EMUType.h"
#include "GameBoy.h"
#include "IEventHandler.h"
#include "IGraphicsHandler.h"
#include "ISerialHandler.h"
#include "JoypadController.h"

namespace
{

void WriteBMP(const std::string& path, const u32* argb, int width, int height)
{
    // 24-bit bottom-up BMP
    int rowBytes = ((width * 3) + 3) & ~3;
    int dataSize = rowBytes * height;
    int fileSize = 54 + dataSize;

    unsigned char header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    header[2] = fileSize & 0xFF; header[3] = (fileSize >> 8) & 0xFF;
    header[4] = (fileSize >> 16) & 0xFF; header[5] = (fileSize >> 24) & 0xFF;
    header[10] = 54;
    header[14] = 40;
    header[18] = width & 0xFF; header[19] = (width >> 8) & 0xFF;
    header[22] = height & 0xFF; header[23] = (height >> 8) & 0xFF;
    header[26] = 1;
    header[28] = 24;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if (!file)
    {
        std::cerr << "Could not write screenshot: " << path << std::endl;
        return;
    }

    file.write(reinterpret_cast<char*>(header), 54);

    std::vector<unsigned char> row(rowBytes, 0);

    for (int y = height - 1; y >= 0; --y)
    {
        for (int x = 0; x < width; ++x)
        {
            u32 px = argb[y * width + x];
            row[x * 3 + 0] = px & 0xFF;         // B
            row[x * 3 + 1] = (px >> 8) & 0xFF;  // G
            row[x * 3 + 2] = (px >> 16) & 0xFF; // R
        }

        file.write(reinterpret_cast<char*>(row.data()), rowBytes);
    }
}

class HeadlessGraphicsHandler : public IGraphicsHandler
{
private:
    std::vector<u32> lastFrame;
    int width = 0, height = 0;
    u64 drawCount = 0;

public:
    void Init() override {}
    void Clear() override {}

    void Draw(const u32* pixelBuffer, int w, int h) override
    {
        width = w;
        height = h;
        lastFrame.assign(pixelBuffer, pixelBuffer + ((size_t)w * h));
        ++drawCount;
    }

    void Quit() override {}

    u64 DrawCount() const { return drawCount; }

    bool SaveBMP(const std::string& path) const
    {
        if (lastFrame.empty())
        {
            std::cerr << "No frame rendered yet, skipping screenshot: " << path << std::endl;
            return false;
        }

        WriteBMP(path, lastFrame.data(), width, height);
        return true;
    }
};

struct Hold
{
    u8 button;
    u64 start;
    u64 end;
    bool pressed = false;
    bool released = false;
};

class ScriptedEventHandler : public IEventHandler
{
private:
    GameBoy* boy = nullptr;
    std::vector<Hold> holds;
    u64 endFrame;

public:
    explicit ScriptedEventHandler(u64 endFrame) : endFrame(endFrame) {}

    void SetGameBoy(GameBoy* gb) { boy = gb; }
    void AddHold(Hold h) { holds.push_back(h); }
    void SetEndFrame(u64 f) { endFrame = f; }

    void HandleInput(JoypadController* joypadController) override
    {
        u64 frame = boy ? boy->FramesElapsed() : 0;

        for (Hold& h : holds)
        {
            if (!h.pressed && frame >= h.start && frame < h.end)
            {
                joypadController->KeyDown(h.button);
                h.pressed = true;
            }

            if (h.pressed && !h.released && frame >= h.end)
            {
                joypadController->KeyUp(h.button);
                h.released = true;
            }
        }
    }

    bool ShouldQuit() override
    {
        return boy != nullptr && boy->FramesElapsed() >= endFrame;
    }

    int SpeedMultiplier() override { return 1 << 30; }
};

// Captures bytes the game pushes out the link port (blargg tests print
// results this way) by acting as a loopback peer that always replies 0xFF.
class CaptureSerialHandler : public ISerialHandler
{
private:
    bool pending = false;
    std::string captured;

public:
    bool IsSerialConnected() override { return true; }

    void SendByte(Byte byte) override
    {
        captured.push_back((char)byte);
        pending = true;
    }

    bool ByteRecieved() override { return pending; }

    Byte RecieveByte() override
    {
        pending = false;
        return 0xFF;
    }

    const std::string& Captured() const { return captured; }
};

struct Shot
{
    u64 frame;
    std::string name;
};

u8 ButtonFromName(const std::string& name)
{
    if (name == "A") return JOYPAD_BUTTONS::BUTTON_A;
    if (name == "B") return JOYPAD_BUTTONS::BUTTON_B;
    if (name == "START") return JOYPAD_BUTTONS::BUTTON_START;
    if (name == "SELECT") return JOYPAD_BUTTONS::BUTTON_SELECT;
    if (name == "UP") return JOYPAD_BUTTONS::BUTTON_UP;
    if (name == "DOWN") return JOYPAD_BUTTONS::BUTTON_DOWN;
    if (name == "LEFT") return JOYPAD_BUTTONS::BUTTON_LEFT;
    if (name == "RIGHT") return JOYPAD_BUTTONS::BUTTON_RIGHT;
    throw std::runtime_error("Unknown button in script: " + name);
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: GBHeadless <romFolder> <romFile> [--frames N] [--script FILE] [--out DIR] [--emutype cart|dmg|cgb] [--save] [diagnostics; see header comment]" << std::endl;
        return 2;
    }

    std::string romFolder = argv[1];
    std::string romFile = argv[2];
    std::string scriptPath;
    std::string outDir = ".";
    u64 frames = 600;
    u64 traceStart = 0, traceEnd = 0;
    u16 watchLow = 0, watchHigh = 0;
    u16 pcWatch = 0, bcLow = 0, bcHigh = 0xFFFF;
    int pcWatchBank = -1;
    int pcWatchA = -1;
    u8 watchMin = 0;
    bool watchSet = false;
    bool ringTrace = false;
    bool debugDump = false;
    bool intLog = false;
    bool profile = false;
    bool save = false;
    EMUType emuType = EMUType::Cartridge;

    for (int i = 3; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--frames" && i + 1 < argc) frames = std::stoull(argv[++i]);
        else if (arg == "--tracestart" && i + 1 < argc) traceStart = std::stoull(argv[++i]);
        else if (arg == "--traceend" && i + 1 < argc) traceEnd = std::stoull(argv[++i]);
        else if (arg == "--watch" && i + 2 < argc)
        {
            watchLow = (u16)std::stoul(argv[++i], nullptr, 16);
            watchHigh = (u16)std::stoul(argv[++i], nullptr, 16);
            watchSet = true;
        }
        else if (arg == "--watchmin" && i + 1 < argc) watchMin = (u8)std::stoul(argv[++i], nullptr, 16);
        else if (arg == "--ring") ringTrace = true;
        else if (arg == "--debug") debugDump = true;
        else if (arg == "--intlog") intLog = true;
        else if (arg == "--profile") profile = true;
        else if (arg == "--pcwatch" && i + 3 < argc)
        {
            pcWatch = (u16)std::stoul(argv[++i], nullptr, 16);
            bcLow = (u16)std::stoul(argv[++i], nullptr, 16);
            bcHigh = (u16)std::stoul(argv[++i], nullptr, 16);
        }
        else if (arg == "--pcwatcha" && i + 2 < argc)
        {
            pcWatch = (u16)std::stoul(argv[++i], nullptr, 16);
            pcWatchA = (int)std::stoul(argv[++i], nullptr, 16);
        }
        else if (arg == "--pcbank" && i + 1 < argc) pcWatchBank = (int)std::stoul(argv[++i], nullptr, 16);
        else if (arg == "--script" && i + 1 < argc) scriptPath = argv[++i];
        else if (arg == "--out" && i + 1 < argc) outDir = argv[++i];
        else if (arg == "--save") save = true;
        else if (arg == "--emutype" && i + 1 < argc)
        {
            std::string mode = argv[++i];
            if (mode == "dmg") emuType = EMUType::DMG;
            else if (mode == "cgb") emuType = EMUType::CGB;
            else emuType = EMUType::Cartridge;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 2;
        }
    }

    auto* gfx = new HeadlessGraphicsHandler();
    auto* events = new ScriptedEventHandler(frames);
    auto* serial = new CaptureSerialHandler();

    std::vector<Shot> shots;

    if (!scriptPath.empty())
    {
        std::ifstream script(scriptPath);

        if (!script)
        {
            std::cerr << "Could not open script: " << scriptPath << std::endl;
            return 2;
        }

        std::string line;

        while (std::getline(script, line))
        {
            size_t hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);

            std::istringstream ss(line);
            std::string cmd;

            if (!(ss >> cmd)) continue;

            if (cmd == "hold")
            {
                std::string button;
                u64 start, end;
                ss >> button >> start >> end;
                events->AddHold({ButtonFromName(button), start, end});
            }
            else if (cmd == "shot")
            {
                Shot s;
                ss >> s.frame >> s.name;
                shots.push_back(s);
            }
            else if (cmd == "end")
            {
                u64 f;
                ss >> f;
                frames = f;
                events->SetEndFrame(f);
            }
            else
            {
                std::cerr << "Unknown script command: " << cmd << std::endl;
                return 2;
            }
        }
    }

    std::sort(shots.begin(), shots.end(), [](const Shot& a, const Shot& b) { return a.frame < b.frame; });

    GameBoy* boy = new GameBoy(romFolder, gfx, events, serial, emuType);
    events->SetGameBoy(boy);
    boy->SetAlwaysRender(true);

    Debugger debugger;
    bool useDebugger = watchSet || ringTrace || debugDump || intLog || pcWatch != 0;


    int exitCode = 0;

    try
    {
        boy->LoadRom(romFile);
        boy->SetDeterministicRTC(true);

        if (useDebugger)
        {
            boy->AttachDebugger(&debugger);

            if (watchSet) debugger.SetWriteWatch(watchLow, watchHigh, watchMin);
            if (ringTrace) debugger.EnableRingTrace();
            if (intLog) debugger.EnableInterruptLog();

            if (pcWatch != 0)
            {
                if (pcWatchA >= 0) debugger.SetPCWatchA(pcWatch, pcWatchBank, (u8)pcWatchA);
                else debugger.SetPCWatch(pcWatch, pcWatchBank, bcLow, bcHigh);
            }
        }

        boy->Start();

        size_t shotIdx = 0;
        std::map<u16, u64> pcHistogram;
        std::vector<u32> pcTrace;

        while (!boy->ShouldStop())
        {
            boy->Step();

            u64 f = boy->FramesElapsed();

            if (profile && f + 120 >= frames)
            {
                // Profile the last ~2 seconds to find wait loops.
                pcHistogram[boy->GetPC()]++;
            }

            if (traceEnd > 0 && f >= traceStart && f <= traceEnd)
            {
                u16 pc = boy->GetPC();
                u32 entry = ((u32)boy->GetROMBank() << 16) | pc;
                if (pcTrace.empty() || pcTrace.back() != entry) pcTrace.push_back(entry);
            }

            while (shotIdx < shots.size() && f >= shots[shotIdx].frame)
            {
                gfx->SaveBMP(outDir + "/" + shots[shotIdx].name + ".bmp");
                std::cout << "[shot] frame " << f << " -> " << shots[shotIdx].name << ".bmp" << std::endl;
                ++shotIdx;
            }
        }

        if (!pcTrace.empty())
        {
            std::ofstream tf(outDir + "/pctrace.bin", std::ios::binary | std::ios::trunc);
            tf.write(reinterpret_cast<char*>(pcTrace.data()), pcTrace.size() * sizeof(u32));
            std::cout << "Trace entries: " << pcTrace.size() << std::endl;
        }

        if (profile)
        {
            std::vector<std::pair<u64, u16>> hot;
            for (auto& kv : pcHistogram) hot.push_back({kv.second, kv.first});
            std::sort(hot.rbegin(), hot.rend());
            std::cout << "Hot PCs (last 120 frames): ";
            for (size_t i = 0; i < hot.size() && i < 120; ++i)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%04X:%llu ", hot[i].second, (unsigned long long)hot[i].first);
                std::cout << buf;
            }
            std::cout << std::endl;
        }

        if (save)
        {
            boy->SaveGame();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "EMULATION ERROR: " << ex.what() << std::endl;
        exitCode = 1;
    }

    std::cout << "Frames: " << boy->FramesElapsed() << " Draws: " << gfx->DrawCount() << std::endl;

    if (debugDump)
    {
        debugger.DumpState();
    }

    if (!serial->Captured().empty())
    {
        std::cout << "--- serial output ---" << std::endl;
        std::cout << serial->Captured() << std::endl;
        std::cout << "---------------------" << std::endl;
    }

    return exitCode;
}
