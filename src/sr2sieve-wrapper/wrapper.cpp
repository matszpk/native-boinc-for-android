// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// wrapper.C
// wrapper program - lets you use non-BOINC apps with BOINC
//
// Handles:
// - suspend/resume/quit/abort
// - reporting CPU time
// - loss of heartbeat from core client
//
// Does NOT handle:
// - checkpointing
// If your app does checkpointing,
// and there's some way to figure out when it's done it,
// this program could be modified to report to the core client.
//
// See http://boinc.berkeley.edu/wrapper.php for details
// Contributor: Andrew J. Younge (ajy4490@umiacs.umd.edu)

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "procinfo.h"
#include "boinc_api.h"
#include "diagnostics.h"
#include "filesys.h"
#include "error_numbers.h"
#include "util.h"

// The name of the executable that does the actual work:
const std::string capp_name = "primegrid_sr2sieve_1.05_arm-android-linux-gnu";

template <typename Out> Out StringTo(const std::string& input)
{
	std::stringstream	convert;
	Out 				output = Out();

	convert << input;
	convert >> output;
	return output;
}

struct TASK
{
	double progress;
	double old_progress;
	double old_time;
	double old_checkpoint_time;
    int pid;

    TASK() : progress(0.0),
				old_progress(0.0), old_time(0.0), old_checkpoint_time(0.0),
				pid(0) {}

    bool poll(int& status);
    int run(const std::string& app);
    void kill();
    void stop();
    void resume();
	double cpu_time();
};

bool app_suspended = false;

int TASK::run(const std::string& app)
{
    int retval;
    char* argv[256];

    pid = fork();
    if (pid == -1)
        boinc_finish(ERR_FORK);

	if (pid == 0)
	{
		// we're in the child process here

		// construct argv
        memset(argv, 0, sizeof(argv));
        argv[0] = (char*)malloc(app.length() + 1);
        strcpy(argv[0], app.c_str());
        argv[0][app.length()] = 0;
        std::cerr << "wrapper: running " << argv[0] << std::endl;
        setpriority(PRIO_PROCESS, 0, PROCESS_IDLE_PRIORITY);
        retval = execv(argv[0], argv);
        std::cerr << "execv failed: " << strerror(errno) << std::endl;
        exit(ERR_EXEC);
    }
    return 0;
}

bool TASK::poll(int& status)
{
    int wpid;
    struct rusage ru;

#ifdef ANDROID
    wpid = waitpid(pid, &status, WNOHANG);
#else
    wpid = wait4(pid, &status, WNOHANG, &ru);
#endif
    if (wpid)
        return true;
    return false;
}

void TASK::kill()
{
    ::kill(pid, SIGKILL);
}

void TASK::stop()
{
    ::kill(pid, SIGSTOP);
}

void TASK::resume()
{
    ::kill(pid, SIGCONT);
}

void poll_boinc_messages(TASK& task)
{
    BOINC_STATUS status;
    boinc_get_status(&status);
    if (status.no_heartbeat)
	{
        task.kill();
        exit(0);
    }
    if (status.quit_request)
	{
        task.kill();
        exit(0);
    }
    if (status.abort_request)
	{
        task.kill();
        exit(0);
    }
    if (status.suspended)
	{
        if (!app_suspended)
		{
            task.stop();
            app_suspended = true;
        }
    }
	else
	{
        if (app_suspended)
		{
            task.resume();
            app_suspended = false;
        }
    }
}

double TASK::cpu_time()
{
    return linux_cpu_time(pid);
}

double read_status()
{
	std::string buffer;
	double cur = 0.0;
	std::ifstream FR("checkpoint.txt");

	if (FR)
	{
		std::getline(FR, buffer);
		if (!buffer.empty())
		{
			std::string::size_type pos = buffer.rfind("frac_done=");
			if (pos != std::string::npos)
			{
				buffer.erase(0, pos + 10);
				if (!buffer.empty())
					cur = StringTo<double>(buffer);
			}
		}
		std::cerr << "Progress: " << cur << std::endl;
	}
	if (cur > 0.0)
		return cur;
	return 0.0;
}

void send_status_message(TASK& task, double checkpoint_period)
{
	double new_checkpoint_time = task.old_checkpoint_time;
	double cputime = task.cpu_time();

	if (((int)cputime) % ((int)(checkpoint_period)) == 0)
	{
		task.old_progress = task.progress;
		task.progress = read_status();
		new_checkpoint_time = cputime + task.old_time;
	}
	if (task.progress > task.old_progress)
		task.old_checkpoint_time = new_checkpoint_time;

    boinc_report_app_status(
        cputime + task.old_time,
        task.old_checkpoint_time,
        task.progress
    );
}

bool unix2dos(const std::string& in, const std::string& out)
{
	std::ifstream in_file(in.c_str());
	std::ofstream out_file(out.c_str());

	if ((!in_file) || (!out_file))
		return false;

	std::string buffer;
	while (std::getline(in_file, buffer))
	{
		if (!buffer.empty())
			out_file << buffer << "\r\n";
	}

	out_file << std::flush;
	out_file.close();
	return true;
}


int main(int argc, char** argv)
{
    BOINC_OPTIONS options;
    int retval;

	boinc_init_diagnostics(
        BOINC_DIAG_DUMPCALLSTACKENABLED |
        BOINC_DIAG_HEAPCHECKENABLED |
//        BOINC_DIAG_MEMORYLEAKCHECKENABLED |
        BOINC_DIAG_TRACETOSTDERR |
        BOINC_DIAG_REDIRECTSTDERR
    );

    memset(&options, 0, sizeof(options));
    options.main_program = true;
    options.check_heartbeat = true;
    options.handle_process_control = true;

	std::cerr << "BOINC Prime Sierpinski Problem sr2sieve 1.05 wrapper" << std::endl;
	std::cerr << "Using Geoffrey Reynolds' sr2sieve 1.8.11 (ARM 32bit)\n" << std::endl;
    boinc_init_options(&options);

	APP_INIT_DATA uc_aid;
	boinc_get_init_data(uc_aid);
	if (uc_aid.checkpoint_period < 1.0)
		uc_aid.checkpoint_period = 60.0;

	// Copy files:
	std::string resolved_file;
	boinc_resolve_filename_s("in", resolved_file);
	boinc_copy(resolved_file.c_str(), "input.txt");

	boinc_resolve_filename_s("cmd", resolved_file);
	boinc_copy(resolved_file.c_str(), "sr2sieve-command-line.txt");

	std::string orig_capp_name = capp_name + std::string(".orig");
	boinc_resolve_filename_s(orig_capp_name.c_str(), resolved_file);
	boinc_copy(resolved_file.c_str(), capp_name.c_str());

	// Start application:
	TASK t;
    boinc_wu_cpu_time(t.old_time);
	t.old_checkpoint_time = t.old_time;

	retval = t.run(capp_name);
	if (retval)
	{
		std::cerr <<  "can't run app: " << capp_name << std::endl;
		boinc_finish(retval);
		return 1;
	}
	for(;;)
	{
		int status;
		if (t.poll(status))
		{
			int child_ret_val = WEXITSTATUS(status);
			if (child_ret_val)
			{
				std::cerr << "app error: " <<  status << std::endl;
				boinc_finish(status);
			}
			break;
		}
		poll_boinc_messages(t);
		send_status_message(t, uc_aid.checkpoint_period);
		boinc_sleep(1.0);
	}

	std::string out_file_name = "factors.txt";
	boinc_resolve_filename_s("psp_sr2sieve.out", resolved_file);

	if (!boinc_file_exists(out_file_name.c_str()))
	{
		std::cerr << "Factors file not found" << std::endl;
		std::ofstream result_file(resolved_file.c_str());
		if (result_file)
			result_file << "no factors";
	}
	else
	{
		std::cerr << "Factors file found" << std::endl;

		//Convert line-endings:
		std::string out_file_name_final = out_file_name + std::string(".dos");
		if (!unix2dos(out_file_name, out_file_name_final))
		{
			std::cerr << "failed to convert line endings!" << std::endl;
			boinc_finish(-1);
			return 1;
		}
		// Save result file:
		boinc_copy(out_file_name_final.c_str(), resolved_file.c_str());
	}


    boinc_finish(0);
}
