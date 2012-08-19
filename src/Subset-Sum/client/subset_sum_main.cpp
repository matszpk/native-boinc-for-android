
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cmath>
#include <time.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>

#include "stdint.h"

/**
 *  Includes required for BOINC
 */
#ifdef _BOINC_
#ifdef _WIN32
    #include "boinc_win.h"
    #include "str_util.h"
#endif

    #include "diagnostics.h"
    #include "util.h"
    #include "filesys.h"
    #include "boinc_api.h"
    #include "mfile.h"
#endif

/**
 *  Includes for subset sum
 */

#include "bit_logic.hpp"
#include "generate_subsets.hpp"
#include "../common/binary_output.hpp"
#include "../common/n_choose_k.hpp"

using namespace std;

string checkpoint_file = "sss_checkpoint.txt";
string output_filename = "failed_sets.txt";

vector<uint64_t> *failed_sets = new vector<uint64_t>();

uint32_t checksum = 0;

#ifdef HTML_OUTPUT
double max_digits;                  //extern
double max_set_digits;              //extern
#endif

unsigned long int max_sums_length;  //extern
uint32_t *sums;                 //extern
#ifndef ARM_OPT
uint32_t *new_sums;             //extern
#endif

#ifdef DUMP_ITER_STATE
static FILE* dump_file = NULL;
static const uint64_t dump_iters_n = 10000000;

void put_sums() {
    for (uint32_t i = 0; i < max_sums_length; i++)
        fprintf(dump_file, "%08x", sums[i]);
    fputs("\n",dump_file);
}
#endif

#ifdef ARM_OPT
extern "C" uint32_t test_subset_main_arm(const uint32_t *subset,
        const uint32_t subset_size, uint32_t* checksum, uint32_t max_sums_length);
#endif

/**
 *  Tests to see if a subset all passes the subset sum hypothesis
 */
static inline bool test_subset(const uint32_t *subset, const uint32_t subset_size) {
    //this is also symmetric.  TODO: Only need to check from the largest element in the set (9) to the sum(S)/2 == (13), need to see if everything between 9 and 13 is a 1
    uint32_t M = subset[subset_size - 1];
#ifdef ARM_OPT
    /* in this routine boinc checksum is embedded */
    uint32_t max_subset_sum = test_subset_main_arm(subset, subset_size, &checksum, max_sums_length);
#ifdef DUMP_ITER_STATE
    put_sums();
    fprintf(dump_file, "cksum:%08x\n", checksum);
#endif
    return all_ones(sums, max_sums_length, M, max_subset_sum - M);
#else
    uint32_t max_subset_sum = 0;

    for (uint32_t i = 0; i < subset_size; i++) max_subset_sum += subset[i];
    
    for (uint32_t i = 0; i < max_sums_length; i++) {
        sums[i] = 0;
        new_sums[i] = 0;
    }

//    *output_target << "\n");
    uint32_t current;
    for (uint32_t i = 0; i < subset_size; i++) {
        current = subset[i];
#ifdef ONE_PASS_OPT
        shift_left_or_equal(sums, max_sums_length, current);
#else
        shift_left(new_sums, max_sums_length, sums, current);                    // new_sums = sums << current;
//        *output_target << "new_sums = sums << %2u    = ", current);
//        print_bit_array(new_sums, sums_length);
//        *output_target << "\n");

        or_equal(sums, max_sums_length, new_sums);                               //sums |= new_sums;
//        *output_target << "sums |= new_sums         = ");
//        print_bit_array(sums, sums_length);
//        *output_target << "\n");
#endif
        or_single(sums, max_sums_length, current - 1);                           //sums |= 1 << (current - 1);
//        *output_target << "sums != 1 << current - 1 = ");
//        print_bit_array(sums, sums_length);
//        *output_target << "\n");
    }
#ifdef DUMP_ITER_STATE
    put_sums();
#endif
    bool success = all_ones(sums, max_sums_length, M, max_subset_sum - M);
#ifdef _BOINC_
    //Calculate a checksum for verification on BOINC
//    for (uint32_t i = 0; i < max_sums_length; i++) checksum += sums[i];

    //Alternate checksum calculation with overflow detection
    for (uint32_t i = 0; i < max_sums_length; i++) {
        if (UINT32_MAX - checksum <= sums[i]) {
            checksum += sums[i];
        } else { // avoid the overflow
            checksum = sums[i] - (UINT32_MAX - checksum);
        } 
    }
#ifdef DUMP_ITER_STATE
    fprintf(dump_file, "cksum:%08x\n", checksum);
#endif
#endif
    return success;
#endif
}

void write_checkpoint(string filename, const uint64_t iteration, const uint64_t pass, const uint64_t fail, const vector<uint64_t> *failed_sets, const uint32_t checksum) {
#ifdef _BOINC_
    string output_path;
    int retval = boinc_resolve_filename_s(filename.c_str(), output_path);
    if (retval) {
        cerr << "APP: error writing checkpoint (resolving checkpoint file name)" << endl;
        return;
    }   

    ofstream checkpoint_file(output_path.c_str());
#else
    ofstream checkpoint_file(filename.c_str());
#endif
    if (!checkpoint_file.is_open()) {
        cerr << "APP: error writing checkpoint (opening checkpoint file)" << endl;
        return;
    }   

    checkpoint_file << "iteration: " << iteration << endl;
    checkpoint_file << "pass: " << pass << endl;
    checkpoint_file << "fail: " << fail << endl;
    checkpoint_file << "checksum: " << checksum << endl;

    checkpoint_file << "failed_sets: " << failed_sets->size() << endl;
    for (uint32_t i = 0; i < failed_sets->size(); i++) {
        checkpoint_file << " " << failed_sets->at(i);
    }
    checkpoint_file << endl;

    checkpoint_file.close();
}

bool read_checkpoint(string sites_filename, uint64_t &iteration, uint64_t &pass, uint64_t &fail, vector<uint64_t> *failed_sets, uint32_t &checksum) {
#ifdef _BOINC_
    string input_path;
    int retval = boinc_resolve_filename_s(sites_filename.c_str(), input_path);
    if (retval) {
        return 0;
    }

    ifstream sites_file(input_path.c_str());
#else
    ifstream sites_file(sites_filename.c_str());
#endif
    if (!sites_file.is_open()) return false;

    string s;
    sites_file >> s >> iteration;
    if (s.compare("iteration:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'iteration'" << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    sites_file >> s >> pass;
    if (s.compare("pass:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'pass'" << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    sites_file >> s >> fail;
    if (s.compare("fail:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'fail'" << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    sites_file >> s >> checksum;
    if (s.compare("checksum:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'checksum'" << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }
    cerr << "read checksum " << checksum << " from checkpoint." << endl;

    uint32_t failed_sets_size = 0;
    sites_file >> s >> failed_sets_size;
    if (s.compare("failed_sets:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'failed_sets'" << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    uint64_t current;
    for (uint32_t i = 0; i < failed_sets_size; i++) {
        sites_file >> current;
        failed_sets->push_back(current);
        if (!sites_file.good()) {
            cerr << "ERROR: malformed checkpoint! only read '" << i << "' of '" << failed_sets_size << "' failed sets." << endl;
#ifdef _BOINC_
            boinc_finish(1);
#endif
            exit(1);
        }
    }

    return true;
}

template <class T>
T parse_t(const char* arg) {
    string n(arg);
    T result = 0;
    T place = 1;
    uint16_t val = 0;

    for (int i = (int)n.size() - 1; i >= 0; i--) {
//        cerr << "char[%d]: '%c'\n", i, n[i]);
        if      (n[i] == '0') val = 0;
        else if (n[i] == '1') val = 1;
        else if (n[i] == '2') val = 2;
        else if (n[i] == '3') val = 3;
        else if (n[i] == '4') val = 4;
        else if (n[i] == '5') val = 5;
        else if (n[i] == '6') val = 6;
        else if (n[i] == '7') val = 7; 
        else if (n[i] == '8') val = 8;
        else if (n[i] == '9') val = 9;
        else {
            cerr << "ERROR in parse_uint64_t, unrecognized character in string: '" <<  n[i] << "', from argument '" << arg << "', string: '" << n << "', position: " << i << endl;
#ifdef _BOINC_
            boinc_finish(1);
#endif
            exit(1);
        }
        
        result += place * val;
        place *= 10;
    }

    return result;
}


int main(int argc, char** argv) {
#ifdef _BOINC_
    int retval = 0;
#ifdef BOINC_APP_GRAPHICS
#if defined(_WIN32) || defined(__APPLE)
    retval = boinc_init_graphics(worker);
#else
    retval = boinc_init_graphics(worker, argv[0]);
#endif
#else
    retval = boinc_init();
#endif
    if (retval) exit(retval);
#endif

#ifdef DUMP_ITER_STATE
    dump_file = fopen("/mnt/sdcard/dump.out", "wb");
#endif

    if (argc != 3 && argc != 5) {
        cerr << "ERROR, wrong command line arguments." << endl;
        cerr << "USAGE:" << endl;
        cerr << "\t./subset_sum <M> <N> [<i> <count>]" << endl << endl;
        cerr << "argumetns:" << endl;
        cerr << "\t<M>      :   The maximum value allowed in the sets." << endl;
        cerr << "\t<N>      :   The number of elements allowed in a set." << endl;
        cerr << "\t<i>      :   (optional) start at the <i>th generated subset." << endl;
        cerr << "\t<count>  :   (optional) only test <count> subsets (starting at the <i>th subset)." << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    uint32_t max_set_value = parse_t<uint32_t>(argv[1]);
#ifdef HTML_OUTPUT
    max_set_digits = ceil(log10(max_set_value)) + 1;
#endif

    uint32_t subset_size = parse_t<uint32_t>(argv[2]);

    uint64_t iteration = 0;
    uint64_t pass = 0;
    uint64_t fail = 0;

    bool doing_slice = false;
    uint64_t starting_subset = 0;
    uint64_t subsets_to_calculate = 0;

    if (argc == 5) {
        doing_slice = true;
        starting_subset = parse_t<uint64_t>(argv[3]);
        subsets_to_calculate = parse_t<uint64_t>(argv[4]);
        cerr << "argv[1]:       " << argv[1]       << ", argv[2]:     " << argv[2]     << ", argv[3]:         " << argv[3]         << ", argv[4]:              " << argv[4] << endl;
	} else {
		subsets_to_calculate = n_choose_k(max_set_value - 1, subset_size - 1);
        cerr << "argv[1]:       " << argv[1]       << ", argv[2]:     " << argv[2]     << endl;
	}

    cerr << "max_set_value: " << max_set_value << ", subset_size: " << subset_size << ", starting_subset: " << starting_subset << ", subsets_to_calculate: " << subsets_to_calculate << endl;


#ifdef ENABLE_CHECKPOINTING
    bool started_from_checkpoint = read_checkpoint(checkpoint_file, iteration, pass, fail, failed_sets, checksum);
#else
    bool started_from_checkpoint = false;
#endif

    ofstream *output_target;

#ifdef _BOINC_
    string output_path;
    retval = boinc_resolve_filename_s(output_filename.c_str(), output_path);
    if (retval) {
        cerr << "APP: error opening output file for failed sets." << endl;
        boinc_finish(1);
        exit(1);
    }

    if (started_from_checkpoint) {
        output_target = new ofstream(output_path.c_str(), ios::out | ios::app);
    } else {
        output_target = new ofstream(output_path.c_str(), ios::out);
    }
#endif

#ifdef HTML_OUTPUT
    *output_target << "<!DOCTYPE html PUBLIC \"-//w3c//dtd html 4.0 transitional//en\">" << endl;
    *output_target << "<html>" << endl;
    *output_target << "<head>" << endl;
    *output_target << "  <meta http-equiv=\"Content-Type\"" << endl;
    *output_target << " content=\"text/html; charset=iso-8859-1\">" << endl;
    *output_target << "  <meta name=\"GENERATOR\"" << endl;
    *output_target << " content=\"Mozilla/4.76 [en] (X11; U; Linux 2.4.2-2 i686) [Netscape]\">" << endl;
    *output_target << "  <title>" << max_set_value << " choose " << subset_size << "</title>" << endl;
    *output_target << "" << endl;
    *output_target << "<style type=\"text/css\">" << endl;
    *output_target << "    .courier_green {" << endl;
    *output_target << "        color: #008000;" << endl;
    *output_target << "    }   " << endl;
    *output_target << "</style>" << endl;
    *output_target << "<style type=\"text/css\">" << endl;
    *output_target << "    .courier_red {" << endl;
    *output_target << "        color: #FF0000;" << endl;
    *output_target << "    }   " << endl;
    *output_target << "</style>" << endl;
    *output_target << "" << endl;
    *output_target << "</head><body>" << endl;
    *output_target << "<h1>" << max_set_value << " choose " << subset_size << "</h1>" << endl;
    *output_target << "<hr width=\"100%%\">" << endl;
    *output_target << "" << endl;
    *output_target << "<br>" << endl;
    *output_target << "<tt>" << endl;
#endif

    if (!started_from_checkpoint) {
#ifndef HTML_OUTPUT
        *output_target << "max_set_value: " << max_set_value << ", subset_size: " << subset_size << endl;
#else
        *output_target << "max_set_value: " << max_set_value << ", subset_size: " << subset_size << "<br>" << endl;
#endif
        if (max_set_value < subset_size) {
            cerr << "Error max_set_value < subset_size. Quitting." << endl;
#ifdef _BOINC_
            boinc_finish(1);
#endif
            exit(1);
        }
    } else {
        cerr << "Starting from checkpoint on iteration " << iteration << ", with " << pass << " pass, " << fail << " fail." << endl;
    }

    //timestamp flag
#ifdef TIMESTAMP
    time_t start_time;
    time( &start_time );
    if (!started_from_checkpoint) {
        *output_target << "start time: " << ctime(&start_time) << endl;
    }
#endif

    /**
     *  Calculate the maximum set length (in bits) so we can use this for printing out the values cleanly.
     */
    uint32_t *max_set = new uint32_t[subset_size];
    for (uint32_t i = 0; i < subset_size; i++) max_set[subset_size - i - 1] = max_set_value - i;
    for (uint32_t i = 0; i < subset_size; i++) max_sums_length += max_set[i];

//    sums_length /= 2;
    max_sums_length /= ELEMENT_SIZE;
    max_sums_length++;

    delete [] max_set;

#ifdef ARM_OPT
    /* allocate and zeroing additional space for main routine */
    uint32_t *subset = (uint32_t*)memalign(8, (subset_size+4)*4);
    memset(subset, 0, (subset_size+4)*4);
#else
    uint32_t *subset = new uint32_t[subset_size];
#endif

//    this caused a problem:
//    
//    *output_target << "%15u ", 296010);
//    generate_ith_subset(296010, subset, subset_size, max_set_value);
//    print_subset(subset, subset_size);
//    *output_target << "\n");

    uint64_t expected_total = n_choose_k(max_set_value - 1, subset_size - 1);

#ifdef HTML_OUTPUT
    max_digits = ceil(log10(expected_total));
#endif

//    for (uint64_t i = 0; i < expected_total; i++) {
//        *output_target << "%15llu ", i);
//        generate_ith_subset(i, subset, subset_size, max_set_value);
//        print_subset(subset, subset_size);
//        *output_target << "\n");
//    }

    if (doing_slice) {
        *output_target << "performing " << subsets_to_calculate << " set evaluations.";
    } else {
        *output_target << "performing " << expected_total << " set evaluations.";
    }
#ifndef HTML_OUTPUT
    *output_target << "<br>";
#endif
    *output_target << endl;


    if (starting_subset + iteration > expected_total) {
        cerr << "starting subset [" << starting_subset + iteration << "] > total subsets [" << expected_total << "]" << endl;
        cerr << "quitting." << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    if (doing_slice && starting_subset + subsets_to_calculate > expected_total) {
        cerr << "starting subset [" << starting_subset << "] + subsets to calculate [" << subsets_to_calculate << "] > total subsets [" << expected_total << "]" << endl;
        cerr << "quitting." << endl;
#ifdef _BOINC_
        boinc_finish(1);
#endif
        exit(1);
    }

    if (started_from_checkpoint || doing_slice) {
        generate_ith_subset(starting_subset + iteration, subset, subset_size, max_set_value);

    } else {
        for (uint32_t i = 0; i < subset_size - 1; i++) subset[i] = i + 1;
        subset[subset_size - 1] = max_set_value;
    }

#ifdef ARM_OPT
    /* allocate and zeroing additional space for main routine */
    sums = (uint32_t*)memalign(8, 4*(max_sums_length+4));
    memset(sums, 0, 4*(max_sums_length+4));
#else
    sums = new uint32_t[max_sums_length];
    new_sums = new uint32_t[max_sums_length];
#endif

    bool success;

    while (subset[0] <= (max_set_value - subset_size + 1)) {
#ifdef DUMP_ITER_STATE
        if (iteration == dump_iters_n) {
            fclose(dump_file);
            exit(0);
        }
#endif

        success = test_subset(subset, subset_size);
        
        if (success) {
            pass++;
        } else {
            fail++;
            failed_sets->push_back(starting_subset + iteration);
        }

        generate_next_subset(subset, subset_size, max_set_value);

        iteration++;
        if (doing_slice && iteration >= subsets_to_calculate) break;

#ifdef VERBOSE
#ifndef FALSE_ONLY
        print_subset_calculation(starting_subset + iteration, subset, subset_size, success);
#endif
#endif

#ifdef ENABLE_CHECKPOINTING
        /**
         *  Need to checkpoint if we found a failed set, otherwise the output file might contain duplicates
         */
        if (!success || (iteration % 1000000) == 0) {
            double progress;
            if (doing_slice) {
                progress = (double)iteration / (double)subsets_to_calculate;
            } else {
                progress = (double)iteration / (double)expected_total;
           }
#ifdef _BOINC_
#ifdef PRINT_PROGRESS
            printf("progress:%f\r", 100.0*progress);
            fflush(stdout);
#endif
            boinc_fraction_done(progress);
#endif
//            printf("\r%lf", progress);
//
            if (!success || (iteration % 60000000) == 0) {      //this works out to be a checkpoint every 10 seconds or so
//                cerr << "\n*****Checkpointing! *****" << endl;
//                cerr << "CHECKSUM: " << checksum << endl;

//                cout << "[";
//                for (uint32_t i = 0; i < subset_size; i++) {
//                    cout << setw(4) << subset[i];
//                }
//                cout << "]";

//                if (!success) cout << " fail: " << fail << ", failed_subsets.size(): " << failed_sets->size() << "\n";
//                else cout << endl;

                write_checkpoint(checkpoint_file, iteration, pass, fail, failed_sets, checksum);
#ifdef _BOINC_
                boinc_checkpoint_completed();
#endif
            }
        }
#endif
    }

#ifdef _BOINC_
    *output_target << "<checksum>" << checksum << "</checksum>" << endl;
    *output_target << "<uint32_max>" << UINT32_MAX << "</uint32_max>" << endl;
    *output_target << "<failed_subsets>";

    cerr << "<checksum>" << checksum << "</checksum>" << endl;
    cerr << "<uint32_max>" << UINT32_MAX << "</uint32_max>" << endl;
    cerr << "<failed_subsets>";
#endif

#ifdef VERBOSE
#ifdef FALSE_ONLY
    for (uint32_t i = 0; i < failed_sets->size(); i++) {
        generate_ith_subset(failed_sets->at(i), subset, subset_size, max_set_value);
#ifdef _BOINC_
        *output_target << " " << failed_sets->at(i);
        cerr << " " << failed_sets->at(i);
#else
        print_subset_calculation(failed_sets->at(i), subset, subset_size, false);
#endif
    }
#endif
#endif

#ifdef _BOINC_
    *output_target << "</failed_subsets>" << endl;
    *output_target << "<extra_info>" << endl;

    cerr << "</failed_subsets>" << endl;
    cerr << "<extra_info>" << endl;
#endif

    /**
     * pass + fail should = M! / (N! * (M - N)!)
     */

#ifndef HTML_OUTPUT
    if (doing_slice) {
        cerr << "expected to compute " << subsets_to_calculate << " sets" << endl;
        *output_target << "expected to compute " << subsets_to_calculate << " sets" << endl;
    } else {
        cerr << "the expected total number of sets is: " << expected_total << endl;
        *output_target << "the expected total number of sets is: " <<  expected_total << endl;
    }
    cerr << pass + fail << " total sets, " << pass << " sets passed, " << fail << " sets failed, " << ((double)pass / ((double)pass + (double)fail)) << " success rate." << endl;
    *output_target << pass + fail << " total sets, " << pass << " sets passed, " << fail << " sets failed, " << ((double)pass / ((double)pass + (double)fail)) << " success rate." << endl;
#else
    if (doing_slice) {
        cerr << "expected to compute " << subsets_to_calculate << " sets <br>" << endl;
        *output_target << "expected to compute " << subsets_to_calculate << " sets < br>" << endl;
    } else {
        cerr << "the expected total number of sets is: " << expected_total << "<br>" << endl;
        *output_target << "the expected total number of sets is: " <<  expected_total << "<br>" << endl;
    }
    cerr << pass + fail << " total sets, " << pass << " sets passed, " << fail << " sets failed, " << ((double)pass / ((double)pass + (double)fail)) << " success rate.<br>" << endl;
    *output_target << pass + fail << " total sets, " << pass << " sets passed, " << fail << " sets failed, " << ((double)pass / ((double)pass + (double)fail)) << " success rate.<br>" << endl;
#endif

#ifdef _BOINC_
    cerr << "</extra_info>" << endl;
    cerr.flush();

    *output_target << "</extra_info>" << endl;
    output_target->flush();
    output_target->close();
#endif
#ifdef ARM_OPT
    free(subset);
    free(sums);
#else
    delete [] subset;
    delete [] sums;
    delete [] new_sums;
#endif

#ifdef TIMESTAMP
    time_t end_time;
    time( &end_time );
    *output_target << "end time: " << ctime(&end_time) << endl;
    *output_target << "running time: " << end_time - start_time << endl;
#endif

#ifdef _BOINC_
    boinc_finish(0);
#endif

#ifdef HTML_OUTPUT
    *output_target << "</tt>" << endl;
    *output_target << "<br>" << endl << endl;
    *output_target << "<hr width=\"100%%\">" << endl;
    *output_target << "Copyright &copy; Travis Desell, Tom O'Neil and the University of North Dakota, 2012" << endl;
    *output_target << "</body>" << endl;
    *output_target << "</html>" << endl;

    if (fail > 0) {
        cerr << "[url=http://volunteer.cs.und.edu/subset_sum/download/set_" << max_set_value << "c" << subset_size << ".html]" << max_set_value << " choose " << subset_size << "[/url] -- " << fail << " failures" << endl;
    } else {
        cerr << "[url=http://volunteer.cs.und.edu/subset_sum/download/set_" << max_set_value << "c" << subset_size << ".html]" << max_set_value << " choose " << subset_size << "[/url] -- pass" << endl;
    }
#endif

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR Args, int WinMode){
    LPSTR command_line;
    char* argv[100];
    int argc;

    command_line = GetCommandLine();
    argc = parse_command_line( command_line, argv );
    return main(argc, argv);
}
#endif

