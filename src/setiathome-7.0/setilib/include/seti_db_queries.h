#define RESERVED_NOT_FLIPPED 0
#define RESERVED_FLIPPED 1
#define IQ_NOT_MODIFIED 0
#define IQ_MODIFIED 1

template<typename T>
int get_next_signal (T &table, std::string where_clause, std::string order_by_clause = "", std::string directive_clause = "") {

    static bool start = true;
    int retval, get_next_retval;

    if (start) {
        retval = table.open_query(where_clause,order_by_clause,directive_clause);
        if (retval) {
            fprintf(stderr, "open_query returns %d, %d\n", retval, sql_last_error_code());
        }
        start = false;
    }
    get_next_retval = table.get_next();
    return get_next_retval;
}

template<typename T>
int get_next_signal_by_tape_id (T &table, int tape_id) {

    char c_string[256];
    std::string where_clause;

    sprintf(c_string, "where result_id in (select id from result where wuid in (select id from workunit where group_info in (select id from workunit_grp where tape_info = %d)))", tape_id);
    where_clause = c_string;

    return (get_next_signal(table, where_clause));
}

template<typename T>
int get_next_signal_by_wug_id (T& table, long wug_id) {

    char c_string[256];
    std::string where_clause;
sprintf(c_string, "where result_id in (select id from result where wuid in (select id from workunit where group_info = %ld))", wug_id);
    where_clause = c_string;

    return (get_next_signal(table, where_clause));
}

// get_next_signal_by_id_range -
//   enumerates over all signals within id range (inclusive)
//   first call will open query and return first signal it finds
//   subsequent calls will return next signal
//   To do: add "restart query" functionality
//          clean up/truly understand retvals from open and get_next
//
template<typename T>
int get_next_signal_by_id_range (T& table, long minid, long maxid) {

    static bool start = true;
    int retval, get_next_retval;
    int i;

    char c_string[256];
    std::string where_clause;

    sprintf(c_string, "where id >= %ld and id <= %ld",  minid, maxid);
    where_clause = c_string;

    return (get_next_signal(table, where_clause));
}

// get_next_signal_by_time_range -
//   enumerates over all signals within time range (inclusive)
//   first call will open query and return first signal it finds
//   subsequent calls will return next signal
//   To do: add "restart query" functionality
//          clean up/truly understand retvals from open and get_next
//
template<typename T>
int get_next_signal_by_time_range (T &table, double mintime, double maxtime, std::string directive_clause = "") {

    static bool start = true;
    int retval, get_next_retval;
    int i;

    char c_string[256];
    std::string where_clause;

    sprintf(c_string, "where time >= %lf and time <= %lf",  mintime, maxtime);
    where_clause = c_string;

    return (get_next_signal(table, where_clause, "", directive_clause));
}

// get_next_signal_by_time_range -
//   enumerates over all signals within time/freq range
//   NOTE: uses *barycentric* freq
//   first call will open query and return first signal it finds
//   subsequent calls will return next signal
//   To do: add "restart query" functionality
//          clean up/truly understand retvals from open and get_next
//
template<typename T>
int get_next_signal_by_time_and_freq_range (T &table, double mintime, double maxtime, double minfreq, double maxfreq, std::string directive_clause = "") {

    static bool start = true;
    int retval, get_next_retval;
    int i;

    char c_string[256];
    std::string where_clause;

//    sprintf(c_string, "where time >= %lf and time <= %lf and barycentric_freq >= %lf and barycentric_freq <= %lf",  mintime, maxtime, minfreq, maxfreq);
    sprintf(c_string, "where time > %lf and time < %lf and barycentric_freq > %lf and barycentric_freq < %lf",  mintime, maxtime, minfreq, maxfreq);
    where_clause = c_string;

    return (get_next_signal(table, where_clause, "", directive_clause));
}

// get_next_signal_by_q_pix_range -
//   enumerates over all signals within q_pix range (inclusive)
//   first call will open query and return first signal it finds
//   subsequent calls will return next signal
//   To do: add "restart query" functionality
//          clean up/truly understand retvals from open and get_next
//
template<typename T>
int get_next_signal_by_q_pix_range (T &table, long long minqpix, long long maxqpix, bool close = false) {

    static bool start = true;
    int retval, get_next_retval;
    int i;

    char c_string[256];
    std::string where_clause;
    char d_string[256];
    std::string directive_clause;

    sprintf(c_string, "where q_pix >= %lld and q_pix <= %lld",  minqpix, maxqpix);
    where_clause = c_string;
    sprintf(d_string, "{+index(pulse pulse_qpix)}");
    directive_clause = d_string;

    if (close) {
        start = true;
        retval = table.close_query();
        return retval;
    }
    if (start) {
        retval = table.open_query(where_clause,"",directive_clause);
        // if (retval) { fprintf(stderr, "open_query returns %d, %d\n", retval, sql_last_error_code()); }
        start = false;
    }
    get_next_retval = table.get_next();
    return get_next_retval;
}

