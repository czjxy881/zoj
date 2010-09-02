/*
 * Copyright 2010 Li, Cheng <hanshuiys@gmail.com>
 *
 * This file is part of ZOJ.
 *
 * ZOJ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ZOJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ZOJ. if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SCRIPT_RUNNER_H__
#define __SCRIPT_RUNNER_H__

#include "native_runner.h"

class TraceCallback;

class ScriptRunner : public NativeRunner {
  public:
    ScriptRunner(int sock, int time_limit, int memory_limit, int output_limit, int uid, int gid, const char* commands[])
        : NativeRunner(sock, time_limit, memory_limit, output_limit, uid, gid) {
        this->commands = commands;
        this->loader_syscall_magic_id_ = 0;
        this->loader_syscall_magic_left_ = 0;
        this->base_time_ = 0;
        this->base_memory_ = 0;
    }
    void SetLoaderSyscallMagic(unsigned long id, int count);
    void SetBaseTime(int time) { this->base_time_ = time; };
    void SetBaseMemory(int memory) { this->base_memory_ = memory; };
    virtual void UpdateStatus();

  protected:
    virtual void InternalRun();
    virtual StartupInfo GetStartupInfo();
    virtual Tracer* CreateTracer(pid_t pid, Runner* runner);

    const char** commands;
    unsigned long loader_syscall_magic_id_;
    int loader_syscall_magic_left_;

    int base_memory_;
    int base_time_;

  friend class ScriptRunnerTest;    
};

#endif // __SCRIPT_RUNNER_H__
