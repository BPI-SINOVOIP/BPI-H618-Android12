/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <android-base/stringprintf.h>
#include <dmabufinfo/dmabufinfo.h>
#include <dmabufinfo/dmabuf_sysfs_stats.h>

using DmaBuffer = ::android::dmabufinfo::DmaBuffer;

[[noreturn]] static void usage(int exit_status) {
    fprintf(stderr,
            "Usage: %s [-abh] [per-process/per-buffer stats] \n"
            "-a\t show all dma buffers (ion) in big table, [buffer x process] grid \n"
            "-b\t show DMA-BUF per-buffer, per-exporter and per-device statistics \n"
            "-h\t show this help\n"
            "  \t If PID is supplied, the dmabuf information for that process is shown.\n"
            "  \t Per-buffer DMA-BUF stats do not take an argument.\n",
            getprogname());

    exit(exit_status);
}

static std::string GetProcessComm(const pid_t pid) {
    std::string pid_path = android::base::StringPrintf("/proc/%d/comm", pid);
    std::ifstream in{pid_path};
    if (!in) return std::string("N/A");
    std::string line;
    std::getline(in, line);
    if (!in) return std::string("N/A");
    return line;
}

static void PrintDmaBufTable(const std::vector<DmaBuffer>& bufs) {
    if (bufs.empty()) {
        printf("dmabuf info not found ¯\\_(ツ)_/¯\n");
        return;
    }

    // Find all unique pids in the input vector, create a set
    std::set<pid_t> pid_set;
    for (auto& buf : bufs) {
        pid_set.insert(buf.pids().begin(), buf.pids().end());
    }

    // Format the header string spaced and separated with '|'
    printf("    Dmabuf Inode |            Size |   Fd Ref Counts |  Map Ref Counts |");
    for (auto pid : pid_set) {
        printf("%16s:%-5d |", GetProcessComm(pid).c_str(), pid);
    }
    printf("\n");

    // holds per-process dmabuf size in kB
    std::map<pid_t, uint64_t> per_pid_size = {};
    uint64_t dmabuf_total_size = 0;

    // Iterate through all dmabufs and collect per-process sizes, refs
    for (auto& buf : bufs) {
        printf("%16ju |%13" PRIu64 " kB |%16zu |%16zu |",
               static_cast<uintmax_t>(buf.inode()), buf.size() / 1024, buf.fdrefs().size(),
               buf.maprefs().size());
        // Iterate through each process to find out per-process references for each buffer,
        // gather total size used by each process etc.
        for (pid_t pid : pid_set) {
            int pid_fdrefs = 0, pid_maprefs = 0;
            if (buf.fdrefs().count(pid) == 1) {
                // Get the total number of ref counts the process is holding
                // on this buffer. We don't differentiate between mmap or fd.
                pid_fdrefs += buf.fdrefs().at(pid);
            }
            if (buf.maprefs().count(pid) == 1) {
                pid_maprefs += buf.maprefs().at(pid);
            }

            if (pid_fdrefs || pid_maprefs) {
                // Add up the per-pid total size. Note that if a buffer is mapped
                // in 2 different processes, the size will be shown as mapped or opened
                // in both processes. This is intended for visibility.
                //
                // If one wants to get the total *unique* dma buffers, they can simply
                // sum the size of all dma bufs shown by the tool
                per_pid_size[pid] += buf.size() / 1024;
                printf("%9d(%6d) refs |", pid_fdrefs, pid_maprefs);
            } else {
                printf("%22s |", "--");
            }
        }
        dmabuf_total_size += buf.size() / 1024;
        printf("\n");
    }

    printf("------------------------------------\n");
    printf("%-16s  %13" PRIu64 " kB |%16s |%16s |", "TOTALS", dmabuf_total_size, "n/a", "n/a");
    for (auto pid : pid_set) {
        printf("%19" PRIu64 " kB |", per_pid_size[pid]);
    }
    printf("\n");

    return;
}

static void PrintDmaBufPerProcess(const std::vector<DmaBuffer>& bufs) {
    if (bufs.empty()) {
        printf("dmabuf info not found ¯\\_(ツ)_/¯\n");
        return;
    }

    // Create a reverse map from pid to dmabufs
    std::unordered_map<pid_t, std::set<ino_t>> pid_to_inodes = {};
    uint64_t total_size = 0;  // Total size of dmabufs in the system
    uint64_t kernel_rss = 0;  // Total size of dmabufs NOT mapped or opened by a process
    for (auto& buf : bufs) {
        for (auto pid : buf.pids()) {
            pid_to_inodes[pid].insert(buf.inode());
        }
        total_size += buf.size();
        if (buf.fdrefs().empty() && buf.maprefs().empty()) {
            kernel_rss += buf.size();
        }
    }
    // Create an inode to dmabuf map. We know inodes are unique..
    std::unordered_map<ino_t, DmaBuffer> inode_to_dmabuf;
    for (auto buf : bufs) {
        inode_to_dmabuf[buf.inode()] = buf;
    }

    uint64_t total_rss = 0, total_pss = 0;
    for (auto& [pid, inodes] : pid_to_inodes) {
        uint64_t pss = 0;
        uint64_t rss = 0;

        printf("%16s:%-5d\n", GetProcessComm(pid).c_str(), pid);
        printf("%22s %16s %16s %16s %16s\n", "Name", "Rss", "Pss", "nr_procs", "Inode");
        for (auto& inode : inodes) {
            DmaBuffer& buf = inode_to_dmabuf[inode];
            uint64_t proc_pss = buf.Pss(pid);
            printf("%22s %13" PRIu64 " kB %13" PRIu64 " kB %16zu %16" PRIuMAX "\n",
                   buf.name().empty() ? "<unknown>" : buf.name().c_str(), buf.size() / 1024,
                   proc_pss / 1024, buf.pids().size(), static_cast<uintmax_t>(buf.inode()));
            rss += buf.size();
            pss += proc_pss;
        }
        printf("%22s %13" PRIu64 " kB %13" PRIu64 " kB %16s\n", "PROCESS TOTAL", rss / 1024,
               pss / 1024, "");
        printf("----------------------\n");
        total_rss += rss;
        total_pss += pss;
    }
    printf("dmabuf total: %" PRIu64 " kB kernel_rss: %" PRIu64 " kB userspace_rss: %" PRIu64
           " kB userspace_pss: %" PRIu64 " kB\n ",
           total_size / 1024, kernel_rss / 1024, total_rss / 1024, total_pss / 1024);
}

static bool ReadDmaBufs(std::vector<DmaBuffer>* bufs) {
    bufs->clear();

    if (!ReadDmaBufInfo(bufs)) {
        printf("debugfs entry for dmabuf not available, using /proc/<pid>/fdinfo instead\n");
    }

    std::unique_ptr<DIR, int (*)(DIR*)> dir(opendir("/proc"), closedir);
    if (!dir) {
        fprintf(stderr, "Failed to open /proc directory\n");
        bufs->clear();
        return false;
    }

    struct dirent* dent;
    while ((dent = readdir(dir.get()))) {
        if (dent->d_type != DT_DIR) continue;

        int pid = atoi(dent->d_name);
        if (pid == 0) {
            continue;
        }

        if (!ReadDmaBufFdRefs(pid, bufs)) {
            fprintf(stderr, "Failed to read dmabuf fd references for pid %d\n", pid);
            bufs->clear();
            return false;
        }

        if (!ReadDmaBufMapRefs(pid, bufs)) {
            fprintf(stderr, "Failed to read dmabuf map references for pid %d\n", pid);
            bufs->clear();
            return false;
        }
    }

    return true;
}

static void DumpDmabufSysfsStats() {
    android::dmabufinfo::DmabufSysfsStats stats;

    if (!android::dmabufinfo::GetDmabufSysfsStats(&stats)) {
        printf("Unable to read DMA-BUF sysfs stats from device\n");
        return;
    }

    auto buffer_stats = stats.buffer_stats();
    auto exporter_stats = stats.exporter_info();

    printf("\n\n----------------------- DMA-BUF per-buffer stats -----------------------\n");
    printf("    Dmabuf Inode |     Size(bytes) |             Exporter Name             |\n");
    for (const auto& buf : buffer_stats) {
        printf("%16u |%16u | %16s \n", buf.inode, buf.size, buf.exp_name.c_str());
    }

    printf("\n\n----------------------- DMA-BUF exporter stats -----------------------\n");
    printf("      Exporter Name              | Total Count |     Total Size(bytes)   |\n");
    for (const auto& it : exporter_stats) {
        printf("%32s | %12u| %" PRIu64 "\n", it.first.c_str(), it.second.buffer_count,
               it.second.size);
    }

    printf("\n\n----------------------- DMA-BUF total stats --------------------------\n");
    printf("Total DMA-BUF count: %u, Total DMA-BUF size(bytes): %" PRIu64 "\n", stats.total_count(),
           stats.total_size());
}

int main(int argc, char* argv[]) {
    struct option longopts[] = {{"all", no_argument, nullptr, 'a'},
                                {"per-buffer", no_argument, nullptr, 'b'},
                                {"help", no_argument, nullptr, 'h'},
                                {0, 0, nullptr, 0}};

    int opt;
    bool show_table = false;
    bool show_dmabuf_sysfs_stats = false;
    while ((opt = getopt_long(argc, argv, "abh", longopts, nullptr)) != -1) {
        switch (opt) {
            case 'a':
                show_table = true;
                break;
            case 'b':
                show_dmabuf_sysfs_stats = true;
                break;
            case 'h':
                usage(EXIT_SUCCESS);
            default:
                usage(EXIT_FAILURE);
        }
    }

    pid_t pid = -1;
    if (optind < argc) {
        if (show_table || show_dmabuf_sysfs_stats) {
            fprintf(stderr, "Invalid arguments: -a and -b does not need arguments\n");
            usage(EXIT_FAILURE);
        }
        if (optind != (argc - 1)) {
            fprintf(stderr, "Invalid arguments - only one [PID] argument is allowed\n");
            usage(EXIT_FAILURE);
        }
        pid = atoi(argv[optind]);
        if (pid == 0) {
            fprintf(stderr, "Invalid process id %s\n", argv[optind]);
            usage(EXIT_FAILURE);
        }
    }

    if (show_dmabuf_sysfs_stats) {
        DumpDmabufSysfsStats();
        return 0;
    }

    std::vector<DmaBuffer> bufs;
    if (pid != -1) {
        if (!ReadDmaBufInfo(pid, &bufs)) {
            fprintf(stderr, "Unable to read dmabuf info for %d\n", pid);
            exit(EXIT_FAILURE);
        }
    } else {
        if (!ReadDmaBufs(&bufs)) exit(EXIT_FAILURE);
    }

    // Show the old dmabuf table, inode x process
    if (show_table) {
        PrintDmaBufTable(bufs);
        return 0;
    }

    PrintDmaBufPerProcess(bufs);

    return 0;
}
