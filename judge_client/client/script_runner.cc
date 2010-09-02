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

#include "script_runner.h"

#include <string>

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common_io.h"
#include "logging.h"
#include "protocol.h"
#include "tracer.h"
#include "util.h"

namespace {

class ScriptTracer : public Tracer {
  public:
    ScriptTracer(pid_t pid, ScriptRunner* runner) :
        Tracer(pid), runner_(runner), loader_syscall_magic_id_(0), loader_syscall_magic_left_(0) {
    }

    void SetLoaderSyscallMagic(unsigned long id, int count) {
        loader_syscall_magic_id_ = id;
        loader_syscall_magic_left_ = count;
    }

  protected:
    virtual void OnExit() {
        runner_->UpdateStatus();
    }

    virtual bool HandleSyscall(struct user_regs_struct& regs) {
        if (loader_syscall_magic_left_ > 0) {
            if (before_syscall_) {
                if (regs.REG_SYSCALL == loader_syscall_magic_id_) {
                    LOG(INFO) << "Got magic syscall: " << loader_syscall_magic_left_;
                    loader_syscall_magic_left_--;
                    runner_->SetBaseMemory(ReadMemoryConsumption(pid_));
                    runner_->SetBaseTime(ReadTimeConsumption(pid_));
                }
            }
            ptrace(PTRACE_SYSCALL, pid_, 0, 0);
            return true;
        }
        return Tracer::HandleSyscall(regs);
    }

  private:
    ScriptRunner* runner_;
    unsigned long loader_syscall_magic_id_;
    int loader_syscall_magic_left_;
};

}

Tracer* ScriptRunner::CreateTracer(pid_t pid, Runner* runner) {
    ScriptRunner* r = dynamic_cast<ScriptRunner*>(runner);
    ScriptTracer* t = new ScriptTracer(pid, r);
    t->SetLoaderSyscallMagic(loader_syscall_magic_id_, loader_syscall_magic_left_);
    return t;
}

void ScriptRunner::InternalRun() {
    RunProgram(commands);
}

void ScriptRunner::SetLoaderSyscallMagic(unsigned long id, int count) {
    loader_syscall_magic_id_ = id;
    loader_syscall_magic_left_ = count;
}

void ScriptRunner::UpdateStatus() {
    int ts = ReadTimeConsumption(pid_) - base_time_;
    int ms = ReadMemoryConsumption(pid_) - base_memory_;
    if (ts > time_consumption_) {
        time_consumption_ = ts;
    }
    if (ms > memory_consumption_) {
        memory_consumption_ = ms;
    }
    if (time_consumption_ > time_limit_ * 1000) {
        result_ = TIME_LIMIT_EXCEEDED;
    }
    if (result_ == TIME_LIMIT_EXCEEDED && time_consumption_ <= time_limit_ * 1000) {
        time_consumption_ = time_limit_ * 1000 + 1;
    }
    if (memory_consumption_ > memory_limit_) {
        result_ = MEMORY_LIMIT_EXCEEDED;
    }
    if (result_ == MEMORY_LIMIT_EXCEEDED && memory_consumption_ <= memory_limit_) {
        memory_consumption_ = memory_limit_ + 1;
    }
    DLOG<<time_consumption_<<' '<<memory_consumption_;
    if (SendRunningMessage() == -1) {
        result_ = INTERNAL_ERROR;
    }
}

StartupInfo ScriptRunner::GetStartupInfo() {
    StartupInfo info;
    info.stdin_filename = "input";
    info.stdout_filename = "p.out";
    info.uid = uid_;
    info.gid = gid_;
    info.time_limit = time_limit_;
    info.memory_limit = memory_limit_ + 64 * 1024;
    info.vm_limit = memory_limit_ + 128 * 1024;
    info.output_limit = output_limit_;
    info.stack_limit = 8192; // Always set stack limit to 8M
    info.proc_limit = 1;
    info.file_limit = 10; // some script languages may need to open more files
    info.trace = 1;
    return info;
}

