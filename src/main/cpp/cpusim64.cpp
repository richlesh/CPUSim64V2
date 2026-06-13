// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
static const char PATH_SEP = '\\';
static const char CP_SEP = ';';
#else
static const char PATH_SEP = '/';
static const char CP_SEP = ':';
#endif

static std::string get_app_dir() {
#ifdef __APPLE__
    return "/Applications/CPUSim64.app/Contents";
#elif defined(_WIN32)
    char path[MAX_PATH];
    if (GetEnvironmentVariableA("LOCALAPPDATA", path, MAX_PATH)) {
        std::string localApp = std::string(path) + "\\CPUSim64";
        if (fs::exists(localApp + "\\runtime\\bin\\java.exe")) {
            return localApp;
        }
    }
    return "C:\\Program Files\\CPUSim64";
#else
    return "/opt/cpusim64/lib";
#endif
}

static std::string get_java_cmd(const std::string& app_dir) {
#ifdef __APPLE__
    return app_dir + "/runtime/Contents/Home/bin/java";
#elif defined(_WIN32)
    return app_dir + "\\runtime\\bin\\java.exe";
#else
    return app_dir + "/runtime/bin/java";
#endif
}

static std::string get_classpath(const std::string& app_dir) {
#ifdef __APPLE__
    return app_dir + "/app/*";
#elif defined(_WIN32)
    return app_dir + "\\app\\*";
#else
    return app_dir + "/app/*";
#endif
}

static const std::string MEMSIZE = "1M";
static const std::string STACKSIZE = "8K";

static std::string quote(const std::string& s) {
#ifdef _WIN32
    return "\"" + s + "\"";
#else
    return "'" + s + "'";
#endif
}

static int run_cmd(const std::string& cmd) {
#ifdef _WIN32
    // cmd.exe requires the entire command wrapped in quotes when paths contain spaces
    std::string wrapped = "\"" + cmd + "\"";
    return system(wrapped.c_str());
#else
    return system(cmd.c_str());
#endif
}

static bool file_exists(const std::string& path) {
    return fs::exists(path);
}

static void remove_file(const std::string& path) {
    if (file_exists(path)) fs::remove(path);
}

static bool has_main_label(const std::string& asm_file) {
    std::ifstream file(asm_file);
    if (!file.is_open()) return false;
    std::regex pattern("^\\s*__MAIN__:", std::regex_constants::icase);
    std::string line;
    while (std::getline(file, line)) {
        if (std::regex_search(line, pattern)) return true;
    }
    return false;
}

static std::string build_java_cmd(const std::string& java_cmd, const std::string& classpath,
                                   const std::string& main_class, const std::vector<std::string>& args) {
    std::string cmd = quote(java_cmd) + " -Dfile.encoding=UTF8 -cp " + quote(classpath) + " " + main_class;
    for (const auto& arg : args) {
        cmd += " " + quote(arg);
    }
    return cmd;
}

static void usage() {
    std::cerr
        << "CPUSim64: A simple 64-bit CPU simulator\n"
        << "Version 2.0.0\n"
        << "Copyright (C) 2026 Richard Lesh\n\n"
        << "Usage: cpusim64 <command> <file> [options...]\n"
        << "Commands: assemble, debug, disassemble, preprocess, run, trace\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) usage();

    std::string command = argv[1];
    std::string app_dir = get_app_dir();
    std::string java_cmd = get_java_cmd(app_dir);
    std::string classpath = get_classpath(app_dir);

    if (!file_exists(java_cmd)) {
        std::cerr << "Error: CPUSim64 application not found. Please install the application first.\n";
        return 1;
    }

    if (argc < 3) usage();
    std::string base = argv[2];

    if (command == "assemble") {
        std::string asm_file = base + ".asm";
        remove_file(base + ".obj.gz");
        std::vector<std::string> args = {asm_file};
        if (has_main_label(asm_file)) args.push_back("--hasMain");
        for (int i = 3; i < argc; i++) args.push_back(argv[i]);
        return run_cmd(build_java_cmd(java_cmd, classpath, "Assembler", args));

    } else if (command == "preprocess") {
        std::string asm_file = base + ".asm";
        remove_file(base + ".pp.asm");
        std::vector<std::string> args = {asm_file};
        if (has_main_label(asm_file)) args.push_back("--hasMain");
        for (int i = 3; i < argc; i++) args.push_back(argv[i]);
        return run_cmd(build_java_cmd(java_cmd, classpath, "Preprocessor", args));

    } else if (command == "disassemble") {
        std::string obj_file = base + ".obj.gz";
        std::vector<std::string> args = {obj_file};
        if (argc > 3) args.push_back(argv[3]);
        return run_cmd(build_java_cmd(java_cmd, classpath, "Disassembler", args));

    } else if (command == "run") {
        std::string asm_file = base + ".asm";
        std::string obj_file = base + ".obj.gz";
        remove_file(obj_file);
        int rc = run_cmd(build_java_cmd(java_cmd, classpath, "Assembler", {asm_file}));
        if (rc != 0 || !file_exists(obj_file)) return rc;
        std::vector<std::string> args = {obj_file, "--verbose", "--mem=" + MEMSIZE, "--stack=" + STACKSIZE};
        for (int i = 3; i < argc; i++) args.push_back(argv[i]);
        return run_cmd(build_java_cmd(java_cmd, classpath, "Simulation", args));

    } else if (command == "debug") {
        std::string asm_file = base + ".asm";
        std::string obj_file = base + ".obj.gz";
        remove_file(obj_file);
        int rc = run_cmd(build_java_cmd(java_cmd, classpath, "Assembler", {asm_file, "--DEBUG"}));
        if (rc != 0 || !file_exists(obj_file)) return rc;
        std::vector<std::string> args = {obj_file, "--verbose", "--debug", "--mem=" + MEMSIZE, "--stack=" + STACKSIZE};
        for (int i = 3; i < argc; i++) args.push_back(argv[i]);
        return run_cmd(build_java_cmd(java_cmd, classpath, "Simulation", args));

    } else if (command == "trace") {
        std::string asm_file = base + ".asm";
        std::string obj_file = base + ".obj.gz";
        remove_file(obj_file);
        int rc = run_cmd(build_java_cmd(java_cmd, classpath, "Assembler", {asm_file, "--DEBUG"}));
        if (rc != 0 || !file_exists(obj_file)) return rc;
        std::vector<std::string> args = {obj_file, "--verbose", "--trace", "--mem=" + MEMSIZE, "--stack=" + STACKSIZE};
        for (int i = 3; i < argc; i++) args.push_back(argv[i]);
        return run_cmd(build_java_cmd(java_cmd, classpath, "Simulation", args));

    } else {
        std::cerr << "Unknown command: " << command << "\n";
        usage();
    }
    return 1;
}
