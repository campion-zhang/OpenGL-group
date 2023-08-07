/*
 * gpu-perf版本号：1.3
 * 更新时间 ： 2022.09.22
*/

#include <map>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include "Node.h"
#include "ObjectFactory.h"
#include "Log.h"
#include "Window.h"
#include "vmath.h"
#include "cmdline.h"

struct Result {
    float value = 0.0;
    int count = 0;
    int weight = 0;
};

std::map<std::string, int> weightMap;
std::vector<Node *> sceneList;
std::map<Node::NodeType, Result> foreverScore;

void writeResult();
void initWeightMap()
{
    std::ifstream stream("../media/weight.txt");
    std::string line;

    if (stream) {
        while (getline(stream, line)) {
            if (!line.empty() && line[0] != '#') {
                std::string name;
                int weight;
                std::stringstream ss(line);
                ss >> name >> weight;
                weightMap.insert(std::make_pair(name, weight));
            }
        }
    }
}

void init(const std::vector<std::string> &scenes)
{
    auto creators = ObjectFactory<Node, std::string>::creator;
    auto itr = creators->begin();
    while (itr != creators->end()) {
        auto key = itr->first;
        auto find = std::find(scenes.begin(), scenes.end(), key);
        if (scenes.empty() || find != scenes.end()) {
            auto scene = ObjectFactory<Node, std::string>::create(key);
            if (scene) {
                auto weight_itr = weightMap.find(key);
                if (weight_itr != weightMap.end())
                    scene->setWeightValue(weight_itr->second);
                sceneList.push_back(scene);
            }
        }
        itr ++;
    }

    std::sort(sceneList.begin(), sceneList.end(), [](Node * left, Node * right) {
        if (left->getNodeType() < right->getNodeType())
            return true;
        return false;
    });
}

void listScene()
{
    auto creators = ObjectFactory<Node, std::string>::creator;
    auto itr = creators->begin();
    while (itr != creators->end()) {
        auto key = itr->first;
        Log::info("[Scene] %s\n", key.c_str());
        itr ++;
    }
}

void release()
{
    std::for_each(sceneList.begin(), sceneList.end(), [](Node * node) {
        delete node;
    });
    sceneList.clear();
    ObjectFactory<Node, std::string>::shutdown();

    delete PerfWindow::get();
}

#ifndef NOT_DISPLAY_TRACE_INFO

static void printTrace(int flag)
{
    (void)flag;
    void *array[12];
    size_t size;

    size = backtrace (array, 12);
    auto trace = backtrace_symbols (array, size);

    printf("obtained crash %zd stack frames.\n", size);

    size_t name_size = 100;
    char *name = (char *)malloc(name_size);
    for (size_t i = 0; i < size; ++i) {
        char *begin_name = 0;
        char *begin_offset = 0;
        char *end_offset = 0;
        for (char *p = trace[i]; *p; ++p) {
            if (*p == '(') {
                begin_name = p;
            } else if (*p == '+' && begin_name) {
                begin_offset = p;
            } else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }
        if (begin_name && begin_offset && end_offset ) {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';
            int status = -4;
            char *ret = abi::__cxa_demangle(begin_name, name, &name_size, &status);
            if (0 == status) {
                name = ret;
                printf("%s:%s+%s\n", trace[i], name, begin_offset);
            } else {
                printf("%s:%s()+%s\n", trace[i], begin_name, begin_offset);
            }
        } else {
            printf("%s\n", trace[i]);
        }
    }

    free(name);
    exit(0);
}

#endif

int main(int argc, char *argv[])
{
    srand(time(0));

#ifndef NOT_DISPLAY_TRACE_INFO
    signal(SIGSEGV, printTrace);
    signal(SIGABRT, printTrace);
#endif

    /*if (!PerfWindow::isOpenGLSupported())
        return -1;*/

    Log::info("==============================GPU-perf ================================\n");

    cmdline::parser parser;
    parser.set_program_name("GPU-perf");
    parser.add<std::string>("benchmark", 'b', "A benchmark or options to run: '--benchmark textures'",
                            false);
    parser.add("help", 'h', "Display help");
    parser.add("fullscreen", 'f', "Run in fullscreen mode");
    parser.add("list-scenes", 'l', "Display information about the available scenes");
    parser.add("run-forever", 'r',
               "Run indefinitely, looping from the last benchmark back to the first");

    parser.parse_check(argc, argv);
    if (parser.exist("help")) {
        std::cerr << parser.usage();
        return 0;
    }

    if (parser.exist("list-scenes")) {
        listScene();
        release();
        return 0;
    }

    PerfWindow::get()->setFullScreen(parser.exist("fullscreen"));
    if (!PerfWindow::get()->create()) {
        std::cout << "perfwindow create failed" << std::endl;
        return -1;
    }

    initWeightMap();

    std::vector<std::string> benchmarks;

    if (parser.exist("benchmark"))
        benchmarks.emplace_back(parser.get<std::string>("benchmark"));

    init(benchmarks);

    if (sceneList.empty()) {
        std::cout << "unavailable scene\n";
        std::cout << "please type GPU-perf --list-scene display registered scenes\n";
        release();
        return 0;
    }

    bool always = parser.exist("run-forever");

    unsigned int i = 0;
    bool flag = false;

    while (true) {
        auto current = sceneList[i];
        auto result = current->run();

        std::string name = current->getNodeName().data();
        std::string type = Node::getNodeTypeStringByType(current->getNodeType());
        int weight = current->getWeightValue();

        if (result != Node::RunningState_Abort) {
            if (!flag)
                Log::info("  |%-15s| %-15s | %-8s | %-8s  |  %s \n", "Name", "Type", "Weight", "Fps", "Result");

            flag = true;
            float fps = current->getAverageFps();

            if (result == Node::RunningState_Success)
                Log::info("  |%-15s| %-15s | %-8d | %-8.2f  |  %s \n", name.data(), type.data(), weight, fps,
                          "success");
            else
                Log::error(" |%-15s| %-15s | %-8d | %-8.2f  |  %s \n", name.data(), type.data(), weight, fps,
                           "failed");

            float value = fps * weight;

            auto data  = foreverScore[current->getNodeType()];
            data.count ++;
            data.value += value;
            data.weight += weight;
            foreverScore[current->getNodeType()] = data;
        } else {
            double fps = current->getAverageFps();
            Log::info("  |%-15s| %-15s | %-8d | %-8.2f  |  %s \n", name.data(), type.data(), weight, fps,
                      "Abort");
            break;
        }

        i++;

        if (!always && i == sceneList.size())
            break;
        else if (always && i == sceneList.size())
            i = 0;
    }

    Log::info("=======================================================================\n");

    writeResult();
    release();
    return 0;
}

void writeResult()
{
    auto itr = foreverScore.begin();
    while (itr != foreverScore.end()) {
        auto type = itr->first;
        int weight = itr->second.weight;
        float value = itr->second.value / (float)(weight);

        std::string typeName = Node::getNodeTypeStringByType(type);
        Log::info("  scene type: %-20s  score: %-15.2f   \n", typeName.data(), value);

        itr ++;
    }

    Log::info("=======================================================================\n");
}


